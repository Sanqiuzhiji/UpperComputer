#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QStringList>

#include "models/AppTypes.h"
#include "models/ConnectionTypes.h"

class AbstractTransport;
class QTimer;

class ConnectionManager final : public QObject
{
    Q_OBJECT

public:
    explicit ConnectionManager(QObject *parent = nullptr);

    [[nodiscard]] ConnectionState state() const noexcept;
    [[nodiscard]] QString deviceName() const;
    [[nodiscard]] QString dataSourceName() const;
    [[nodiscard]] double receiveRate() const noexcept;
    [[nodiscard]] double transmitRate() const noexcept;
    [[nodiscard]] quint64 receiveTotal() const noexcept;
    [[nodiscard]] quint64 transmitTotal() const noexcept;
    [[nodiscard]] TransportType transportType() const noexcept;
    [[nodiscard]] bool canSend() const noexcept;
    [[nodiscard]] bool firmwareOperationActive() const noexcept;
    [[nodiscard]] bool cescNativeActive() const noexcept;

public slots:
    void connectTransport(TransportType type, const TransportConfig &config);
    void disconnectTransport();
    bool send(const QByteArray &data, QString *errorMessage = nullptr,
              CommunicationTrafficSource source =
                  CommunicationTrafficSource::NormalCommunication);
    void setFirmwareOperationActive(bool active);
    void setCescNativeActive(bool active);
    void connectTransportForFirmwareRecovery(
        TransportType type, const TransportConfig &config);
    void disconnectTransportForFirmwareRecovery();
    void setTcpServerTarget(const QString &clientId);

signals:
    void stateChanged(ConnectionState state);
    void deviceNameChanged(const QString &name);
    void dataSourceNameChanged(const QString &name);
    void receiveRateChanged(double bytesPerSecond);
    void transmitRateChanged(double bytesPerSecond);
    void receiveTotalChanged(quint64 bytes);
    void transmitTotalChanged(quint64 bytes);
    void dataReceived(const QByteArray &data);
    void dataSent(const QByteArray &data);
    void monitorDataReceived(const QByteArray &data,
                             CommunicationTrafficSource source);
    void monitorDataSent(const QByteArray &data,
                         CommunicationTrafficSource source);
    void firmwareOperationActiveChanged(bool active);
    void errorOccurred(const QString &message);
    void tcpClientsChanged(const QStringList &clients);

private:
    void connectTransportImpl(TransportType type,
                              const TransportConfig &config,
                              bool firmwareRecovery);
    void disconnectTransportImpl(bool firmwareRecovery);
    void createTransport(TransportType type);
    void setState(ConnectionState state);
    void setDeviceName(const QString &name);
    void setDataSourceName(const QString &name);
    void updateRates();
    [[nodiscard]] QString sourceName(TransportType type) const;
    [[nodiscard]] QString deviceName(
        TransportType type, const TransportConfig &config) const;

    ConnectionState m_state{ConnectionState::Disconnected};
    TransportType m_transportType{TransportType::SerialPort};
    AbstractTransport *m_transport{};
    QTimer *m_rateTimer{};
    QString m_deviceName;
    QString m_dataSourceName;
    double m_receiveRate{};
    double m_transmitRate{};
    quint64 m_receiveTotal{};
    quint64 m_transmitTotal{};
    quint64 m_lastReceiveTotal{};
    quint64 m_lastTransmitTotal{};
    bool m_firmwareOperationActive{};
    bool m_cescNativeActive{};
};
