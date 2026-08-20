#include "ReceiveDataPipeline.h"

#include "app/AppSettings.h"
#include "services/ChannelDataHub.h"
#include "services/BuiltInStreamParser.h"
#include "services/ConnectionManager.h"
#include "services/CustomBinaryCodec.h"
#include "services/ProtocolRepository.h"

#include <QDateTime>
#include <QDataStream>
#include <QSet>

#include <algorithm>

ReceiveDataPipeline::ReceiveDataPipeline(
    AppSettings *settings,
    ConnectionManager *connectionManager,
    ProtocolRepository *protocolRepository,
    ChannelDataHub *channelDataHub,
    QObject *parent)
    : QObject(parent),
      m_settings(settings),
      m_connectionManager(connectionManager),
      m_protocolRepository(protocolRepository),
      m_channelDataHub(channelDataHub),
      m_parserMode(settings->parserMode()),
      m_protocolId(settings->customProtocolId()),
      m_receiveMessageId(settings->customReceiveCommandId()),
      m_epochBaseUs(QDateTime::currentMSecsSinceEpoch() * 1000)
{
    m_monotonicClock.start();
    connect(connectionManager, &ConnectionManager::monitorDataReceived,
            this, &ReceiveDataPipeline::handleData);
    connect(protocolRepository, &ProtocolRepository::protocolLibraryChanged,
            this, &ReceiveDataPipeline::rebuildParser);
    rebuildParser();
}

ReceiveDataPipeline::~ReceiveDataPipeline() = default;

ParserMode ReceiveDataPipeline::parserMode() const noexcept
{
    return m_parserMode;
}

QString ReceiveDataPipeline::customProtocolId() const
{
    return m_protocolId;
}

QString ReceiveDataPipeline::customReceiveMessageId() const
{
    return m_receiveMessageId;
}

void ReceiveDataPipeline::setParserMode(const ParserMode mode)
{
    if (m_parserMode == mode) return;
    m_parserMode = mode;
    m_settings->setParserMode(mode);
    rebuildParser();
}

void ReceiveDataPipeline::setCustomProtocol(
    const QString &protocolId, const QString &receiveMessageId)
{
    if (m_protocolId == protocolId
        && m_receiveMessageId == receiveMessageId) {
        return;
    }
    m_protocolId = protocolId;
    m_receiveMessageId = receiveMessageId;
    m_settings->setCustomProtocolId(protocolId);
    m_settings->setCustomReceiveCommandId(receiveMessageId);
    rebuildParser();
}

void ReceiveDataPipeline::handleData(
    const QByteArray &data, const CommunicationTrafficSource source)
{
    if (data.isEmpty()) return;
    if (source == CommunicationTrafficSource::CescFirmware
        && !m_settings->showCescFirmwareTraffic()) {
        return;
    }
    const qint64 nowUs = timestampUs();
    emit rawDataReceived(nowUs, data);
    if (source == CommunicationTrafficSource::CescNative
        || source == CommunicationTrafficSource::CescFirmware) return;
    if (m_connectionManager->transportType()
        == TransportType::VirtualData) {
        publishVirtualData(nowUs, data);
        return;
    }
    if (m_builtInParser) {
        QList<ParsedMessage> messages;
        QString error;
        const bool valid = m_builtInParser->parse(data, &messages, &error);
        if (!valid) emit parseFailed(error);
        if (messages.isEmpty()) return;
        const QString protocolId = m_builtInParser->protocolId();
        m_channelDataHub->publish(nowUs, protocolId, messages);
        emit parsedMessagesReceived(nowUs, protocolId, messages);
        return;
    }
    if (m_parserMode != ParserMode::CustomBinary || !m_customBinaryParser) {
        return;
    }
    QList<ParsedMessage> messages;
    QString error;
    if (!m_customBinaryParser->parse(data, &messages, &error)) {
        emit parseFailed(
            error.isEmpty() ? tr("接收数据解析失败") : error);
        return;
    }
    if (messages.isEmpty()) return;
    m_channelDataHub->publish(nowUs, m_protocolId, messages);
    emit parsedMessagesReceived(nowUs, m_protocolId, messages);
}

