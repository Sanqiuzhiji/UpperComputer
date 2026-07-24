#include "AppSettings.h"

namespace {
constexpr auto kThemeModeKey = "appearance/themeMode";
constexpr auto kNavigationModeKey = "navigation/mode";
constexpr auto kUserCardVisibleKey = "navigation/userCardVisible";
constexpr auto kLastPageKey = "navigation/lastPage";
constexpr auto kWindowGeometryKey = "window/geometry";
constexpr auto kWindowMaximizedKey = "window/maximized";
constexpr auto kTransportTypeKey = "connection/transportType";
constexpr auto kSerialPortKey = "connection/serial/portName";
constexpr auto kSerialBaudKey = "connection/serial/baudRate";
constexpr auto kSerialDataBitsKey = "connection/serial/dataBits";
constexpr auto kSerialParityKey = "connection/serial/parity";
constexpr auto kSerialStopBitsKey = "connection/serial/stopBits";
constexpr auto kUdpRemoteAddressKey = "connection/udp/remoteAddress";
constexpr auto kUdpRemotePortKey = "connection/udp/remotePort";
constexpr auto kUdpLocalPortKey = "connection/udp/localPort";
constexpr auto kTcpServerPortKey = "connection/tcpServer/listenPort";
constexpr auto kTcpClientHostKey = "connection/tcpClient/host";
constexpr auto kTcpClientPortKey = "connection/tcpClient/port";
constexpr auto kTcpClientNameKey = "connection/tcpClient/name";
constexpr auto kVirtualIntervalKey = "connection/virtual/sampleIntervalMs";
constexpr auto kVirtualFrequencyKey = "connection/virtual/signalFrequencyHz";
constexpr auto kVirtualAmplitudeKey = "connection/virtual/amplitude";
constexpr auto kVirtualChannelsKey = "connection/virtual/channelCount";
constexpr auto kParserModeKey = "connection/parser/mode";
constexpr auto kCustomProtocolKey = "connection/parser/customProtocolId";
constexpr auto kTerminalDisplayKey = "connection/terminal/displayMode";
constexpr auto kTimestampKey = "connection/terminal/timestamp";
constexpr auto kEncodingKey = "connection/terminal/encoding";
constexpr auto kReceiveVisibleKey = "connection/terminal/receiveVisible";
constexpr auto kTransmitVisibleKey = "connection/terminal/transmitVisible";
constexpr auto kAnsiKey = "connection/terminal/ansi";
constexpr auto kInputModeKey = "connection/send/inputMode";
constexpr auto kChecksumModeKey = "connection/send/checksumMode";
constexpr auto kLineEndingKey = "connection/send/lineEnding";

template<typename Enum>
int enumValue(const Enum value)
{
    return static_cast<int>(value);
}
}

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
{
    load();
}

ThemeMode AppSettings::themeMode() const noexcept
{
    return m_themeMode;
}

NavigationMode AppSettings::navigationMode() const noexcept
{
    return m_navigationMode;
}

bool AppSettings::userCardVisible() const noexcept
{
    return m_userCardVisible;
}

PageId AppSettings::lastPage() const noexcept
{
    return m_lastPage;
}

QByteArray AppSettings::windowGeometry() const
{
    return m_windowGeometry;
}

bool AppSettings::windowMaximized() const noexcept
{
    return m_windowMaximized;
}

TransportType AppSettings::transportType() const noexcept
{
    return m_transportType;
}

SerialConfig AppSettings::serialConfig() const
{
    return m_serialConfig;
}

UdpConfig AppSettings::udpConfig() const
{
    return m_udpConfig;
}

TcpServerConfig AppSettings::tcpServerConfig() const
{
    return m_tcpServerConfig;
}

TcpClientConfig AppSettings::tcpClientConfig() const
{
    return m_tcpClientConfig;
}

VirtualDataConfig AppSettings::virtualDataConfig() const
{
    return m_virtualDataConfig;
}

ParserMode AppSettings::parserMode() const noexcept
{
    return m_parserMode;
}

QString AppSettings::customProtocolId() const
{
    return m_customProtocolId;
}

TerminalDisplayMode AppSettings::terminalDisplayMode() const noexcept
{
    return m_terminalDisplayMode;
}

bool AppSettings::terminalTimestampEnabled() const noexcept
{
    return m_terminalTimestampEnabled;
}

TextEncoding AppSettings::textEncoding() const noexcept
{
    return m_textEncoding;
}

bool AppSettings::receiveVisible() const noexcept
{
    return m_receiveVisible;
}

