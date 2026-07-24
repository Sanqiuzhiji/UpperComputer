#include "app/AppContext.h"
#include "pages/ConnectionPage.h"
#include "services/ConnectionManager.h"
#include "theme/ThemeManager.h"

#include <QApplication>
#include <QEventLoop>
#include <QSettings>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QTimer>

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
    qputenv("QTFRAMEWORK_BYPASS_LICENSE_CHECK", QByteArrayLiteral("1"));
    QApplication app(argc, argv);
    QTemporaryDir settingsDirectory;
    if (!settingsDirectory.isValid()) return 1;
    QCoreApplication::setOrganizationName(QStringLiteral("UpperComputerTest"));
    QCoreApplication::setApplicationName(QStringLiteral("ConnectionPageSmoke"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       settingsDirectory.path());

    AppContext context;
    ConnectionPage page(&context);
    page.resize(1280, 820);
    page.show();
    if (!page.findChild<QTextEdit *>(QStringLiteral("connectionTerminal"))) {
        return 2;
    }

    int receivedFrames = 0;
    QObject::connect(context.connectionManager(),
                     &ConnectionManager::dataReceived,
                     &app, [&](const QByteArray &) { ++receivedFrames; });
    context.connectionManager()->connectTransport(
        TransportType::VirtualData,
        VirtualDataConfig{2.0, 0.5, 1.0, 4});
    context.themeManager()->setMode(ThemeMode::Light);

    QEventLoop loop;
    QTimer::singleShot(80, &loop, &QEventLoop::quit);
    loop.exec();
    QWidget *hardware =
        page.findChild<QWidget *>(QStringLiteral("hardwareConfigPanel"));
    QWidget *parser =
        page.findChild<QWidget *>(QStringLiteral("parserConfigPanel"));
    QWidget *terminal =
        page.findChild<QWidget *>(QStringLiteral("terminalPanel"));
    QWidget *send =
        page.findChild<QWidget *>(QStringLiteral("sendPanel"));
    QStackedWidget *hardwareStack =
        page.findChild<QStackedWidget *>(QStringLiteral("hardwareConfigStack"));
    const bool layoutIsValid =
        hardware && parser && terminal && send && hardwareStack
        && hardwareStack->count() == 5
        && hardware->geometry().bottom() < parser->geometry().top()
        && parser->geometry().bottom() < terminal->geometry().top()
        && terminal->geometry().bottom() < send->geometry().top()
        && terminal->height() >= 220;
    page.hide();
    QCoreApplication::processEvents();
    const bool remainedConnected =
        context.connectionManager()->state() == ConnectionState::Connected;
    context.connectionManager()->disconnectTransport();
    return receivedFrames > 0 && remainedConnected && layoutIsValid ? 0 : 3;
}
