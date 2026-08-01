#include "RealtimePlotWidget.h"

#include "app/AppContext.h"
#include "pages/plot/PlotChannelDialog.h"
#include "pages/plot/PlotTheme.h"
#include "services/ChannelDataHub.h"
#include "theme/ThemeManager.h"

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QFileDialog>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <limits>

RealtimePlotWidget::RealtimePlotWidget(
    AppContext *context, QWidget *parent)
    : QWidget(parent),
      m_context(context),
      m_hub(context->channelDataHub()),
      m_refreshTimer(new QTimer(this))
{
    setObjectName(QStringLiteral("realtimePlotWidget"));
    setMinimumSize(300, 200);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setPalette(PlotTheme::palette(context->themeManager()->mode()));
    m_refreshTimer->setInterval(33);
    connect(m_refreshTimer, &QTimer::timeout,
            this, &RealtimePlotWidget::refreshData);
    connect(context->themeManager(), &ThemeManager::themeChanged,
            this, [this](const ThemeMode mode) {
                setPalette(PlotTheme::palette(mode));
                update();
            });
    m_refreshTimer->start();
}

QList<PlotChannelStyle> RealtimePlotWidget::channelStyles() const
{
    return m_styles;
}

void RealtimePlotWidget::setChannelStyles(
    const QList<PlotChannelStyle> &styles)
{
    m_styles = styles;
    m_visibleSamples.clear();
    refreshData();
}

bool RealtimePlotWidget::paused() const noexcept
{
    return m_paused;
}

bool RealtimePlotWidget::followingLatest() const noexcept
{
    return m_followLatest;
}

void RealtimePlotWidget::setPaused(const bool paused)
{
    if (m_paused == paused) return;
    m_paused = paused;
    update();
}

void RealtimePlotWidget::resetView()
{
    m_followLatest = true;
    m_autoY = true;
    m_windowDurationUs = 10000000;
    fitYOnce();
    refreshData();
}

void RealtimePlotWidget::fitYOnce()
{
    updateYRange(true);
    update();
}

void RealtimePlotWidget::openChannelDialog()
{
    PlotChannelDialog dialog(m_context, m_styles, this);
    if (dialog.exec() == QDialog::Accepted) {
        setChannelStyles(dialog.styles());
    }
}

void RealtimePlotWidget::refreshData()
{
    if (m_paused) return;
    qint64 latest = 0;
    for (const PlotChannelStyle &style : m_styles) {
        if (style.visible && m_hub->containsChannel(style.channelId)) {
            latest = qMax(latest, m_hub->latestTimestamp(style.channelId));
        }
    }
    if (m_followLatest && latest > 0) {
        m_minimumTimeUs = latest - static_cast<qint64>(
            m_latestLineRatio * m_windowDurationUs);
        m_maximumTimeUs = m_minimumTimeUs + m_windowDurationUs;
    } else if (m_maximumTimeUs <= m_minimumTimeUs) {
        m_maximumTimeUs = latest > 0
            ? latest : QDateTime::currentMSecsSinceEpoch() * 1000;
        m_minimumTimeUs = m_maximumTimeUs - m_windowDurationUs;
    }
    if (latest > 0) m_latestTimeUs = latest;
    QHash<QString, QVector<ChannelSample>> next;
    const int pixels = qMax(1, static_cast<int>(plotRect().width()));
    for (const PlotChannelStyle &style : m_styles) {
        if (!style.visible || !m_hub->containsChannel(style.channelId)) {
            continue;
        }
        next.insert(
            style.channelId,
            downsample(
                m_hub->snapshot(
                    style.channelId, m_minimumTimeUs, m_maximumTimeUs),
                pixels));
    }
    m_visibleSamples = std::move(next);
    refreshTracer(&m_tracerA);
    refreshTracer(&m_tracerB);
    if (m_autoY) updateYRange(false);
    update();
}

