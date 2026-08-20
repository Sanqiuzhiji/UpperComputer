#include "ChannelDataHub.h"

#include <QReadLocker>
#include <QWriteLocker>

#include <algorithm>
#include <cmath>

ChannelDataHub::ChannelDataHub(
    QObject *parent, const qsizetype maximumSamplesPerChannel)
    : QObject(parent),
      m_maximumSamplesPerChannel(qMax<qsizetype>(
          1, maximumSamplesPerChannel))
{
}

QList<ChannelDescriptor> ChannelDataHub::channels() const
{
    QReadLocker locker(&m_lock);
    QList<ChannelDescriptor> result;
    result.reserve(m_records.size());
    for (auto it = m_records.cbegin(); it != m_records.cend(); ++it) {
        result.append(it->descriptor);
    }
    std::ranges::sort(
        result, [](const ChannelDescriptor &left,
                   const ChannelDescriptor &right) {
            return left.displayName.localeAwareCompare(right.displayName) < 0;
        });
    return result;
}

bool ChannelDataHub::containsChannel(const QString &channelId) const
{
    QReadLocker locker(&m_lock);
    return m_records.contains(channelId);
}

qint64 ChannelDataHub::latestTimestamp(const QString &channelId) const
{
    QReadLocker locker(&m_lock);
    const auto record = m_records.constFind(channelId);
    if (record == m_records.cend() || record->samples.isEmpty()) return 0;
    const qsizetype last = (
        record->firstSample + record->samples.size() - 1)
        % record->samples.size();
    return record->samples.at(last).timestampUs;
}

QVector<ChannelSample> ChannelDataHub::snapshot(
    const QString &channelId,
    const qint64 minimumTimestampUs,
    const qint64 maximumTimestampUs) const
{
    if (minimumTimestampUs > maximumTimestampUs) return {};
    QReadLocker locker(&m_lock);
    const auto record = m_records.constFind(channelId);
    if (record == m_records.cend()) return {};
    const QVector<ChannelSample> &samples = record->samples;
    const auto sampleAt = [&record, &samples](const qsizetype index)
        -> const ChannelSample & {
        return samples.at(
            (record->firstSample + index) % samples.size());
    };
    qsizetype lower = 0;
    qsizetype upper = samples.size();
    while (lower < upper) {
        const qsizetype middle = lower + (upper - lower) / 2;
        if (sampleAt(middle).timestampUs < minimumTimestampUs) {
            lower = middle + 1;
        } else {
            upper = middle;
        }
    }
    const qsizetype first = lower;
    upper = samples.size();
    while (lower < upper) {
        const qsizetype middle = lower + (upper - lower) / 2;
        if (sampleAt(middle).timestampUs <= maximumTimestampUs) {
            lower = middle + 1;
        } else {
            upper = middle;
        }
    }
    QVector<ChannelSample> result;
    result.reserve(lower - first);
    for (qsizetype index = first; index < lower; ++index) {
        result.append(sampleAt(index));
    }
    return result;
}

QString ChannelDataHub::channelId(
    const QString &protocolId,
    const QString &messageId,
    const QString &fieldId)
{
    return QStringLiteral("%1/%2/%3")
        .arg(protocolId, messageId, fieldId);
}

void ChannelDataHub::clear()
{
    bool changed = false;
    {
        QWriteLocker locker(&m_lock);
        changed = !m_records.isEmpty();
        m_records.clear();
    }
    if (changed) emit channelRegistryChanged();
}

void ChannelDataHub::publish(
    const qint64 timestampUs,
    const QString &protocolId,
    const QList<ParsedMessage> &messages)
{
    QStringList appended;
    bool registryChanged = false;
    {
        QWriteLocker locker(&m_lock);
        for (const ParsedMessage &message : messages) {
            if (message.messageId.trimmed().isEmpty()) continue;
            for (const ParsedField &field : message.fields) {
                if (field.role != ProtocolFieldRole::Value
                    || field.fieldId.trimmed().isEmpty()) {
                    continue;
                }
                bool ok = false;
                const double value = field.value.toDouble(&ok);
                if (!ok || !std::isfinite(value)) continue;
                const QString id = channelId(
                    protocolId, message.messageId, field.fieldId);
                auto record = m_records.find(id);
                if (record == m_records.end()) {
                    ChannelRecord created;
                    created.descriptor = {
                        id,
                        protocolId,
                        message.messageId,
                        field.fieldId,
                        field.displayName.trimmed().isEmpty()
                            ? field.fieldId : field.displayName,
                        field.unit
                    };
                    record = m_records.insert(id, std::move(created));
                    registryChanged = true;
                } else {
                    const QString displayName =
                        field.displayName.trimmed().isEmpty()
                        ? field.fieldId : field.displayName;
                    if (record->descriptor.displayName != displayName
                        || record->descriptor.unit != field.unit) {
                        record->descriptor.displayName = displayName;
                        record->descriptor.unit = field.unit;
                        registryChanged = true;
                    }
                }
                if (record->samples.size()
                    < m_maximumSamplesPerChannel) {
                    record->samples.append({timestampUs, value});
                } else {
                    record->samples[record->firstSample] =
                        {timestampUs, value};
                    record->firstSample =
                        (record->firstSample + 1)
                        % m_maximumSamplesPerChannel;
                }
                appended.append(id);
            }
        }
    }
    appended.removeDuplicates();
    if (registryChanged) emit channelRegistryChanged();
    if (!appended.isEmpty()) emit samplesAppended(appended);
}

void ChannelDataHub::retainChannels(
    const QSet<QString> &validChannelIds)
{
    bool changed = false;
    {
        QWriteLocker locker(&m_lock);
        for (auto record = m_records.begin();
             record != m_records.end();) {
            if (record->descriptor.protocolId == QStringLiteral("cesc-v1")
                || validChannelIds.contains(record.key())) {
                ++record;
            } else {
                record = m_records.erase(record);
                changed = true;
            }
        }
    }
    if (changed) emit channelRegistryChanged();
}
