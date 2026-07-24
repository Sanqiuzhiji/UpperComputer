#include "ConnectionManager.h"

#include <QtMath>

ConnectionManager::ConnectionManager(QObject *parent)
    : QObject(parent)
{
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

void ConnectionManager::setState(const ConnectionState state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged(state);
}

void ConnectionManager::setDeviceName(const QString &name)
{
    if (m_deviceName == name) {
        return;
    }
    m_deviceName = name;
    emit deviceNameChanged(name);
}

void ConnectionManager::setDataSourceName(const QString &name)
{
    if (m_dataSourceName == name) {
        return;
    }
    m_dataSourceName = name;
    emit dataSourceNameChanged(name);
}

void ConnectionManager::setReceiveRate(const double bytesPerSecond)
{
    const double normalized = qMax(0.0, bytesPerSecond);
    if (qFuzzyCompare(m_receiveRate + 1.0, normalized + 1.0)) {
        return;
    }
    m_receiveRate = normalized;
    emit receiveRateChanged(normalized);
}

void ConnectionManager::setTransmitRate(const double bytesPerSecond)
{
    const double normalized = qMax(0.0, bytesPerSecond);
    if (qFuzzyCompare(m_transmitRate + 1.0, normalized + 1.0)) {
        return;
    }
    m_transmitRate = normalized;
    emit transmitRateChanged(normalized);
}
