#pragma once

#include "services/cesc/CescProtocolTypes.h"
#include <QList>

class CescPacketCodec final
{
public:
    explicit CescPacketCodec(quint16 maximumPayload = Cesc::AbsoluteMaximumPayload);
    [[nodiscard]] static quint16 crc16(const QByteArray &bytes);
    [[nodiscard]] static QByteArray encode(const Cesc::Packet &packet);
    QList<Cesc::Packet> feed(const QByteArray &bytes);
    void reset();
    void setMaximumPayload(quint16 maximumPayload);
    [[nodiscard]] Cesc::CodecDiagnostics diagnostics() const noexcept;
private:
    QByteArray m_buffer;
    quint16 m_maximumPayload;
    Cesc::CodecDiagnostics m_diagnostics;
};
