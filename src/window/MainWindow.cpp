#include "MainWindow.h"

#include "pages/AboutPage.h"
#include "pages/CanBusPage.h"
#include "pages/ConnectionPage.h"
#include "pages/MdfViewerPage.h"
#include "pages/PageId.h"
#include "pages/PlotPage.h"
#include "pages/ProtocolEditorPage.h"
#include "pages/SettingsPage.h"
#include "theme/ThemeManager.h"
#include "theme/IconManager.h"
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
#include <QSettings>
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
}

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

MainWindow::MainWindow(ThemeManager *themeManager, QWidget *parent)
    : QMainWindow(parent),
      m_themeManager(themeManager)
{
    m_iconManager = new IconManager(m_themeManager, this);
    setWindowTitle(QStringLiteral("UpperComputer"));
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_NativeWindow);
    setMinimumSize(960, 640);
    resize(1440, 900);
    createUi();
    createPages();

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
        m_statusBar->setTheme(m_themeManager->modeName());
    });

    m_titleBar->installEventFilter(this);
    m_statusBar->setTheme(m_themeManager->modeName());
    restoreWindowSettings();
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
    if (m_navigation->property("autoMode").toBool()) {
        m_navigation->setExpanded(width() >= 1150);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    cancelThemeTransition();
    QSettings settings;
    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("window/state"), saveState());
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

void MainWindow::switchPage(const PageId page, const QString &title)
{
    const auto iterator = m_pageIndexes.constFind(page);
    if (iterator == m_pageIndexes.cend()) {
        return;
    }
    m_pages->setCurrentIndex(iterator.value());
    m_navigation->setCurrentPage(page);
    m_statusBar->setCurrentPage(title);
    m_titleBar->setCurrentPageTitle(title);
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

void MainWindow::applyNavigationMode(const QString &mode)
{
    const bool automatic = mode == tr("自动");
    m_navigation->setProperty("autoMode", automatic);
    if (automatic) {
        m_navigation->setMode(SideNavigation::Mode::Auto);
        m_navigation->setExpanded(width() >= 1150);
    } else if (mode == tr("紧凑")) {
        m_navigation->setMode(SideNavigation::Mode::Compact);
    } else {
        m_navigation->setMode(SideNavigation::Mode::Expanded);
    }
}

void MainWindow::showNotice(const QString &message, const ToastWidget::Type type)
{
    auto *toast = new ToastWidget(
        m_iconManager, tr("状态提示"), message, type, m_root);
    connect(toast, &ToastWidget::closeRequested,
            this, &MainWindow::dismissToast);
    m_toasts.append(toast);
    toast->move(width() - toast->width() - kToastRightMargin, height());
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
    if (m_themeTransitionActive || targetMode == m_themeManager->mode()
        || isMinimized() || !isVisible()) {
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

void MainWindow::createPages()
{
    addPage(PageId::Plot, new PlotPage(m_pages));
    addPage(PageId::Connection, new ConnectionPage(m_pages));
    addPage(PageId::ProtocolEditor, new ProtocolEditorPage(m_pages));
    addPage(PageId::CanBus, new CanBusPage(m_pages));
    addPage(PageId::MdfViewer, new MdfViewerPage(m_pages));
    addPage(PageId::About, new AboutPage(m_pages));

    auto *settings = new SettingsPage(m_themeManager, m_pages);
    connect(settings, &SettingsPage::userCardVisibilityChanged,
            m_navigation, &SideNavigation::setUserCardVisible);
    connect(settings, &SettingsPage::navigationModeChanged,
            this, &MainWindow::applyNavigationMode);
    connect(settings, &SettingsPage::themeModeRequested,
            this, &MainWindow::startThemeTransition);
    connect(settings, &SettingsPage::unavailableSettingRequested, this,
            [this](const QString &name) {
                showNotice(tr("%1将在后续版本实现").arg(name),
                           ToastWidget::Type::Warning);
            });
    addPage(PageId::Settings, settings);
    switchPage(PageId::Plot, tr("Plot"));
}

void MainWindow::addPage(const PageId id, QWidget *page)
{
    m_pageIndexes.insert(id, m_pages->addWidget(page));
}

void MainWindow::updateWindowStateUi()
{
    m_titleBar->setMaximized(isMaximized());
}

void MainWindow::restoreWindowSettings()
{
    QSettings settings;
    const QByteArray geometry =
        settings.value(QStringLiteral("window/geometry")).toByteArray();
    if (geometry.isEmpty()) {
        setWindowState(windowState() | Qt::WindowMaximized);
        return;
    }
    restoreGeometry(geometry);
    restoreState(settings.value(QStringLiteral("window/state")).toByteArray());
}

void MainWindow::cancelThemeTransition()
{
    if (m_themeOverlay) {
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
    const int toastHeight = m_toasts.constFirst()->height();
    const int totalHeight = m_toasts.size() * toastHeight
        + (m_toasts.size() - 1) * kToastSpacing;
    int y = height() - m_statusBar->height()
        - kToastBottomMargin - totalHeight;
    const int x = width() - m_toasts.constFirst()->width() - kToastRightMargin;

    for (ToastWidget *toast : std::as_const(m_toasts)) {
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
        y += toastHeight + kToastSpacing;
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
