#include "CescFirmwareUploader.h"

#include "app/AppSettings.h"
#include "services/ConnectionManager.h"

#include <QTimer>
#include <QtSerialPort/QSerialPortInfo>

namespace {
constexpr char kJumpToBootloader = 1;
constexpr char kEraseNewApp = 2;
constexpr char kWriteNewAppData = 3;
constexpr qsizetype kChunkSize = 384;
constexpr qsizetype kMaximumFirmwareSize = 8'000'000;
constexpr int kReconnectIntervalMs = 750;
constexpr int kMaximumReconnectAttempts = 40;
}

CescFirmwareUploader::CescFirmwareUploader(ConnectionManager *connection,
                                           AppSettings *settings,
                                           QObject *parent)
    : QObject(parent), m_connection(connection), m_settings(settings),
      m_timeout(new QTimer(this)), m_reconnectTimer(new QTimer(this))
{
    m_timeout->setSingleShot(true);
    m_reconnectTimer->setInterval(kReconnectIntervalMs);
    connect(m_timeout, &QTimer::timeout, this, &CescFirmwareUploader::handleTimeout);
    connect(m_reconnectTimer, &QTimer::timeout,
            this, &CescFirmwareUploader::tryReconnect);
    connect(m_connection, &ConnectionManager::dataReceived,
            this, &CescFirmwareUploader::processReceived);
    connect(m_connection, &ConnectionManager::stateChanged, this,
            [this](ConnectionState state) {
                if (!isBusy()) return;
                if (m_stage == Stage::Connecting
                    && state == ConnectionState::Connected) {
                    m_reconnectTimer->stop();
                    sendFirmwareVersionRequest(true);
                    return;
                }
                if (m_stage == Stage::Connecting
                    && (state == ConnectionState::Error
                        || state == ConnectionState::Disconnected)) {
                    m_stage = Stage::WaitingForPort;
                    return;
                }
                if ((m_stage == Stage::Erasing
                     || m_stage == Stage::Writing
                     || m_stage == Stage::ReadingVersion)
                    && state != ConnectionState::Connected) {
                    complete(false, tr("设备连接已断开"));
                }
            });
}

bool CescFirmwareUploader::isBusy() const noexcept
{
    return m_stage != Stage::Idle;
}

CescFirmwareUploader::State CescFirmwareUploader::state() const noexcept { return m_publicState; }
int CescFirmwareUploader::progress() const noexcept { return m_progress; }
QString CescFirmwareUploader::statusText() const { return m_statusText; }
QString CescFirmwareUploader::firmwareVersion() const { return m_firmwareVersion; }

void CescFirmwareUploader::updateState(const State state, const int percent,
                                       const QString &status)
{
    m_publicState = state;
    m_progress = percent;
    m_statusText = status;
    emit stateChanged(state, percent, status);
    emit progressChanged(percent, status);
}

void CescFirmwareUploader::start(const QByteArray &firmware)
{
    if (isBusy()) return;
    if (!m_connection->canSend()) {
        emit finished(false, tr("请先连接下位机（不支持虚拟数据源）"));
        return;
    }
    if (m_connection->transportType() != TransportType::SerialPort) {
        emit finished(false, tr("CESC 固件只能通过串口连接烧录"));
        return;
    }
    if (firmware.isEmpty() || firmware.size() > kMaximumFirmwareSize) {
        emit finished(false, tr("固件为空或超过 8 MB"));
        return;
    }

    updateState(State::Preparing, 0, tr("正在准备..."));
    QByteArray raw = firmware;
    if (raw.size() > 6) {
        const quint32 declared = (quint8(raw[0]) << 24) | (quint8(raw[1]) << 16)
            | (quint8(raw[2]) << 8) | quint8(raw[3]);
        const QByteArray body = raw.mid(6);
        const quint16 storedCrc = (quint8(raw[4]) << 8) | quint8(raw[5]);
        if (storedCrc == crc16(body)) {
            if ((declared >> 24) == 0xCC) {
                emit finished(false, tr("暂不支持 Heatshrink 压缩固件，请选择原始 .bin 文件"));
                return;
            }
            if (declared == quint32(body.size())) raw = body;
        }
    }

    const quint16 checksum = crc16(raw);
    m_image.clear();
    appendUint32(m_image, quint32(raw.size()));
    m_image.append(char(checksum >> 8));
    m_image.append(char(checksum & 0xff));
    m_image.append(raw);
    m_offset = 0;
    m_attempt = 0;
    m_rxBuffer.clear();
    m_stage = Stage::Erasing;
    m_connection->setFirmwareOperationActive(true);
    updateState(State::Preparing, 0, tr("正在擦除固件缓冲区..."));
    sendErase();
}

