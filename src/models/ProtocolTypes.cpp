#include "ProtocolTypes.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>

namespace ProtocolModel {
namespace {

QString uuidText(const QUuid &id)
{
    return id.toString(QUuid::WithoutBraces);
}

QUuid readUuid(const QJsonObject &object, const char *key)
{
    return QUuid(object.value(QLatin1String(key)).toString());
}

template<typename Enum>
Enum enumFromKey(const QString &key,
                 const QList<QPair<QString, Enum>> &values,
                 const Enum fallback)
{
    for (const auto &[candidate, value] : values) {
        if (candidate.compare(key, Qt::CaseInsensitive) == 0) {
            return value;
        }
    }
    return fallback;
}

QString uniqueDefaultName(const FieldRole role)
{
    switch (role) {
    case FieldRole::Header: return QStringLiteral("header");
    case FieldRole::FrameId: return QStringLiteral("frameId");
    case FieldRole::Length: return QStringLiteral("length");
    case FieldRole::Data: return QStringLiteral("data");
    case FieldRole::Checksum: return QStringLiteral("checksum");
    case FieldRole::Tail: return QStringLiteral("tail");
    case FieldRole::Skip: return QStringLiteral("skip");
    }
    return QStringLiteral("data");
}

QString frameDirectionKey(const FrameDirection direction)
{
    switch (direction) {
    case FrameDirection::Bidirectional:
        return QStringLiteral("bidirectional");
    case FrameDirection::TransmitOnly:
        return QStringLiteral("transmitOnly");
    case FrameDirection::ReceiveOnly:
        return QStringLiteral("receiveOnly");
    }
    return QStringLiteral("bidirectional");
}

QJsonObject fieldToJson(const Field &field)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), uuidText(field.id));
    object.insert(QStringLiteral("name"), field.name);
    object.insert(QStringLiteral("role"), roleKey(field.role));
    object.insert(QStringLiteral("dataType"), dataTypeKey(field.dataType));
    object.insert(QStringLiteral("byteCount"), field.byteCount);
    object.insert(QStringLiteral("byteOrder"), byteOrderKey(field.byteOrder));
    object.insert(QStringLiteral("scale"), field.scale);
    object.insert(QStringLiteral("offset"), field.offset);
    object.insert(QStringLiteral("fixedHex"), formatHex(field.fixedBytes));
    object.insert(
        QStringLiteral("checksumAlgorithm"),
        checksumKey(field.checksumAlgorithm));
    return object;
}

Field fieldFromJson(const QJsonObject &object)
{
    Field field;
    field.id = readUuid(object, "id");
    if (field.id.isNull()) field.id = QUuid::createUuid();
    field.name = object.value(QStringLiteral("name")).toString();
    field.role = enumFromKey<FieldRole>(
        object.value(QStringLiteral("role")).toString(),
        {
            {QStringLiteral("header"), FieldRole::Header},
            {QStringLiteral("frameId"), FieldRole::FrameId},
            {QStringLiteral("length"), FieldRole::Length},
            {QStringLiteral("data"), FieldRole::Data},
            {QStringLiteral("checksum"), FieldRole::Checksum},
            {QStringLiteral("tail"), FieldRole::Tail},
            {QStringLiteral("skip"), FieldRole::Skip}
        },
        FieldRole::Data);
    field.dataType = enumFromKey<DataType>(
        object.value(QStringLiteral("dataType")).toString(),
        {
            {QStringLiteral("uint8"), DataType::UInt8},
            {QStringLiteral("int8"), DataType::Int8},
            {QStringLiteral("uint16"), DataType::UInt16},
            {QStringLiteral("int16"), DataType::Int16},
            {QStringLiteral("uint32"), DataType::UInt32},
            {QStringLiteral("int32"), DataType::Int32},
            {QStringLiteral("uint64"), DataType::UInt64},
            {QStringLiteral("int64"), DataType::Int64},
            {QStringLiteral("float32"), DataType::Float32},
            {QStringLiteral("float64"), DataType::Float64},
            {QStringLiteral("bytes"), DataType::Bytes}
        },
        DataType::UInt8);
    field.byteCount = object.value(QStringLiteral("byteCount")).toInt(1);
    field.byteOrder = enumFromKey<ByteOrder>(
        object.value(QStringLiteral("byteOrder")).toString(),
        {
            {QStringLiteral("littleEndian"), ByteOrder::LittleEndian},
            {QStringLiteral("bigEndian"), ByteOrder::BigEndian}
        },
        ByteOrder::LittleEndian);
    field.scale = object.value(QStringLiteral("scale")).toDouble(1.0);
    field.offset = object.value(QStringLiteral("offset")).toDouble(0.0);
    bool validHex = false;
    field.fixedBytes = parseHex(
        object.value(QStringLiteral("fixedHex")).toString(), &validHex);
    if (!validHex) field.fixedBytes.clear();
    field.checksumAlgorithm = enumFromKey<ChecksumAlgorithm>(
        object.value(QStringLiteral("checksumAlgorithm")).toString(),
        {
            {QStringLiteral("sum8"), ChecksumAlgorithm::Sum8},
            {QStringLiteral("xor8"), ChecksumAlgorithm::Xor8},
            {QStringLiteral("crc8"), ChecksumAlgorithm::Crc8}
        },
        ChecksumAlgorithm::Sum8);
    return field;
}

} // namespace

