#include "VirtualTransport.h"

#include <QChronoTimer>
#include <QDataStream>

#include <chrono>
#include <cmath>

namespace {
constexpr double kTwoPi = 6.28318530717958647692;
}

VirtualTransport::VirtualTransport(QObject *parent)
    : AbstractTransport(parent),
      m_timer(new QChronoTimer(this))
{
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QChronoTimer::timeout,
            this, &VirtualTransport::generateFrame);
}

void VirtualTransport::open(const TransportConfig &config)
{
    const auto *virtualConfig = std::get_if<VirtualDataConfig>(&config);
    if (!virtualConfig || virtualConfig->sampleIntervalMs <= 0.0
        || virtualConfig->signalFrequencyHz < 0.0
        || virtualConfig->channelCount < 1
        || virtualConfig->channelCount > 256) {
        emit errorOccurred(tr("虚拟数据配置无效"));
        emit stateChanged(ConnectionState::Error);
        return;
    }
    m_config = *virtualConfig;
    m_sampleIndex = 0;
    emit stateChanged(ConnectionState::Connecting);
    const auto interval = std::chrono::nanoseconds(
        qMax<qint64>(1, qRound64(m_config.sampleIntervalMs * 1000000.0)));
    m_timer->setInterval(interval);
    m_timer->start();
    emit stateChanged(ConnectionState::Connected);
    generateFrame();
}

void VirtualTransport::close()
{
    m_timer->stop();
    emit stateChanged(ConnectionState::Disconnected);
}

bool VirtualTransport::isOpen() const
{
    return m_timer->isActive();
}

bool VirtualTransport::write(const QByteArray &, QString *errorMessage)
{
    if (errorMessage) {
        *errorMessage = tr("虚拟数据模式不支持发送");
    }
    return false;
}

void VirtualTransport::generateFrame()
{
    QByteArray frame;
    frame.reserve(m_config.channelCount * static_cast<int>(sizeof(float)));
    QDataStream stream(&frame, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    const double seconds =
        m_sampleIndex * m_config.sampleIntervalMs / 1000.0;
    for (int channel = 0; channel < m_config.channelCount; ++channel) {
        const double phase = channel * kTwoPi / m_config.channelCount;
        const float value = static_cast<float>(
            m_config.amplitude
            * std::sin(kTwoPi * m_config.signalFrequencyHz * seconds + phase));
        stream << value;
    }
    ++m_sampleIndex;
    emit dataReceived(frame);
}