void CescFirmwareUploader::cancel()
{
    if (isBusy()) complete(false, tr("刷写已取消"));
}

void CescFirmwareUploader::requestFirmwareVersion()
{
    if (isBusy() || !m_connection->canSend()
        || m_connection->transportType() != TransportType::SerialPort) {
        return;
    }
    m_connection->setFirmwareOperationActive(true);
    m_rxBuffer.clear();
    m_attempt = 0;
    sendFirmwareVersionRequest(false);
}

void CescFirmwareUploader::sendErase()
{
    QByteArray payload(1, kEraseNewApp);
    appendUint32(payload, quint32(m_image.size() - 6));
    QString error;
    if (!m_connection->send(frame(payload), &error,
            CommunicationTrafficSource::CescFirmware)) {
        complete(false, error);
        return;
    }
    m_timeout->start(20'000);
}

void CescFirmwareUploader::sendCurrentChunk()
{
    while (m_offset < m_image.size()) {
        m_chunkLength = qMin(kChunkSize, m_image.size() - m_offset);
        const QByteArray chunk = m_image.mid(m_offset, m_chunkLength);
        bool blank = true;
        for (const char byte : chunk) {
            if (quint8(byte) != 0xff) { blank = false; break; }
        }
        if (!blank) {
            QByteArray payload(1, kWriteNewAppData);
            appendUint32(payload, quint32(m_offset));
            payload.append(chunk);
            QString error;
            if (!m_connection->send(frame(payload), &error,
                    CommunicationTrafficSource::CescFirmware)) {
                complete(false, error);
                return;
            }
            m_timeout->start(3'000);
            return;
        }
        m_offset += m_chunkLength;
    }

    if (!m_connection->send(frame(QByteArray(1, kJumpToBootloader)), nullptr,
                            CommunicationTrafficSource::CescFirmware)) {
        complete(false, tr("固件已写入，但发送重启命令失败"));
        return;
    }
    beginPostUploadRecovery();
}

void CescFirmwareUploader::processReceived(const QByteArray &data)
{
    if (!isBusy()) return;
    m_rxBuffer.append(data);
    while (!m_rxBuffer.isEmpty()) {
        const quint8 marker = quint8(m_rxBuffer[0]);
        int header = 0;
        qsizetype length = 0;
        if (marker == 2) {
            if (m_rxBuffer.size() < 2) return;
            header = 2; length = quint8(m_rxBuffer[1]);
        } else if (marker == 3) {
            if (m_rxBuffer.size() < 3) return;
            header = 3; length = (quint8(m_rxBuffer[1]) << 8) | quint8(m_rxBuffer[2]);
        } else if (marker == 4) {
            if (m_rxBuffer.size() < 4) return;
            header = 4; length = (quint8(m_rxBuffer[1]) << 16)
                | (quint8(m_rxBuffer[2]) << 8) | quint8(m_rxBuffer[3]);
        } else {
            m_rxBuffer.remove(0, 1); continue;
        }
        const qsizetype total = header + length + 3;
        if (length < 1 || length > 10'000) { m_rxBuffer.remove(0, 1); continue; }
        if (m_rxBuffer.size() < total) return;
        const QByteArray payload = m_rxBuffer.mid(header, length);
        const quint16 received = (quint8(m_rxBuffer[header + length]) << 8)
            | quint8(m_rxBuffer[header + length + 1]);
        if (quint8(m_rxBuffer[total - 1]) == 3 && received == crc16(payload)) {
            handlePacket(payload);
        }
        m_rxBuffer.remove(0, total);
    }
}

void CescFirmwareUploader::handlePacket(const QByteArray &packet)
{
    if (packet.size() < 2) return;
    const quint8 command = quint8(packet[0]);
    if (m_stage == Stage::ReadingVersion && command == 0) {
        handleFirmwareVersion(packet);
        return;
    }
    const bool ok = packet[1] != 0;
    if (m_stage == Stage::Erasing && command == quint8(kEraseNewApp)) {
        m_timeout->stop();
        if (!ok) { complete(false, tr("下位机擦除固件缓冲区失败")); return; }
        m_stage = Stage::Writing;
        m_attempt = 0;
        updateState(State::Uploading, 0, tr("正在烧录..."));
        sendCurrentChunk();
    } else if (m_stage == Stage::Writing && command == quint8(kWriteNewAppData)) {
        m_timeout->stop();
        if (!ok) { complete(false, tr("下位机拒绝写入固件数据")); return; }
        m_offset += m_chunkLength;
        m_attempt = 0;
        updateState(State::Uploading,
                    qMin(100, int(m_offset * 100 / m_image.size())),
                    tr("正在烧录..."));
        sendCurrentChunk();
    }
}

void CescFirmwareUploader::handleTimeout()
{
    if (m_stage == Stage::ReadingVersion) {
        if (++m_attempt < 3) {
            sendFirmwareVersionRequest(m_postUploadVersionRequest);
            return;
        }
        if (m_postUploadVersionRequest) {
            complete(false, tr("固件已烧录并重新连接，但读取固件版本超时"));
        } else {
            m_stage = Stage::Idle;
            m_connection->setFirmwareOperationActive(false);
            updateState(State::Idle, m_progress, tr("固件版本读取超时"));
        }
        return;
    }
    if (++m_attempt >= 3) {
        complete(false, m_stage == Stage::Erasing
            ? tr("擦除超时，请检查连接后重试") : tr("写入超时，请检查连接后重试"));
        return;
    }
    if (m_stage == Stage::Erasing) sendErase(); else sendCurrentChunk();
}

void CescFirmwareUploader::beginPostUploadRecovery()
{
    m_reconnectConfig = m_settings->serialConfig();
    const QSerialPortInfo currentPort(m_reconnectConfig.portName);
    m_usbSerialNumber = currentPort.serialNumber();
    m_usbVendorId = currentPort.hasVendorIdentifier()
        ? currentPort.vendorIdentifier() : 0;
    m_usbProductId = currentPort.hasProductIdentifier()
        ? currentPort.productIdentifier() : 0;
    m_reconnectAttempts = 0;
    m_stage = Stage::WaitingForPort;
    updateState(State::Rebooting, 100,
                tr("固件烧录完成，下位机正在重启..."));
    // Keep the port alive briefly so the queued reboot frame reaches the
    // device before closing the host side of the serial connection.
    QTimer::singleShot(500, this, [this] {
        if (m_stage != Stage::WaitingForPort) return;
        m_connection->disconnectTransportForFirmwareRecovery();
        m_reconnectTimer->start();
    });
}

QString CescFirmwareUploader::findReappearedPort() const
{
    QString originalPort;
    QString identityPort;
    int identityMatches = 0;
    for (const QSerialPortInfo &port : QSerialPortInfo::availablePorts()) {
        if (!m_usbSerialNumber.isEmpty()
            && port.serialNumber() == m_usbSerialNumber) {
            return port.portName();
        }
        if (m_usbVendorId != 0 && m_usbProductId != 0
            && port.hasVendorIdentifier() && port.hasProductIdentifier()
            && port.vendorIdentifier() == m_usbVendorId
            && port.productIdentifier() == m_usbProductId) {
            identityPort = port.portName();
            ++identityMatches;
        }
        if (port.portName().compare(m_reconnectConfig.portName,
                                    Qt::CaseInsensitive) == 0) {
            originalPort = port.portName();
        }
    }
    if (identityMatches == 1) return identityPort;
    return originalPort;
}

void CescFirmwareUploader::tryReconnect()
{
    if (m_stage != Stage::WaitingForPort
        && m_stage != Stage::Connecting) return;
    if (++m_reconnectAttempts > kMaximumReconnectAttempts) {
        complete(false, tr("固件已烧录，但等待下位机重启并重新连接超时"));
        return;
    }
    if (m_stage == Stage::Connecting) return;
    const QString portName = findReappearedPort();
    if (portName.isEmpty()) return;
    m_reconnectConfig.portName = portName;
    m_settings->setSerialConfig(m_reconnectConfig);
    m_stage = Stage::Connecting;
    updateState(State::Rebooting, 100,
                tr("检测到 %1，正在重新连接...").arg(portName));
    m_connection->connectTransportForFirmwareRecovery(
        TransportType::SerialPort, m_reconnectConfig);
}

void CescFirmwareUploader::sendFirmwareVersionRequest(const bool postUpload)
{
    m_postUploadVersionRequest = postUpload;
    m_stage = Stage::ReadingVersion;
    updateState(State::ReadingVersion, postUpload ? 100 : m_progress,
                postUpload ? tr("下位机已重新连接，正在读取固件版本...")
                           : tr("正在读取固件版本..."));
    QString error;
    if (!m_connection->send(frame(QByteArray(1, char(0))), &error,
                            CommunicationTrafficSource::CescFirmware)) {
        complete(false, postUpload
            ? tr("重新连接成功，但无法读取固件版本：%1").arg(error)
            : error);
        return;
    }
    m_timeout->start(3'000);
}

void CescFirmwareUploader::handleFirmwareVersion(const QByteArray &packet)
{
    if (packet.size() < 3) return;
    m_timeout->stop();
    const int major = quint8(packet[1]);
    const int minor = quint8(packet[2]);
    const qsizetype hardwareStart = 3;
    qsizetype hardwareEnd = packet.indexOf('\0', hardwareStart);
    if (hardwareEnd < 0) hardwareEnd = packet.size();
    const QString hardware = QString::fromUtf8(
        packet.mid(hardwareStart, hardwareEnd - hardwareStart));
    m_firmwareVersion = hardware.trimmed().isEmpty()
        ? QStringLiteral("%1.%2").arg(major).arg(minor)
        : QStringLiteral("%1.%2 · %3").arg(major).arg(minor).arg(hardware);
    emit firmwareVersionChanged(m_firmwareVersion);
    const bool postUpload = m_postUploadVersionRequest;
    m_stage = Stage::Idle;
    m_connection->setFirmwareOperationActive(false);
    if (postUpload) {
        updateState(State::Completed, 100,
                    tr("下位机重启完成，固件版本：%1")
                        .arg(m_firmwareVersion));
        emit finished(true, tr("固件烧录完成，下位机已重新连接"));
    } else {
        updateState(State::Idle, m_progress, tr("等待烧录"));
    }
}

void CescFirmwareUploader::complete(bool success, const QString &message)
{
    m_timeout->stop();
    m_reconnectTimer->stop();
    m_stage = Stage::Idle;
    m_image.clear();
    m_rxBuffer.clear();
    m_connection->setFirmwareOperationActive(false);
    updateState(success ? State::Completed : State::Failed,
                success ? 100 : m_progress, message);
    emit finished(success, message);
}

QByteArray CescFirmwareUploader::frame(const QByteArray &payload) const
{
    QByteArray result;
    if (payload.size() <= 255) {
        result.append(char(2)); result.append(char(payload.size()));
    } else {
        result.append(char(3)); result.append(char(payload.size() >> 8));
        result.append(char(payload.size() & 0xff));
    }
    result.append(payload);
    const quint16 checksum = crc16(payload);
    result.append(char(checksum >> 8)); result.append(char(checksum & 0xff));
    result.append(char(3));
    return result;
}

quint16 CescFirmwareUploader::crc16(const QByteArray &data)
{
    quint16 crc = 0;
    for (const char value : data) {
        crc ^= quint16(quint8(value)) << 8;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x8000) ? quint16((crc << 1) ^ 0x1021) : quint16(crc << 1);
        }
    }
    return crc;
}

void CescFirmwareUploader::appendUint32(QByteArray &data, quint32 value)
{
    data.append(char(value >> 24)); data.append(char(value >> 16));
    data.append(char(value >> 8)); data.append(char(value));
}
