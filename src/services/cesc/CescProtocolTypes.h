#pragma once

#include <QByteArray>
#include <QString>
#include <QtGlobal>
#include <QMetaType>
#include <cstring>

namespace Cesc {
constexpr quint8 Version = 1;
constexpr quint16 AbsoluteMaximumPayload = 4096;

enum class MessageType : quint8 { Request = 0, Response = 1, Event = 2, Stream = 3 };
enum class Service : quint8 { System = 0, Firmware = 1, Sensor = 2, Telemetry = 3 };
enum class Status : quint16 {
    Ok = 0, InvalidService, InvalidCommand, InvalidLength, InvalidArgument,
    NotReady, Busy, Timeout, CrcError, IoError, NotSupported,
    VersionMismatch, InternalError, AccessDenied, OutOfRange, VerifyFailed
};
enum Capability : quint64 {
    FirmwareUpdate = 1ULL << 0, SensorService = 1ULL << 1,
    TelemetryStreaming = 1ULL << 2, ConfigurationService = 1ULL << 3,
    MotorService = 1ULL << 4, DiagnosticEvents = 1ULL << 5
};
enum class DataType : quint8 {
    UInt8, Int8, UInt16, Int16, UInt32, Int32, UInt64, Int64, Float32, Float64
};

struct Packet {
    quint8 version{Version};
    MessageType messageType{MessageType::Request};
    quint8 serviceId{};
    quint8 commandId{};
    quint16 sequence{};
    QByteArray payload;
    bool operator==(const Packet &) const = default;
};

struct CodecDiagnostics {
    quint64 validFrames{};
    quint64 crcErrors{};
    quint64 headerErrors{};
    quint64 lengthErrors{};
    quint64 discardedBytes{};
};

inline void appendU16(QByteArray &b, quint16 v) { b.append(char(v)); b.append(char(v >> 8)); }
inline void appendU32(QByteArray &b, quint32 v) { appendU16(b, quint16(v)); appendU16(b, quint16(v >> 16)); }
inline void appendU64(QByteArray &b, quint64 v) { appendU32(b, quint32(v)); appendU32(b, quint32(v >> 32)); }
inline bool readU8(const QByteArray &b, qsizetype &p, quint8 &v) { if (p + 1 > b.size()) return false; v = quint8(b[p++]); return true; }
inline bool readU16(const QByteArray &b, qsizetype &p, quint16 &v) { if (p + 2 > b.size()) return false; v = quint8(b[p]) | (quint16(quint8(b[p + 1])) << 8); p += 2; return true; }
inline bool readU32(const QByteArray &b, qsizetype &p, quint32 &v) { quint16 a, c; if (!readU16(b,p,a)||!readU16(b,p,c)) return false; v = a | (quint32(c)<<16); return true; }
inline bool readU64(const QByteArray &b, qsizetype &p, quint64 &v) { quint32 a,c; if(!readU32(b,p,a)||!readU32(b,p,c)) return false; v=a|(quint64(c)<<32); return true; }
inline void appendFloat32(QByteArray &b, float v) { quint32 u; memcpy(&u,&v,4); appendU32(b,u); }
inline void appendFloat64(QByteArray &b, double v) { quint64 u; memcpy(&u,&v,8); appendU64(b,u); }
inline bool readFloat32(const QByteArray &b, qsizetype &p, float &v) { quint32 u; if(!readU32(b,p,u)) return false; memcpy(&v,&u,4); return true; }
inline bool readFloat64(const QByteArray &b, qsizetype &p, double &v) { quint64 u; if(!readU64(b,p,u)) return false; memcpy(&v,&u,8); return true; }
inline bool readString(const QByteArray &b, qsizetype &p, QString &v) { quint8 n; if(!readU8(b,p,n)||p+n>b.size()) return false; v=QString::fromUtf8(b.constData()+p,n); p+=n; return true; }
}

Q_DECLARE_METATYPE(Cesc::Packet)
