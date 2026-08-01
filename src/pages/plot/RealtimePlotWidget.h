#pragma once

#include <QHash>
#include <QWidget>

#include "models/TelemetryTypes.h"
#include "pages/plot/PlotTypes.h"

class AppContext;
class ChannelDataHub;
class QTimer;

class RealtimePlotWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit RealtimePlotWidget(
        AppContext *context, QWidget *parent = nullptr);
    [[nodiscard]] QList<PlotChannelStyle> channelStyles() const;
    void setChannelStyles(const QList<PlotChannelStyle> &styles);
    [[nodiscard]] bool paused() const noexcept;
    [[nodiscard]] bool followingLatest() const noexcept;

public slots:
    void setPaused(bool paused);
    void resetView();
    void fitYOnce();
    void openChannelDialog();

signals:
    void deleteRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    enum class TracerMarker {
        Cross,
        Circle,
        Diamond
    };

    struct Tracer {
        bool active{};
        qreal positionRatio{};
        qint64 timestampUs{};
        QHash<QString, double> values;
    };

    void refreshData();
    void updateYRange(bool immediate);
    [[nodiscard]] QRectF plotRect() const;
    [[nodiscard]] QPointF mapSample(
        const ChannelSample &sample, const QRectF &area) const;
    [[nodiscard]] QVector<ChannelSample> downsample(
        const QVector<ChannelSample> &samples, int pixelWidth) const;
    void placeTracer(Tracer *tracer, qreal x);
    void refreshTracer(Tracer *tracer);
    [[nodiscard]] QString tracerText() const;
    void drawAxes(QPainter &painter, const QRectF &area) const;
    void drawCurves(QPainter &painter, const QRectF &area) const;
    void drawLegend(QPainter &painter, const QRectF &area) const;
    void drawCrosshair(QPainter &painter, const QRectF &area) const;
    void drawLatestLine(QPainter &painter, const QRectF &area) const;
    void drawTracers(QPainter &painter, const QRectF &area) const;
    void saveScreenshot();
    void copyScreenshot();

    AppContext *m_context{};
    ChannelDataHub *m_hub{};
    QTimer *m_refreshTimer{};
    QList<PlotChannelStyle> m_styles;
    QHash<QString, QVector<ChannelSample>> m_visibleSamples;
    qint64 m_minimumTimeUs{};
    qint64 m_maximumTimeUs{};
    qint64 m_latestTimeUs{};
    qint64 m_windowDurationUs{10000000};
    double m_minimumY{-1.0};
    double m_maximumY{1.0};
    bool m_paused{};
    bool m_followLatest{true};
    bool m_autoY{true};
    bool m_showGrid{true};
    bool m_showLegend{true};
    bool m_showCrosshair{true};
    PlotRenderMode m_renderMode{PlotRenderMode::Line};
    QPointF m_mousePosition;
    QPoint m_pressPosition;
    bool m_mouseInside{};
    bool m_panning{};
    bool m_draggingLatestLine{};
    Tracer *m_draggedTracer{};
    Tracer m_tracerA;
    Tracer m_tracerB;
    qreal m_latestLineRatio{1.0};
    TracerMarker m_tracerMarker{TracerMarker::Cross};
    bool m_showTracerLeaders{true};
};