void RealtimePlotWidget::updateYRange(const bool immediate)
{
    double minimum = (std::numeric_limits<double>::max)();
    double maximum = (std::numeric_limits<double>::lowest)();
    for (auto it = m_visibleSamples.cbegin();
         it != m_visibleSamples.cend(); ++it) {
        for (const ChannelSample &sample : it.value()) {
            if (!std::isfinite(sample.value)) continue;
            minimum = qMin(minimum, sample.value);
            maximum = qMax(maximum, sample.value);
        }
    }
    if (minimum > maximum) return;
    double range = maximum - minimum;
    if (range < 1.0e-12) range = qMax(1.0, std::abs(maximum) * 0.1);
    const double targetMinimum = minimum - range * 0.08;
    const double targetMaximum = maximum + range * 0.08;
    if (immediate || targetMinimum < m_minimumY) {
        m_minimumY = targetMinimum;
    } else {
        m_minimumY += (targetMinimum - m_minimumY) * 0.12;
    }
    if (immediate || targetMaximum > m_maximumY) {
        m_maximumY = targetMaximum;
    } else {
        m_maximumY += (targetMaximum - m_maximumY) * 0.12;
    }
}

QRectF RealtimePlotWidget::plotRect() const
{
    return QRectF(rect()).adjusted(62.0, 18.0, -18.0, -42.0);
}

QPointF RealtimePlotWidget::mapSample(
    const ChannelSample &sample, const QRectF &area) const
{
    const double timeRange = qMax<qint64>(
        1, m_maximumTimeUs - m_minimumTimeUs);
    const double yRange = qMax(1.0e-12, m_maximumY - m_minimumY);
    return {
        area.left()
            + (sample.timestampUs - m_minimumTimeUs)
                / timeRange * area.width(),
        area.bottom()
            - (sample.value - m_minimumY) / yRange * area.height()
    };
}

QVector<ChannelSample> RealtimePlotWidget::downsample(
    const QVector<ChannelSample> &samples, const int pixelWidth) const
{
    if (samples.size() <= pixelWidth * 2 || pixelWidth <= 0) return samples;
    QVector<ChannelSample> result;
    result.reserve(pixelWidth * 2);
    const double span = qMax<qint64>(
        1, m_maximumTimeUs - m_minimumTimeUs);
    int start = 0;
    while (start < samples.size()) {
        const int bucket = qBound(
            0,
            static_cast<int>(
                (samples.at(start).timestampUs - m_minimumTimeUs)
                / span * pixelWidth),
            pixelWidth - 1);
        int end = start + 1;
        while (end < samples.size()) {
            const int nextBucket = qBound(
                0,
                static_cast<int>(
                    (samples.at(end).timestampUs - m_minimumTimeUs)
                    / span * pixelWidth),
                pixelWidth - 1);
            if (nextBucket != bucket) break;
            ++end;
        }
        int minimumIndex = start;
        int maximumIndex = start;
        for (int index = start + 1; index < end; ++index) {
            if (samples.at(index).value < samples.at(minimumIndex).value) {
                minimumIndex = index;
            }
            if (samples.at(index).value > samples.at(maximumIndex).value) {
                maximumIndex = index;
            }
        }
        if (minimumIndex < maximumIndex) {
            result.append(samples.at(minimumIndex));
            if (maximumIndex != minimumIndex) {
                result.append(samples.at(maximumIndex));
            }
        } else {
            result.append(samples.at(maximumIndex));
            if (minimumIndex != maximumIndex) {
                result.append(samples.at(minimumIndex));
            }
        }
        start = end;
    }
    return result;
}

void RealtimePlotWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), palette().color(QPalette::Base));
    const QRectF area = plotRect();
    drawAxes(painter, area);
    bool hasSamples = false;
    for (auto it = m_visibleSamples.cbegin();
         it != m_visibleSamples.cend(); ++it) {
        hasSamples = hasSamples || !it.value().isEmpty();
    }
    if (hasSamples) {
        drawCurves(painter, area);
        drawLatestLine(painter, area);
        drawTracers(painter, area);
        drawCrosshair(painter, area);
        drawLegend(painter, area);
    } else {
        painter.setPen(palette().color(QPalette::PlaceholderText));
        painter.drawText(
            area, Qt::AlignCenter,
            m_styles.isEmpty()
                ? tr("暂无绑定通道\n右键打开“通道配置”")
                : tr("当前时间范围内暂无数据"));
    }
    if (m_paused) {
        painter.setPen(QColor(QStringLiteral("#F59E0B")));
        painter.drawText(
            QRectF(area.left(), 2, area.width(), 20),
            Qt::AlignCenter, tr("已暂停"));
    }
}