Field makeField(const FieldRole role)
{
    Field field;
    field.id = QUuid::createUuid();
    field.role = role;
    field.name = uniqueDefaultName(role);
    switch (role) {
    case FieldRole::Header:
        field.byteCount = 2;
        field.fixedBytes = QByteArray::fromHex("AA55");
        field.dataType = DataType::Bytes;
        break;
    case FieldRole::Tail:
        field.byteCount = 1;
        field.fixedBytes = QByteArray::fromHex("0D");
        field.dataType = DataType::Bytes;
        break;
    case FieldRole::FrameId:
    case FieldRole::Length:
    case FieldRole::Checksum:
    case FieldRole::Skip:
        field.byteCount = 1;
        field.dataType = DataType::UInt8;
        break;
    case FieldRole::Data:
        field.byteCount = 1;
        field.dataType = DataType::UInt8;
        break;
    }
    return field;
}

Frame makeFrame(const QString &name)
{
    Frame frame;
    frame.id = QUuid::createUuid();
    frame.name = name;
    return frame;
}

Document makeDocument(const QString &name)
{
    Document document;
    document.id = QUuid::createUuid();
    document.name = name;
    document.frames.append(makeFrame());
    return document;
}

Field duplicatedField(const Field &source)
{
    Field result = source;
    result.id = QUuid::createUuid();
    return result;
}

Frame duplicatedFrame(const Frame &source)
{
    Frame result = source;
    result.id = QUuid::createUuid();
    for (Field &field : result.fields) {
        field.id = QUuid::createUuid();
    }
    return result;
}

int frameByteCount(const Frame &frame)
{
    int total = 0;
    for (const Field &field : frame.fields) {
        total += qMax(0, field.byteCount);
    }
    return total;
}

int documentByteCount(const Document &document)
{
    int total = 0;
    for (const Frame &frame : document.frames) {
        total += frameByteCount(frame);
    }
    return total;
}

int naturalByteCount(const DataType type)
{
    switch (type) {
    case DataType::UInt8:
    case DataType::Int8: return 1;
    case DataType::UInt16:
    case DataType::Int16: return 2;
    case DataType::UInt32:
    case DataType::Int32:
    case DataType::Float32: return 4;
    case DataType::UInt64:
    case DataType::Int64:
    case DataType::Float64: return 8;
    case DataType::Bytes: return 0;
    }
    return 0;
}

QVector<int> fieldOffsets(const Frame &frame)
{
    QVector<int> offsets;
    offsets.reserve(frame.fields.size());
    int offset = 0;
    for (const Field &field : frame.fields) {
        offsets.append(offset);
        offset += qMax(0, field.byteCount);
    }
    return offsets;
}

QString roleKey(const FieldRole role)
{
    switch (role) {
    case FieldRole::Header: return QStringLiteral("header");
    case FieldRole::FrameId: return QStringLiteral("frameId");
    case FieldRole::Length: return QStringLiteral("length");
    case FieldRole::Data: return QStringLiteral("data");
    case FieldRole::Checksum: return QStringLiteral("checksum");
    case FieldRole::Tail: return QStringLiteral("tail");
    case FieldRole::Skip: return QStringLiteral("skip");
    }
    return QStringLiteral("data");
}

QString roleDisplayName(const FieldRole role)
{
    switch (role) {
    case FieldRole::Header: return QStringLiteral("帧头 Header");
    case FieldRole::FrameId: return QStringLiteral("帧 ID FrameId");
    case FieldRole::Length: return QStringLiteral("帧长度 Length");
    case FieldRole::Data: return QStringLiteral("数据 Data");
    case FieldRole::Checksum: return QStringLiteral("校验 Checksum");
    case FieldRole::Tail: return QStringLiteral("帧尾 Tail");
    case FieldRole::Skip: return QStringLiteral("跳过 Skip");
    }
    return QStringLiteral("数据 Data");
}

