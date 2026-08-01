#include "app/AppContext.h"
#include "models/ProtocolDefinition.h"
#include "pages/CommunicationPage.h"
#include "pages/communication/CommunicationMonitorPanel.h"
#include "pages/communication/SendPanel.h"
#include "services/ConnectionManager.h"
#include "services/CustomBinaryCodec.h"
#include "theme/ThemeManager.h"

#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QEventLoop>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextEdit>
#include <QTextFragment>
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

bool hasDirectionBadge(
    const QTextEdit *display, const QString &label, const QColor &background)
{
    for (QTextBlock block = display->document()->begin();
         block.isValid();
         block = block.next()) {
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            if (fragment.isValid()
                && fragment.text().contains(label)
                && fragment.charFormat().background().color() == background) {
                return true;
            }
        }
    }
    return false;
}

bool hasDirectionText(
    const QTextEdit *display, const QString &text, const QColor &foreground)
{
    for (QTextBlock block = display->document()->begin();
         block.isValid();
         block = block.next()) {
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            const QTextCharFormat format = fragment.charFormat();
            if (fragment.isValid()
                && fragment.text().contains(text)
                && format.foreground().color() == foreground
                && format.background().style() == Qt::NoBrush) {
                return true;
            }
        }
    }
    return false;
}

ProtocolDefinition testProtocol()
{
    FieldDefinition header;
    header.id = QStringLiteral("header");
    header.displayName = QStringLiteral("Header");
    header.type = ProtocolFieldType::ByteArray;
    header.bitWidth = 16;
    header.editable = false;
    header.fixedValue = QByteArray::fromHex("AA55");
    header.role = ProtocolFieldRole::FrameHeader;

    FieldDefinition length;
    length.id = QStringLiteral("length");
    length.displayName = QStringLiteral("Length");
    length.type = ProtocolFieldType::UInt;
    length.bitWidth = 8;
    length.editable = false;
    length.role = ProtocolFieldRole::Length;

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

    FieldDefinition checksum;
    checksum.id = QStringLiteral("checksum");
    checksum.displayName = QStringLiteral("Checksum");
    checksum.type = ProtocolFieldType::UInt;
    checksum.bitWidth = 8;
    checksum.editable = false;
    checksum.role = ProtocolFieldRole::Checksum;
    checksum.checksumAlgorithm = ProtocolChecksumAlgorithm::Sum8;

    FieldDefinition tail;
    tail.id = QStringLiteral("tail");
    tail.displayName = QStringLiteral("Tail");
    tail.type = ProtocolFieldType::ByteArray;
    tail.bitWidth = 8;
    tail.editable = false;
    tail.fixedValue = QByteArray::fromHex("0D");
    tail.role = ProtocolFieldRole::Constant;

    MessageDefinition message;
    message.id = QStringLiteral("set-values");
    message.displayName = QStringLiteral("Set values");
    message.fields = {
        header, length, signedField, unsignedField, checksum, tail};

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
    protocol.receiveMessages = {message, otherMessage};
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
    auto *monitor =
        page.findChild<CommunicationMonitorPanel *>(
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
            QStringLiteral("虚拟测试信号"))
        || !display->toPlainText().contains(
            QStringLiteral("正弦波"))) {
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
    QByteArray lastPayload;
    QObject::connect(send, &SendPanel::sendRequested,
                     &app, [&](const QByteArray &payload) {
                         ++sentPayloads;
                         lastPayload = payload;
                     });
    unsignedEditor->setText(QStringLiteral("10"));
    if (!customSendButton || !customSendButton->isEnabled()) return 19;
    customSendButton->click();
    if (sentPayloads != 1
        || lastPayload != QByteArray::fromHex("AA550819000A2A0D")) {
        return 20;
    }

    CustomBinaryCodec chunkedCodec(testProtocol());
    QList<ParsedMessage> parsedMessages;
    QString parseError;
    if (!chunkedCodec.parse(
            lastPayload.left(3), &parsedMessages, &parseError)
        || !parsedMessages.isEmpty()
        || !chunkedCodec.parse(
            lastPayload.mid(3), &parsedMessages, &parseError)
        || parsedMessages.size() != 1
        || parsedMessages.constFirst().displayName
            != QStringLiteral("Set values")) {
        return 21;
    }
    QByteArray damagedPayload = lastPayload;
    damagedPayload[6] =
        static_cast<char>(static_cast<quint8>(damagedPayload.at(6)) ^ 0x01U);
    CustomBinaryCodec rejectingCodec(testProtocol());
    parsedMessages.clear();
    parseError.clear();
    if (rejectingCodec.parse(
            damagedPayload, &parsedMessages, &parseError)
        || !parsedMessages.isEmpty()
        || !parseError.contains(QStringLiteral("校验失败"))) {
        return 22;
    }

    if (!selectComboData(
            receiveCombo, QVariant::fromValue(ParserMode::CustomBinary))) {
        return 23;
    }
    monitor->addEntry(DataDirection::Transmit, lastPayload);
    CustomBinaryCodec monitorCodec(testProtocol());
    parsedMessages.clear();
    parseError.clear();
    if (!monitorCodec.parse(
            lastPayload, &parsedMessages, &parseError)) {
        return 24;
    }
    monitor->addParsedMessages(
        QDateTime::currentMSecsSinceEpoch() * 1000, parsedMessages);
    waitForEvents(60);
    const QString customBinaryDisplay = display->toPlainText();
    if (!customBinaryDisplay.contains(QStringLiteral("RX  Set values"))
        || !customBinaryDisplay.contains(
            QStringLiteral("TX  HEX: AA 55 08 19 00 0A 2A 0D"))
        || !hasDirectionBadge(
            display, QStringLiteral("RX"), QColor(QStringLiteral("#45B97C")))
        || !hasDirectionBadge(
            display, QStringLiteral("TX"), QColor(QStringLiteral("#28A9E0")))
        || !hasDirectionText(
            display, QStringLiteral("Set values"),
            QColor(QStringLiteral("#45B97C")))
        || !hasDirectionText(
            display, QStringLiteral("HEX: AA 55"),
            QColor(QStringLiteral("#28A9E0")))
        || !display->toPlainText().contains(QStringLiteral("Signed value"))
        || !display->toPlainText().contains(QStringLiteral("25"))
        || customBinaryDisplay.contains(QStringLiteral("Header:"))
        || customBinaryDisplay.contains(QStringLiteral("Length:"))
        || customBinaryDisplay.contains(QStringLiteral("Checksum:"))
        || customBinaryDisplay.contains(QStringLiteral("Tail:"))) {
        return 24;
    }

    page.resize(960, 640);
    context.themeManager()->setMode(ThemeMode::Light);
    QCoreApplication::processEvents();
    context.themeManager()->setMode(ThemeMode::Dark);
    QCoreApplication::processEvents();
    if (!hardware->isVisible() || !mode->isVisible()
        || !monitor->isVisible() || !send->isVisible()
        || page.width() < 960 || page.height() != 640) {
        return 25;
    }

    context.connectionManager()->disconnectTransport();
    return 0;
}
