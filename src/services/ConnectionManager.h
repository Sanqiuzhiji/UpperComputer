#pragma once

#include <QObject>
#include <QString>

#include "models/AppTypes.h"

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

public slots:
    // These setters are intentionally transport-agnostic. Future transport
    // implementations can publish their state through this small interface.
    void setState(ConnectionState state);
    void setDeviceName(const QString &name);
    void setDataSourceName(const QString &name);
    void setReceiveRate(double bytesPerSecond);
    void setTransmitRate(double bytesPerSecond);

signals:
    void stateChanged(ConnectionState state);
    void deviceNameChanged(const QString &name);
    void dataSourceNameChanged(const QString &name);
    void receiveRateChanged(double bytesPerSecond);
    void transmitRateChanged(double bytesPerSecond);

private:
    ConnectionState m_state{ConnectionState::Disconnected};
    QString m_deviceName;
    QString m_dataSourceName;
    double m_receiveRate{};
    double m_transmitRate{};
};
