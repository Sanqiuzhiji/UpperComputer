#pragma once

#include <QByteArray>

#include "models/ConnectionTypes.h"

class ChecksumUtils final
{
public:
    [[nodiscard]] static quint8 xor8(const QByteArray &data);
    [[nodiscard]] static quint8 crc8(const QByteArray &data);
    [[nodiscard]] static quint8 crc8Maxim(const QByteArray &data);
    [[nodiscard]] static QByteArray append(
        const QByteArray &payload, ChecksumMode mode);
};