void ReceiveDataPipeline::publishVirtualData(
    const qint64 timestampUs, const QByteArray &data)
{
    constexpr qsizetype floatBytes = sizeof(float);
    if (data.isEmpty() || data.size() % floatBytes != 0) {
        emit parseFailed(tr("虚拟数据帧长度无效"));
        return;
    }
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    ParsedMessage message;
    message.messageId = QStringLiteral("generated-signals");
    message.displayName = tr("虚拟测试信号");
    const qsizetype channelCount = data.size() / floatBytes;
    const QStringList signalNames{
        tr("正弦波"),
        tr("方波"),
        tr("方波傅里叶 1 阶"),
        tr("方波傅里叶 3 阶"),
        tr("方波傅里叶 5 阶")};
    message.fields.reserve(channelCount);
    for (qsizetype index = 0; index < channelCount; ++index) {
        float value = 0.0F;
        stream >> value;
        if (stream.status() != QDataStream::Ok) {
            emit parseFailed(tr("虚拟数据帧读取失败"));
            return;
        }
        const QString number = QString::number(index + 1);
        message.fields.append({
            QStringLiteral("signal-%1").arg(number),
            index < signalNames.size()
                ? signalNames.at(index)
                : tr("虚拟信号 %1").arg(number),
            value,
            tr("a.u."),
            ProtocolFieldRole::Value
        });
    }
    const QList<ParsedMessage> messages{message};
    const QString protocolId = QStringLiteral("virtual-data");
    m_channelDataHub->publish(timestampUs, protocolId, messages);
    if (m_parserMode == ParserMode::JustFloat) {
        emit parsedMessagesReceived(timestampUs, protocolId, messages);
    }
}

void ReceiveDataPipeline::rebuildParser()
{
    m_customBinaryParser.reset();
    m_builtInParser.reset();
    QList<ProtocolDefinition> definitions =
        m_protocolRepository->communicationDefinitions();
    QSet<QString> validChannelIds;
    for (int channel = 1; channel <= 256; ++channel) {
        validChannelIds.insert(ChannelDataHub::channelId(
            QStringLiteral("virtual-data"),
            QStringLiteral("generated-signals"),
            QStringLiteral("signal-%1").arg(channel)));
    }
    for (const ProtocolDefinition &protocol : definitions) {
        for (const MessageDefinition &message : protocol.receiveMessages) {
            for (const FieldDefinition &field : message.fields) {
                if (field.role != ProtocolFieldRole::Value
                    || protocol.id.isEmpty()
                    || message.id.isEmpty()
                    || field.id.isEmpty()) {
                    continue;
                }
                validChannelIds.insert(ChannelDataHub::channelId(
                    protocol.id, message.id, field.id));
            }
        }
    }
    m_channelDataHub->retainChannels(validChannelIds);
    if (m_parserMode == ParserMode::FireWater
        || m_parserMode == ParserMode::JustFloat) {
        m_builtInParser = std::make_unique<BuiltInStreamParser>(m_parserMode);
        return;
    }
    if (m_parserMode != ParserMode::CustomBinary) return;
    for (ProtocolDefinition &protocol : definitions) {
        if (protocol.id != m_protocolId) continue;
        if (!m_receiveMessageId.isEmpty()) {
            protocol.receiveMessages.erase(
                std::remove_if(
                    protocol.receiveMessages.begin(),
                    protocol.receiveMessages.end(),
                    [this](const MessageDefinition &message) {
                        return message.id != m_receiveMessageId;
                    }),
                protocol.receiveMessages.end());
        }
        if (!protocol.receiveMessages.isEmpty()) {
            m_customBinaryParser =
                std::make_unique<CustomBinaryCodec>(std::move(protocol));
        }
        return;
    }
}

qint64 ReceiveDataPipeline::timestampUs() const
{
    return m_epochBaseUs + m_monotonicClock.nsecsElapsed() / 1000;
}
