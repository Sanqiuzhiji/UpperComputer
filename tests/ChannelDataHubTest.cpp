#include "services/ChannelDataHub.h"

#include <QCoreApplication>

#include <cmath>
#include <limits>

namespace {
ParsedMessage message(
    const QString &messageId,
    const QList<ParsedField> &fields)
{
    return {messageId, messageId, fields};
}

ParsedField field(
    const QString &id,
    const QVariant &value,
    const ProtocolFieldRole role = ProtocolFieldRole::Value)
{
    return {id, id, value, QStringLiteral("V"), role};
}
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    ChannelDataHub hub(nullptr, 3);
    hub.publish(10, QStringLiteral("protocol"), {
        message(QStringLiteral("message"), {
            field(QStringLiteral("voltage"), 1.0),
            field(QStringLiteral("constant"), 2.0,
                  ProtocolFieldRole::Constant),
            field(QStringLiteral("checksum"), 3.0,
                  ProtocolFieldRole::Checksum),
            field(QStringLiteral("text"), QStringLiteral("not-a-number"))
        })
    });
    const QString voltageId = ChannelDataHub::channelId(
        QStringLiteral("protocol"),
        QStringLiteral("message"),
        QStringLiteral("voltage"));
    if (!hub.containsChannel(voltageId) || hub.channels().size() != 1) return 1;
    hub.publish(20, QStringLiteral("protocol"), {
        message(QStringLiteral("message"), {
            field(QStringLiteral("voltage"),
                  std::numeric_limits<double>::quiet_NaN()),
            field(QStringLiteral("current"), 4.0)
        })
    });
    hub.publish(30, QStringLiteral("protocol"), {
        message(QStringLiteral("message"), {
            field(QStringLiteral("voltage"),
                  std::numeric_limits<double>::infinity())
        })
    });
    hub.publish(40, QStringLiteral("protocol"), {
        message(QStringLiteral("message"), {
            field(QStringLiteral("voltage"), 5.0)
        })
    });
    hub.publish(50, QStringLiteral("protocol"), {
        message(QStringLiteral("message"), {
            field(QStringLiteral("voltage"), 6.0)
        })
    });
    hub.publish(60, QStringLiteral("protocol"), {
        message(QStringLiteral("message"), {
            field(QStringLiteral("voltage"), 7.0)
        })
    });
    const QVector<ChannelSample> bounded =
        hub.snapshot(voltageId, 0, 100);
    if (bounded.size() != 3
        || bounded.constFirst().timestampUs != 40
        || bounded.constLast().timestampUs != 60) {
        return 2;
    }
    const QVector<ChannelSample> ranged =
        hub.snapshot(voltageId, 45, 55);
    if (ranged.size() != 1 || ranged.constFirst().timestampUs != 50) return 3;

    ChannelDataHub reordered;
    reordered.publish(1, QStringLiteral("protocol"), {
        message(QStringLiteral("message"), {
            field(QStringLiteral("current"), 1.0),
            field(QStringLiteral("voltage"), 2.0)
        })
    });
    if (!reordered.containsChannel(voltageId)
        || !reordered.containsChannel(ChannelDataHub::channelId(
            QStringLiteral("protocol"),
            QStringLiteral("message"),
            QStringLiteral("current")))) {
        return 4;
    }
    reordered.publish(2, QStringLiteral("other-protocol"), {
        message(QStringLiteral("other-message"), {
            field(QStringLiteral("voltage"), 9.0)
        })
    });
    if (reordered.channels().size() != 3) return 5;
    reordered.retainChannels(QSet<QString>{voltageId});
    if (reordered.channels().size() != 1
        || !reordered.containsChannel(voltageId)) {
        return 6;
    }
    return 0;
}
