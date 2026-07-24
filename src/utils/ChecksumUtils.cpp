#include "ChecksumUtils.h"

quint8 ChecksumUtils::xor8(const QByteArray &data)
{
    quint8 result = 0;
    for (const char byte : data) {
        result ^= static_cast<quint8>(byte);
    }
    return result;
}

quint8 ChecksumUtils::crc8(const QByteArray &data)
{
    quint8 crc = 0x00;
    for (const char byte : data) {
        crc ^= static_cast<quint8>(byte);
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x80U)
                ? static_cast<quint8>((crc << 1U) ^ 0x07U)
                : static_cast<quint8>(crc << 1U);
        }
    }
    return crc;
}

quint8 ChecksumUtils::crc8Maxim(const QByteArray &data)
{
    quint8 crc = 0x00;
    for (const char byte : data) {
        crc ^= static_cast<quint8>(byte);
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x01U)
                ? static_cast<quint8>((crc >> 1U) ^ 0x8CU)
                : static_cast<quint8>(crc >> 1U);
        }
    }
    return crc;
}

QByteArray ChecksumUtils::append(
    const QByteArray &payload, const ChecksumMode mode)
{
    QByteArray result = payload;
    switch (mode) {
    case ChecksumMode::None:
        break;
    case ChecksumMode::Xor8:
        result.append(static_cast<char>(xor8(payload)));
        break;
    case ChecksumMode::Crc8:
        result.append(static_cast<char>(crc8(payload)));
        break;
    case ChecksumMode::Crc8Maxim:
        result.append(static_cast<char>(crc8Maxim(payload)));
        break;
    }
    return result;
}