bool AppSettings::transmitVisible() const noexcept
{
    return m_transmitVisible;
}

bool AppSettings::ansiEnabled() const noexcept
{
    return m_ansiEnabled;
}

InputMode AppSettings::inputMode() const noexcept
{
    return m_inputMode;
}

ChecksumMode AppSettings::checksumMode() const noexcept
{
    return m_checksumMode;
}

LineEnding AppSettings::lineEnding() const noexcept
{
    return m_lineEnding;
}

void AppSettings::setThemeMode(const ThemeMode mode)
{
    if (m_themeMode == mode) {
        return;
    }
    m_themeMode = mode;
    m_store.setValue(QLatin1String(kThemeModeKey), enumValue(mode));
}

void AppSettings::setNavigationMode(const NavigationMode mode)
{
    if (m_navigationMode == mode) {
        return;
    }
    m_navigationMode = mode;
    m_store.setValue(QLatin1String(kNavigationModeKey), enumValue(mode));
}

void AppSettings::setUserCardVisible(const bool visible)
{
    if (m_userCardVisible == visible) {
        return;
    }
    m_userCardVisible = visible;
    m_store.setValue(QLatin1String(kUserCardVisibleKey), visible);
}

void AppSettings::setLastPage(const PageId page)
{
    if (m_lastPage == page) {
        return;
    }
    m_lastPage = page;
    m_store.setValue(QLatin1String(kLastPageKey), enumValue(page));
}

void AppSettings::setWindowGeometry(const QByteArray &geometry)
{
    if (m_windowGeometry == geometry) {
        return;
    }
    m_windowGeometry = geometry;
    m_store.setValue(QLatin1String(kWindowGeometryKey), geometry);
}

void AppSettings::setWindowMaximized(const bool maximized)
{
    if (m_windowMaximized == maximized) {
        return;
    }
    m_windowMaximized = maximized;
    m_store.setValue(QLatin1String(kWindowMaximizedKey), maximized);
}

void AppSettings::setTransportType(const TransportType type)
{
    if (m_transportType == type) return;
    m_transportType = type;
    m_store.setValue(QLatin1String(kTransportTypeKey), enumValue(type));
}

void AppSettings::setSerialConfig(const SerialConfig &config)
{
    if (m_serialConfig == config) return;
    m_serialConfig = config;
    m_store.setValue(QLatin1String(kSerialPortKey), config.portName);
    m_store.setValue(QLatin1String(kSerialBaudKey), config.baudRate);
    m_store.setValue(QLatin1String(kSerialDataBitsKey), enumValue(config.dataBits));
    m_store.setValue(QLatin1String(kSerialParityKey), enumValue(config.parity));
    m_store.setValue(QLatin1String(kSerialStopBitsKey), enumValue(config.stopBits));
}

void AppSettings::setUdpConfig(const UdpConfig &config)
{
    if (m_udpConfig == config) return;
    m_udpConfig = config;
    m_store.setValue(QLatin1String(kUdpRemoteAddressKey), config.remoteAddress);
    m_store.setValue(QLatin1String(kUdpRemotePortKey), config.remotePort);
    m_store.setValue(QLatin1String(kUdpLocalPortKey), config.localPort);
}

void AppSettings::setTcpServerConfig(const TcpServerConfig &config)
{
    if (m_tcpServerConfig == config) return;
    m_tcpServerConfig = config;
    m_store.setValue(QLatin1String(kTcpServerPortKey), config.listenPort);
}

void AppSettings::setTcpClientConfig(const TcpClientConfig &config)
{
    if (m_tcpClientConfig == config) return;
    m_tcpClientConfig = config;
    m_store.setValue(QLatin1String(kTcpClientHostKey), config.host);
    m_store.setValue(QLatin1String(kTcpClientPortKey), config.port);
    m_store.setValue(QLatin1String(kTcpClientNameKey), config.connectionName);
}

void AppSettings::setVirtualDataConfig(const VirtualDataConfig &config)
{
    if (m_virtualDataConfig == config) return;
    m_virtualDataConfig = config;
    m_store.setValue(QLatin1String(kVirtualIntervalKey), config.sampleIntervalMs);
    m_store.setValue(QLatin1String(kVirtualFrequencyKey), config.signalFrequencyHz);
    m_store.setValue(QLatin1String(kVirtualAmplitudeKey), config.amplitude);
    m_store.setValue(QLatin1String(kVirtualChannelsKey), config.channelCount);
}

