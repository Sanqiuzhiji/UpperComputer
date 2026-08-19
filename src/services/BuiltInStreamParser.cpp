#include "BuiltInStreamParser.h"

#include <QtEndian>

#include <bit>
#include <cmath>

namespace {
constexpr qsizetype kMaximumBufferedBytes = 1024 * 1024;
const QByteArray kJustFloatTail("\x00\x00\x80\x7f", 4);
}

BuiltInStreamParser::BuiltInStreamParser(const ParserMode mode) : m_mode(mode) {}

QString BuiltInStreamParser::protocolId() const
{
    return m_mode == ParserMode::FireWater
        ? QStringLiteral("firewater") : QStringLiteral("just-float");
}

bool BuiltInStreamParser::parse(const QByteArray &data,
                                QList<ParsedMessage> *messages,
                                QString *errorMessage)
{
    if (!messages) return false;
    if (errorMessage) errorMessage->clear();
    m_buffer.append(data);
    if (m_buffer.size() > kMaximumBufferedBytes) {
        m_buffer.clear();
        if (errorMessage) *errorMessage = QStringLiteral("接收缓存超过 1 MiB，已清空");
        return false;
    }
    if (m_mode == ParserMode::FireWater) return parseFireWater(messages, errorMessage);
    if (m_mode == ParserMode::JustFloat) return parseJustFloat(messages, errorMessage);
    if (errorMessage) *errorMessage = QStringLiteral("不是内置流式解析模式");
    return false;
}

bool BuiltInStreamParser::parseFireWater(QList<ParsedMessage> *messages,
                                         QString *errorMessage)
{
    bool valid = true;
    while (true) {
        const qsizetype newline = m_buffer.indexOf('\n');
        if (newline < 0) break;
        QByteArray line = m_buffer.left(newline);
        m_buffer.remove(0, newline + 1);
        if (line.endsWith('\r')) line.chop(1);
        line = line.trimmed();
        if (line.isEmpty()) continue;
        const qsizetype colon = line.lastIndexOf(':');
        if (colon >= 0) line = line.mid(colon + 1).trimmed();

        QList<float> values;
        const QList<QByteArray> fields = line.split(',');
        values.reserve(fields.size());
        for (const QByteArray &field : fields) {
            bool ok = false;
            const double value = QString::fromLatin1(field.trimmed()).toDouble(&ok);
            if (!ok || !std::isfinite(value)) {
                valid = false;
                values.clear();
                break;
            }
            const float sample = static_cast<float>(value);
            if (!std::isfinite(sample)) {
                valid = false;
                values.clear();
                break;
            }
            values.append(sample);
        }
        if (!values.isEmpty()) messages->append(makeMessage(values));
    }
    if (!valid && errorMessage)
        *errorMessage = QStringLiteral("FireWater 数据行包含无效浮点数");
    return valid;
}

bool BuiltInStreamParser::parseJustFloat(QList<ParsedMessage> *messages,
                                         QString *errorMessage)
{
    bool valid = true;
    while (true) {
        const qsizetype tail = m_buffer.indexOf(kJustFloatTail);
        if (tail < 0) break;
        const QByteArray payload = m_buffer.left(tail);
        m_buffer.remove(0, tail + kJustFloatTail.size());
        if (payload.isEmpty() || payload.size() % qsizetype(sizeof(float)) != 0) {
            valid = false;
            continue;
        }
        QList<float> values;
        values.reserve(payload.size() / qsizetype(sizeof(float)));
        for (qsizetype offset = 0; offset < payload.size(); offset += sizeof(float)) {
            const auto *bytes = reinterpret_cast<const uchar *>(payload.constData() + offset);
            const float value = std::bit_cast<float>(qFromLittleEndian<quint32>(bytes));
            if (!std::isfinite(value)) {
                valid = false;
                values.clear();
                break;
            }
            values.append(value);
        }
        if (!values.isEmpty()) messages->append(makeMessage(values));
    }
    if (!valid && errorMessage)
        *errorMessage = QStringLiteral("JustFloat 帧长度无效或数据包含非有限浮点数");
    return valid;
}

ParsedMessage BuiltInStreamParser::makeMessage(const QList<float> &values) const
{
    ParsedMessage message;
    message.messageId = QStringLiteral("samples");
    message.displayName = m_mode == ParserMode::FireWater
        ? QStringLiteral("FireWater") : QStringLiteral("JustFloat");
    message.fields.reserve(values.size());
    for (qsizetype index = 0; index < values.size(); ++index) {
        const QString name = QStringLiteral("ch%1").arg(index);
        message.fields.append({name, name, values.at(index), {}, ProtocolFieldRole::Value});
    }
    return message;
}
