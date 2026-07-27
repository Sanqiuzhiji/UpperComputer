#pragma once

#include <QList>
#include <QString>
#include <QVector>

struct ChannelDescriptor
{
    QString id;
    QString protocolId;
    QString messageId;
    QString fieldId;
    QString displayName;
    QString unit;
};

struct ChannelSample
{
    qint64 timestampUs{};
    double value{};
};

Q_DECLARE_METATYPE(ChannelDescriptor)
Q_DECLARE_METATYPE(ChannelSample)
Q_DECLARE_METATYPE(QList<ChannelDescriptor>)
Q_DECLARE_METATYPE(QVector<ChannelSample>)
