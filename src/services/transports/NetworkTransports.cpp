#include "NetworkTransports.h"

#include <QHostAddress>
#include <QNetworkDatagram>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUdpSocket>

UdpTransport::UdpTransport(QObject *parent)
    : AbstractTransport(parent),
      m_socket(new QUdpSocket(this))
{
    connect(m_socket, &QUdpSocket::readyRead, this, [this] {
        while (m_socket->hasPendingDatagrams()) {
            const QNetworkDatagram datagram = m_socket->receiveDatagram();
            if (datagram.isValid() && !datagram.data().isEmpty()) {
                emit dataReceived(datagram.data());
            }
        }
    });
    connect(m_socket, &QUdpSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError) {
                emit errorOccurred(tr("UDP 错误：%1").arg(m_socket->errorString()));
            });
}

void UdpTransport::open(const TransportConfig &config)
{
    const auto *udp = std::get_if<UdpConfig>(&config);
    QHostAddress remote;
    if (!udp || !remote.setAddress(udp->remoteAddress)
        || udp->remotePort == 0 || udp->localPort == 0) {
        emit errorOccurred(tr("UDP 配置无效，请检查 IP 和端口"));
        emit stateChanged(ConnectionState::Error);
        return;
    }

    emit stateChanged(ConnectionState::Connecting);
    m_remoteAddress = udp->remoteAddress;
    m_remotePort = udp->remotePort;
    if (!m_socket->bind(QHostAddress::AnyIPv4, udp->localPort,
                        QUdpSocket::DontShareAddress)) {
        emit errorOccurred(
            tr("UDP 本地端口绑定失败：%1").arg(m_socket->errorString()));
        emit stateChanged(ConnectionState::Error);
        return;
    }
    emit stateChanged(ConnectionState::Connected);
}

void UdpTransport::close()
{
    m_socket->close();
    emit stateChanged(ConnectionState::Disconnected);
}

bool UdpTransport::isOpen() const
{
    return m_socket->state() == QAbstractSocket::BoundState;
}

bool UdpTransport::write(const QByteArray &data, QString *errorMessage)
{
    if (!isOpen()) {
        if (errorMessage) *errorMessage = tr("UDP 本地端口尚未绑定");
        return false;
    }
    const qint64 accepted = m_socket->writeDatagram(
        data, QHostAddress(m_remoteAddress), m_remotePort);
    if (accepted < 0) {
        if (errorMessage) *errorMessage = m_socket->errorString();
        return false;
    }
    emit dataWritten(accepted);
    return true;
}

TcpClientTransport::TcpClientTransport(QObject *parent)
    : AbstractTransport(parent),
      m_socket(new QTcpSocket(this)),
      m_connectTimeout(new QTimer(this))
{
    m_connectTimeout->setSingleShot(true);
    m_connectTimeout->setInterval(10000);
    connect(m_socket, &QTcpSocket::connected, this, [this] {
        m_connectTimeout->stop();
        emit stateChanged(ConnectionState::Connected);
    });
    connect(m_socket, &QTcpSocket::readyRead, this, [this] {
        const QByteArray data = m_socket->readAll();
        if (!data.isEmpty()) emit dataReceived(data);
    });
    connect(m_socket, &QTcpSocket::disconnected, this, [this] {
        emit stateChanged(m_closing
            ? ConnectionState::Disconnected : ConnectionState::Error);
        m_closing = false;
    });
    connect(m_socket, &QTcpSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError) {
                m_connectTimeout->stop();
                if (m_closing) return;
                emit errorOccurred(socketErrorText());
                emit stateChanged(ConnectionState::Error);
            });
    connect(m_connectTimeout, &QTimer::timeout, this, [this] {
        if (m_socket->state() != QAbstractSocket::HostLookupState
            && m_socket->state() != QAbstractSocket::ConnectingState) {
            return;
        }
        m_closing = true;
        m_socket->abort();
        m_closing = false;
        emit errorOccurred(tr("TCP 连接超时"));
        emit stateChanged(ConnectionState::Error);
    });
}

void TcpClientTransport::open(const TransportConfig &config)
{
    const auto *tcp = std::get_if<TcpClientConfig>(&config);
    if (!tcp || tcp->host.trimmed().isEmpty() || tcp->port == 0) {
        emit errorOccurred(tr("TCP 客户端配置无效"));
        emit stateChanged(ConnectionState::Error);
        return;
    }
    m_closing = false;
    emit stateChanged(ConnectionState::Connecting);
    m_connectTimeout->start();
    m_socket->connectToHost(tcp->host.trimmed(), tcp->port);
}

void TcpClientTransport::close()
{
    m_closing = true;
    m_connectTimeout->stop();
    if (m_socket->state() == QAbstractSocket::UnconnectedState) {
        emit stateChanged(ConnectionState::Disconnected);
        m_closing = false;
        return;
    }
    m_socket->abort();
    emit stateChanged(ConnectionState::Disconnected);
    m_closing = false;
}

