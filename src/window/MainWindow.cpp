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
#include "widgets/SideNavigation.h"
#include "widgets/StatusBarWidget.h"
#include "widgets/TitleBar.h"

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
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

MainWindow::MainWindow(ThemeManager *themeManager, QWidget *parent)
    : QMainWindow(parent),
      m_themeManager(themeManager)
{
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
            m_themeManager, &ThemeManager::toggleMode);
    connect(m_titleBar, &TitleBar::minimizeRequested, this, &QWidget::showMinimized);
    connect(m_titleBar, &TitleBar::maximizeRestoreRequested,
            this, &MainWindow::toggleMaximized);
    connect(m_titleBar, &TitleBar::closeRequested, this, &QWidget::close);
    connect(m_titleBar, &TitleBar::pinToggleRequested, this, &MainWindow::setPinned);
    connect(m_navigation, &SideNavigation::pageRequested,
            this, &MainWindow::switchPage);
    connect(m_themeManager, &ThemeManager::themeChanged, this, [this](ThemeMode) {
        m_statusBar->setTheme(m_themeManager->modeName());
        showNotice(tr("Theme switched to %1").arg(m_themeManager->modeName()));
    });

    m_titleBar->installEventFilter(this);
    m_statusBar->setTheme(m_themeManager->modeName());
    restoreWindowSettings();
}

bool MainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
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
    if (m_notice) {
        m_notice->adjustSize();
        m_notice->move(width() - m_notice->width() - 24,
                       height() - m_notice->height() - m_statusBar->height() - 20);
    }
    // Auto mode is deliberately width-aware, while preserving the selected page.
    if (m_navigation->property("autoMode").toBool()) {
        m_navigation->setExpanded(width() >= 1150);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
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
}

void MainWindow::toggleMaximized()
{
    isMaximized() ? showNormal() : showMaximized();
}

void MainWindow::setPinned(const bool pinned)
{
    setWindowFlag(Qt::WindowStaysOnTopHint, pinned);
    show();
    showNotice(pinned ? tr("Window pinned on top") : tr("Always-on-top disabled"));
}

void MainWindow::applyNavigationMode(const QString &mode)
{
    const bool automatic = mode == tr("Auto");
    m_navigation->setProperty("autoMode", automatic);
    if (automatic) {
        m_navigation->setMode(SideNavigation::Mode::Auto);
        m_navigation->setExpanded(width() >= 1150);
    } else if (mode == tr("Compact")) {
        m_navigation->setMode(SideNavigation::Mode::Compact);
    } else {
        m_navigation->setMode(SideNavigation::Mode::Expanded);
    }
}

void MainWindow::showNotice(const QString &message)
{
    auto *label = qobject_cast<QLabel *>(m_notice);
    if (!label) {
        return;
    }
    label->setText(message);
    label->adjustSize();
    label->move(width() - label->width() - 24,
                height() - label->height() - m_statusBar->height() - 20);
    label->show();
    label->raise();

    auto *effect = qobject_cast<QGraphicsOpacityEffect *>(label->graphicsEffect());
    effect->setOpacity(0.0);
    auto *animation = new QPropertyAnimation(effect, "opacity", label);
    animation->setDuration(180);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    QTimer::singleShot(2200, label, &QWidget::hide);
}

void MainWindow::createUi()
{
    auto *root = new QWidget(this);
    root->setObjectName(QStringLiteral("windowRoot"));
    setCentralWidget(root);

    auto *layout = new QVBoxLayout(root);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    m_titleBar = new TitleBar(root);
    layout->addWidget(m_titleBar);

    auto *body = new QWidget(root);
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);
    m_navigation = new SideNavigation(body);
    m_pages = new QStackedWidget(body);
    m_pages->setObjectName(QStringLiteral("pageArea"));
    bodyLayout->addWidget(m_navigation);
    bodyLayout->addWidget(m_pages, 1);
    layout->addWidget(body, 1);

    m_statusBar = new StatusBarWidget(root);
    layout->addWidget(m_statusBar);

    auto *notice = new QLabel(root);
    notice->setProperty("card", true);
    notice->setStyleSheet(QStringLiteral(
        "padding:10px 16px;border-left:3px solid #28A9E0;"));
    notice->setGraphicsEffect(new QGraphicsOpacityEffect(notice));
    notice->hide();
    m_notice = notice;
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
    connect(settings, &SettingsPage::unavailableSettingRequested, this,
            [this](const QString &name) {
                showNotice(tr("%1 will be implemented in a future version").arg(name));
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
