#include "services/ConnectionManager.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QHostAddress>
#include <QNetworkDatagram>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUdpSocket>

namespace {
bool runWithTimeout(QEventLoop *loop, const int timeoutMs = 3000)
{
    bool timedOut = false;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, loop, [&] {
        timedOut = true;
        loop->quit();
    });
    timeout.start(timeoutMs);
    loop->exec();
    return !timedOut;
}

quint16 availableUdpPort()
{
    QUdpSocket probe;
    if (!probe.bind(QHostAddress::LocalHost, 0)) return 0;
    return probe.localPort();
}

quint16 availableTcpPort()
{
    QTcpServer probe;
    if (!probe.listen(QHostAddress::LocalHost, 0)) return 0;
    return probe.serverPort();
}

bool testUdp()
{
    ConnectionManager manager;
    QUdpSocket peer;
    if (!peer.bind(QHostAddress::LocalHost, 0)) return false;
    const quint16 managerPort = availableUdpPort();
    if (managerPort == 0) return false;

    bool receivedByManager = false;
    bool receivedByPeer = false;
    QEventLoop loop;
    const auto finish = [&] {
        if (receivedByManager && receivedByPeer) loop.quit();
    };
    QObject::connect(&manager, &ConnectionManager::dataReceived,
                     &loop, [&](const QByteArray &bytes) {
        receivedByManager = bytes == QByteArrayLiteral("udp-rx");
        finish();
    });
    QObject::connect(&peer, &QUdpSocket::readyRead, &loop, [&] {
        while (peer.hasPendingDatagrams()) {
            const QNetworkDatagram datagram = peer.receiveDatagram();
            if (datagram.data() == QByteArrayLiteral("udp-tx")) {
                receivedByPeer = true;
            }
        }
        finish();
    });

    manager.connectTransport(
        TransportType::Udp,
        UdpConfig{QStringLiteral("127.0.0.1"),
                  peer.localPort(), managerPort});
    if (manager.state() != ConnectionState::Connected) return false;
    QString error;
    if (!manager.send(QByteArrayLiteral("udp-tx"), &error)) return false;
    peer.writeDatagram(QByteArrayLiteral("udp-rx"),
                       QHostAddress::LocalHost, managerPort);
    const bool completed = runWithTimeout(&loop);
    manager.disconnectTransport();
    return completed && receivedByManager && receivedByPeer;
}

bool testUdpPortInUse()
{
    QUdpSocket occupied;
    if (!occupied.bind(QHostAddress::AnyIPv4, 0,
                       QUdpSocket::DontShareAddress)) {
        return false;
    }
    ConnectionManager manager;
    bool reportedError = false;
    QObject::connect(&manager, &ConnectionManager::errorOccurred,
                     &manager, [&](const QString &) { reportedError = true; });
    manager.connectTransport(
        TransportType::Udp,
        UdpConfig{QStringLiteral("127.0.0.1"), 9526,
                  occupied.localPort()});
    return manager.state() == ConnectionState::Error && reportedError;
}

bool testTcpServer()
{
    ConnectionManager manager;
    QTcpSocket client;
    const quint16 port = availableTcpPort();
    if (port == 0) return false;
    bool receivedByManager = false;
    bool receivedByClient = false;
    bool sent = false;
    QEventLoop loop;
    const auto finish = [&] {
        if (receivedByManager && receivedByClient) loop.quit();
    };
    QObject::connect(&manager, &ConnectionManager::tcpClientsChanged,
                     &loop, [&](const QStringList &clients) {
        if (clients.isEmpty() || sent) return;
        sent = true;
        manager.setTcpServerTarget(clients.front());
        client.write(QByteArrayLiteral("server-rx"));
        QString error;
        if (!manager.send(QByteArrayLiteral("server-tx"), &error)) {
            loop.quit();
        }
    });
    QObject::connect(&manager, &ConnectionManager::dataReceived,
                     &loop, [&](const QByteArray &bytes) {
        receivedByManager = bytes == QByteArrayLiteral("server-rx");
        finish();
    });
    QObject::connect(&client, &QTcpSocket::readyRead, &loop, [&] {
        receivedByClient =
            client.readAll() == QByteArrayLiteral("server-tx");
        finish();
    });
    manager.connectTransport(TransportType::TcpServer,
                             TcpServerConfig{port});
    if (manager.state() != ConnectionState::Connected) return false;
    client.connectToHost(QHostAddress::LocalHost, port);
    const bool completed = runWithTimeout(&loop);
    manager.disconnectTransport();
    client.abort();
    return completed && receivedByManager && receivedByClient;
}

