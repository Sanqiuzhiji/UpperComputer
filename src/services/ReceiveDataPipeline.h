#pragma once

#include <QElapsedTimer>
#include <QObject>

#include <memory>

#include "models/ConnectionTypes.h"
#include "models/ProtocolDefinition.h"

class AppSettings;
class ChannelDataHub;
class ConnectionManager;
class CustomBinaryCodec;
class ProtocolRepository;

class ReceiveDataPipeline final : public QObject
{
    Q_OBJECT

public:
    ReceiveDataPipeline(
        AppSettings *settings,
        ConnectionManager *connectionManager,
        ProtocolRepository *protocolRepository,
        ChannelDataHub *channelDataHub,
        QObject *parent = nullptr);
    ~ReceiveDataPipeline() override;

    [[nodiscard]] ParserMode parserMode() const noexcept;
    [[nodiscard]] QString customProtocolId() const;
    [[nodiscard]] QString customReceiveMessageId() const;

public slots:
    void setParserMode(ParserMode mode);
    void setCustomProtocol(
        const QString &protocolId,
        const QString &receiveMessageId);

signals:
    void rawDataReceived(qint64 timestampUs, const QByteArray &data);
    void parsedMessagesReceived(
        qint64 timestampUs,
        const QString &protocolId,
        const QList<ParsedMessage> &messages);
    void parseFailed(const QString &message);

private:
    void handleData(const QByteArray &data);
    void publishVirtualData(qint64 timestampUs, const QByteArray &data);
    void rebuildParser();
    [[nodiscard]] qint64 timestampUs() const;

    AppSettings *m_settings{};
    ConnectionManager *m_connectionManager{};
    ProtocolRepository *m_protocolRepository{};
    ChannelDataHub *m_channelDataHub{};
    ParserMode m_parserMode{ParserMode::RawData};
    QString m_protocolId;
    QString m_receiveMessageId;
    std::unique_ptr<CustomBinaryCodec> m_customBinaryParser;
    QElapsedTimer m_monotonicClock;
    qint64 m_epochBaseUs{};
};
