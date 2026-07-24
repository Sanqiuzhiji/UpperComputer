#pragma once

#include <QMainWindow>
#include <QHash>
#include <QList>

#include "models/AppTypes.h"
#include "widgets/ToastWidget.h"

enum class PageId;
class AppContext;
class AppSettings;
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
    explicit MainWindow(AppContext *context, QWidget *parent = nullptr);

protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#endif

private slots:
    void switchPage(PageId page);
    void toggleMaximized();
    void setPinned(bool pinned);
    void applyNavigationMode(NavigationMode mode);
    void showNotice(const QString &message,
                    ToastWidget::Type type = ToastWidget::Type::Information);
    void toggleThemeWithTransition();
    void startThemeTransition(ThemeMode targetMode);

private:
    void createUi();
    [[nodiscard]] QWidget *ensurePage(PageId id);
    void configurePage(PageId id, QWidget *page);
    void bindConnectionStatus();
    void updateWindowStateUi();
    void restoreWindowSettings();
    void cancelThemeTransition();
    void layoutToasts(bool animated);
    void dismissToast(ToastWidget *toast);

    AppContext *m_context;
    AppSettings *m_settings;
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
    QHash<PageId, QWidget *> m_createdPages;
};
