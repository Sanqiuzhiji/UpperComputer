#include "MainWindow.h"

#include "app/AppContext.h"
#include "app/AppSettings.h"
#include "pages/PageRegistry.h"
#include "pages/SettingsPage.h"
#include "services/ConnectionManager.h"
#include "theme/IconManager.h"
#include "theme/ThemeManager.h"
#include "widgets/SideNavigation.h"
#include "widgets/StatusBarWidget.h"
#include "widgets/TitleBar.h"
#include "widgets/ThemeTransitionOverlay.h"
#include "widgets/ToastWidget.h"

#include <QEvent>
#include <QCloseEvent>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWindow>
#include <QPainter>

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
constexpr int kThemeTransitionDurationMs = 620;
constexpr int kToastDurationMs = 2000;
constexpr int kToastSpacing = 10;
constexpr int kToastRightMargin = 18;
constexpr int kToastBottomMargin = 16;
constexpr int kMaximumToastCount = 3;
}

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

MainWindow::MainWindow(AppContext *context, QWidget *parent)
    : QMainWindow(parent),
      m_context(context),
      m_settings(context->settings()),
      m_themeManager(context->themeManager())
{
    m_iconManager = new IconManager(m_themeManager, this);
    setWindowTitle(QStringLiteral("UpperComputer"));
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_NativeWindow);
    setMinimumSize(960, 640);
    resize(1440, 900);
    createUi();

    connect(m_titleBar, &TitleBar::navigationToggleRequested,
            m_navigation, &SideNavigation::toggleExpanded);
    connect(m_titleBar, &TitleBar::themeToggleRequested,
            this, &MainWindow::toggleThemeWithTransition);
    connect(m_titleBar, &TitleBar::minimizeRequested, this, &QWidget::showMinimized);
    connect(m_titleBar, &TitleBar::maximizeRestoreRequested,
            this, &MainWindow::toggleMaximized);
    connect(m_titleBar, &TitleBar::closeRequested, this, &QWidget::close);
    connect(m_titleBar, &TitleBar::pinToggleRequested, this, &MainWindow::setPinned);
    connect(m_navigation, &SideNavigation::pageRequested,
            this, &MainWindow::switchPage);
    connect(m_themeManager, &ThemeManager::themeChanged, this, [this](ThemeMode) {
        m_statusBar->setThemeMode(m_themeManager->mode());
    });

    m_titleBar->installEventFilter(this);
    m_statusBar->setThemeMode(m_themeManager->mode());
    bindConnectionStatus();
    m_navigation->setUserCardVisible(m_settings->userCardVisible());
    applyNavigationMode(m_settings->navigationMode());
    restoreWindowSettings();
    switchPage(m_settings->lastPage());
}

bool MainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        if (m_themeTransitionActive) {
            cancelThemeTransition();
        }
        updateWindowStateUi();
    }
    return QMainWindow::event(event);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_titleBar && event->type() == QEvent::MouseButtonPress) {
        const auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton && !isMaximized()) {
            // Child controls receive their own events; empty title-bar space starts
            // the native move loop and therefore keeps Windows snapping behavior.
            if (childAt(mapFromGlobal(mouseEvent->globalPosition().toPoint())) == m_titleBar) {
                windowHandle()->startSystemMove();
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    layoutToasts(false);
    // Auto mode is deliberately width-aware, while preserving the selected page.
    if (m_navigation->mode() == NavigationMode::Automatic) {
        m_navigation->setExpanded(width() >= 1150);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    cancelThemeTransition();
    m_settings->setWindowGeometry(saveGeometry());
    m_settings->setWindowMaximized(isMaximized());
    QMainWindow::closeEvent(event);
}

#ifdef Q_OS_WIN
bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(eventType)
    auto *msg = static_cast<MSG *>(message);
    if (msg->message != WM_NCHITTEST) {
        return QMainWindow::nativeEvent(eventType, message, result);
    }

    const QPoint global(GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam));
    const QPoint local = mapFromGlobal(global);
    const int border = isMaximized() ? 0 : 7;

    const bool left = local.x() < border;
    const bool right = local.x() >= width() - border;
    const bool top = local.y() < border;
    const bool bottom = local.y() >= height() - border;

    if (top && left) *result = HTTOPLEFT;
    else if (top && right) *result = HTTOPRIGHT;
    else if (bottom && left) *result = HTBOTTOMLEFT;
    else if (bottom && right) *result = HTBOTTOMRIGHT;
    else if (left) *result = HTLEFT;
    else if (right) *result = HTRIGHT;
    else if (top) *result = HTTOP;
    else if (bottom) *result = HTBOTTOM;
    else {
        const QPoint titlePoint = m_titleBar->mapFromGlobal(global);
        if (m_titleBar->rect().contains(titlePoint)
            && m_titleBar->childAt(titlePoint) == nullptr) {
            // HTCAPTION delegates move, double-click maximize, drag-to-restore,
            // work-area constraints and Windows snapping to the OS.
            *result = HTCAPTION;
            return true;
        }
        return QMainWindow::nativeEvent(eventType, message, result);
    }
    return true;
}
#endif