void RealtimePlotWidget::drawAxes(
    QPainter &painter, const QRectF &area) const
{
    const QColor text = palette().color(QPalette::Text);
    QColor grid = palette().color(QPalette::Mid);
    grid.setAlpha(90);
    for (int index = 0; index <= 5; ++index) {
        const qreal ratio = index / 5.0;
        const qreal x = area.left() + area.width() * ratio;
        const qreal y = area.bottom() - area.height() * ratio;
        if (m_showGrid) {
            painter.setPen(QPen(grid, 1, Qt::DashLine));
            painter.drawLine(QPointF(x, area.top()), QPointF(x, area.bottom()));
            painter.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
        }
        painter.setPen(text);
        const qint64 timestamp = m_minimumTimeUs
            + static_cast<qint64>(
                (m_maximumTimeUs - m_minimumTimeUs) * ratio);
        painter.drawText(
            QRectF(x - 45, area.bottom() + 6, 90, 18),
            Qt::AlignHCenter | Qt::AlignTop,
            QDateTime::fromMSecsSinceEpoch(timestamp / 1000)
                .toString(QStringLiteral("HH:mm:ss")));
        const double value =
            m_minimumY + (m_maximumY - m_minimumY) * ratio;
        painter.drawText(
            QRectF(2, y - 9, area.left() - 8, 18),
            Qt::AlignRight | Qt::AlignVCenter,
            QString::number(value, 'g', 5));
    }
    painter.setPen(QPen(text, 1.2));
    painter.drawLine(area.bottomLeft(), area.bottomRight());
    painter.drawLine(area.bottomLeft(), area.topLeft());
}

void RealtimePlotWidget::drawCurves(
    QPainter &painter, const QRectF &area) const
{
    painter.save();
    painter.setClipRect(area);
    for (const PlotChannelStyle &style : m_styles) {
        if (!style.visible) continue;
        const QVector<ChannelSample> samples =
            m_visibleSamples.value(style.channelId);
        if (samples.isEmpty()) continue;
        painter.setPen(QPen(style.color, style.lineWidth));
        if (m_renderMode == PlotRenderMode::Points) {
            for (const ChannelSample &sample : samples) {
                painter.drawEllipse(mapSample(sample, area), 2.2, 2.2);
            }
            continue;
        }
        QPainterPath path;
        path.moveTo(mapSample(samples.constFirst(), area));
        for (qsizetype index = 1; index < samples.size(); ++index) {
            path.lineTo(mapSample(samples.at(index), area));
        }
        painter.drawPath(path);
    }
    painter.restore();
}

void RealtimePlotWidget::drawLegend(
    QPainter &painter, const QRectF &area) const
{
    if (!m_showLegend) return;
    const QList<ChannelDescriptor> channels = m_hub->channels();
    QHash<QString, ChannelDescriptor> byId;
    for (const ChannelDescriptor &channel : channels) {
        byId.insert(channel.id, channel);
    }
    int y = static_cast<int>(area.top()) + 8;
    for (const PlotChannelStyle &style : m_styles) {
        if (!style.visible) continue;
        const ChannelDescriptor channel = byId.value(style.channelId);
        const QString label = channel.id.isEmpty()
            ? tr("%1（未绑定）").arg(style.channelId)
            : channel.unit.isEmpty()
                ? channel.displayName
                : QStringLiteral("%1 [%2]")
                    .arg(channel.displayName, channel.unit);
        painter.setPen(QPen(style.color, 3));
        painter.drawLine(
            QPointF(area.left() + 10, y + 7),
            QPointF(area.left() + 28, y + 7));
        painter.setPen(palette().color(QPalette::Text));
        painter.drawText(
            QPointF(area.left() + 34, y + 11), label);
        y += 18;
    }
}

