#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

#include "models/ConnectionTypes.h"
#include "models/ProtocolDefinition.h"

class BuiltInStreamParser final
{
public:
    explicit BuiltInStreamParser(ParserMode mode);
    [[nodiscard]] QString protocolId() const;
    bool parse(const QByteArray &data, QList<ParsedMessage> *messages,
               QString *errorMessage = nullptr);

private:
    bool parseFireWater(QList<ParsedMessage> *messages, QString *errorMessage);
    bool parseJustFloat(QList<ParsedMessage> *messages, QString *errorMessage);
    [[nodiscard]] ParsedMessage makeMessage(const QList<float> &values) const;

    ParserMode m_mode;
    QByteArray m_buffer;
};
