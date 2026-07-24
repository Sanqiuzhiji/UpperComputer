#pragma once

#include "services/transports/AbstractTransport.h"

#include <QHash>
#include <QPointer>

class QTcpServer;
class QTcpSocket;
class QTimer;
class QUdpSocket;

class UdpTransport final : public AbstractTransport
{
    Q_OBJECT

public:
    explicit UdpTransport(QObject *parent = nullptr);

    void open(const TransportConfig &config) override;
    void close() override;
    [[nodiscard]] bool isOpen() const override;
    [[nodiscard]] bool write(const QByteArray &data,
                             QString *errorMessage) override;

private:
    QUdpSocket *m_socket;
    QString m_remoteAddress;
    quint16 m_remotePort{};
};

class TcpClientTransport final : public AbstractTransport
{
    Q_OBJECT

public:
    explicit TcpClientTransport(QObject *parent = nullptr);

    void open(const TransportConfig &config) override;
    void close() override;
    [[nodiscard]] bool isOpen() const override;
    [[nodiscard]] bool write(const QByteArray &data,
                             QString *errorMessage) override;

private:
    [[nodiscard]] QString socketErrorText() const;

    QTcpSocket *m_socket;
    QTimer *m_connectTimeout;
    bool m_closing{};
};

class TcpServerTransport final : public AbstractTransport
{
    Q_OBJECT

public:
    explicit TcpServerTransport(QObject *parent = nullptr);

    void open(const TransportConfig &config) override;
    void close() override;
    [[nodiscard]] bool isOpen() const override;
    [[nodiscard]] bool write(const QByteArray &data,
                             QString *errorMessage) override;
    void setSelectedClient(const QString &clientId);

signals:
    void clientsChanged(const QStringList &clientIds);

private:
    void acceptPendingClients();
    void removeClient(const QString &clientId);
    void publishClients();

    QTcpServer *m_server;
    QHash<QString, QPointer<QTcpSocket>> m_clients;
    QString m_selectedClient;
};
