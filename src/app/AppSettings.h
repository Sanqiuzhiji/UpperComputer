#pragma once

#include <QByteArray>
#include <QObject>
#include <QSettings>
#include <QVariantMap>

#include "models/AppTypes.h"
#include "models/ConnectionTypes.h"
#include "pages/PageId.h"

class AppSettings final : public QObject
{
    Q_OBJECT

public:
    explicit AppSettings(QObject *parent = nullptr);

    [[nodiscard]] ThemeMode themeMode() const noexcept;
    [[nodiscard]] NavigationMode navigationMode() const noexcept;
    [[nodiscard]] bool userCardVisible() const noexcept;
    [[nodiscard]] PageId lastPage() const noexcept;
    [[nodiscard]] QByteArray windowGeometry() const;
    [[nodiscard]] bool windowMaximized() const noexcept;
    [[nodiscard]] TransportType transportType() const noexcept;
    [[nodiscard]] SerialConfig serialConfig() const;
    [[nodiscard]] UdpConfig udpConfig() const;
    [[nodiscard]] TcpServerConfig tcpServerConfig() const;
    [[nodiscard]] TcpClientConfig tcpClientConfig() const;
    [[nodiscard]] VirtualDataConfig virtualDataConfig() const;
    [[nodiscard]] ParserMode parserMode() const noexcept;
    [[nodiscard]] SendMode sendMode() const noexcept;
    [[nodiscard]] QString customProtocolId() const;
    [[nodiscard]] QString customReceiveCommandId() const;
    [[nodiscard]] TerminalDisplayMode terminalDisplayMode() const noexcept;
    [[nodiscard]] bool terminalTimestampEnabled() const noexcept;
    [[nodiscard]] TextEncoding textEncoding() const noexcept;
    [[nodiscard]] bool receiveVisible() const noexcept;
    [[nodiscard]] bool transmitVisible() const noexcept;
    [[nodiscard]] bool ansiEnabled() const noexcept;
    [[nodiscard]] InputMode inputMode() const noexcept;
    [[nodiscard]] ChecksumMode checksumMode() const noexcept;
    [[nodiscard]] LineEnding lineEnding() const noexcept;
    [[nodiscard]] QString rawTextDraft() const;
    [[nodiscard]] QString rawHexDraft() const;
    [[nodiscard]] QString customCommandId() const;
    [[nodiscard]] QVariantMap customFieldDrafts() const;
    [[nodiscard]] QString workspaceDirectory() const;
    [[nodiscard]] static QString defaultWorkspaceDirectory();

public slots:
    void setThemeMode(ThemeMode mode);
    void setNavigationMode(NavigationMode mode);
    void setUserCardVisible(bool visible);
    void setLastPage(PageId page);
    void setWindowGeometry(const QByteArray &geometry);
    void setWindowMaximized(bool maximized);
    void setTransportType(TransportType type);
    void setSerialConfig(const SerialConfig &config);
    void setUdpConfig(const UdpConfig &config);
    void setTcpServerConfig(const TcpServerConfig &config);
    void setTcpClientConfig(const TcpClientConfig &config);
    void setVirtualDataConfig(const VirtualDataConfig &config);
    void setParserMode(ParserMode mode);
    void setSendMode(SendMode mode);
    void setCustomProtocolId(const QString &protocolId);
    void setCustomReceiveCommandId(const QString &commandId);
    void setTerminalDisplayMode(TerminalDisplayMode mode);
    void setTerminalTimestampEnabled(bool enabled);
    void setTextEncoding(TextEncoding encoding);
    void setReceiveVisible(bool visible);
    void setTransmitVisible(bool visible);
    void setAnsiEnabled(bool enabled);
    void setInputMode(InputMode mode);
    void setChecksumMode(ChecksumMode mode);
    void setLineEnding(LineEnding ending);
    void setRawTextDraft(const QString &draft);
    void setRawHexDraft(const QString &draft);
    void setCustomCommandId(const QString &commandId);
    void setCustomFieldDrafts(const QVariantMap &drafts);
    void setWorkspaceDirectory(const QString &directory);

signals:
    void workspaceDirectoryChanged(const QString &directory);

private:
    void load();

    QSettings m_store;
    ThemeMode m_themeMode{ThemeMode::Dark};
    NavigationMode m_navigationMode{NavigationMode::Expanded};
    bool m_userCardVisible{true};
    PageId m_lastPage{PageId::Plot};
    QByteArray m_windowGeometry;
    bool m_windowMaximized{true};
    TransportType m_transportType{TransportType::SerialPort};
    SerialConfig m_serialConfig;
    UdpConfig m_udpConfig;
    TcpServerConfig m_tcpServerConfig;
    TcpClientConfig m_tcpClientConfig;
    VirtualDataConfig m_virtualDataConfig;
    ParserMode m_parserMode{ParserMode::RawData};
    SendMode m_sendMode{SendMode::RawData};
    QString m_customProtocolId;
    QString m_customReceiveCommandId;
    TerminalDisplayMode m_terminalDisplayMode{TerminalDisplayMode::Text};
    bool m_terminalTimestampEnabled{true};
    TextEncoding m_textEncoding{TextEncoding::Utf8};
    bool m_receiveVisible{true};
    bool m_transmitVisible{true};
    bool m_ansiEnabled{};
    InputMode m_inputMode{InputMode::Text};
    ChecksumMode m_checksumMode{ChecksumMode::None};
    LineEnding m_lineEnding{LineEnding::None};
    QString m_rawTextDraft;
    QString m_rawHexDraft;
    QString m_customCommandId;
    QVariantMap m_customFieldDrafts;
    QString m_workspaceDirectory;
};