void MainWindow::switchPage(const PageId page)
{
    const PageDescriptor *descriptor = findPageDescriptor(page);
    QWidget *pageWidget = ensurePage(page);
    if (!descriptor || !pageWidget) {
        return;
    }
    m_pages->setCurrentWidget(pageWidget);
    m_navigation->setCurrentPage(page);
    m_statusBar->setCurrentPageTitle(descriptor->title);
    m_titleBar->setCurrentPageTitle(descriptor->title);
    m_settings->setLastPage(page);
}

void MainWindow::toggleMaximized()
{
    isMaximized() ? showNormal() : showMaximized();
}

void MainWindow::setPinned(const bool pinned)
{
#ifdef Q_OS_WIN
    const HWND insertAfter = pinned ? HWND_TOPMOST : HWND_NOTOPMOST;
    SetWindowPos(reinterpret_cast<HWND>(winId()), insertAfter,
                 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#else
    setWindowFlag(Qt::WindowStaysOnTopHint, pinned);
    show();
#endif
    showNotice(pinned ? tr("窗口已置顶") : tr("已取消窗口置顶"),
               pinned ? ToastWidget::Type::Success
                      : ToastWidget::Type::Information);
}

void MainWindow::applyNavigationMode(const NavigationMode mode)
{
    m_settings->setNavigationMode(mode);
    m_navigation->setMode(mode);
    if (mode == NavigationMode::Automatic) {
        m_navigation->setExpanded(width() >= 1150);
    }
}

void MainWindow::showNotice(const QString &message, const ToastWidget::Type type)
{
    if (m_toasts.size() >= kMaximumToastCount) {
        ToastWidget *oldest = m_toasts.takeFirst();
        oldest->hide();
        oldest->deleteLater();
    }
    auto *toast = new ToastWidget(
        m_iconManager, tr("状态提示"), message, type, m_root);
    connect(toast, &ToastWidget::closeRequested,
            this, &MainWindow::dismissToast);
    m_toasts.append(toast);
    toast->move(m_root->width() - toast->width() - kToastRightMargin,
                m_root->height());
    toast->show();
    toast->raise();
    layoutToasts(true);
    toast->startCountdown(kToastDurationMs);
}

void MainWindow::toggleThemeWithTransition()
{
    const ThemeMode target = m_themeManager->mode() == ThemeMode::Dark
        ? ThemeMode::Light : ThemeMode::Dark;
    startThemeTransition(target);
}

void MainWindow::startThemeTransition(const ThemeMode targetMode)
{
    if (m_themeTransitionActive) {
        // A rapid second request replaces the current transition instead of
        // stacking another full-window overlay.
        cancelThemeTransition();
    }
    if (targetMode == m_themeManager->mode() || isMinimized() || !isVisible()) {
        return;
    }

    m_themeTransitionActive = true;
    const QPixmap oldFrame = m_root->grab();
    const QPoint center = m_titleBar->themeButtonCenter(m_root);

    m_themeOverlay = new ThemeTransitionOverlay(
        oldFrame, oldFrame, center, m_root);
    m_themeOverlay->setGeometry(m_root->rect());
    m_themeOverlay->show();
    m_themeOverlay->raise();
    m_themeOverlay->repaint();
    m_themeManager->setMode(targetMode);

    if (!m_themeTransitionActive || !m_themeOverlay
        || isMinimized() || !isVisible()) {
        cancelThemeTransition();
        return;
    }

    // Render the new themed frame immediately; there is no scheduled delay.
    // This full-window snapshot transition is appropriate for the current
    // QWidget-only shell. Real-time plots or native OpenGL surfaces should
    // later use a background-layer transition instead of captured frames.
    m_themeOverlay->hide();
    QPixmap newFrame(m_root->size() * m_root->devicePixelRatioF());
    newFrame.setDevicePixelRatio(m_root->devicePixelRatioF());
    newFrame.fill(Qt::transparent);
    m_root->render(&newFrame);
    m_themeOverlay->setFrames(oldFrame, newFrame);
    m_themeOverlay->show();
    m_themeOverlay->raise();
    // Toasts are independent status surfaces and must remain above the
    // full-window theme transition overlay.
    for (ToastWidget *toast : std::as_const(m_toasts)) {
        toast->raise();
    }

    const QPointF origin(center);
    const QList<QPointF> corners{
        QPointF(0, 0), QPointF(m_root->width(), 0),
        QPointF(0, m_root->height()),
        QPointF(m_root->width(), m_root->height())
    };
    qreal maximumRadius = 0.0;
    for (const QPointF &corner : corners) {
        const qreal dx = corner.x() - origin.x();
        const qreal dy = corner.y() - origin.y();
        maximumRadius = (std::max)(maximumRadius, std::hypot(dx, dy));
    }

    connect(m_themeOverlay, &ThemeTransitionOverlay::transitionFinished,
            this, [this] {
                if (m_themeOverlay) {
                    m_themeOverlay->deleteLater();
                    m_themeOverlay = nullptr;
                }
                m_themeTransitionActive = false;
                showNotice(
                    tr("主题已切换为%1模式").arg(m_themeManager->modeName()),
                    m_themeManager->mode() == ThemeMode::Light
                        ? ToastWidget::Type::Success
                        : ToastWidget::Type::Information);
            });
    m_themeOverlay->start(maximumRadius + 2.0,
                          kThemeTransitionDurationMs);
}

void MainWindow::createUi()
{
    m_root = new QWidget(this);
    m_root->setObjectName(QStringLiteral("windowRoot"));
    setCentralWidget(m_root);

    auto *layout = new QVBoxLayout(m_root);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    m_titleBar = new TitleBar(m_iconManager, m_root);
    layout->addWidget(m_titleBar);

    auto *body = new QWidget(m_root);
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);
    m_navigation = new SideNavigation(m_iconManager, body);
    m_pages = new QStackedWidget(body);
    m_pages->setObjectName(QStringLiteral("pageArea"));
    bodyLayout->addWidget(m_navigation);
    bodyLayout->addWidget(m_pages, 1);
    layout->addWidget(body, 1);

    m_statusBar = new StatusBarWidget(m_root);
    layout->addWidget(m_statusBar);

}

