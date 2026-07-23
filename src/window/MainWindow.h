#pragma once

#include <QMainWindow>
#include <QHash>

enum class PageId;
class QStackedWidget;
class QCloseEvent;
class ThemeManager;
class TitleBar;
class SideNavigation;
class StatusBarWidget;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(ThemeManager *themeManager, QWidget *parent = nullptr);

protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#endif

private slots:
    void switchPage(PageId page, const QString &title);
    void toggleMaximized();
    void setPinned(bool pinned);
    void applyNavigationMode(const QString &mode);
    void showNotice(const QString &message);

private:
    void createUi();
    void createPages();
    void addPage(PageId id, QWidget *page);
    void updateWindowStateUi();
    void restoreWindowSettings();

    ThemeManager *m_themeManager;
    TitleBar *m_titleBar{};
    SideNavigation *m_navigation{};
    QStackedWidget *m_pages{};
    StatusBarWidget *m_statusBar{};
    QWidget *m_notice{};
    QHash<PageId, int> m_pageIndexes;
};
