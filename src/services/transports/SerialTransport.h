#pragma once

#include "services/transports/AbstractTransport.h"

class QSerialPort;

class SerialTransport final : public AbstractTransport
{
    Q_OBJECT

public:
    explicit SerialTransport(QObject *parent = nullptr);

    void open(const TransportConfig &config) override;
    void close() override;
    [[nodiscard]] bool isOpen() const override;
    [[nodiscard]] bool write(const QByteArray &data,
                             QString *errorMessage) override;

private:
    QSerialPort *m_port;
    bool m_closing{};
};
