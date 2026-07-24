#include "ConnectionPage.h"

#include "app/AppContext.h"
#include "pages/connection/HardwareConfigPanel.h"
#include "pages/connection/ParserConfigPanel.h"
#include "pages/connection/SendPanel.h"
#include "pages/connection/TerminalPanel.h"
#include "services/ConnectionManager.h"

#include <QVBoxLayout>

ConnectionPage::ConnectionPage(AppContext *context, QWidget *parent)
    : QWidget(parent),
      m_context(context)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(10);

    m_hardwarePanel = new HardwareConfigPanel(context, this);
    m_parserPanel = new ParserConfigPanel(context, this);
    m_terminalPanel = new TerminalPanel(context, this);
    m_sendPanel = new SendPanel(context, this);
    layout->addWidget(m_hardwarePanel);
    layout->addWidget(m_parserPanel);
    layout->addWidget(m_terminalPanel, 1);
    layout->addWidget(m_sendPanel);

    ConnectionManager *manager = context->connectionManager();
    m_previousState = manager->state();
    m_hardwarePanel->setConnectionState(manager->state());
    m_sendPanel->setTextEncoding(m_terminalPanel->textEncoding());

    connect(m_hardwarePanel, &HardwareConfigPanel::connectRequested,
            manager, &ConnectionManager::connectTransport);
    connect(m_hardwarePanel, &HardwareConfigPanel::disconnectRequested,
            manager, &ConnectionManager::disconnectTransport);
    connect(m_hardwarePanel, &HardwareConfigPanel::tcpServerTargetChanged,
            manager, &ConnectionManager::setTcpServerTarget);
    connect(manager, &ConnectionManager::stateChanged,
            m_hardwarePanel, &HardwareConfigPanel::setConnectionState);
    connect(manager, &ConnectionManager::tcpClientsChanged,
            m_hardwarePanel, &HardwareConfigPanel::setTcpClients);
    connect(manager, &ConnectionManager::dataReceived, this,
            [this](const QByteArray &bytes) {
                m_terminalPanel->addEntry(DataDirection::Receive, bytes);
            });
    connect(manager, &ConnectionManager::dataSent, this,
            [this](const QByteArray &bytes) {
                m_terminalPanel->addEntry(DataDirection::Transmit, bytes);
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
                    notify(tr("已断开连接"), NotificationType::Information);
                }
                m_previousState = state;
            });

    connect(m_terminalPanel, &TerminalPanel::textEncodingChanged,
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
    connect(m_terminalPanel, &TerminalPanel::notificationRequested,
            this, forwardNotification);
    connect(m_sendPanel, &SendPanel::notificationRequested,
            this, forwardNotification);
    connect(m_parserPanel, &ParserConfigPanel::helpRequested, this,
            [this](const QString &message) {
                notify(message, NotificationType::Information);
            });
    connect(m_parserPanel, &ParserConfigPanel::parserModeChanged,
            this, [this](const ParserMode mode) {
                emit parserModeChanged(mode);
                if (mode == ParserMode::JustFloat
                    || mode == ParserMode::FireWater) {
                    notify(tr("当前协议解析待接入，数据将按 RawData 显示"),
                           NotificationType::Information);
                }
            });
    connect(m_parserPanel, &ParserConfigPanel::customProtocolChanged,
            this, &ConnectionPage::customProtocolChanged);
    connect(m_parserPanel, &ParserConfigPanel::requestProtocolLibrary,
            this, &ConnectionPage::requestProtocolLibrary);
}

void ConnectionPage::notify(
    const QString &message, const NotificationType type) const
{
    m_context->notify(message, type);
}