void AppSettings::setParserMode(const ParserMode mode)
{
    if (m_parserMode == mode) return;
    m_parserMode = mode;
    m_store.setValue(QLatin1String(kParserModeKey), enumValue(mode));
}

void AppSettings::setCustomProtocolId(const QString &protocolId)
{
    if (m_customProtocolId == protocolId) return;
    m_customProtocolId = protocolId;
    m_store.setValue(QLatin1String(kCustomProtocolKey), protocolId);
}

void AppSettings::setTerminalDisplayMode(const TerminalDisplayMode mode)
{
    if (m_terminalDisplayMode == mode) return;
    m_terminalDisplayMode = mode;
    m_store.setValue(QLatin1String(kTerminalDisplayKey), enumValue(mode));
}

void AppSettings::setTerminalTimestampEnabled(const bool enabled)
{
    if (m_terminalTimestampEnabled == enabled) return;
    m_terminalTimestampEnabled = enabled;
    m_store.setValue(QLatin1String(kTimestampKey), enabled);
}

void AppSettings::setTextEncoding(const TextEncoding encoding)
{
    if (m_textEncoding == encoding) return;
    m_textEncoding = encoding;
    m_store.setValue(QLatin1String(kEncodingKey), enumValue(encoding));
}

void AppSettings::setReceiveVisible(const bool visible)
{
    if (m_receiveVisible == visible) return;
    m_receiveVisible = visible;
    m_store.setValue(QLatin1String(kReceiveVisibleKey), visible);
}

void AppSettings::setTransmitVisible(const bool visible)
{
    if (m_transmitVisible == visible) return;
    m_transmitVisible = visible;
    m_store.setValue(QLatin1String(kTransmitVisibleKey), visible);
}

void AppSettings::setAnsiEnabled(const bool enabled)
{
    if (m_ansiEnabled == enabled) return;
    m_ansiEnabled = enabled;
    m_store.setValue(QLatin1String(kAnsiKey), enabled);
}

void AppSettings::setInputMode(const InputMode mode)
{
    if (m_inputMode == mode) return;
    m_inputMode = mode;
    m_store.setValue(QLatin1String(kInputModeKey), enumValue(mode));
}

void AppSettings::setChecksumMode(const ChecksumMode mode)
{
    if (m_checksumMode == mode) return;
    m_checksumMode = mode;
    m_store.setValue(QLatin1String(kChecksumModeKey), enumValue(mode));
}

void AppSettings::setLineEnding(const LineEnding ending)
{
    if (m_lineEnding == ending) return;
    m_lineEnding = ending;
    m_store.setValue(QLatin1String(kLineEndingKey), enumValue(ending));
}

