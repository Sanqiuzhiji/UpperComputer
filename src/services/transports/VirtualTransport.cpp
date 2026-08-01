#include "VirtualTransport.h"

#include <QChronoTimer>
#include <QDataStream>

#include <chrono>
#include <cmath>

namespace {
constexpr double kTwoPi = 6.28318530717958647692;
constexpr int kVirtualSignalCount = 5;

double squareFourier(const double phase, const int maximumHarmonic)
{
    double value = 0.0;
    for (int harmonic = 1;
         harmonic <= maximumHarmonic; harmonic += 2) {
        value += std::sin(harmonic * phase) / harmonic;
    }
    return 4.0 / 3.14159265358979323846 * value;
}
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
        || virtualConfig->signalFrequencyHz < 0.0) {
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
    frame.reserve(kVirtualSignalCount * static_cast<int>(sizeof(float)));
    QDataStream stream(&frame, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    const double seconds =
        m_sampleIndex * m_config.sampleIntervalMs / 1000.0;
    const double phase = kTwoPi * m_config.signalFrequencyHz * seconds;
    const double sine = std::sin(phase);
    const double square = sine >= 0.0 ? 1.0 : -1.0;
    const double generatedValues[kVirtualSignalCount]{
        sine,
        square,
        squareFourier(phase, 1),
        squareFourier(phase, 3),
        squareFourier(phase, 5)};
    for (const double generatedValue : generatedValues) {
        stream << static_cast<float>(m_config.amplitude * generatedValue);
    }
    ++m_sampleIndex;
    emit dataReceived(frame);
}
