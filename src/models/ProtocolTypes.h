#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QUuid>
#include <QVector>

namespace ProtocolModel {

inline constexpr int CurrentSchemaVersion = 1;

enum class FieldRole {
    Header,
    FrameId,
    Length,
    Data,
    Checksum,
    Tail,
    Skip
};

enum class DataType {
    UInt8,
    Int8,
    UInt16,
    Int16,
    UInt32,
    Int32,
    UInt64,
    Int64,
    Float32,
    Float64,
    Bytes
};

enum class ByteOrder {
    LittleEndian,
    BigEndian
};

enum class ChecksumAlgorithm {
    Sum8,
    Xor8,
    Crc8
};

struct Field {
    QUuid id;
    QString name;
    FieldRole role{FieldRole::Data};
    DataType dataType{DataType::UInt8};
    int byteCount{1};
    ByteOrder byteOrder{ByteOrder::LittleEndian};
    double scale{1.0};
    double offset{0.0};
    QByteArray fixedBytes;
    ChecksumAlgorithm checksumAlgorithm{ChecksumAlgorithm::Sum8};

    bool operator==(const Field &) const = default;
};

struct Frame {
    QUuid id;
    QString name;
    QVector<Field> fields;

    bool operator==(const Frame &) const = default;
};

struct Document {
    int schemaVersion{CurrentSchemaVersion};
    QUuid id;
    QString name;
    QVector<Frame> frames;

    bool operator==(const Document &) const = default;
};

enum class ValidationSeverity {
    Warning,
    Error
};

struct ValidationIssue {
    ValidationSeverity severity{ValidationSeverity::Error};
    QString message;
    QUuid frameId;
    QUuid fieldId;
};

[[nodiscard]] Field makeField(FieldRole role);
[[nodiscard]] Frame makeFrame(const QString &name = QStringLiteral("Frame1"));
[[nodiscard]] Document makeDocument(const QString &name);
[[nodiscard]] Field duplicatedField(const Field &source);
[[nodiscard]] Frame duplicatedFrame(const Frame &source);

[[nodiscard]] int frameByteCount(const Frame &frame);
[[nodiscard]] int documentByteCount(const Document &document);
[[nodiscard]] int naturalByteCount(DataType type);
[[nodiscard]] QVector<int> fieldOffsets(const Frame &frame);

[[nodiscard]] QString roleKey(FieldRole role);
[[nodiscard]] QString roleDisplayName(FieldRole role);
[[nodiscard]] QString dataTypeKey(DataType type);
[[nodiscard]] QString dataTypeDisplayName(DataType type);
[[nodiscard]] QString byteOrderKey(ByteOrder order);
[[nodiscard]] QString checksumKey(ChecksumAlgorithm algorithm);
[[nodiscard]] QString fieldRoleColor(FieldRole role);

[[nodiscard]] QJsonObject toJson(const Document &document);
[[nodiscard]] bool fromJson(
    const QJsonObject &object, Document *document, QString *errorMessage);
[[nodiscard]] QVector<ValidationIssue> validate(const Document &document);
[[nodiscard]] bool hasValidationErrors(
    const QVector<ValidationIssue> &issues);

[[nodiscard]] QByteArray parseHex(
    const QString &text, bool *ok = nullptr);
[[nodiscard]] QString formatHex(const QByteArray &bytes);

} // namespace ProtocolModel

Q_DECLARE_METATYPE(ProtocolModel::FieldRole)
Q_DECLARE_METATYPE(ProtocolModel::DataType)
Q_DECLARE_METATYPE(ProtocolModel::ByteOrder)
Q_DECLARE_METATYPE(ProtocolModel::ChecksumAlgorithm)
