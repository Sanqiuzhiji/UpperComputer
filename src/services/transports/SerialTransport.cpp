#include "SerialTransport.h"

#include <QSerialPort>

SerialTransport::SerialTransport(QObject *parent)
    : AbstractTransport(parent),
      m_port(new QSerialPort(this))
{
    connect(m_port, &QSerialPort::readyRead, this, [this] {
        const QByteArray data = m_port->readAll();
        if (!data.isEmpty()) {
            emit dataReceived(data);
        }
    });
    connect(m_port, &QSerialPort::errorOccurred, this,
            [this](const QSerialPort::SerialPortError error) {
                if (error == QSerialPort::NoError || m_closing) {
                    return;
                }
                emit errorOccurred(tr("串口错误：%1").arg(m_port->errorString()));
                if (error == QSerialPort::ResourceError
                    || error == QSerialPort::DeviceNotFoundError
                    || error == QSerialPort::PermissionError) {
                    m_port->close();
                    emit stateChanged(ConnectionState::Error);
                }
            });
}

void SerialTransport::open(const TransportConfig &config)
{
    const auto *serial = std::get_if<SerialConfig>(&config);
    if (!serial || serial->portName.trimmed().isEmpty()) {
        emit errorOccurred(tr("串口打开失败：未选择串口设备"));
        emit stateChanged(ConnectionState::Error);
        return;
    }

    m_closing = false;
    emit stateChanged(ConnectionState::Connecting);
    m_port->setPortName(serial->portName);
    const bool configured =
        m_port->setBaudRate(serial->baudRate)
        && m_port->setDataBits(serial->dataBits)
        && m_port->setParity(serial->parity)
        && m_port->setStopBits(serial->stopBits)
        && m_port->setFlowControl(QSerialPort::NoFlowControl);
    if (!configured || !m_port->open(QIODevice::ReadWrite)) {
        emit errorOccurred(
            tr("串口打开失败：%1").arg(m_port->errorString()));
        emit stateChanged(ConnectionState::Error);
        return;
    }
    emit stateChanged(ConnectionState::Connected);
}

void SerialTransport::close()
{
    m_closing = true;
    if (m_port->isOpen()) {
        m_port->close();
    }
    emit stateChanged(ConnectionState::Disconnected);
    m_closing = false;
}

bool SerialTransport::isOpen() const
{
    return m_port->isOpen();
}

bool SerialTransport::write(const QByteArray &data, QString *errorMessage)
{
    if (!m_port->isOpen()) {
        if (errorMessage) *errorMessage = tr("串口尚未打开");
        return false;
    }
    const qint64 accepted = m_port->write(data);
    if (accepted < 0) {
        if (errorMessage) *errorMessage = m_port->errorString();
        return false;
    }
    emit dataWritten(accepted);
    return true;
}
