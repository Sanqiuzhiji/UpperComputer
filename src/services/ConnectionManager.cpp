#include "ConnectionManager.h"

#include "services/transports/AbstractTransport.h"
#include "services/transports/NetworkTransports.h"
#include "services/transports/SerialTransport.h"
#include "services/transports/VirtualTransport.h"

#include <QTimer>

ConnectionManager::ConnectionManager(QObject *parent)
    : QObject(parent),
      m_rateTimer(new QTimer(this))
{
    m_rateTimer->setInterval(1000);
    connect(m_rateTimer, &QTimer::timeout,
            this, &ConnectionManager::updateRates);
    m_rateTimer->start();
}

ConnectionState ConnectionManager::state() const noexcept
{
    return m_state;
}

QString ConnectionManager::deviceName() const
{
    return m_deviceName;
}

QString ConnectionManager::dataSourceName() const
{
    return m_dataSourceName;
}

double ConnectionManager::receiveRate() const noexcept
{
    return m_receiveRate;
}

double ConnectionManager::transmitRate() const noexcept
{
    return m_transmitRate;
}

quint64 ConnectionManager::receiveTotal() const noexcept
{
    return m_receiveTotal;
}

quint64 ConnectionManager::transmitTotal() const noexcept
{
    return m_transmitTotal;
}

TransportType ConnectionManager::transportType() const noexcept
{
    return m_transportType;
}

bool ConnectionManager::canSend() const noexcept
{
    return m_state == ConnectionState::Connected
        && m_transport && m_transport->isOpen()
        && m_transportType != TransportType::VirtualData;
}

void ConnectionManager::connectTransport(
    const TransportType type, const TransportConfig &config)
{
    if (m_state == ConnectionState::Connecting
        || m_state == ConnectionState::Connected
        || m_state == ConnectionState::Disconnecting) {
        emit errorOccurred(tr("请先断开当前连接"));
        return;
    }

    if (m_transport) {
        m_transport->disconnect(this);
        m_transport->close();
        m_transport->deleteLater();
        m_transport = nullptr;
    }
    m_transportType = type;
    setDeviceName(deviceName(type, config));
    setDataSourceName(sourceName(type));
    createTransport(type);
    if (!m_transport) {
        setState(ConnectionState::Error);
        emit errorOccurred(tr("无法创建通信传输对象"));
        return;
    }
    m_transport->open(config);
}

void ConnectionManager::disconnectTransport()
{
    if (!m_transport
        || m_state == ConnectionState::Disconnected
        || m_state == ConnectionState::Disconnecting) {
        return;
    }
    setState(ConnectionState::Disconnecting);
    m_transport->close();
}

bool ConnectionManager::send(const QByteArray &data, QString *errorMessage)
{
    if (data.isEmpty()) {
        if (errorMessage) *errorMessage = tr("发送数据为空");
        return false;
    }
    if (m_transportType == TransportType::VirtualData) {
        if (errorMessage) *errorMessage = tr("虚拟数据模式不支持发送");
        return false;
    }
    if (!canSend()) {
        if (errorMessage) {
            *errorMessage = tr("当前连接尚未就绪，无法发送命令。");
        }
        return false;
    }
    QString transportError;
    if (!m_transport->write(data, &transportError)) {
        if (errorMessage) *errorMessage = transportError;
        return false;
    }
    emit dataSent(data);
    return true;
}

void ConnectionManager::setTcpServerTarget(const QString &clientId)
{
    if (auto *server = qobject_cast<TcpServerTransport *>(m_transport)) {
        server->setSelectedClient(clientId);
    }
}

void ConnectionManager::createTransport(const TransportType type)
{
    switch (type) {
    case TransportType::SerialPort:
        m_transport = new SerialTransport(this);
        break;
    case TransportType::Udp:
        m_transport = new UdpTransport(this);
        break;
    case TransportType::TcpServer:
        m_transport = new TcpServerTransport(this);
        break;
    case TransportType::TcpClient:
        m_transport = new TcpClientTransport(this);
        break;
    case TransportType::VirtualData:
        m_transport = new VirtualTransport(this);
        break;
    }

    connect(m_transport, &AbstractTransport::stateChanged,
            this, &ConnectionManager::setState);
    connect(m_transport, &AbstractTransport::errorOccurred,
            this, &ConnectionManager::errorOccurred);
    connect(m_transport, &AbstractTransport::dataReceived, this,
            [this](const QByteArray &data) {
                m_receiveTotal += static_cast<quint64>(data.size());
                emit receiveTotalChanged(m_receiveTotal);
                emit dataReceived(data);
            });
    connect(m_transport, &AbstractTransport::dataWritten, this,
            [this](const qint64 bytes) {
                if (bytes <= 0) return;
                m_transmitTotal += static_cast<quint64>(bytes);
                emit transmitTotalChanged(m_transmitTotal);
            });
    if (auto *server = qobject_cast<TcpServerTransport *>(m_transport)) {
        connect(server, &TcpServerTransport::clientsChanged,
                this, &ConnectionManager::tcpClientsChanged);
    }
}

void ConnectionManager::setState(const ConnectionState state)
{
    if (m_state == state) return;
    m_state = state;
    if (state == ConnectionState::Disconnected) {
        setDeviceName(QString());
        setDataSourceName(QString());
        emit tcpClientsChanged({});
    }
    emit stateChanged(state);
}

void ConnectionManager::setDeviceName(const QString &name)
{
    if (m_deviceName == name) return;
    m_deviceName = name;
    emit deviceNameChanged(name);
}

void ConnectionManager::setDataSourceName(const QString &name)
{
    if (m_dataSourceName == name) return;
    m_dataSourceName = name;
    emit dataSourceNameChanged(name);
}

void ConnectionManager::updateRates()
{
    m_receiveRate = static_cast<double>(m_receiveTotal - m_lastReceiveTotal);
    m_transmitRate = static_cast<double>(m_transmitTotal - m_lastTransmitTotal);
    m_lastReceiveTotal = m_receiveTotal;
    m_lastTransmitTotal = m_transmitTotal;
    emit receiveRateChanged(m_receiveRate);
    emit transmitRateChanged(m_transmitRate);
}

QString ConnectionManager::sourceName(const TransportType type) const
{
    switch (type) {
    case TransportType::SerialPort: return tr("串口");
    case TransportType::Udp: return QStringLiteral("UDP");
    case TransportType::TcpServer: return tr("TCP 服务端");
    case TransportType::TcpClient: return tr("TCP 客户端");
    case TransportType::VirtualData: return tr("虚拟数据");
    }
    return {};
}

QString ConnectionManager::deviceName(
    const TransportType type, const TransportConfig &config) const
{
    switch (type) {
    case TransportType::SerialPort:
        if (const auto *serial = std::get_if<SerialConfig>(&config)) {
            return serial->portName;
        }
        break;
    case TransportType::Udp:
        if (const auto *udp = std::get_if<UdpConfig>(&config)) {
            return tr("本地端口 %1").arg(udp->localPort);
        }
        break;
    case TransportType::TcpServer:
        if (const auto *server = std::get_if<TcpServerConfig>(&config)) {
            return tr("监听端口 %1").arg(server->listenPort);
        }
        break;
    case TransportType::TcpClient:
        if (const auto *client = std::get_if<TcpClientConfig>(&config)) {
            return client->connectionName.isEmpty()
                ? QStringLiteral("%1:%2").arg(client->host).arg(client->port)
                : client->connectionName;
        }
        break;
    case TransportType::VirtualData:
        return tr("模拟信号");
    }
    return {};
}
