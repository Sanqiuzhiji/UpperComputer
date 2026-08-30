#include "CescConfigurationClient.h"
#include "services/cesc/CescSession.h"

CescConfigurationClient::CescConfigurationClient(CescSession *session, QObject *parent)
    : QObject(parent), m_session(session)
{
    connect(session->transactions(), &CescTransactionManager::completed, this,
            [this](quint16, quint8 service, quint8 command, Cesc::Status status,
                   const QByteArray &data) {
        if (service != quint8(Cesc::Service::Configuration)) return;
        if (status != Cesc::Status::Ok) {
            emit commandFailed(command, tr("Configuration command 0x%1 failed (status %2)")
                .arg(command,2,16,QLatin1Char('0')).arg(quint16(status))); return;
        }
        if (command == 0x01) {
            Configuration c; if (!decode(data,&c)) { emit commandFailed(command,tr("Malformed configuration response")); return; }
            emit configurationReceived(c);
        } else if (command == 0x06) {
            SaveState s; qsizetype p=0;
            if(!Cesc::readU8(data,p,s.source)||!Cesc::readU8(data,p,s.dirty)
               ||!Cesc::readU8(data,p,s.slotAValid)||!Cesc::readU8(data,p,s.slotBValid)
               ||!Cesc::readU32(data,p,s.sequence)||!Cesc::readU8(data,p,s.savePending)
               ||!Cesc::readU8(data,p,s.lastSaveOk)) { emit commandFailed(command,tr("Malformed save state response")); return; }
            emit saveStateReceived(s);
        } else emit commandSucceeded(command);
    });
}

QByteArray CescConfigurationClient::encode(const Configuration &c)
{
    QByteArray b; b.reserve(41); b.append(char(c.polePairs)); Cesc::appendFloat32(b,c.torqueConstant);
    Cesc::appendFloat32(b,c.phaseResistance); Cesc::appendFloat32(b,c.phaseInductance);
    Cesc::appendFloat32(b,c.fluxLinkage); Cesc::appendI32(b,c.maximumIqMa);
    Cesc::appendI32(b,c.maximumSpeedMdps); Cesc::appendI32(b,c.maximumPositionMdeg);
    Cesc::appendU32(b,c.minimumBusVoltageMv); Cesc::appendU32(b,c.maximumBusVoltageMv);
    Cesc::appendU32(b,c.watchdogMs); return b;
}
bool CescConfigurationClient::decode(const QByteArray &b, Configuration *c)
{
    if(!c || b.size()!=41) return false; qsizetype p=0;
    return Cesc::readU8(b,p,c->polePairs)&&Cesc::readFloat32(b,p,c->torqueConstant)
      &&Cesc::readFloat32(b,p,c->phaseResistance)&&Cesc::readFloat32(b,p,c->phaseInductance)
      &&Cesc::readFloat32(b,p,c->fluxLinkage)&&Cesc::readI32(b,p,c->maximumIqMa)
      &&Cesc::readI32(b,p,c->maximumSpeedMdps)&&Cesc::readI32(b,p,c->maximumPositionMdeg)
      &&Cesc::readU32(b,p,c->minimumBusVoltageMv)&&Cesc::readU32(b,p,c->maximumBusVoltageMv)
      &&Cesc::readU32(b,p,c->watchdogMs)&&p==b.size();
}
void CescConfigurationClient::request(quint8 c,const QByteArray&p){m_session->transactions()->request(quint8(Cesc::Service::Configuration),c,p);}
void CescConfigurationClient::readConfiguration(){request(0x01);} void CescConfigurationClient::writeRam(const Configuration&c){request(0x02,encode(c));}
void CescConfigurationClient::saveFlash(){request(0x03);} void CescConfigurationClient::reloadFlash(){request(0x04);}
void CescConfigurationClient::restoreDefaults(){request(0x05);} void CescConfigurationClient::requestSaveState(){request(0x06);}
