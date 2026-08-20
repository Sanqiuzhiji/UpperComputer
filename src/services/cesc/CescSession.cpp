#include "CescSession.h"
#include "services/ConnectionManager.h"

CescSession::CescSession(ConnectionManager *connection,QObject *parent)
    :QObject(parent),m_connection(connection),m_transactions(this)
{
    connect(connection,&ConnectionManager::stateChanged,this,[this](ConnectionState s){transportStateChanged(int(s));});
    connect(connection,&ConnectionManager::dataReceived,this,[this](const QByteArray &data){
        if(!m_enabled||m_state==State::Disconnected)return;
        for(const auto &packet:m_codec.feed(data)){
            if(packet.messageType==Cesc::MessageType::Response)m_transactions.handleResponse(packet);
            else emit packetReceived(packet);
        }
    });
    connect(&m_transactions,&CescTransactionManager::sendPacket,this,[this](const Cesc::Packet &p){
        QString error; if(!m_connection->send(CescPacketCodec::encode(p),&error,CommunicationTrafficSource::CescNative)) emit protocolError(error);
    });
    connect(&m_transactions,&CescTransactionManager::completed,this,[this](quint16,quint8 service,quint8 command,Cesc::Status status,const QByteArray &payload){
        if(service!=0||command!=0||m_state!=State::Negotiating)return;
        if(status!=Cesc::Status::Ok){setState(State::Error);emit protocolError(tr("CESC HELLO rejected"));return;}
        qsizetype p=0; quint8 version; quint16 maximum; quint64 caps; quint32 session;
        if(!Cesc::readU8(payload,p,version)||!Cesc::readU16(payload,p,maximum)||!Cesc::readU64(payload,p,caps)||!Cesc::readU32(payload,p,session)||version!=1||!maximum){setState(State::Error);emit protocolError(tr("Malformed CESC HELLO response"));return;}
        const quint32 old=m_sessionId; m_maximumPayload=qMin(maximum,Cesc::AbsoluteMaximumPayload);m_codec.setMaximumPayload(m_maximumPayload);m_capabilities=caps;m_sessionId=session;
        if(old&&old!=session){m_transactions.cancelAll(tr("Device session changed"));emit sessionChanged(old,session);} setState(State::Ready);emit identityChanged();
    });
    connect(&m_transactions,&CescTransactionManager::failed,this,[this](quint16, const QString &reason){if(m_state==State::Negotiating){setState(State::Error);emit protocolError(reason);}});
}

void CescSession::transportStateChanged(int raw)
{
    const auto s=ConnectionState(raw);
    if(s==ConnectionState::Connected){setState(State::TransportOpen);if(m_enabled)hello();}
    else if(s==ConnectionState::Disconnected||s==ConnectionState::Error){m_codec.reset();m_transactions.cancelAll(tr("Transport disconnected"));m_maximumPayload=Cesc::AbsoluteMaximumPayload;m_capabilities=0;m_sessionId=0;setState(State::Disconnected);emit identityChanged();}
}
void CescSession::hello(){setState(State::Negotiating);QByteArray p;p.append(char(1));p.append(char(1));Cesc::appendU32(p,0);m_transactions.request(0,0,p,1000,2);}
void CescSession::setState(State s){if(m_state==s)return;m_state=s;emit stateChanged(s);}
CescSession::State CescSession::state()const noexcept{return m_state;} bool CescSession::isReady()const noexcept{return m_state==State::Ready;}
quint16 CescSession::maximumPayload()const noexcept{return m_maximumPayload;} quint64 CescSession::capabilities()const noexcept{return m_capabilities;} quint32 CescSession::sessionId()const noexcept{return m_sessionId;}
CescTransactionManager *CescSession::transactions()noexcept{return &m_transactions;} Cesc::CodecDiagnostics CescSession::diagnostics()const noexcept{return m_codec.diagnostics();}
void CescSession::setEnabled(bool enabled){if(m_enabled==enabled)return;m_enabled=enabled;m_connection->setCescNativeActive(enabled);m_codec.reset();m_transactions.cancelAll(tr("CESC mode changed"));if(enabled&&m_connection->state()==ConnectionState::Connected)hello();else if(!enabled)setState(m_connection->state()==ConnectionState::Connected?State::TransportOpen:State::Disconnected);}
bool CescSession::isEnabled()const noexcept{return m_enabled;}
