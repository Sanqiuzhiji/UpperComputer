#pragma once

#include "services/transports/AbstractTransport.h"

class QChronoTimer;

class VirtualTransport final : public AbstractTransport
{
    Q_OBJECT

public:
    explicit VirtualTransport(QObject *parent = nullptr);

    void open(const TransportConfig &config) override;
    void close() override;
    [[nodiscard]] bool isOpen() const override;
    [[nodiscard]] bool write(const QByteArray &data,
                             QString *errorMessage) override;

private:
    void generateFrame();

    QChronoTimer *m_timer;
    VirtualDataConfig m_config;
    quint64 m_sampleIndex{};
};
