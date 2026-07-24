#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QtSerialPort/QSerialPort>

#include <variant>

enum class TransportType {
    SerialPort,
    Udp,
    TcpServer,
    TcpClient,
    VirtualData
};

enum class ParserMode {
    RawData,
    JustFloat,
    FireWater,
    CustomBinary
};

enum class DataDirection {
    Receive,
    Transmit
};

enum class TerminalDisplayMode {
    Text,
    Hex
};

enum class TextEncoding {
    Utf8,
    Local8Bit,
    Latin1
};

enum class InputMode {
    Text,
    Hex
};

enum class ChecksumMode {
    None,
    Xor8,
    Crc8,
    Crc8Maxim
};

enum class LineEnding {
    None,
    LF,
    CR,
    LFCR,
    CRLF
};

struct SerialConfig {
    QString portName;
    qint32 baudRate{115200};
    QSerialPort::DataBits dataBits{QSerialPort::Data8};
    QSerialPort::Parity parity{QSerialPort::NoParity};
    QSerialPort::StopBits stopBits{QSerialPort::OneStop};

    bool operator==(const SerialConfig &) const = default;
};

struct UdpConfig {
    QString remoteAddress{QStringLiteral("127.0.0.1")};
    quint16 remotePort{9526};
    quint16 localPort{9527};

    bool operator==(const UdpConfig &) const = default;
};

struct TcpServerConfig {
    quint16 listenPort{9526};

    bool operator==(const TcpServerConfig &) const = default;
};

struct TcpClientConfig {
    QString host{QStringLiteral("127.0.0.1")};
    quint16 port{9527};
    QString connectionName;

    bool operator==(const TcpClientConfig &) const = default;
};

struct VirtualDataConfig {
    double sampleIntervalMs{20.0};
    double signalFrequencyHz{0.5};
    double amplitude{1.0};
    int channelCount{10};

    bool operator==(const VirtualDataConfig &) const = default;
};

using TransportConfig = std::variant<
    SerialConfig,
    UdpConfig,
    TcpServerConfig,
    TcpClientConfig,
    VirtualDataConfig>;

struct TerminalEntry {
    QDateTime timestamp;
    DataDirection direction{DataDirection::Receive};
    QByteArray rawData;
};

Q_DECLARE_METATYPE(TransportType)
Q_DECLARE_METATYPE(ParserMode)
Q_DECLARE_METATYPE(DataDirection)
Q_DECLARE_METATYPE(TerminalDisplayMode)
Q_DECLARE_METATYPE(TextEncoding)
Q_DECLARE_METATYPE(InputMode)
Q_DECLARE_METATYPE(ChecksumMode)
Q_DECLARE_METATYPE(LineEnding)
Q_DECLARE_METATYPE(SerialConfig)
Q_DECLARE_METATYPE(UdpConfig)
Q_DECLARE_METATYPE(TcpServerConfig)
Q_DECLARE_METATYPE(TcpClientConfig)
Q_DECLARE_METATYPE(VirtualDataConfig)
Q_DECLARE_METATYPE(TransportConfig)
