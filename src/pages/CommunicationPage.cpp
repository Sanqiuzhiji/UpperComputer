#include "CommunicationPage.h"

#include "app/AppContext.h"
#include "app/AppSettings.h"
#include "pages/communication/CommunicationModePanel.h"
#include "pages/communication/CommunicationMonitorPanel.h"
#include "pages/communication/HardwareConfigPanel.h"
#include "pages/communication/SendPanel.h"
#include "services/ConnectionManager.h"
#include "services/CustomBinaryCodec.h"
#include "services/ProtocolRepository.h"
#include "services/ReceiveDataPipeline.h"

#include <QSplitter>
#include <QVBoxLayout>

#include <algorithm>

CommunicationPage::CommunicationPage(
    AppContext *context, QWidget *parent)
    : QWidget(parent),
      m_context(context)
{
    setObjectName(QStringLiteral("communicationPage"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 12, 20, 12);
    layout->setSpacing(8);

    m_hardwarePanel = new HardwareConfigPanel(context, this);
    m_modePanel = new CommunicationModePanel(context, this);
    layout->addWidget(m_hardwarePanel);
    layout->addWidget(m_modePanel);

    m_dataSplitter = new QSplitter(Qt::Vertical, this);
    m_dataSplitter->setObjectName(QStringLiteral("communicationDataSplitter"));
    m_dataSplitter->setChildrenCollapsible(false);
    m_monitorPanel = new CommunicationMonitorPanel(context, this);
    m_sendPanel = new SendPanel(context, this);
    m_dataSplitter->addWidget(m_monitorPanel);
    m_dataSplitter->addWidget(m_sendPanel);
    m_dataSplitter->setStretchFactor(0, 3);
    m_dataSplitter->setStretchFactor(1, 2);
    m_dataSplitter->setSizes({360, 130});
    layout->addWidget(m_dataSplitter, 1);

    ConnectionManager *manager = context->connectionManager();
    m_previousState = manager->state();
    m_hardwarePanel->setConnectionState(manager->state());
    m_sendPanel->setTextEncoding(m_monitorPanel->textEncoding());
    m_monitorPanel->setReceiveMode(m_modePanel->receiveMode());
    m_sendPanel->setSendMode(m_modePanel->sendMode());
    setProtocols(context->protocolRepository()->communicationDefinitions());
    connect(context->protocolRepository(),
            &ProtocolRepository::protocolLibraryChanged,
            this, [this, context] {
                setProtocols(
                    context->protocolRepository()->communicationDefinitions());
            });

    connect(m_hardwarePanel, &HardwareConfigPanel::connectRequested,
            manager, &ConnectionManager::connectTransport);
    connect(m_hardwarePanel, &HardwareConfigPanel::disconnectRequested,
            manager, &ConnectionManager::disconnectTransport);
    connect(m_hardwarePanel, &HardwareConfigPanel::tcpServerTargetChanged,
            manager, &ConnectionManager::setTcpServerTarget);
    connect(manager, &ConnectionManager::stateChanged,
            m_hardwarePanel, &HardwareConfigPanel::setConnectionState);
    connect(manager, &ConnectionManager::firmwareOperationActiveChanged,
            m_hardwarePanel,
            &HardwareConfigPanel::setFirmwareOperationActive);
    m_hardwarePanel->setFirmwareOperationActive(
        manager->firmwareOperationActive());
    connect(manager, &ConnectionManager::tcpClientsChanged,
            m_hardwarePanel, &HardwareConfigPanel::setTcpClients);
    ReceiveDataPipeline *pipeline = context->receiveDataPipeline();
    connect(pipeline, &ReceiveDataPipeline::rawDataReceived,
            m_monitorPanel, &CommunicationMonitorPanel::addReceivedData);
    connect(pipeline, &ReceiveDataPipeline::parsedMessagesReceived,
            this, [this](
                const qint64 timestampUs,
                const QString &,
                const QList<ParsedMessage> &messages) {
                m_monitorPanel->addParsedMessages(timestampUs, messages);
            });
    connect(pipeline, &ReceiveDataPipeline::parseFailed,
            this, [this](const QString &message) {
                notify(message, NotificationType::Warning);
            });
    connect(manager, &ConnectionManager::monitorDataSent, this,
            [this](const QByteArray &bytes,
                   const CommunicationTrafficSource source) {
                if (source == CommunicationTrafficSource::CescFirmware
                    && !m_context->settings()->showCescFirmwareTraffic()) {
                    return;
                }
                m_monitorPanel->addEntry(DataDirection::Transmit, bytes);
            });
    connect(manager, &ConnectionManager::errorOccurred, this,
            [this](const QString &error) {
                notify(tr("连接或通信失败：%1").arg(error),
                       NotificationType::Error);
            });
    connect(manager, &ConnectionManager::stateChanged, this,
            [this](const ConnectionState state) {
                if (state == ConnectionState::Connected) {
                    notify(tr("连接成功"), NotificationType::Success);
                } else if (state == ConnectionState::Disconnected
                           && (m_previousState == ConnectionState::Connected
                               || m_previousState
                                   == ConnectionState::Disconnecting)) {
                    notify(tr("已断开连接"),
                           NotificationType::Information);
                }
                m_previousState = state;
            });

    connect(m_monitorPanel,
            &CommunicationMonitorPanel::textEncodingChanged,
            m_sendPanel, &SendPanel::setTextEncoding);
    connect(m_sendPanel, &SendPanel::sendRequested, this,
            [this, manager](const QByteArray &bytes) {
                QString error;
                if (!manager->send(bytes, &error)) {
                    notify(error.isEmpty() ? tr("发送失败") : error,
                           NotificationType::Warning);
                }
            });

    const auto forwardNotification =
        [this](const QString &message, const NotificationType type) {
            notify(message, type);
        };
    connect(m_hardwarePanel, &HardwareConfigPanel::notificationRequested,
            this, forwardNotification);
    connect(m_monitorPanel,
            &CommunicationMonitorPanel::notificationRequested,
            this, forwardNotification);
    connect(m_sendPanel, &SendPanel::notificationRequested,
            this, forwardNotification);
    connect(m_modePanel, &CommunicationModePanel::helpRequested, this,
            [this](const QString &message) {
                notify(message, NotificationType::Information);
            });
    connect(m_modePanel, &CommunicationModePanel::receiveModeChanged,
            this, [this](const ParserMode mode) {
                m_monitorPanel->setReceiveMode(mode);
                m_context->receiveDataPipeline()->setParserMode(mode);
                emit receiveModeChanged(mode);
                notify(tr("接收解析模式已切换，当前显示内容已清空"),
                       NotificationType::Information);
            });
    connect(m_modePanel, &CommunicationModePanel::sendModeChanged,
            this, [this](const SendMode mode) {
                m_sendPanel->setSendMode(mode);
                m_dataSplitter->setSizes(
                    mode == SendMode::RawData
                        ? QList<int>{420, 82}
                        : QList<int>{290, 210});
                emit sendModeChanged(mode);
            });
    connect(m_modePanel, &CommunicationModePanel::customProtocolChanged,
            this, [this](const QString &protocolId) {
                m_sendPanel->setCurrentProtocolId(protocolId);
                updateReceivePipeline();
                emit customProtocolChanged(protocolId);
            });
    connect(m_modePanel, &CommunicationModePanel::receiveCommandChanged,
            this, [this](const QString &) { updateReceivePipeline(); });
    connect(m_modePanel, &CommunicationModePanel::sendCommandChanged,
            m_sendPanel, &SendPanel::setCurrentCommandId);
    connect(m_modePanel, &CommunicationModePanel::requestProtocolLibrary,
            this, &CommunicationPage::requestProtocolLibrary);
}

void CommunicationPage::setProtocols(
    const QList<ProtocolDefinition> &protocols)
{
    m_protocols = protocols;
    m_customCodecs.clear();
    m_modePanel->setProtocols(protocols);
    m_sendPanel->setProtocols(protocols);
    for (const ProtocolDefinition &protocol : protocols) {
        auto codec = std::make_shared<CustomBinaryCodec>(protocol);
        m_customCodecs.insert(protocol.id, codec);
        m_sendPanel->setEncoder(protocol.id, codec);
    }
    m_sendPanel->setCurrentProtocolId(m_modePanel->currentProtocolId());
    m_sendPanel->setCurrentCommandId(
        m_modePanel->currentSendCommandId());
    updateReceivePipeline();
}

void CommunicationPage::setCustomBinaryEncoder(
    const QString &protocolId,
    std::shared_ptr<const CustomBinaryEncoder> encoder)
{
    m_sendPanel->setEncoder(protocolId, std::move(encoder));
}

void CommunicationPage::updateReceivePipeline()
{
    m_context->receiveDataPipeline()->setCustomProtocol(
        m_modePanel->currentProtocolId(),
        m_modePanel->currentReceiveCommandId());
}

void CommunicationPage::notify(
    const QString &message, const NotificationType type) const
{
    m_context->notify(message, type);
}
