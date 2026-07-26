#include "CustomBinaryCodec.h"

#include "utils/ChecksumUtils.h"

#include <QtEndian>

#include <bit>
#include <cmath>
#include <limits>

namespace {

int fieldByteCount(const FieldDefinition &field)
{
    return qMax(1, (field.bitWidth + 7) / 8);
}

int messageByteCount(const MessageDefinition &message)
{
    int result = 0;
    for (const FieldDefinition &field : message.fields) {
        result += fieldByteCount(field);
    }
    return result;
}

QByteArray unsignedBytes(
    quint64 value, const int size, const bool littleEndian)
{
    QByteArray result(size, '\0');
    for (int index = 0; index < size; ++index) {
        const int target = littleEndian ? index : size - index - 1;
        result[target] = static_cast<char>(value & 0xFFU);
        value >>= 8U;
    }
    return result;
}

quint64 bytesToUnsigned(
    const QByteArray &bytes, const bool littleEndian)
{
    quint64 result = 0;
    for (int index = 0; index < bytes.size(); ++index) {
        const int source = littleEndian
            ? bytes.size() - index - 1 : index;
        result = (result << 8U)
            | static_cast<quint8>(bytes.at(source));
    }
    return result;
}

QByteArray checksumBytes(
    const QByteArray &covered,
    const FieldDefinition &field)
{
    quint8 value = 0;
    switch (field.checksumAlgorithm) {
    case ProtocolChecksumAlgorithm::Sum8:
        for (const char byte : covered) {
            value = static_cast<quint8>(
                value + static_cast<quint8>(byte));
        }
        break;
    case ProtocolChecksumAlgorithm::Xor8:
        value = ChecksumUtils::xor8(covered);
        break;
    case ProtocolChecksumAlgorithm::Crc8:
        value = ChecksumUtils::crc8(covered);
        break;
    }
    return unsignedBytes(
        value, fieldByteCount(field), field.littleEndian);
}

QString fieldName(const FieldDefinition &field, const int index)
{
    if (!field.displayName.trimmed().isEmpty()) {
        return field.displayName;
    }
    if (!field.id.trimmed().isEmpty()) return field.id;
    return QStringLiteral("data%1").arg(index + 1);
}

QVariant automaticValue(
    const ProtocolDefinition &protocol,
    const MessageDefinition &message,
    const FieldDefinition &field,
    const int totalSize)
{
    if (field.role == ProtocolFieldRole::Length) {
        return totalSize;
    }
    if (field.role == ProtocolFieldRole::MessageId) {
        if (field.fixedValue.isValid()) return field.fixedValue;
        for (int index = 0; index < protocol.sendMessages.size(); ++index) {
            if (protocol.sendMessages.at(index).id == message.id) {
                return index;
            }
        }
        for (int index = 0; index < protocol.receiveMessages.size(); ++index) {
            if (protocol.receiveMessages.at(index).id == message.id) {
                return index;
            }
        }
        return 0;
    }
    if (field.fixedValue.isValid()) return field.fixedValue;
    if (field.defaultValue.isValid()) return field.defaultValue;
    return {};
}

bool encodeValue(
    const FieldDefinition &field,
    const QVariant &value,
    QByteArray *bytes,
    QString *error)
{
    const int size = fieldByteCount(field);
    if (field.type == ProtocolFieldType::ByteArray
        || field.type == ProtocolFieldType::String) {
        QByteArray raw = value.toByteArray();
        if (field.type == ProtocolFieldType::String) {
            raw = value.toString().toUtf8();
        }
        if (raw.isEmpty() && !value.isValid()) raw = QByteArray(size, '\0');
        if (raw.size() != size) {
            *error = QStringLiteral("字段“%1”需要 %2 字节，当前为 %3 字节")
                         .arg(field.displayName)
                         .arg(size)
                         .arg(raw.size());
            return false;
        }
        *bytes = raw;
        return true;
    }

    bool ok = false;
    const double physical = value.toDouble(&ok);
    if (!ok || !std::isfinite(physical)) {
        *error = QStringLiteral("字段“%1”的值无效")
                     .arg(field.displayName);
        return false;
    }
    if (qFuzzyIsNull(field.scale)) {
        *error = QStringLiteral("字段“%1”的缩放值不能为 0")
                     .arg(field.displayName);
        return false;
    }
    const double rawValue = (physical - field.offset) / field.scale;

    if (field.type == ProtocolFieldType::Float) {
        if (size != 4) {
            *error = QStringLiteral("Float 字段“%1”必须为 4 字节")
                         .arg(field.displayName);
            return false;
        }
        const quint32 bits = std::bit_cast<quint32>(
            static_cast<float>(rawValue));
        *bytes = unsignedBytes(bits, size, field.littleEndian);
        return true;
    }
    if (field.type == ProtocolFieldType::Double) {
        if (size != 8) {
            *error = QStringLiteral("Double 字段“%1”必须为 8 字节")
                         .arg(field.displayName);
            return false;
        }
        const quint64 bits = std::bit_cast<quint64>(rawValue);
        *bytes = unsignedBytes(bits, size, field.littleEndian);
        return true;
    }

    const double rounded = std::round(rawValue);
    if (!qFuzzyCompare(rawValue + 1.0, rounded + 1.0)) {
        *error = QStringLiteral("字段“%1”按缩放换算后不是整数")
                     .arg(field.displayName);
        return false;
    }
    if (field.type == ProtocolFieldType::Int) {
        const int bits = qMin(64, size * 8);
        const long double minimum = bits == 64
            ? static_cast<long double>((std::numeric_limits<qint64>::min)())
            : -std::ldexp(1.0L, bits - 1);
        const long double maximum = bits == 64
            ? static_cast<long double>((std::numeric_limits<qint64>::max)())
            : std::ldexp(1.0L, bits - 1) - 1.0L;
        if (rawValue < minimum || rawValue > maximum) {
            *error = QStringLiteral("字段“%1”超出 %2 位有符号范围")
                         .arg(field.displayName).arg(bits);
            return false;
        }
        *bytes = unsignedBytes(
            static_cast<quint64>(static_cast<qint64>(rounded)),
            size, field.littleEndian);
        return true;
    }

    const int bits = qMin(64, size * 8);
    const long double maximum = bits == 64
        ? static_cast<long double>((std::numeric_limits<quint64>::max)())
        : std::ldexp(1.0L, bits) - 1.0L;
    if (rawValue < 0.0 || rawValue > maximum) {
        *error = QStringLiteral("字段“%1”超出 %2 位无符号范围")
                     .arg(field.displayName).arg(bits);
        return false;
    }
    *bytes = unsignedBytes(
        static_cast<quint64>(rounded), size, field.littleEndian);
    return true;
}

QVariant decodeValue(
    const FieldDefinition &field, const QByteArray &bytes)
{
    if (field.type == ProtocolFieldType::ByteArray) {
        return QString::fromLatin1(bytes.toHex(' ').toUpper());
    }
    if (field.type == ProtocolFieldType::String) {
        return QString::fromUtf8(bytes);
    }
    const quint64 raw = bytesToUnsigned(bytes, field.littleEndian);
    double numeric = 0.0;
    if (field.type == ProtocolFieldType::Float && bytes.size() == 4) {
        numeric = std::bit_cast<float>(static_cast<quint32>(raw));
    } else if (field.type == ProtocolFieldType::Double
               && bytes.size() == 8) {
        numeric = std::bit_cast<double>(raw);
    } else if (field.type == ProtocolFieldType::Int) {
        quint64 extended = raw;
        const int bits = qMin(64, bytes.size() * 8);
        if (bits < 64 && (raw & (quint64{1} << (bits - 1)))) {
            extended |= (~quint64{0}) << bits;
        }
        const qint64 signedValue = static_cast<qint64>(extended);
        if (qFuzzyCompare(field.scale, 1.0)
            && qFuzzyIsNull(field.offset)) {
            return signedValue;
        }
        numeric = static_cast<double>(signedValue);
    } else {
        if (qFuzzyCompare(field.scale, 1.0)
            && qFuzzyIsNull(field.offset)) {
            return raw;
        }
        numeric = static_cast<double>(raw);
    }
    return numeric * field.scale + field.offset;
}

bool fixedFieldBytes(
    const ProtocolDefinition &protocol,
    const MessageDefinition &message,
    const FieldDefinition &field,
    QByteArray *bytes)
{
    if (field.role != ProtocolFieldRole::FrameHeader
        && field.role != ProtocolFieldRole::Constant
        && field.role != ProtocolFieldRole::MessageId) {
        return false;
    }
    QString ignored;
    return encodeValue(
        field,
        automaticValue(
            protocol, message, field, messageByteCount(message)),
        bytes, &ignored);
}

bool staticFieldsMatch(
    const ProtocolDefinition &protocol,
    const MessageDefinition &message,
    const QByteArray &buffer)
{
    int offset = 0;
    for (const FieldDefinition &field : message.fields) {
        QByteArray expected;
        if (fixedFieldBytes(protocol, message, field, &expected)) {
            const int available =
                qMin(expected.size(), qMax(0, buffer.size() - offset));
            if (available > 0
                && buffer.mid(offset, available)
                    != expected.left(available)) {
                return false;
            }
        }
        offset += fieldByteCount(field);
    }
    return true;
}

int staticSpecificity(
    const ProtocolDefinition &protocol,
    const MessageDefinition &message)
{
    int result = 0;
    for (const FieldDefinition &field : message.fields) {
        QByteArray expected;
        if (fixedFieldBytes(protocol, message, field, &expected)) {
            result += expected.size();
        }
    }
    return result;
}

bool decodeMessage(
    const ProtocolDefinition &protocol,
    const MessageDefinition &message,
    const QByteArray &frame,
    ParsedMessage *parsed,
    QString *error)
{
    const int totalSize = messageByteCount(message);
    if (frame.size() != totalSize) return false;
    parsed->displayName = message.displayName.trimmed().isEmpty()
        ? message.id : message.displayName;
    parsed->fields.clear();
    int offset = 0;
    for (int index = 0; index < message.fields.size(); ++index) {
        const FieldDefinition &field = message.fields.at(index);
        const int size = fieldByteCount(field);
        const QByteArray bytes = frame.mid(offset, size);

        if (field.role == ProtocolFieldRole::Checksum) {
            const QByteArray expected =
                checksumBytes(frame.left(offset), field);
            if (bytes != expected) {
                *error = QStringLiteral("协议“%1”校验失败")
                             .arg(parsed->displayName);
                return false;
            }
        } else if (field.role == ProtocolFieldRole::Length) {
            const quint64 stated =
                bytesToUnsigned(bytes, field.littleEndian);
            if (stated != static_cast<quint64>(totalSize)) {
                *error = QStringLiteral(
                    "协议“%1”长度字段为 %2，实际应为 %3")
                             .arg(parsed->displayName)
                             .arg(stated)
                             .arg(totalSize);
                return false;
            }
        } else {
            QByteArray expected;
            if (fixedFieldBytes(protocol, message, field, &expected)
                && bytes != expected) {
                return false;
            }
        }

        parsed->fields.append(
            ParsedField{
                fieldName(field, index),
                decodeValue(field, bytes),
                field.unit,
                field.role});
        offset += size;
    }
    return true;
}

} // namespace

