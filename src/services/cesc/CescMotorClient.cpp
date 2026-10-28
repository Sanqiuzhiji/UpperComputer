#include "CescMotorClient.h"
#include "services/cesc/CescSession.h"

CescMotorClient::CescMotorClient(CescSession *session, QObject *parent)
    : QObject(parent), m_session(session)
{
    connect(session->transactions(), &CescTransactionManager::completed, this,
            [this](quint16, quint8 service, quint8 command, Cesc::Status status,
                   const QByteArray &data) {
        if (service != quint8(Cesc::Service::Motor)) return;
        if (status != Cesc::Status::Ok) {
            emit commandFailed(command, tr("Motor command 0x%1 failed (status %2)")
                .arg(command, 2, 16, QLatin1Char('0')).arg(quint16(status)));
            return;
        }
        if (command != 0x0f) { emit commandSucceeded(command); return; }
        State s;
        if (!Cesc::parseMotorState(data, s)) {
            emit commandFailed(command, tr("Malformed motor state response")); return;
        }
        emit stateReceived(s);
    });
}

void CescMotorClient::request(quint8 command, const QByteArray &payload)
{ m_session->transactions()->request(quint8(Cesc::Service::Motor), command, payload); }
void CescMotorClient::stop(){ request(0x02); }
void CescMotorClient::alignEncoder(){ request(0x03); }
void CescMotorClient::setIq(qint32 v){ QByteArray b; Cesc::appendI32(b,v); request(0x09,b); }
void CescMotorClient::setSpeed(qint32 v){ QByteArray b; Cesc::appendI32(b,v); request(0x0a,b); }
void CescMotorClient::setPosition(qint32 v){ QByteArray b; Cesc::appendI32(b,v); request(0x0b,b); }
void CescMotorClient::setTrajectory(qint32 p,qint32 v,qint32 a,qint32 d){ QByteArray b; Cesc::appendI32(b,p);Cesc::appendI32(b,v);Cesc::appendI32(b,a);Cesc::appendI32(b,d);request(0x0c,b); }
void CescMotorClient::setTorque(qint32 v){ QByteArray b; Cesc::appendI32(b,v); request(0x0e,b); }
void CescMotorClient::requestState(){ request(0x0f); }
