#pragma once

#include <QWidget>

class AppContext;
class DetachableTabWidget;
class PlotToolBar;
class PlotWorkspacePage;
class QShowEvent;

class PlotPage final : public QWidget
{
    Q_OBJECT

public:
    explicit PlotPage(AppContext *context, QWidget *parent = nullptr);
    [[nodiscard]] DetachableTabWidget *tabWidget() const noexcept;

public slots:
    PlotWorkspacePage *addPage();
    void addRealtimePlot();
    void saveCurrentPage();
    void importPage();

protected:
    void showEvent(QShowEvent *event) override;

private:
    [[nodiscard]] PlotWorkspacePage *currentWorkspace() const;
    bool loadPageFile(const QString &path, bool notifySuccess);
    void loadDefaultPages();

    AppContext *m_context{};

    PlotToolBar *m_toolBar{};
    DetachableTabWidget *m_tabs{};
    bool m_defaultPagesLoaded{};
};
