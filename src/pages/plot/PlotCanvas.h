#pragma once

#include <QWidget>

class AppContext;
class DashboardItem;
class RealtimePlotWidget;

class PlotCanvas final : public QWidget
{
    Q_OBJECT

public:
    explicit PlotCanvas(AppContext *context, QWidget *parent = nullptr);
    [[nodiscard]] QList<RealtimePlotWidget *> plots() const;
    [[nodiscard]] bool editMode() const noexcept;

public slots:
    RealtimePlotWidget *addRealtimePlot();
    void setEditMode(bool enabled);
    void setPaused(bool paused);
    void resetPlots();
    void fitPlotsY();

signals:
    void plotAdded(RealtimePlotWidget *plot);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    [[nodiscard]] QRect nextItemGeometry() const;

    AppContext *m_context{};
    QList<DashboardItem *> m_items;
    bool m_editMode{};
    bool m_paused{};
};