void RealtimePlotWidget::drawCrosshair(
    QPainter &painter, const QRectF &area) const
{
    if (!m_showCrosshair || !m_mouseInside
        || !area.contains(m_mousePosition)) {
        return;
    }
    QColor color = palette().color(QPalette::Text);
    color.setAlpha(150);
    painter.setPen(QPen(color, 1, Qt::DashLine));
    painter.drawLine(
        QPointF(m_mousePosition.x(), area.top()),
        QPointF(m_mousePosition.x(), area.bottom()));
    painter.drawLine(
        QPointF(area.left(), m_mousePosition.y()),
        QPointF(area.right(), m_mousePosition.y()));
    const qint64 timestamp = m_minimumTimeUs
        + static_cast<qint64>(
            (m_mousePosition.x() - area.left()) / area.width()
            * (m_maximumTimeUs - m_minimumTimeUs));
    QStringList values;
    values.append(
        QDateTime::fromMSecsSinceEpoch(timestamp / 1000)
            .toString(QStringLiteral("HH:mm:ss.zzz")));
    for (const PlotChannelStyle &style : m_styles) {
        const QVector<ChannelSample> samples =
            m_visibleSamples.value(style.channelId);
        if (samples.isEmpty()) continue;
        auto nearest = std::lower_bound(
            samples.cbegin(), samples.cend(), timestamp,
            [](const ChannelSample &sample, const qint64 time) {
                return sample.timestampUs < time;
            });
        if (nearest == samples.cend()) --nearest;
        if (nearest != samples.cbegin()) {
            const auto previous = nearest - 1;
            if (std::abs(previous->timestampUs - timestamp)
                < std::abs(nearest->timestampUs - timestamp)) {
                nearest = previous;
            }
        }
        values.append(QStringLiteral("%1=%2")
            .arg(style.channelId.section(u'/', -1),
                 QString::number(nearest->value, 'g', 6)));
    }
    painter.setPen(palette().color(QPalette::Text));
    painter.drawText(
        QRectF(
            area.left() + 6, area.bottom() - 38,
            area.width() - 12, 34),
        Qt::AlignLeft | Qt::AlignBottom,
        values.join(QStringLiteral("  ")));
}

void RealtimePlotWidget::placeTracer(Tracer *tracer, const qreal x)
{
    if (!tracer || m_visibleSamples.isEmpty()) return;
    const QRectF area = plotRect();
    const qreal boundedX = qBound(area.left(), x, area.right());
    tracer->active = true;
    tracer->positionRatio =
        (boundedX - area.left()) / qMax<qreal>(1.0, area.width());
    refreshTracer(tracer);
    update();
}

void RealtimePlotWidget::refreshTracer(Tracer *tracer)
{
    if (!tracer || !tracer->active || m_visibleSamples.isEmpty()) return;
    const qint64 target = m_minimumTimeUs
        + static_cast<qint64>(
            qBound<qreal>(0.0, tracer->positionRatio, 1.0)
            * (m_maximumTimeUs - m_minimumTimeUs));
    qint64 bestTimestamp = 0;
    qint64 bestDistance = (std::numeric_limits<qint64>::max)();
    for (auto it = m_visibleSamples.cbegin();
         it != m_visibleSamples.cend(); ++it) {
        for (const ChannelSample &sample : it.value()) {
            const qint64 distance = std::abs(sample.timestampUs - target);
            if (distance < bestDistance) {
                bestDistance = distance;
                bestTimestamp = sample.timestampUs;
            }
        }
    }
    if (bestTimestamp == 0) return;
    tracer->timestampUs = bestTimestamp;
    tracer->values.clear();
    for (auto it = m_visibleSamples.cbegin();
         it != m_visibleSamples.cend(); ++it) {
        const QVector<ChannelSample> &samples = it.value();
        if (samples.isEmpty()) continue;
        auto nearest = std::lower_bound(
            samples.cbegin(), samples.cend(), bestTimestamp,
            [](const ChannelSample &sample, const qint64 time) {
                return sample.timestampUs < time;
            });
        if (nearest == samples.cend()) --nearest;
        if (nearest != samples.cbegin()) {
            const auto previous = nearest - 1;
            if (std::abs(previous->timestampUs - bestTimestamp)
                < std::abs(nearest->timestampUs - bestTimestamp)) {
                nearest = previous;
            }
        }
        tracer->values.insert(it.key(), nearest->value);
    }
}