QWidget *MainWindow::ensurePage(const PageId id)
{
    if (const auto iterator = m_createdPages.constFind(id);
        iterator != m_createdPages.cend()) {
        return iterator.value();
    }

    const PageDescriptor *descriptor = findPageDescriptor(id);
    if (!descriptor || !descriptor->factory) {
        showNotice(tr("找不到页面注册信息"), ToastWidget::Type::Error);
        return nullptr;
    }

    QWidget *page = nullptr;
    try {
        page = descriptor->factory(m_context, m_pages);
    } catch (...) {
        showNotice(tr("页面“%1”创建失败").arg(descriptor->title),
                   ToastWidget::Type::Error);
        return nullptr;
    }
    if (!page) {
        showNotice(tr("页面“%1”创建失败").arg(descriptor->title),
                   ToastWidget::Type::Error);
        return nullptr;
    }

    m_pages->addWidget(page);
    m_createdPages.insert(id, page);
    configurePage(id, page);
    return page;
}

void MainWindow::configurePage(const PageId id, QWidget *page)
{
    if (id != PageId::Settings) {
        return;
    }

    auto *settingsPage = qobject_cast<SettingsPage *>(page);
    if (!settingsPage) {
        return;
    }
    connect(settingsPage, &SettingsPage::userCardVisibilityChanged,
            this, [this](const bool visible) {
                m_settings->setUserCardVisible(visible);
                m_navigation->setUserCardVisible(visible);
            });
    connect(settingsPage, &SettingsPage::navigationModeChanged,
            this, &MainWindow::applyNavigationMode);
    connect(settingsPage, &SettingsPage::themeModeRequested,
            this, &MainWindow::startThemeTransition);
    connect(settingsPage, &SettingsPage::unavailableSettingRequested, this,
            [this](const QString &name) {
                showNotice(tr("%1将在后续版本实现").arg(name),
                           ToastWidget::Type::Warning);
            });
}

