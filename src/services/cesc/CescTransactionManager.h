#pragma once

#include "services/cesc/CescProtocolTypes.h"
#include <QHash>
#include <QObject>

class QTimer;

class CescTransactionManager final : public QObject
{
    Q_OBJECT
public:
    explicit CescTransactionManager(QObject *parent=nullptr);
    quint16 request(quint8 serviceId, quint8 commandId, const QByteArray &payload,
                    int timeoutMs=1000, int retries=2);
    bool handleResponse(const Cesc::Packet &packet);
    void cancelAll(const QString &reason);
    [[nodiscard]] int outstandingCount() const noexcept;
signals:
    void sendPacket(const Cesc::Packet &packet);
    void completed(quint16 sequence, quint8 serviceId, quint8 commandId,
                   Cesc::Status status, const QByteArray &payload);
    void failed(quint16 sequence, const QString &reason);
    void retried(quint16 sequence, int attempt);
    void unmatchedResponse(const Cesc::Packet &packet);
private:
    struct Pending { Cesc::Packet packet; QTimer *timer{}; int timeoutMs{}; int retriesLeft{}; int attempt{}; };
    quint16 allocateSequence();
    void timeout(quint16 sequence);
    QHash<quint16, Pending> m_pending;
    quint16 m_nextSequence{};
};
