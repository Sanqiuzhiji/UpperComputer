#pragma once

#include "services/CommunicationCodec.h"

class CustomBinaryCodec final
    : public CommunicationParser,
      public CustomBinaryEncoder
{
public:
    explicit CustomBinaryCodec(ProtocolDefinition protocol);

    [[nodiscard]] bool parse(
        const QByteArray &data,
        QList<ParsedMessage> *messages,
        QString *errorMessage) const override;

    [[nodiscard]] bool encode(
        const ProtocolDefinition &protocol,
        const MessageDefinition &message,
        const ProtocolFieldValues &values,
        QByteArray *payload,
        QString *errorMessage) const override;

private:
    ProtocolDefinition m_protocol;
    mutable QByteArray m_receiveBuffer;
};