QString dataTypeKey(const DataType type)
{
    switch (type) {
    case DataType::UInt8: return QStringLiteral("uint8");
    case DataType::Int8: return QStringLiteral("int8");
    case DataType::UInt16: return QStringLiteral("uint16");
    case DataType::Int16: return QStringLiteral("int16");
    case DataType::UInt32: return QStringLiteral("uint32");
    case DataType::Int32: return QStringLiteral("int32");
    case DataType::UInt64: return QStringLiteral("uint64");
    case DataType::Int64: return QStringLiteral("int64");
    case DataType::Float32: return QStringLiteral("float32");
    case DataType::Float64: return QStringLiteral("float64");
    case DataType::Bytes: return QStringLiteral("bytes");
    }
    return QStringLiteral("bytes");
}

QString dataTypeDisplayName(const DataType type)
{
    return dataTypeKey(type);
}

QString byteOrderKey(const ByteOrder order)
{
    return order == ByteOrder::LittleEndian
        ? QStringLiteral("littleEndian") : QStringLiteral("bigEndian");
}

QString checksumKey(const ChecksumAlgorithm algorithm)
{
    switch (algorithm) {
    case ChecksumAlgorithm::Sum8: return QStringLiteral("sum8");
    case ChecksumAlgorithm::Xor8: return QStringLiteral("xor8");
    case ChecksumAlgorithm::Crc8: return QStringLiteral("crc8");
    }
    return QStringLiteral("sum8");
}

QString fieldRoleColor(const FieldRole role)
{
    switch (role) {
    case FieldRole::Header: return QStringLiteral("#3B82F6");
    case FieldRole::FrameId: return QStringLiteral("#22B8CF");
    case FieldRole::Length: return QStringLiteral("#8B5CF6");
    case FieldRole::Data: return QStringLiteral("#22C55E");
    case FieldRole::Checksum: return QStringLiteral("#F59E0B");
    case FieldRole::Tail: return QStringLiteral("#64748B");
    case FieldRole::Skip: return QStringLiteral("#8B929E");
    }
    return QStringLiteral("#8B929E");
}

QJsonObject toJson(const Document &document)
{
    QJsonObject object;
    object.insert(QStringLiteral("schemaVersion"), document.schemaVersion);
    object.insert(QStringLiteral("id"), uuidText(document.id));
    object.insert(QStringLiteral("name"), document.name);
    QJsonArray frames;
    for (const Frame &frame : document.frames) {
        QJsonObject frameObject;
        frameObject.insert(QStringLiteral("id"), uuidText(frame.id));
        frameObject.insert(QStringLiteral("name"), frame.name);
        frameObject.insert(
            QStringLiteral("direction"),
            frameDirectionKey(frame.direction));
        QJsonArray fields;
        for (const Field &field : frame.fields) {
            fields.append(fieldToJson(field));
        }
        frameObject.insert(QStringLiteral("fields"), fields);
        frames.append(frameObject);
    }
    object.insert(QStringLiteral("frames"), frames);
    return object;
}

bool fromJson(
    const QJsonObject &object, Document *document, QString *errorMessage)
{
    if (!document) {
        if (errorMessage) *errorMessage = QStringLiteral("没有可写入的协议文档");
        return false;
    }
    const int schemaVersion =
        object.value(QStringLiteral("schemaVersion")).toInt(-1);
    if (schemaVersion < 1 || schemaVersion > CurrentSchemaVersion) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("不支持的 schemaVersion：%1")
                                .arg(schemaVersion);
        }
        return false;
    }
    Document result;
    result.schemaVersion = schemaVersion;
    result.id = readUuid(object, "id");
    if (result.id.isNull()) {
        if (errorMessage) *errorMessage = QStringLiteral("协议文档 ID 无效");
        return false;
    }
    result.name = object.value(QStringLiteral("name")).toString();
    const QJsonValue framesValue = object.value(QStringLiteral("frames"));
    if (!framesValue.isArray()) {
        if (errorMessage) *errorMessage = QStringLiteral("frames 必须是数组");
        return false;
    }
    for (const QJsonValue &frameValue : framesValue.toArray()) {
        if (!frameValue.isObject()) {
            if (errorMessage) *errorMessage = QStringLiteral("协议帧格式无效");
            return false;
        }
        const QJsonObject frameObject = frameValue.toObject();
        Frame frame;
        frame.id = readUuid(frameObject, "id");
        if (frame.id.isNull()) frame.id = QUuid::createUuid();
        frame.name = frameObject.value(QStringLiteral("name")).toString();
        frame.direction = enumFromKey<FrameDirection>(
            frameObject.value(QStringLiteral("direction")).toString(),
            {
                {QStringLiteral("bidirectional"),
                 FrameDirection::Bidirectional},
                {QStringLiteral("transmitOnly"),
                 FrameDirection::TransmitOnly},
                {QStringLiteral("receiveOnly"),
                 FrameDirection::ReceiveOnly}
            },
            FrameDirection::Bidirectional);
        const QJsonValue fieldsValue = frameObject.value(QStringLiteral("fields"));
        if (!fieldsValue.isArray()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("协议帧 fields 必须是数组");
            }
            return false;
        }
        for (const QJsonValue &fieldValue : fieldsValue.toArray()) {
            if (!fieldValue.isObject()) {
                if (errorMessage) *errorMessage = QStringLiteral("字段格式无效");
                return false;
            }
            frame.fields.append(fieldFromJson(fieldValue.toObject()));
        }
        result.frames.append(frame);
    }
    *document = result;
    return true;
}

