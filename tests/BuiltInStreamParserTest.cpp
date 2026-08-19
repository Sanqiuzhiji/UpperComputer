#include "services/BuiltInStreamParser.h"

#include <QCoreApplication>
#include <QtEndian>

#include <bit>
#include <cmath>

namespace {
QByteArray floatBytes(const float value)
{
    const quint32 littleEndian = qToLittleEndian(std::bit_cast<quint32>(value));
    return QByteArray(reinterpret_cast<const char *>(&littleEndian), sizeof(littleEndian));
}

bool closeTo(const QVariant &actual, const double expected)
{
    return std::abs(actual.toDouble() - expected) < 0.00001;
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    BuiltInStreamParser fireWater(ParserMode::FireWater);
    QList<ParsedMessage> messages;
    QString error;
    if (!fireWater.parse("samples: 1.1, 3.2", &messages, &error)
        || !messages.isEmpty()) return 1;
    if (!fireWater.parse(", -0.6, -0.9\r\n2,4\n", &messages, &error)
        || messages.size() != 2
        || messages.at(0).fields.size() != 4
        || !closeTo(messages.at(0).fields.at(2).value, -0.6)
        || !closeTo(messages.at(1).fields.at(1).value, 4.0)) return 2;
    messages.clear();
    if (fireWater.parse("1,nope\n", &messages, &error)
        || !messages.isEmpty() || error.isEmpty()) return 3;

    BuiltInStreamParser justFloat(ParserMode::JustFloat);
    QByteArray frame = floatBytes(1.0F) + floatBytes(-2.5F)
        + QByteArray("\x00\x00\x80\x7f", 4);
    if (!justFloat.parse(frame.left(5), &messages, &error)
        || !messages.isEmpty()) return 4;
    if (!justFloat.parse(frame.mid(5) + floatBytes(7.25F)
                           + QByteArray("\x00\x00\x80\x7f", 4),
                         &messages, &error)
        || messages.size() != 2
        || messages.at(0).fields.size() != 2
        || !closeTo(messages.at(0).fields.at(1).value, -2.5)
        || !closeTo(messages.at(1).fields.at(0).value, 7.25)) return 5;

    messages.clear();
    if (justFloat.parse(QByteArray("x\x00\x00\x80\x7f", 5),
                        &messages, &error)
        || !messages.isEmpty() || error.isEmpty()) return 6;
    return 0;
}
