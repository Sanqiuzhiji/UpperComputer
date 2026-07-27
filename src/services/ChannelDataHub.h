#pragma once

#include <QHash>
#include <QObject>
#include <QReadWriteLock>
#include <QSet>

#include "models/ProtocolDefinition.h"
#include "models/TelemetryTypes.h"

class ChannelDataHub final : public QObject
{
    Q_OBJECT

public:
    explicit ChannelDataHub(
        QObject *parent = nullptr,
        qsizetype maximumSamplesPerChannel = 200000);

    [[nodiscard]] QList<ChannelDescriptor> channels() const;
    [[nodiscard]] bool containsChannel(const QString &channelId) const;
    [[nodiscard]] qint64 latestTimestamp(const QString &channelId) const;
    [[nodiscard]] QVector<ChannelSample> snapshot(
        const QString &channelId,
        qint64 minimumTimestampUs,
        qint64 maximumTimestampUs) const;
    [[nodiscard]] static QString channelId(
        const QString &protocolId,
        const QString &messageId,
        const QString &fieldId);

public slots:
    void clear();
    void publish(
        qint64 timestampUs,
        const QString &protocolId,
        const QList<ParsedMessage> &messages);
    void retainChannels(const QSet<QString> &validChannelIds);

signals:
    void channelRegistryChanged();
    void samplesAppended(const QStringList &channelIds);

private:
    struct ChannelRecord {
        ChannelDescriptor descriptor;
        QVector<ChannelSample> samples;
        qsizetype firstSample{};
    };

    mutable QReadWriteLock m_lock;
    QHash<QString, ChannelRecord> m_records;
    qsizetype m_maximumSamplesPerChannel;
};
