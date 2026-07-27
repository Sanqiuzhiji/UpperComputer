#include "DashboardItem.h"

#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

namespace {
constexpr int kGrid = 20;
constexpr int kHandle = 14;
constexpr int kMinimumWidth = 320;
constexpr int kMinimumHeight = 220;
}

DashboardItem::DashboardItem(QWidget *content, QWidget *parent)
    : QFrame(parent),
      m_content(content)
{
    setObjectName(QStringLiteral("plotDashboardItem"));
    setProperty("card", true);
    setMouseTracking(true);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(content);
    content->installEventFilter(this);
}

void DashboardItem::setEditMode(const bool enabled)
{
    if (!enabled) endInteraction();
    m_editMode = enabled;
    if (m_content != nullptr) {
        m_content->setCursor(
            enabled ? Qt::SizeAllCursor : Qt::ArrowCursor);
    }
    setCursor(enabled ? Qt::SizeAllCursor : Qt::ArrowCursor);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    update();
}

QWidget *DashboardItem::content() const noexcept
{
    return m_content;
}

bool DashboardItem::eventFilter(QObject *watched, QEvent *event)
{
    if (!m_editMode || watched != m_content) {
        return QFrame::eventFilter(watched, event);
    }
    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() != Qt::LeftButton) break;
        beginInteraction(
            mouse->globalPosition().toPoint(),
            mapFromGlobal(mouse->globalPosition().toPoint()));
        return true;
    }
    case QEvent::MouseMove: {
        auto *mouse = static_cast<QMouseEvent *>(event);
        if (m_dragging || m_resizing) {
            continueInteraction(mouse->globalPosition().toPoint());
        } else {
            const QPoint local =
                mapFromGlobal(mouse->globalPosition().toPoint());
            setCursor(onResizeHandle(local)
                ? Qt::SizeFDiagCursor : Qt::SizeAllCursor);
        }
        return true;
    }
    case QEvent::MouseButtonRelease: {
        auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() != Qt::LeftButton) break;
        endInteraction();
        return true;
    }
    default:
        break;
    }
    return QFrame::eventFilter(watched, event);
}

void DashboardItem::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);
    if (!m_editMode) return;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(QStringLiteral("#28A9E0")), 2));
    painter.drawRoundedRect(rect().adjusted(1, 1, -2, -2), 6, 6);
    painter.fillRect(
        QRect(width() - kHandle, height() - kHandle, kHandle, kHandle),
        QColor(QStringLiteral("#28A9E0")));
}

void DashboardItem::mousePressEvent(QMouseEvent *event)
{
    if (!m_editMode || event->button() != Qt::LeftButton) {
        QFrame::mousePressEvent(event);
        return;
    }
    beginInteraction(
        event->globalPosition().toPoint(),
        event->position().toPoint());
    event->accept();
}

void DashboardItem::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_editMode || (!m_dragging && !m_resizing)) {
        if (m_editMode) {
            setCursor(onResizeHandle(event->position().toPoint())
                ? Qt::SizeFDiagCursor : Qt::SizeAllCursor);
        }
        QFrame::mouseMoveEvent(event);
        return;
    }
    continueInteraction(event->globalPosition().toPoint());
    event->accept();
}

void DashboardItem::beginInteraction(
    const QPoint &globalPosition, const QPoint &localPosition)
{
    m_pressGlobal = globalPosition;
    m_startGeometry = geometry();
    m_resizing = onResizeHandle(localPosition);
    m_dragging = !m_resizing;
    grabMouse();
}

void DashboardItem::continueInteraction(const QPoint &globalPosition)
{
    const QPoint delta = globalPosition - m_pressGlobal;
    QRect candidate = m_startGeometry;
    if (m_resizing) {
        candidate.setSize(QSize(
            snap(qMax(kMinimumWidth, candidate.width() + delta.x())),
            snap(qMax(kMinimumHeight, candidate.height() + delta.y()))));
    } else {
        candidate.moveTo(
            snap(candidate.x() + delta.x()),
            snap(candidate.y() + delta.y()));
    }
    setGeometry(boundedGeometry(candidate));
}

void DashboardItem::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragging || m_resizing) {
        endInteraction();
        event->accept();
        return;
    }
    QFrame::mouseReleaseEvent(event);
}

void DashboardItem::endInteraction()
{
    if (!m_dragging && !m_resizing) return;
    releaseMouse();
    m_dragging = false;
    m_resizing = false;
    emit geometryEdited(geometry());
}

bool DashboardItem::onResizeHandle(const QPoint &point) const
{
    return point.x() >= width() - kHandle
        && point.y() >= height() - kHandle;
}

QRect DashboardItem::boundedGeometry(const QRect &candidate) const
{
    const QRect area = parentWidget()->rect();
    QRect result = candidate;
    result.setWidth(qMin(result.width(), area.width()));
    result.setHeight(qMin(result.height(), area.height()));
    result.moveLeft(qBound(0, result.left(), area.width() - result.width()));
    result.moveTop(qBound(0, result.top(), area.height() - result.height()));
    return result;
}

int DashboardItem::snap(const int value) const
{
    return qRound(static_cast<qreal>(value) / kGrid) * kGrid;
}