QVector<ValidationIssue> validate(const Document &document)
{
    QVector<ValidationIssue> issues;
    if (document.name.trimmed().isEmpty()) {
        issues.append({
            ValidationSeverity::Error,
            QStringLiteral("工作空间名称不能为空"), {}, {}});
    }

    QSet<QString> frameNames;
    for (const Frame &frame : document.frames) {
        const QString frameName = frame.name.trimmed().toCaseFolded();
        if (frameName.isEmpty()) {
            issues.append({
                ValidationSeverity::Error,
                QStringLiteral("协议帧名称不能为空"), frame.id, {}});
        } else if (frameNames.contains(frameName)) {
            issues.append({
                ValidationSeverity::Error,
                QStringLiteral("协议帧名称“%1”重复").arg(frame.name),
                frame.id, {}});
        } else {
            frameNames.insert(frameName);
        }

        QSet<QString> fieldNames;
        for (const Field &field : frame.fields) {
            const QString fieldName = field.name.trimmed().toCaseFolded();
            if (fieldName.isEmpty()) {
                issues.append({
                    ValidationSeverity::Error,
                    QStringLiteral("字段名称不能为空"), frame.id, field.id});
            } else if (fieldNames.contains(fieldName)) {
                issues.append({
                    ValidationSeverity::Error,
                    QStringLiteral("协议帧“%1”中的字段“%2”名称重复")
                        .arg(frame.name, field.name),
                    frame.id, field.id});
            } else {
                fieldNames.insert(fieldName);
            }

            if (field.byteCount <= 0) {
                issues.append({
                    ValidationSeverity::Error,
                    QStringLiteral("字段“%1”的字节数必须大于 0")
                        .arg(field.name),
                    frame.id, field.id});
            }
            if (field.role == FieldRole::Header
                || field.role == FieldRole::Tail) {
                if (field.fixedBytes.size() != field.byteCount) {
                    issues.append({
                        ValidationSeverity::Error,
                        QStringLiteral("字段“%1”的固定 Hex 长度应为 %2 字节")
                            .arg(field.name).arg(field.byteCount),
                        frame.id, field.id});
                }
            }
            const bool typedRole = field.role == FieldRole::Data
                || field.role == FieldRole::FrameId
                || field.role == FieldRole::Length;
            const int natural = naturalByteCount(field.dataType);
            if (typedRole && natural > 0 && natural != field.byteCount) {
                issues.append({
                    ValidationSeverity::Warning,
                    QStringLiteral("%1 通常占 %2 字节，当前设置为 %3 字节")
                        .arg(dataTypeDisplayName(field.dataType))
                        .arg(natural)
                        .arg(field.byteCount),
                    frame.id, field.id});
            }
        }
    }
    return issues;
}

bool hasValidationErrors(const QVector<ValidationIssue> &issues)
{
    return std::ranges::any_of(
        issues, [](const ValidationIssue &issue) {
            return issue.severity == ValidationSeverity::Error;
        });
}

QByteArray parseHex(const QString &text, bool *ok)
{
    QString compact = text;
    compact.remove(QRegularExpression(QStringLiteral("\\s")));
    const bool valid = compact.size() % 2 == 0
        && QRegularExpression(QStringLiteral("^[0-9A-Fa-f]*$"))
               .match(compact)
               .hasMatch();
    if (ok) *ok = valid;
    return valid ? QByteArray::fromHex(compact.toLatin1()) : QByteArray{};
}

QString formatHex(const QByteArray &bytes)
{
    const QByteArray compact = bytes.toHex(' ').toUpper();
    return QString::fromLatin1(compact);
}

} // namespace ProtocolModel