void MainWindow::bindConnectionStatus()
{
    ConnectionManager *connection = m_context->connectionManager();
    connect(connection, &ConnectionManager::stateChanged,
            m_statusBar, &StatusBarWidget::setConnectionState);
    connect(connection, &ConnectionManager::deviceNameChanged,
            m_statusBar, &StatusBarWidget::setDeviceName);
    connect(connection, &ConnectionManager::dataSourceNameChanged,
            m_statusBar, &StatusBarWidget::setDataSourceName);
    connect(connection, &ConnectionManager::receiveRateChanged,
            m_statusBar, &StatusBarWidget::setReceiveRate);
    connect(connection, &ConnectionManager::transmitRateChanged,
            m_statusBar, &StatusBarWidget::setTransmitRate);

    m_statusBar->setConnectionState(connection->state());
    m_statusBar->setDeviceName(connection->deviceName());
    m_statusBar->setDataSourceName(connection->dataSourceName());
    m_statusBar->setReceiveRate(connection->receiveRate());
    m_statusBar->setTransmitRate(connection->transmitRate());
}

void MainWindow::updateWindowStateUi()
{
    m_titleBar->setMaximized(isMaximized());
}

void MainWindow::restoreWindowSettings()
{
    const QByteArray geometry = m_settings->windowGeometry();
    if (geometry.isEmpty()) {
        resize(1440, 900);
    } else {
        restoreGeometry(geometry);
    }
    if (m_settings->windowMaximized()) {
        setWindowState(windowState() | Qt::WindowMaximized);
    }
}

void MainWindow::cancelThemeTransition()
{
    if (m_themeOverlay) {
        m_themeOverlay->hide();
        m_themeOverlay->deleteLater();
        m_themeOverlay = nullptr;
    }
    m_themeTransitionActive = false;
}

void MainWindow::layoutToasts(const bool animated)
{
    if (m_toasts.isEmpty() || !m_statusBar) {
        return;
    }
    const int availableBottom = m_root->height() - m_statusBar->height()
        - kToastBottomMargin;
    const int minimumTop = m_titleBar->height() + 8;
    const int availableHeight = qMax(0, availableBottom - minimumTop);
    const auto totalToastHeight = [this] {
        int total = (m_toasts.size() - 1) * kToastSpacing;
        for (const ToastWidget *toast : std::as_const(m_toasts)) {
            total += toast->height();
        }
        return total;
    };
    while (m_toasts.size() > 1 && totalToastHeight() > availableHeight) {
        ToastWidget *oldest = m_toasts.takeFirst();
        oldest->hide();
        oldest->deleteLater();
    }
    const int totalHeight = totalToastHeight();
    int y = qMax(minimumTop, availableBottom - totalHeight);

    for (ToastWidget *toast : std::as_const(m_toasts)) {
        const int x = qMax(
            8, m_root->width() - toast->width() - kToastRightMargin);
        const QRect target(x, y, toast->width(), toast->height());
        if (animated) {
            auto *movement = new QPropertyAnimation(toast, "geometry", toast);
            movement->setDuration(190);
            movement->setStartValue(toast->geometry());
            movement->setEndValue(target);
            movement->setEasingCurve(QEasingCurve::OutCubic);
            movement->start(QAbstractAnimation::DeleteWhenStopped);
        } else {
            toast->setGeometry(target);
        }
        toast->raise();
        y += toast->height() + kToastSpacing;
    }
}

void MainWindow::dismissToast(ToastWidget *toast)
{
    if (!toast || !m_toasts.removeOne(toast)) {
        return;
    }
    layoutToasts(true);

    auto *effect = new QGraphicsOpacityEffect(toast);
    toast->setGraphicsEffect(effect);
    auto *fade = new QPropertyAnimation(effect, "opacity", toast);
    fade->setDuration(150);
    fade->setStartValue(1.0);
    fade->setEndValue(0.0);
    connect(fade, &QPropertyAnimation::finished, toast, &QObject::deleteLater);
    fade->start(QAbstractAnimation::DeleteWhenStopped);
}
