#include "app/AppSettings.h"

#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTemporaryDir settingsDirectory;
    if (!settingsDirectory.isValid()) return 1;
    QCoreApplication::setOrganizationName(QStringLiteral("UpperComputerTest"));
    QCoreApplication::setApplicationName(QStringLiteral("SettingsPersistence"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       settingsDirectory.path());

    const SerialConfig serial{
        QStringLiteral("COM42"), 921600, QSerialPort::Data7,
        QSerialPort::EvenParity, QSerialPort::TwoStop};
    const UdpConfig udp{QStringLiteral("127.0.0.2"), 12001, 12002};
    const TcpServerConfig server{12003};
    const TcpClientConfig client{
        QStringLiteral("localhost"), 12004, QStringLiteral("local test")};
    const VirtualDataConfig virtualData{5.5, 12.0, 3.25, 6};
    {
        AppSettings settings;
        settings.setTransportType(TransportType::VirtualData);
        settings.setSerialConfig(serial);
        settings.setUdpConfig(udp);
        settings.setTcpServerConfig(server);
        settings.setTcpClientConfig(client);
        settings.setVirtualDataConfig(virtualData);
        settings.setParserMode(ParserMode::CustomBinary);
        settings.setSendMode(SendMode::CustomBinary);
        settings.setCustomProtocolId(QStringLiteral("protocol-id"));
        settings.setTerminalDisplayMode(TerminalDisplayMode::Hex);
        settings.setTerminalTimestampEnabled(false);
        settings.setTextEncoding(TextEncoding::Latin1);
        settings.setReceiveVisible(false);
        settings.setTransmitVisible(false);
        settings.setAnsiEnabled(true);
        settings.setInputMode(InputMode::Hex);
        settings.setChecksumMode(ChecksumMode::Crc8Maxim);
        settings.setLineEnding(LineEnding::CRLF);
        settings.setRawTextDraft(QStringLiteral("text draft"));
        settings.setRawHexDraft(QStringLiteral("AA 55"));
        settings.setCustomCommandId(QStringLiteral("command-id"));
        settings.setCustomFieldDrafts(
            {{QStringLiteral("protocol/command/field"), 42}});
    }
    {
        AppSettings settings;
        if (settings.transportType() != TransportType::VirtualData) return 2;
        if (settings.serialConfig() != serial) return 3;
        if (settings.udpConfig() != udp) return 4;
        if (settings.tcpServerConfig() != server) return 5;
        if (settings.tcpClientConfig() != client) return 6;
        if (settings.virtualDataConfig() != virtualData) return 7;
        if (settings.parserMode() != ParserMode::CustomBinary
            || settings.sendMode() != SendMode::CustomBinary
            || settings.customProtocolId() != QStringLiteral("protocol-id")) {
            return 8;
        }
        if (settings.terminalDisplayMode() != TerminalDisplayMode::Hex
            || settings.terminalTimestampEnabled()
            || settings.textEncoding() != TextEncoding::Latin1
            || settings.receiveVisible() || settings.transmitVisible()
            || !settings.ansiEnabled()) {
            return 9;
        }
        if (settings.inputMode() != InputMode::Hex
            || settings.checksumMode() != ChecksumMode::Crc8Maxim
            || settings.lineEnding() != LineEnding::CRLF) {
            return 10;
        }
        if (settings.rawTextDraft() != QStringLiteral("text draft")
            || settings.rawHexDraft() != QStringLiteral("AA 55")
            || settings.customCommandId() != QStringLiteral("command-id")
            || settings.customFieldDrafts().value(
                   QStringLiteral("protocol/command/field")).toInt() != 42) {
            return 11;
        }
    }
    return 0;
}
