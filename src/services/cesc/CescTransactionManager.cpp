#include "CescTransactionManager.h"
#include <QTimer>

CescTransactionManager::CescTransactionManager(QObject *parent):QObject(parent){}

quint16 CescTransactionManager::allocateSequence()
{
    do { ++m_nextSequence; if (!m_nextSequence) ++m_nextSequence; } while(m_pending.contains(m_nextSequence));
    return m_nextSequence;
}

quint16 CescTransactionManager::request(quint8 serviceId, quint8 commandId,
                                        const QByteArray &payload, int timeoutMs, int retries)
{
    if (m_pending.size() >= 8) return 0;
    const quint16 sequence=allocateSequence();
    Pending p; p.packet={Cesc::Version,Cesc::MessageType::Request,serviceId,commandId,sequence,payload};
    p.timeoutMs=timeoutMs; p.retriesLeft=qMax(0,retries); p.timer=new QTimer(this); p.timer->setSingleShot(true);
    connect(p.timer,&QTimer::timeout,this,[this,sequence]{timeout(sequence);});
    m_pending.insert(sequence,p); m_pending[sequence].timer->start(timeoutMs); emit sendPacket(p.packet); return sequence;
}

bool CescTransactionManager::handleResponse(const Cesc::Packet &packet)
{
    auto it=m_pending.find(packet.sequence);
    if (packet.messageType!=Cesc::MessageType::Response || it==m_pending.end()
        || it->packet.serviceId!=packet.serviceId || it->packet.commandId!=packet.commandId) {
        emit unmatchedResponse(packet); return false;
    }
    quint16 rawStatus; qsizetype pos=0;
    if(!Cesc::readU16(packet.payload,pos,rawStatus)) { emit failed(packet.sequence,tr("Malformed response status")); }
    else emit completed(packet.sequence,packet.serviceId,packet.commandId,Cesc::Status(rawStatus),packet.payload.mid(pos));
    it->timer->stop(); it->timer->deleteLater(); m_pending.erase(it); return true;
}

void CescTransactionManager::timeout(quint16 sequence)
{
    auto it=m_pending.find(sequence); if(it==m_pending.end()) return;
    if(it->retriesLeft-- > 0) { ++it->attempt; emit retried(sequence,it->attempt); emit sendPacket(it->packet); it->timer->start(it->timeoutMs); return; }
    auto *timer=it->timer; m_pending.erase(it); timer->deleteLater(); emit failed(sequence,tr("Request timed out"));
}

void CescTransactionManager::cancelAll(const QString &reason)
{
    const auto keys=m_pending.keys();
    for(quint16 key:keys){ auto p=m_pending.take(key); p.timer->stop(); p.timer->deleteLater(); emit failed(key,reason); }
}
int CescTransactionManager::outstandingCount() const noexcept{return m_pending.size();}