CustomBinaryCodec::CustomBinaryCodec(ProtocolDefinition protocol)
    : m_protocol(std::move(protocol))
{
}

bool CustomBinaryCodec::encode(
    const ProtocolDefinition &protocol,
    const MessageDefinition &message,
    const ProtocolFieldValues &values,
    QByteArray *payload,
    QString *errorMessage) const
{
    if (!payload || !errorMessage) return false;
    payload->clear();
    errorMessage->clear();
    const int totalSize = messageByteCount(message);
    for (int index = 0; index < message.fields.size(); ++index) {
        const FieldDefinition &field = message.fields.at(index);
        if (field.role == ProtocolFieldRole::Checksum) {
            payload->append(checksumBytes(*payload, field));
            continue;
        }
        const QString id = field.id.trimmed().isEmpty()
            ? QStringLiteral("data%1").arg(index + 1) : field.id;
        const QVariant value =
            field.role == ProtocolFieldRole::Value && field.editable
            ? values.value(id)
            : automaticValue(protocol, message, field, totalSize);
        QByteArray bytes;
        if (!encodeValue(field, value, &bytes, errorMessage)) return false;
        payload->append(bytes);
    }
    return payload->size() == totalSize;
}

bool CustomBinaryCodec::parse(
    const QByteArray &data,
    QList<ParsedMessage> *messages,
    QString *errorMessage) const
{
    if (!messages || !errorMessage) return false;
    messages->clear();
    errorMessage->clear();
    if (data.isEmpty()) return true;
    m_receiveBuffer.append(data);
    constexpr qsizetype maximumBuffer = 1024 * 1024;
    if (m_receiveBuffer.size() > maximumBuffer) {
        m_receiveBuffer =
            m_receiveBuffer.right(maximumBuffer / 2);
    }

    while (!m_receiveBuffer.isEmpty()) {
        QList<const MessageDefinition *> candidates;
        int bestSpecificity = -1;
        QString candidateFailure;
        for (const MessageDefinition &message : m_protocol.receiveMessages) {
            if (!staticFieldsMatch(m_protocol, message, m_receiveBuffer)) {
                continue;
            }
            const int specificity =
                staticSpecificity(m_protocol, message);
            if (specificity < bestSpecificity) continue;
            if (specificity > bestSpecificity) {
                candidates.clear();
                bestSpecificity = specificity;
            }
            candidates.append(&message);
        }
        if (candidates.isEmpty()) {
            m_receiveBuffer.remove(0, 1);
            continue;
        }

        bool waiting = false;
        bool consumed = false;
        for (const MessageDefinition *message : candidates) {
            const int size = messageByteCount(*message);
            if (m_receiveBuffer.size() < size) {
                waiting = true;
                continue;
            }
            ParsedMessage parsed;
            QString candidateError;
            if (decodeMessage(
                    m_protocol, *message,
                    m_receiveBuffer.left(size),
                    &parsed, &candidateError)) {
                messages->append(parsed);
                m_receiveBuffer.remove(0, size);
                consumed = true;
                break;
            }
            if (!candidateError.isEmpty()) {
                candidateFailure = candidateError;
            }
        }
        if (consumed) continue;
        if (waiting) break;
        if (bestSpecificity > 0 && !candidateFailure.isEmpty()) {
            int discardSize = 1;
            for (const MessageDefinition *message : candidates) {
                const int size = messageByteCount(*message);
                if (m_receiveBuffer.size() >= size) {
                    discardSize = size;
                    break;
                }
            }
            m_receiveBuffer.remove(0, discardSize);
            *errorMessage = candidateFailure;
            return false;
        }
        m_receiveBuffer.remove(0, 1);
    }
    return true;
}