void RealtimePlotWidget::drawLatestLine(
    QPainter &painter, const QRectF &area) const
{
    if (m_latestTimeUs <= 0 || m_maximumTimeUs <= m_minimumTimeUs) return;
    const qreal ratio = m_followLatest
        ? m_latestLineRatio
        : (m_latestTimeUs - m_minimumTimeUs)
            / static_cast<double>(m_maximumTimeUs - m_minimumTimeUs);
    if (ratio < 0.0 || ratio > 1.0) return;
    const qreal x = area.left() + ratio * area.width();
    QColor color(QStringLiteral("#28A9E0"));
    color.setAlpha(210);
    painter.setPen(QPen(color, 1.5, Qt::DashLine));
    painter.drawLine(QPointF(x, area.top()), QPointF(x, area.bottom()));
}

QString RealtimePlotWidget::tracerText() const
{
    QStringList lines;
    const auto appendTracer =
        [this, &lines](const QString &name, const Tracer &tracer) {
        if (!tracer.active) return;
        lines.append(QStringLiteral("%1  %2")
            .arg(name,
                 QDateTime::fromMSecsSinceEpoch(tracer.timestampUs / 1000)
                     .toString(QStringLiteral("HH:mm:ss.zzz"))));
        for (const PlotChannelStyle &style : m_styles) {
            if (!style.visible
                || !tracer.values.contains(style.channelId)) {
                continue;
            }
            lines.append(QStringLiteral("  %1=%2")
                .arg(style.channelId.section(u'/', -1),
                     QString::number(
                         tracer.values.value(style.channelId), 'g', 7)));
        }
    };
    appendTracer(QStringLiteral("A"), m_tracerA);
    appendTracer(QStringLiteral("B"), m_tracerB);
    if (m_tracerA.active && m_tracerB.active) {
        const double seconds =
            (m_tracerB.timestampUs - m_tracerA.timestampUs) / 1000000.0;
        lines.append(QStringLiteral("Δt=%1 s").arg(seconds, 0, 'g', 7));
        if (!qFuzzyIsNull(seconds)) {
            lines.append(
                QStringLiteral("1/Δt=%1 Hz").arg(1.0 / std::abs(seconds), 0, 'g', 7));
        }
        for (const PlotChannelStyle &style : m_styles) {
            if (!m_tracerA.values.contains(style.channelId)
                || !m_tracerB.values.contains(style.channelId)) {
                continue;
            }
            lines.append(QStringLiteral("Δ%1=%2")
                .arg(style.channelId.section(u'/', -1),
                     QString::number(
                         m_tracerB.values.value(style.channelId)
                         - m_tracerA.values.value(style.channelId),
                         'g', 7)));
        }
    }
    return lines.join(u'\n');
}

