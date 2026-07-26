#pragma once

#include "models/ProtocolDefinition.h"

class CommunicationParser
{
public:
    virtual ~CommunicationParser() = default;

    [[nodiscard]] virtual bool parse(
        const QByteArray &data,
        QList<ParsedMessage> *messages,
        QString *errorMessage) const = 0;
};

class CustomBinaryEncoder
{
public:
    virtual ~CustomBinaryEncoder() = default;

    [[nodiscard]] virtual bool encode(
        const ProtocolDefinition &protocol,
        const MessageDefinition &message,
        const ProtocolFieldValues &values,
        QByteArray *payload,
        QString *errorMessage) const = 0;
};

