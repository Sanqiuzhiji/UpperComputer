#pragma once

#include <QHash>
#include <QPointer>
#include <QTabWidget>

class AppContext;
class PlotFloatingWindow;
class PlotWorkspacePage;

class DetachableTabWidget final : public QTabWidget
{
    Q_OBJECT

public:
    explicit DetachableTabWidget(
        AppContext *context, QWidget *parent = nullptr);
    ~DetachableTabWidget() override;

    PlotWorkspacePage *addWorkspace(const QString &title = {});
    [[nodiscard]] PlotWorkspacePage *currentWorkspace() const;
    [[nodiscard]] int workspaceCount() const;

signals:
    void currentWorkspaceChanged(PlotWorkspacePage *workspace);

private:
    [[nodiscard]] bool hasWorkspaceTitle(
        const QString &title, const QWidget *excludedPage = nullptr) const;
    void installCloseButton(int index, QWidget *page);
    void closeWorkspace(int index);
    void renameWorkspace(int index);
    void detachWorkspace(int index, const QPoint &globalPosition);
    void attachWorkspace(QWidget *page, const QString &title);

    AppContext *m_context{};
    int m_pageCounter{};
    QHash<QWidget *, QPointer<PlotFloatingWindow>> m_windows;
    bool m_shuttingDown{};
};