bool testTcpClient()
{
    ConnectionManager manager;
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0)) return false;
    QTcpSocket *peer = nullptr;
    bool managerConnected = false;
    bool exchangeStarted = false;
    bool receivedByManager = false;
    bool receivedByServer = false;
    QEventLoop loop;
    const auto finish = [&] {
        if (receivedByManager && receivedByServer) loop.quit();
    };
    const auto startExchange = [&] {
        if (!peer || !managerConnected || exchangeStarted) return;
        exchangeStarted = true;
        peer->write(QByteArrayLiteral("client-rx"));
        QString error;
        if (!manager.send(QByteArrayLiteral("client-tx"), &error)) loop.quit();
    };
    QObject::connect(&server, &QTcpServer::newConnection, &loop, [&] {
        peer = server.nextPendingConnection();
        if (!peer) {
            loop.quit();
            return;
        }
        QObject::connect(peer, &QTcpSocket::readyRead, &loop, [&] {
            receivedByServer =
                peer->readAll() == QByteArrayLiteral("client-tx");
            finish();
        });
        startExchange();
    });
    QObject::connect(&manager, &ConnectionManager::stateChanged,
                     &loop, [&](const ConnectionState state) {
        if (state == ConnectionState::Connected) {
            managerConnected = true;
            startExchange();
        }
    });
    QObject::connect(&manager, &ConnectionManager::dataReceived,
                     &loop, [&](const QByteArray &bytes) {
        receivedByManager = bytes == QByteArrayLiteral("client-rx");
        finish();
    });
    manager.connectTransport(
        TransportType::TcpClient,
        TcpClientConfig{QStringLiteral("127.0.0.1"),
                        server.serverPort(), QStringLiteral("test")});
    const bool completed = runWithTimeout(&loop);
    manager.disconnectTransport();
    if (peer) peer->abort();
    return completed && receivedByManager && receivedByServer;
}

bool testTcpRemoteClose()
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0)) return false;
    ConnectionManager manager;
    bool reportedError = false;
    QEventLoop loop;
    QObject::connect(&server, &QTcpServer::newConnection, &loop, [&] {
        if (QTcpSocket *peer = server.nextPendingConnection()) {
            peer->disconnectFromHost();
            peer->deleteLater();
        }
    });
    QObject::connect(&manager, &ConnectionManager::errorOccurred,
                     &loop, [&](const QString &) {
        reportedError = true;
    });
    QObject::connect(&manager, &ConnectionManager::stateChanged,
                     &loop, [&](const ConnectionState state) {
        if (state == ConnectionState::Error) loop.quit();
    });
    manager.connectTransport(
        TransportType::TcpClient,
        TcpClientConfig{QStringLiteral("127.0.0.1"),
                        server.serverPort(), {}});
    const bool completed = runWithTimeout(&loop);
    return completed && reportedError
        && manager.state() == ConnectionState::Error;
}

bool testSerialFailure()
{
    ConnectionManager manager;
    bool reportedError = false;
    QObject::connect(&manager, &ConnectionManager::errorOccurred,
                     &manager, [&](const QString &) { reportedError = true; });
    manager.connectTransport(
        TransportType::SerialPort,
        SerialConfig{QStringLiteral("COM65535"), 115200,
                     QSerialPort::Data8, QSerialPort::NoParity,
                     QSerialPort::OneStop});
    return manager.state() == ConnectionState::Error && reportedError;
}

bool testVirtualData()
{
    ConnectionManager manager;
    bool received = false;
    QEventLoop loop;
    QObject::connect(&manager, &ConnectionManager::dataReceived,
                     &loop, [&](const QByteArray &bytes) {
        received = bytes.size() == 4 * 3;
        loop.quit();
    });
    manager.connectTransport(
        TransportType::VirtualData,
        VirtualDataConfig{2.0, 1.0, 2.0, 3});
    if (manager.state() != ConnectionState::Connected) return false;
    if (!runWithTimeout(&loop) || !received) return false;
    QString error;
    if (manager.send(QByteArrayLiteral("unsupported"), &error)
        || error.isEmpty()) {
        return false;
    }
    manager.disconnectTransport();
    return manager.state() == ConnectionState::Disconnected;
}
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    if (!testUdp()) return 1;
    if (!testUdpPortInUse()) return 2;
    if (!testTcpServer()) return 3;
    if (!testTcpClient()) return 4;
    if (!testTcpRemoteClose()) return 5;
    if (!testSerialFailure()) return 6;
    if (!testVirtualData()) return 7;
    return 0;
}
