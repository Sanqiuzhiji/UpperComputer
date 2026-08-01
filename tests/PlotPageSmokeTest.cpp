#include "app/AppContext.h"
#include "app/AppSettings.h"
#include "pages/PlotPage.h"
#include "pages/plot/DetachableTabWidget.h"
#include "pages/plot/DetachableTabBar.h"
#include "pages/plot/DashboardItem.h"
#include "pages/plot/PlotCanvas.h"
#include "pages/plot/PlotChannelDialog.h"
#include "pages/plot/PlotFloatingWindow.h"
#include "pages/plot/PlotWorkspacePage.h"
#include "pages/plot/RealtimePlotWidget.h"
#include "services/ChannelDataHub.h"
#include "services/ConnectionManager.h"
#include "theme/ThemeManager.h"

#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QEventLoop>
#include <QMouseEvent>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolButton>

#include <limits>

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
    qputenv("QTFRAMEWORK_BYPASS_LICENSE_CHECK", QByteArrayLiteral("1"));
    QApplication app(argc, argv);
    QTemporaryDir settingsDirectory;
    if (!settingsDirectory.isValid()) return 1;
    QCoreApplication::setOrganizationName(QStringLiteral("UpperComputerTest"));
    QCoreApplication::setApplicationName(QStringLiteral("PlotPageSmoke"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(
        QSettings::IniFormat, QSettings::UserScope,
        settingsDirectory.path());

    AppContext context;
    context.settings()->setWorkspaceDirectory(settingsDirectory.path());
    PlotPage page(&context);
    page.resize(1100, 720);
    page.show();
    QCoreApplication::processEvents();
    const VirtualDataConfig virtualConfig{
        5.0,
        0.75,
        2.0,
        3
    };
    context.connectionManager()->connectTransport(
        TransportType::VirtualData, virtualConfig);
    QEventLoop dataWait;
    QTimer::singleShot(40, &dataWait, &QEventLoop::quit);
    dataWait.exec();
    const QString channelId = ChannelDataHub::channelId(
        QStringLiteral("virtual-data"),
        QStringLiteral("generated-signals"),
        QStringLiteral("signal-1"));
    if (context.channelDataHub()->channels().size() != 5
        || context.channelDataHub()->snapshot(
            channelId, 0, std::numeric_limits<qint64>::max()).isEmpty()) {
        return 2;
    }
    auto *tabs = page.findChild<DetachableTabWidget *>(
        QStringLiteral("plotTabWidget"));
    if (!tabs || tabs->count() != 0 || tabs->currentWorkspace()) return 3;
    auto *addPage = page.findChild<QToolButton *>(
        QStringLiteral("plotAddPageButton"));
    auto *addPlot = page.findChild<QToolButton *>(
        QStringLiteral("plotAddWidgetButton"));
    auto *edit = page.findChild<QToolButton *>(
        QStringLiteral("plotEditLayoutButton"));
    if (!addPage || !addPlot || !edit) return 4;
    addPage->click();
    addPlot->click();
    edit->click();
    QCoreApplication::processEvents();
    if (tabs->count() != 1
        || !tabs->currentWorkspace()->canvas()->editMode()) {
        return 5;
    }
    const QString firstPageTitle = tabs->tabText(0);
    if (tabs->addWorkspace(QStringLiteral("  ") + firstPageTitle.toUpper()
                           + QStringLiteral("  "))
        || tabs->count() != 1) {
        return 18;
    }
    auto *plot = page.findChild<RealtimePlotWidget *>(
        QStringLiteral("realtimePlotWidget"));
    if (!plot) return 6;
    auto *dashboardItem =
        qobject_cast<DashboardItem *>(plot->parentWidget());
    if (!dashboardItem) return 7;
    const QRect initialGeometry = dashboardItem->geometry();
    const QPoint pressPosition(100, 80);
    const QPoint pressGlobal = plot->mapToGlobal(pressPosition);
    QMouseEvent press(
        QEvent::MouseButtonPress,
        QPointF(pressPosition), QPointF(pressPosition),
        QPointF(pressGlobal),
        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(plot, &press);
    QMouseEvent move(
        QEvent::MouseMove,
        QPointF(pressPosition + QPoint(40, 20)),
        QPointF(pressPosition + QPoint(40, 20)),
        QPointF(pressGlobal + QPoint(40, 20)),
        Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(plot, &move);
    QMouseEvent release(
        QEvent::MouseButtonRelease,
        QPointF(pressPosition + QPoint(40, 20)),
        QPointF(pressPosition + QPoint(40, 20)),
        QPointF(pressGlobal + QPoint(40, 20)),
        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(plot, &release);
    if (dashboardItem->geometry().topLeft()
        == initialGeometry.topLeft()) {
        return 8;
    }

    edit->click();
    const QRect lockedGeometry = dashboardItem->geometry();
    QApplication::sendEvent(plot, &press);
    QApplication::sendEvent(plot, &move);
    QApplication::sendEvent(plot, &release);
    if (dashboardItem->geometry() != lockedGeometry) return 9;

    auto *tabBar = tabs->findChild<DetachableTabBar *>(
        QStringLiteral("plotTabBar"));
    if (!tabBar) return 10;
    PlotChannelDialog themeDialog(&context, {}, &page);
    const QColor darkBase =
        plot->palette().color(QPalette::Base);
    const QColor darkCanvas =
        tabs->currentWorkspace()->canvas()->palette().color(
            QPalette::Window);
    const QColor darkTab =
        tabBar->palette().color(QPalette::Button);
    const QColor darkDialog =
        themeDialog.palette().color(QPalette::Window);
    context.themeManager()->setMode(ThemeMode::Light);
    QCoreApplication::processEvents();
    const QColor lightBase =
        plot->palette().color(QPalette::Base);
    const QColor lightCanvas =
        tabs->currentWorkspace()->canvas()->palette().color(
            QPalette::Window);
    const QColor lightTab =
        tabBar->palette().color(QPalette::Button);
    const QColor lightDialog =
        themeDialog.palette().color(QPalette::Window);
    if (lightBase == darkBase
        || lightCanvas == darkCanvas
        || lightTab == darkTab
        || lightDialog == darkDialog
        || lightBase.lightness() <= darkBase.lightness()
        || lightCanvas.lightness() <= darkCanvas.lightness()
        || lightTab.lightness() <= darkTab.lightness()
        || lightDialog.lightness() <= darkDialog.lightness()) {
        return 11;
    }
    context.themeManager()->setMode(ThemeMode::Dark);
    QCoreApplication::processEvents();

    plot->setChannelStyles({
        {channelId, true, QColor(QStringLiteral("#28A9E0")), 1.5}});
    QCoreApplication::processEvents();
    if (plot->channelStyles().size() != 1) return 12;

    QTimer::singleShot(0, &app, [] {
        for (QWidget *widget : QApplication::topLevelWidgets()) {
            if (auto *dialog = qobject_cast<PlotChannelDialog *>(widget)) {
                dialog->accept();
            }
        }
    });
    plot->openChannelDialog();

    if (!QMetaObject::invokeMethod(
            tabBar, "detachRequested", Qt::DirectConnection,
            Q_ARG(int, tabs->currentIndex()),
            Q_ARG(QPoint, QPoint(300, 200)))) {
        return 13;
    }
    QCoreApplication::processEvents();
    const QList<PlotFloatingWindow *> floatingWindows =
        tabs->findChildren<PlotFloatingWindow *>();
    if (tabs->count() != 0 || floatingWindows.size() != 1) return 14;
    floatingWindows.constFirst()->close();
    QCoreApplication::processEvents();
    if (tabs->count() != 1 || tabs->workspaceCount() != 1) return 15;
    if (!QMetaObject::invokeMethod(
            tabs, "tabCloseRequested", Qt::DirectConnection,
            Q_ARG(int, tabs->currentIndex()))) {
        return 16;
    }
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    if (tabs->count() != 0 || tabs->workspaceCount() != 0) return 17;
    context.connectionManager()->disconnectTransport();
    return 0;
}