void AppSettings::load()
{
    const int theme = m_store.value(
        QLatin1String(kThemeModeKey), enumValue(ThemeMode::Dark)).toInt();
    if (theme == enumValue(ThemeMode::Light)
        || theme == enumValue(ThemeMode::Dark)) {
        m_themeMode = static_cast<ThemeMode>(theme);
    }

    const int navigation = m_store.value(
        QLatin1String(kNavigationModeKey),
        enumValue(NavigationMode::Expanded)).toInt();
    if (navigation >= enumValue(NavigationMode::Automatic)
        && navigation <= enumValue(NavigationMode::Expanded)) {
        m_navigationMode = static_cast<NavigationMode>(navigation);
    }

    m_userCardVisible = m_store.value(
        QLatin1String(kUserCardVisibleKey), true).toBool();

    const int page = m_store.value(
        QLatin1String(kLastPageKey), enumValue(PageId::Plot)).toInt();
    if (page >= enumValue(PageId::Plot)
        && page <= enumValue(PageId::Settings)) {
        m_lastPage = static_cast<PageId>(page);
    }

    m_windowGeometry =
        m_store.value(QLatin1String(kWindowGeometryKey)).toByteArray();
    m_windowMaximized = m_store.value(
        QLatin1String(kWindowMaximizedKey), true).toBool();

    m_transportType = static_cast<TransportType>(qBound(
        enumValue(TransportType::SerialPort),
        m_store.value(QLatin1String(kTransportTypeKey),
                      enumValue(TransportType::SerialPort)).toInt(),
        enumValue(TransportType::VirtualData)));
    m_serialConfig.portName =
        m_store.value(QLatin1String(kSerialPortKey)).toString();
    m_serialConfig.baudRate = m_store.value(
        QLatin1String(kSerialBaudKey), 115200).toInt();
    m_serialConfig.dataBits = static_cast<QSerialPort::DataBits>(
        m_store.value(QLatin1String(kSerialDataBitsKey),
                      enumValue(QSerialPort::Data8)).toInt());
    m_serialConfig.parity = static_cast<QSerialPort::Parity>(
        m_store.value(QLatin1String(kSerialParityKey),
                      enumValue(QSerialPort::NoParity)).toInt());
    m_serialConfig.stopBits = static_cast<QSerialPort::StopBits>(
        m_store.value(QLatin1String(kSerialStopBitsKey),
                      enumValue(QSerialPort::OneStop)).toInt());

    m_udpConfig.remoteAddress = m_store.value(
        QLatin1String(kUdpRemoteAddressKey),
        QStringLiteral("127.0.0.1")).toString();
    m_udpConfig.remotePort = static_cast<quint16>(m_store.value(
        QLatin1String(kUdpRemotePortKey), 9526).toUInt());
    m_udpConfig.localPort = static_cast<quint16>(m_store.value(
        QLatin1String(kUdpLocalPortKey), 9527).toUInt());
    m_tcpServerConfig.listenPort = static_cast<quint16>(m_store.value(
        QLatin1String(kTcpServerPortKey), 9526).toUInt());
    m_tcpClientConfig.host = m_store.value(
        QLatin1String(kTcpClientHostKey),
        QStringLiteral("127.0.0.1")).toString();
    m_tcpClientConfig.port = static_cast<quint16>(m_store.value(
        QLatin1String(kTcpClientPortKey), 9527).toUInt());
    m_tcpClientConfig.connectionName =
        m_store.value(QLatin1String(kTcpClientNameKey)).toString();
    m_virtualDataConfig.sampleIntervalMs = m_store.value(
        QLatin1String(kVirtualIntervalKey), 20.0).toDouble();
    m_virtualDataConfig.signalFrequencyHz = m_store.value(
        QLatin1String(kVirtualFrequencyKey), 0.5).toDouble();
    m_virtualDataConfig.amplitude = m_store.value(
        QLatin1String(kVirtualAmplitudeKey), 1.0).toDouble();
    m_virtualDataConfig.channelCount = m_store.value(
        QLatin1String(kVirtualChannelsKey), 10).toInt();

    m_parserMode = static_cast<ParserMode>(qBound(
        enumValue(ParserMode::RawData),
        m_store.value(QLatin1String(kParserModeKey),
                      enumValue(ParserMode::RawData)).toInt(),
        enumValue(ParserMode::CustomBinary)));
    m_customProtocolId =
        m_store.value(QLatin1String(kCustomProtocolKey)).toString();
    m_terminalDisplayMode = static_cast<TerminalDisplayMode>(qBound(
        enumValue(TerminalDisplayMode::Text),
        m_store.value(QLatin1String(kTerminalDisplayKey),
                      enumValue(TerminalDisplayMode::Text)).toInt(),
        enumValue(TerminalDisplayMode::Hex)));
    m_terminalTimestampEnabled =
        m_store.value(QLatin1String(kTimestampKey), true).toBool();
    m_textEncoding = static_cast<TextEncoding>(qBound(
        enumValue(TextEncoding::Utf8),
        m_store.value(QLatin1String(kEncodingKey),
                      enumValue(TextEncoding::Utf8)).toInt(),
        enumValue(TextEncoding::Latin1)));
    m_receiveVisible =
        m_store.value(QLatin1String(kReceiveVisibleKey), true).toBool();
    m_transmitVisible =
        m_store.value(QLatin1String(kTransmitVisibleKey), true).toBool();
    m_ansiEnabled = m_store.value(QLatin1String(kAnsiKey), false).toBool();
    m_inputMode = static_cast<InputMode>(qBound(
        enumValue(InputMode::Text),
        m_store.value(QLatin1String(kInputModeKey),
                      enumValue(InputMode::Text)).toInt(),
        enumValue(InputMode::Hex)));
    m_checksumMode = static_cast<ChecksumMode>(qBound(
        enumValue(ChecksumMode::None),
        m_store.value(QLatin1String(kChecksumModeKey),
                      enumValue(ChecksumMode::None)).toInt(),
        enumValue(ChecksumMode::Crc8Maxim)));
    m_lineEnding = static_cast<LineEnding>(qBound(
        enumValue(LineEnding::None),
        m_store.value(QLatin1String(kLineEndingKey),
                      enumValue(LineEnding::None)).toInt(),
        enumValue(LineEnding::CRLF)));
}
