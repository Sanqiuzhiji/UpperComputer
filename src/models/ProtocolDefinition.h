#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>
#include <QVariant>

enum class ProtocolFieldType {
    Int,
    UInt,
    Float,
    Double,
    Bool,
    Enum,
    String,
    ByteArray
};

enum class ProtocolFieldRole {
    Value,
    Constant,
    FrameHeader,
    Length,
    MessageId,
    Checksum
};

enum class ProtocolChecksumAlgorithm {
    Sum8,
    Xor8,
    Crc8
};

struct EnumOption {
    QString displayName;
    QVariant value;
};

struct FieldDefinition {
    QString id;
    QString displayName;
    ProtocolFieldType type{ProtocolFieldType::String};
    int bitWidth{32};
    QString unit;
    QVariant minimum;
    QVariant maximum;
    QVariant defaultValue;
    QList<EnumOption> enumOptions;
    bool editable{true};
    QVariant fixedValue;
    ProtocolFieldRole role{ProtocolFieldRole::Value};
    QString description;
    bool littleEndian{true};
    double scale{1.0};
    double offset{0.0};
    ProtocolChecksumAlgorithm checksumAlgorithm{
        ProtocolChecksumAlgorithm::Sum8};
};

struct MessageDefinition {
    QString id;
    QString displayName;
    QList<FieldDefinition> fields;
};

struct ProtocolDefinition {
    QString id;
    QString displayName;
    QList<MessageDefinition> receiveMessages;
    QList<MessageDefinition> sendMessages;
};

struct ParsedField {
    QString displayName;
    QVariant value;
    QString unit;
    ProtocolFieldRole role{ProtocolFieldRole::Value};
};

struct ParsedMessage {
    QString displayName;
    QList<ParsedField> fields;
};

using ProtocolFieldValues = QHash<QString, QVariant>;

Q_DECLARE_METATYPE(ProtocolFieldType)
Q_DECLARE_METATYPE(ProtocolFieldRole)
Q_DECLARE_METATYPE(ProtocolChecksumAlgorithm)
