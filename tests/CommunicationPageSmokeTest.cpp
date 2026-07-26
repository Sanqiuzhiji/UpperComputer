#include "app/AppContext.h"
#include "models/ProtocolDefinition.h"
#include "pages/CommunicationPage.h"
#include "pages/communication/SendPanel.h"
#include "services/ConnectionManager.h"
#include "theme/ThemeManager.h"

#include <QApplication>
#include <QComboBox>
#include <QEventLoop>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>


namespace {
void waitForEvents(const int milliseconds)
{
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

bool selectComboData(QComboBox *combo, const QVariant &value)
{
    if (!combo) return false;
    const int index = combo->findData(value);
    if (index < 0) return false;
    combo->setCurrentIndex(index);
    QCoreApplication::processEvents();
    return true;
}

ProtocolDefinition testProtocol()
{
    FieldDefinition signedField;
    signedField.id = QStringLiteral("signedValue");
    signedField.displayName = QStringLiteral("Signed value");
    signedField.type = ProtocolFieldType::Int;
    signedField.bitWidth = 16;
    signedField.defaultValue = QStringLiteral("0");

    FieldDefinition unsignedField;
    unsignedField.id = QStringLiteral("byteValue");
    unsignedField.displayName = QStringLiteral("Byte value");
    unsignedField.type = ProtocolFieldType::UInt;
    unsignedField.bitWidth = 8;
    unsignedField.minimum = 0;
    unsignedField.maximum = 255;
    unsignedField.defaultValue = QStringLiteral("0");

    MessageDefinition message;
    message.id = QStringLiteral("set-values");
    message.displayName = QStringLiteral("Set values");
    message.fields = {signedField, unsignedField};

    FieldDefinition noteField;
    noteField.id = QStringLiteral("note");
    noteField.displayName = QStringLiteral("Note");
    noteField.type = ProtocolFieldType::String;
    MessageDefinition otherMessage;
    otherMessage.id = QStringLiteral("other-command");
    otherMessage.displayName = QStringLiteral("Other command");
    otherMessage.fields = {noteField};

    ProtocolDefinition protocol;
    protocol.id = QStringLiteral("test-protocol");
    protocol.displayName = QStringLiteral("Test protocol");
    protocol.sendMessages = {message, otherMessage};
    return protocol;
}
}

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
    qputenv("QTFRAMEWORK_BYPASS_LICENSE_CHECK", QByteArrayLiteral("1"));
    QApplication app(argc, argv);
    QTemporaryDir settingsDirectory;
    if (!settingsDirectory.isValid()) return 1;
    QCoreApplication::setOrganizationName(QStringLiteral("UpperComputerTest"));
    QCoreApplication::setApplicationName(
        QStringLiteral("CommunicationPageSmoke"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       settingsDirectory.path());

    AppContext context;
    CommunicationPage page(&context);
    page.resize(1280, 820);
    page.show();
    QCoreApplication::processEvents();

    QWidget *hardware =
        page.findChild<QWidget *>(QStringLiteral("hardwareConfigPanel"));
    QWidget *mode =
        page.findChild<QWidget *>(QStringLiteral("communicationModePanel"));
    QWidget *monitor =
        page.findChild<QWidget *>(
            QStringLiteral("communicationMonitorPanel"));
    auto *send =
        page.findChild<SendPanel *>(QStringLiteral("sendPanel"));
    auto *hardwareStack =
        page.findChild<QStackedWidget *>(
            QStringLiteral("hardwareConfigStack"));
    auto *sendStack =
        page.findChild<QStackedWidget *>(QStringLiteral("sendModeStack"));
    QWidget *dataSplitter =
        page.findChild<QWidget *>(
            QStringLiteral("communicationDataSplitter"));
    auto *display =
        page.findChild<QTextEdit *>(
            QStringLiteral("communicationMonitorDisplay"));
    if (!hardware || !mode || !monitor || !send || !hardwareStack
        || !sendStack || !dataSplitter || !display
        || hardwareStack->count() != 5) {
        return 2;
    }
    if (page.findChildren<QTextEdit *>().size() != 1
        || !page.findChildren<QPlainTextEdit *>().isEmpty()) {
        return 3;
    }
    if (!(hardware->geometry().bottom() < mode->geometry().top()
          && mode->geometry().bottom() < dataSplitter->geometry().top()
          && monitor->geometry().bottom() < send->geometry().top())) {
        return 4;
    }

    int receivedFrames = 0;
    QObject::connect(context.connectionManager(),
                     &ConnectionManager::dataReceived,
                     &app, [&](const QByteArray &) { ++receivedFrames; });
    context.connectionManager()->connectTransport(
        TransportType::VirtualData,
        VirtualDataConfig{2.0, 0.5, 1.0, 4});
    waitForEvents(80);
    if (receivedFrames <= 0 || display->toPlainText().isEmpty()) return 5;
    const quint64 receivedBeforeSwitch =
        context.connectionManager()->receiveTotal();

    page.hide();
    auto *receiveCombo =
        page.findChild<QComboBox *>(QStringLiteral("receiveModeCombo"));
    if (!selectComboData(
            receiveCombo, QVariant::fromValue(ParserMode::JustFloat))) {
        return 6;
    }
    page.show();
    waitForEvents(40);
    if (context.connectionManager()->state() != ConnectionState::Connected
        || context.connectionManager()->receiveTotal() < receivedBeforeSwitch
        || !display->toPlainText().contains(
            QStringLiteral("JustFloat 解析器尚未接入"))) {
        return 7;
    }

    auto *sendCombo =
        page.findChild<QComboBox *>(QStringLiteral("sendModeCombo"));
    if (!selectComboData(
            sendCombo, QVariant::fromValue(SendMode::CustomBinary))
        || sendStack->currentIndex() != 1) {
        return 8;
    }
    if (!selectComboData(
            sendCombo, QVariant::fromValue(SendMode::RawData))
        || sendStack->currentIndex() != 0) {
        return 9;
    }

    QString lastNotification;
    QObject::connect(&context, &AppContext::notificationRequested,
                     &app, [&](const QString &message, NotificationType) {
                         lastNotification = message;
                     });
    auto *inputModeButton =
        page.findChild<QToolButton *>(QStringLiteral("rawInputModeButton"));
    auto *rawInput =
        page.findChild<QLineEdit *>(QStringLiteral("rawSendInput"));
    auto *rawSendButton =
        page.findChild<QPushButton *>(QStringLiteral("rawSendButton"));
    if (!inputModeButton || !rawInput || !rawSendButton) return 10;
    inputModeButton->click();
    rawInput->setText(QStringLiteral("GG"));
    rawSendButton->click();
    if (!lastNotification.contains(QStringLiteral("HEX"))
        || !rawInput->property("invalid").toBool()) {
        return 11;
    }

    page.setProtocols({testProtocol()});
    if (!selectComboData(
            sendCombo, QVariant::fromValue(SendMode::CustomBinary))) {
        return 12;
    }
    QCoreApplication::processEvents();
    if (send->dynamicFieldCount() != 2) return 13;
    auto *signedEditor =
        page.findChild<QLineEdit *>(QStringLiteral("customField_signedValue"));
    auto *unsignedEditor =
        page.findChild<QLineEdit *>(QStringLiteral("customField_byteValue"));
    if (!signedEditor || !unsignedEditor) return 14;

    signedEditor->setText(QStringLiteral("12.8"));
    unsignedEditor->setText(QStringLiteral("10"));
    QMetaObject::invokeMethod(
        signedEditor, "editingFinished", Qt::DirectConnection);
    ProtocolFieldValues values;
    QString validationError;
    if (signedEditor->text() != QStringLiteral("12")
        || !send->collectCustomValues(&values, &validationError)
        || values.value(QStringLiteral("signedValue")).toLongLong() != 12) {
        return 15;
    }

    signedEditor->setText(QStringLiteral("25"));
    auto *commandCombo =
        page.findChild<QComboBox *>(QStringLiteral("customCommandCombo"));
    if (!commandCombo || commandCombo->count() != 2) return 16;
    commandCombo->setCurrentIndex(1);
    commandCombo->setCurrentIndex(0);
    signedEditor =
        page.findChild<QLineEdit *>(QStringLiteral("customField_signedValue"));
    unsignedEditor =
        page.findChild<QLineEdit *>(QStringLiteral("customField_byteValue"));
    if (!signedEditor || !unsignedEditor
        || signedEditor->text() != QStringLiteral("25")) {
        return 17;
    }

    unsignedEditor->setText(QStringLiteral("300"));
    values.clear();
    validationError.clear();
    if (send->collectCustomValues(&values, &validationError)
        || !unsignedEditor->property("invalid").toBool()
        || !validationError.contains(QStringLiteral("0"))
        || !validationError.contains(QStringLiteral("255"))) {
        return 18;
    }

    auto *customSendButton =
        page.findChild<QPushButton *>(QStringLiteral("customSendButton"));
    int sentPayloads = 0;
    QObject::connect(send, &SendPanel::sendRequested,
                     &app, [&](const QByteArray &) { ++sentPayloads; });
    if (!customSendButton || customSendButton->isEnabled()) return 19;
    customSendButton->click();
    if (sentPayloads != 0) return 20;

    page.resize(960, 640);
    context.themeManager()->setMode(ThemeMode::Light);
    QCoreApplication::processEvents();
    context.themeManager()->setMode(ThemeMode::Dark);
    QCoreApplication::processEvents();
    if (!hardware->isVisible() || !mode->isVisible()
        || !monitor->isVisible() || !send->isVisible()
        || page.size() != QSize(960, 640)) {
        return 21;
    }

    context.connectionManager()->disconnectTransport();
    return 0;
}
