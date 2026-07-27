#pragma once

#include <QWidget>

class AppContext;
class DetachableTabWidget;
class PlotToolBar;
class PlotWorkspacePage;

class PlotPage final : public QWidget
{
    Q_OBJECT

public:
    explicit PlotPage(AppContext *context, QWidget *parent = nullptr);
    [[nodiscard]] DetachableTabWidget *tabWidget() const noexcept;

public slots:
    PlotWorkspacePage *addPage();
    void addRealtimePlot();

private:
    [[nodiscard]] PlotWorkspacePage *currentWorkspace() const;

    PlotToolBar *m_toolBar{};
    DetachableTabWidget *m_tabs{};
};