void RealtimePlotWidget::drawTracers(
    QPainter &painter, const QRectF &area) const
{
    const auto xFor = [&area](const Tracer &tracer) {
        return area.left()
            + qBound<qreal>(0.0, tracer.positionRatio, 1.0) * area.width();
    };
    if (m_tracerA.active && m_tracerB.active) {
        const qreal left = qMin(xFor(m_tracerA), xFor(m_tracerB));
        const qreal right = qMax(xFor(m_tracerA), xFor(m_tracerB));
        painter.fillRect(
            QRectF(left, area.top(), right - left, area.height()),
            QColor(40, 169, 224, 35));
    }
    const auto draw = [this, &painter, &area, &xFor](
                          const Tracer &tracer,
                          const QString &name,
                          const QColor &color) {
        if (!tracer.active) return;
        const qreal x = xFor(tracer);
        painter.setPen(QPen(color, 2));
        painter.drawLine(QPointF(x, area.top()), QPointF(x, area.bottom()));
        painter.drawText(QPointF(x + 4, area.top() + 14), name);

        for (const PlotChannelStyle &style : m_styles) {
            if (!style.visible || !tracer.values.contains(style.channelId)) {
                continue;
            }
            const double value = tracer.values.value(style.channelId);
            const double yRange = qMax(1.0e-12, m_maximumY - m_minimumY);
            const qreal y = area.bottom()
                - (value - m_minimumY) / yRange * area.height();
            if (y < area.top() || y > area.bottom()) continue;

            const QPointF intersection(x, y);
            const qreal markerRadius = 4.5;
            painter.setPen(QPen(style.color, 1.8));
            painter.setBrush(palette().color(QPalette::Base));
            switch (m_tracerMarker) {
            case TracerMarker::Cross:
                painter.drawLine(
                    intersection + QPointF(-markerRadius, 0),
                    intersection + QPointF(markerRadius, 0));
                painter.drawLine(
                    intersection + QPointF(0, -markerRadius),
                    intersection + QPointF(0, markerRadius));
                break;
            case TracerMarker::Circle:
                painter.drawEllipse(intersection, markerRadius, markerRadius);
                break;
            case TracerMarker::Diamond: {
                QPolygonF diamond;
                diamond << intersection + QPointF(0, -markerRadius)
                        << intersection + QPointF(markerRadius, 0)
                        << intersection + QPointF(0, markerRadius)
                        << intersection + QPointF(-markerRadius, 0);
                painter.drawPolygon(diamond);
                break;
            }
            }

            if (m_showTracerLeaders) {
                QColor leaderColor = style.color;
                leaderColor.setAlpha(180);
                painter.setPen(QPen(leaderColor, 1, Qt::DashLine));
                painter.drawLine(intersection, QPointF(area.left(), y));
                painter.setPen(style.color);
                painter.drawText(
                    QRectF(2, y - 9, area.left() - 7, 18),
                    Qt::AlignRight | Qt::AlignVCenter,
                    QString::number(value, 'g', 6));
            }
        }
    };
    draw(m_tracerA, QStringLiteral("A"), QColor("#F59E0B"));
    draw(m_tracerB, QStringLiteral("B"), QColor("#E5484D"));
    const QString info = tracerText();
    if (info.isEmpty()) return;
    const QRectF box(
        area.right() - 210, area.top() + 6, 204,
        qMin<qreal>(area.height() - 12, 38 + info.count(u'\n') * 16));
    painter.fillRect(box, QColor(0, 0, 0, 120));
    painter.setPen(Qt::white);
    painter.drawText(box.adjusted(6, 4, -4, -4), Qt::AlignLeft, info);
}

void RealtimePlotWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton
        || !plotRect().contains(event->position())) {
        QWidget::mousePressEvent(event);
        return;
    }
    m_pressPosition = event->position().toPoint();
    m_mousePosition = event->position();
    if (m_followLatest && m_latestTimeUs > 0) {
        const qreal latestX = plotRect().left()
            + m_latestLineRatio * plotRect().width();
        if (std::abs(latestX - m_mousePosition.x()) <= 7.0) {
            m_draggingLatestLine = true;
            event->accept();
            return;
        }
    }
    const auto tracerDistance = [this](const Tracer &tracer) {
        if (!tracer.active) return 1000.0;
        const QRectF area = plotRect();
        const qreal x = area.left()
            + qBound<qreal>(0.0, tracer.positionRatio, 1.0)
                * area.width();
        return std::abs(x - m_mousePosition.x());
    };
    if (tracerDistance(m_tracerA) <= 7.0) {
        m_draggedTracer = &m_tracerA;
    } else if (tracerDistance(m_tracerB) <= 7.0) {
        m_draggedTracer = &m_tracerB;
    } else {
        m_panning = true;
    }
    event->accept();
}

void RealtimePlotWidget::mouseMoveEvent(QMouseEvent *event)
{
    m_mouseInside = true;
    const QPointF previous = m_mousePosition;
    m_mousePosition = event->position();
    if (m_draggingLatestLine) {
        const QRectF area = plotRect();
        m_latestLineRatio = qBound<qreal>(
            0.0,
            (m_mousePosition.x() - area.left())
                / qMax<qreal>(1.0, area.width()),
            1.0);
        refreshData();
        return;
    }
    if (m_draggedTracer) {
        placeTracer(m_draggedTracer, m_mousePosition.x());
        return;
    }
    if (m_panning && (event->buttons() & Qt::LeftButton)) {
        const QRectF area = plotRect();
        const QPointF delta = m_mousePosition - previous;
        const qint64 timeDelta = static_cast<qint64>(
            -delta.x() / area.width()
            * (m_maximumTimeUs - m_minimumTimeUs));
        m_minimumTimeUs += timeDelta;
        m_maximumTimeUs += timeDelta;
        const double yDelta = delta.y() / area.height()
            * (m_maximumY - m_minimumY);
        m_minimumY += yDelta;
        m_maximumY += yDelta;
        m_followLatest = false;
        m_autoY = false;
        refreshData();
        return;
    }
    update();
}

void RealtimePlotWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_panning = false;
        m_draggingLatestLine = false;
        m_draggedTracer = nullptr;
    }
    QWidget::mouseReleaseEvent(event);
}

void RealtimePlotWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton
        && plotRect().contains(event->position())) {
        m_panning = false;
        m_draggingLatestLine = false;
        m_draggedTracer = nullptr;
        if (!m_tracerA.active) {
            placeTracer(&m_tracerA, event->position().x());
        } else if (!m_tracerB.active) {
            placeTracer(&m_tracerB, event->position().x());
        } else {
            m_tracerA = {};
            m_tracerB = {};
            update();
        }
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void RealtimePlotWidget::wheelEvent(QWheelEvent *event)
{
    const QRectF area = plotRect();
    const QPointF position = event->position();
    const bool overPlot = area.contains(position);
    const bool overHorizontalAxis =
        position.x() >= area.left() && position.x() <= area.right()
        && position.y() > area.bottom() && position.y() <= rect().bottom();
    const bool overVerticalAxis =
        position.x() >= rect().left() && position.x() < area.left()
        && position.y() >= area.top() && position.y() <= area.bottom();
    if (!overPlot && !overHorizontalAxis && !overVerticalAxis) {
        QWidget::wheelEvent(event);
        return;
    }
    const double factor = event->angleDelta().y() > 0 ? 0.8 : 1.25;
    const bool scaleX = overHorizontalAxis || overPlot;
    const bool scaleY = overVerticalAxis || overPlot;
    if (scaleX) {
        double ratio =
            qBound(0.0, (position.x() - area.left()) / area.width(), 1.0);
        qint64 anchor = m_minimumTimeUs
            + static_cast<qint64>(ratio
                * (m_maximumTimeUs - m_minimumTimeUs));
        if (m_latestTimeUs >= m_minimumTimeUs
            && m_latestTimeUs <= m_maximumTimeUs) {
            anchor = m_latestTimeUs;
            ratio = m_followLatest
                ? m_latestLineRatio
                : (m_latestTimeUs - m_minimumTimeUs)
                    / static_cast<double>(qMax<qint64>(
                        1, m_maximumTimeUs - m_minimumTimeUs));
        }
        const qint64 newDuration = qBound<qint64>(
            10000,
            static_cast<qint64>(
                (m_maximumTimeUs - m_minimumTimeUs) * factor),
            3600000000LL);
        m_minimumTimeUs =
            anchor - static_cast<qint64>(newDuration * ratio);
        m_maximumTimeUs = m_minimumTimeUs + newDuration;
        m_windowDurationUs = newDuration;
    }
    if (scaleY) {
        const double ratio =
            qBound(0.0, (area.bottom() - position.y()) / area.height(), 1.0);
        const double anchor =
            m_minimumY + ratio * (m_maximumY - m_minimumY);
        const double range = (m_maximumY - m_minimumY) * factor;
        m_minimumY = anchor - range * ratio;
        m_maximumY = m_minimumY + range;
        m_autoY = false;
    }
    refreshData();
    event->accept();
}

void RealtimePlotWidget::contextMenuEvent(QContextMenuEvent *event)
{
    m_mousePosition = event->pos();
    QMenu menu(this);
    menu.setObjectName(QStringLiteral("plotContextMenu"));
    menu.setPalette(PlotTheme::palette(
        m_context->themeManager()->mode()));
    QAction *configure = menu.addAction(tr("通道配置"));
    menu.addSeparator();
    QAction *pause = menu.addAction(m_paused ? tr("继续") : tr("暂停"));
    QAction *follow = menu.addAction(tr("跟随最新数据"));
    follow->setCheckable(true);
    follow->setChecked(m_followLatest);
    QAction *autoY = menu.addAction(tr("自动调整 Y 轴"));
    autoY->setCheckable(true);
    autoY->setChecked(m_autoY);
    QAction *fit = menu.addAction(tr("单次适配 Y 轴"));
    QAction *reset = menu.addAction(tr("恢复视图"));
    QMenu *mode = menu.addMenu(tr("绘图模式"));
    QAction *line = mode->addAction(tr("线图"));
    QAction *points = mode->addAction(tr("点图"));
    line->setCheckable(true);
    points->setCheckable(true);
    QActionGroup group(mode);
    group.setExclusive(true);
    group.addAction(line);
    group.addAction(points);
    line->setChecked(m_renderMode == PlotRenderMode::Line);
    points->setChecked(m_renderMode == PlotRenderMode::Points);
    QAction *legend = menu.addAction(tr("显示图例"));
    legend->setCheckable(true);
    legend->setChecked(m_showLegend);
    QAction *grid = menu.addAction(tr("显示网格"));
    grid->setCheckable(true);
    grid->setChecked(m_showGrid);
    QAction *cross = menu.addAction(tr("显示十字光标"));
    cross->setCheckable(true);
    cross->setChecked(m_showCrosshair);
    QMenu *tracers = menu.addMenu(tr("Tracer 设置"));
    QMenu *markerMenu = tracers->addMenu(tr("交点标记"));
    QAction *crossMarker = markerMenu->addAction(tr("十字形"));
    QAction *circleMarker = markerMenu->addAction(tr("圆圈"));
    QAction *diamondMarker = markerMenu->addAction(tr("菱形"));
    QActionGroup markerGroup(markerMenu);
    markerGroup.setExclusive(true);
    for (QAction *action : {crossMarker, circleMarker, diamondMarker}) {
        action->setCheckable(true);
        markerGroup.addAction(action);
    }
    crossMarker->setChecked(m_tracerMarker == TracerMarker::Cross);
    circleMarker->setChecked(m_tracerMarker == TracerMarker::Circle);
    diamondMarker->setChecked(m_tracerMarker == TracerMarker::Diamond);
    QAction *showLeaders = tracers->addAction(tr("显示纵轴数据引出线"));
    showLeaders->setCheckable(true);
    showLeaders->setChecked(m_showTracerLeaders);
    menu.addSeparator();
    QAction *copy = menu.addAction(tr("复制截图"));
    QAction *save = menu.addAction(tr("保存截图"));
    menu.addSeparator();
    QAction *remove = menu.addAction(tr("删除控件"));
    QAction *selected = menu.exec(event->globalPos());
    if (selected == configure) openChannelDialog();
    else if (selected == pause) setPaused(!m_paused);
    else if (selected == follow) {
        m_followLatest = follow->isChecked();
        refreshData();
    } else if (selected == autoY) {
        m_autoY = autoY->isChecked();
        if (m_autoY) fitYOnce();
    } else if (selected == fit) fitYOnce();
    else if (selected == reset) resetView();
    else if (selected == line) {
        m_renderMode = PlotRenderMode::Line;
        update();
    } else if (selected == points) {
        m_renderMode = PlotRenderMode::Points;
        update();
    } else if (selected == legend) {
        m_showLegend = legend->isChecked();
        update();
    } else if (selected == grid) {
        m_showGrid = grid->isChecked();
        update();
    } else if (selected == cross) {
        m_showCrosshair = cross->isChecked();
        update();
    } else if (selected == crossMarker) {
        m_tracerMarker = TracerMarker::Cross;
        update();
    } else if (selected == circleMarker) {
        m_tracerMarker = TracerMarker::Circle;
        update();
    } else if (selected == diamondMarker) {
        m_tracerMarker = TracerMarker::Diamond;
        update();
    } else if (selected == showLeaders) {
        m_showTracerLeaders = showLeaders->isChecked();
        update();
    } else if (selected == copy) copyScreenshot();
    else if (selected == save) saveScreenshot();
    else if (selected == remove) emit deleteRequested();
}

void RealtimePlotWidget::leaveEvent(QEvent *event)
{
    m_mouseInside = false;
    update();
    QWidget::leaveEvent(event);
}

void RealtimePlotWidget::copyScreenshot()
{
    QApplication::clipboard()->setPixmap(grab());
}

void RealtimePlotWidget::saveScreenshot()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("保存绘图截图"), QString(),
        tr("PNG 图片 (*.png);;JPEG 图片 (*.jpg *.jpeg)"));
    if (!path.isEmpty()) grab().save(path);
}
