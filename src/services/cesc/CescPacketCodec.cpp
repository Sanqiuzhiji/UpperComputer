#include "CescPacketCodec.h"

CescPacketCodec::CescPacketCodec(quint16 maximumPayload)
    : m_maximumPayload(qMin(maximumPayload, Cesc::AbsoluteMaximumPayload)) {}

quint16 CescPacketCodec::crc16(const QByteArray &bytes)
{
    quint16 crc = 0;
    for (char value : bytes) {
        crc ^= quint16(quint8(value)) << 8;
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc & 0x8000) ? quint16((crc << 1) ^ 0x1021) : quint16(crc << 1);
    }
    return crc;
}

QByteArray CescPacketCodec::encode(const Cesc::Packet &p)
{
    QByteArray out("CE", 2);
    out.append(char(p.version)); out.append(char(p.messageType));
    out.append(char(p.serviceId)); out.append(char(p.commandId));
    Cesc::appendU16(out, p.sequence); Cesc::appendU16(out, quint16(p.payload.size()));
    out.append(p.payload);
    Cesc::appendU16(out, crc16(out.mid(2)));
    return out;
}

QList<Cesc::Packet> CescPacketCodec::feed(const QByteArray &bytes)
{
    m_buffer.append(bytes);
    QList<Cesc::Packet> result;
    for (;;) {
        const qsizetype magic = m_buffer.indexOf("CE");
        if (magic < 0) {
            const bool keepC = !m_buffer.isEmpty() && m_buffer.back() == 'C';
            m_diagnostics.discardedBytes += quint64(m_buffer.size() - (keepC ? 1 : 0));
            m_buffer = keepC ? QByteArray(1, 'C') : QByteArray();
            break;
        }
        if (magic > 0) { m_diagnostics.discardedBytes += quint64(magic); m_buffer.remove(0, magic); }
        if (m_buffer.size() < 10) break;
        const quint8 version = quint8(m_buffer[2]);
        const quint8 type = quint8(m_buffer[3]);
        const quint16 length = quint8(m_buffer[8]) | (quint16(quint8(m_buffer[9])) << 8);
        if (version != Cesc::Version || type > quint8(Cesc::MessageType::Stream)) {
            ++m_diagnostics.headerErrors; ++m_diagnostics.discardedBytes; m_buffer.remove(0,1); continue;
        }
        if (length > m_maximumPayload) {
            ++m_diagnostics.lengthErrors; ++m_diagnostics.discardedBytes; m_buffer.remove(0,1); continue;
        }
        const qsizetype total = 12 + length;
        if (m_buffer.size() < total) break;
        const quint16 expected = quint8(m_buffer[total-2]) | (quint16(quint8(m_buffer[total-1]))<<8);
        if (crc16(m_buffer.mid(2, 8 + length)) != expected) {
            ++m_diagnostics.crcErrors; ++m_diagnostics.discardedBytes; m_buffer.remove(0,1); continue;
        }
        Cesc::Packet p;
        p.version=version; p.messageType=Cesc::MessageType(type); p.serviceId=quint8(m_buffer[4]);
        p.commandId=quint8(m_buffer[5]); p.sequence=quint8(m_buffer[6])|(quint16(quint8(m_buffer[7]))<<8);
        p.payload=m_buffer.mid(10,length); result.append(p); ++m_diagnostics.validFrames; m_buffer.remove(0,total);
    }
    return result;
}

void CescPacketCodec::reset() { m_buffer.clear(); }
void CescPacketCodec::setMaximumPayload(quint16 n) { m_maximumPayload=qMin(n,Cesc::AbsoluteMaximumPayload); }
Cesc::CodecDiagnostics CescPacketCodec::diagnostics() const noexcept { return m_diagnostics; }