bool TcpClientTransport::isOpen() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

bool TcpClientTransport::write(const QByteArray &data, QString *errorMessage)
{
    if (!isOpen()) {
        if (errorMessage) *errorMessage = tr("TCP 客户端尚未连接");
        return false;
    }
    const qint64 accepted = m_socket->write(data);
    if (accepted < 0) {
        if (errorMessage) *errorMessage = m_socket->errorString();
        return false;
    }
    emit dataWritten(accepted);
    return true;
}

QString TcpClientTransport::socketErrorText() const
{
    switch (m_socket->error()) {
    case QAbstractSocket::HostNotFoundError:
        return tr("TCP 连接失败：找不到服务端");
    case QAbstractSocket::ConnectionRefusedError:
        return tr("TCP 连接失败：服务端拒绝连接");
    case QAbstractSocket::RemoteHostClosedError:
        return tr("TCP 连接已被远端关闭");
    case QAbstractSocket::SocketTimeoutError:
        return tr("TCP 连接超时");
    default:
        return tr("TCP 错误：%1").arg(m_socket->errorString());
    }
}

TcpServerTransport::TcpServerTransport(QObject *parent)
    : AbstractTransport(parent),
      m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection,
            this, &TcpServerTransport::acceptPendingClients);
}

void TcpServerTransport::open(const TransportConfig &config)
{
    const auto *tcp = std::get_if<TcpServerConfig>(&config);
    if (!tcp || tcp->listenPort == 0) {
        emit errorOccurred(tr("TCP 服务端监听端口无效"));
        emit stateChanged(ConnectionState::Error);
        return;
    }
    emit stateChanged(ConnectionState::Connecting);
    if (!m_server->listen(QHostAddress::Any, tcp->listenPort)) {
        emit errorOccurred(
            tr("TCP 服务端监听失败：%1").arg(m_server->errorString()));
        emit stateChanged(ConnectionState::Error);
        return;
    }
    emit stateChanged(ConnectionState::Connected);
}

void TcpServerTransport::close()
{
    m_server->close();
    const auto clients = m_clients;
    m_clients.clear();
    m_selectedClient.clear();
    for (QTcpSocket *socket : clients) {
        if (!socket) continue;
        socket->disconnect(this);
        socket->abort();
        socket->deleteLater();
    }
    publishClients();
    emit stateChanged(ConnectionState::Disconnected);
}

bool TcpServerTransport::isOpen() const
{
    return m_server->isListening();
}

bool TcpServerTransport::write(const QByteArray &data, QString *errorMessage)
{
    QTcpSocket *socket = m_clients.value(m_selectedClient);
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        if (errorMessage) *errorMessage = tr("当前没有可用的 TCP 客户端");
        return false;
    }
    const qint64 accepted = socket->write(data);
    if (accepted < 0) {
        if (errorMessage) *errorMessage = socket->errorString();
        return false;
    }
    emit dataWritten(accepted);
    return true;
}

void TcpServerTransport::setSelectedClient(const QString &clientId)
{
    if (m_clients.contains(clientId)) {
        m_selectedClient = clientId;
    }
}

void TcpServerTransport::acceptPendingClients()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        if (!socket) continue;
        QString clientId = QStringLiteral("%1:%2")
            .arg(socket->peerAddress().toString())
            .arg(socket->peerPort());
        int suffix = 2;
        const QString baseId = clientId;
        while (m_clients.contains(clientId)) {
            clientId = QStringLiteral("%1 #%2").arg(baseId).arg(suffix++);
        }
        m_clients.insert(clientId, socket);
        if (m_selectedClient.isEmpty()) m_selectedClient = clientId;
        connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
            const QByteArray data = socket->readAll();
            if (!data.isEmpty()) emit dataReceived(data);
        });
        connect(socket, &QTcpSocket::disconnected, this,
                [this, clientId] { removeClient(clientId); });
        connect(socket, &QTcpSocket::errorOccurred, this,
                [this, socket](QAbstractSocket::SocketError) {
                    emit errorOccurred(
                        tr("TCP 客户端错误：%1").arg(socket->errorString()));
                });
    }
    publishClients();
}

void TcpServerTransport::removeClient(const QString &clientId)
{
    QTcpSocket *socket = m_clients.take(clientId);
    if (socket) socket->deleteLater();
    if (m_selectedClient == clientId) {
        m_selectedClient = m_clients.isEmpty()
            ? QString() : m_clients.constBegin().key();
    }
    publishClients();
}

void TcpServerTransport::publishClients()
{
    QStringList clients = m_clients.keys();
    clients.sort(Qt::CaseInsensitive);
    emit clientsChanged(clients);
}
