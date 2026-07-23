#pragma once

#include <QMainWindow>
#include <QHash>
#include <QList>

#include "widgets/ToastWidget.h"

enum class PageId;
enum class ThemeMode;
class QStackedWidget;
class QCloseEvent;
class ThemeManager;
class IconManager;
class TitleBar;
class SideNavigation;
class StatusBarWidget;
class ThemeTransitionOverlay;

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
    void showNotice(const QString &message,
                    ToastWidget::Type type = ToastWidget::Type::Information);
    void toggleThemeWithTransition();
    void startThemeTransition(ThemeMode targetMode);

private:
    void createUi();
    void createPages();
    void addPage(PageId id, QWidget *page);
    void updateWindowStateUi();
    void restoreWindowSettings();
    void cancelThemeTransition();
    void layoutToasts(bool animated);
    void dismissToast(ToastWidget *toast);

    ThemeManager *m_themeManager;
    IconManager *m_iconManager{};
    QWidget *m_root{};
    TitleBar *m_titleBar{};
    SideNavigation *m_navigation{};
    QStackedWidget *m_pages{};
    StatusBarWidget *m_statusBar{};
    ThemeTransitionOverlay *m_themeOverlay{};
    bool m_themeTransitionActive{};
    QList<ToastWidget *> m_toasts;
    QHash<PageId, int> m_pageIndexes;
};
