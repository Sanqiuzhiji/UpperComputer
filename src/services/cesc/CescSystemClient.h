#pragma once
#include "services/cesc/CescProtocolTypes.h"
#include <QObject>
class CescSession;
class CescSystemClient final:public QObject{
 Q_OBJECT
public:
 struct DeviceInfo{quint16 firmwareMajor{},firmwareMinor{},firmwarePatch{},bootloaderMajor{},bootloaderMinor{},bootloaderPatch{};QByteArray uuid;QString hardwareName,buildIdentifier;};
 explicit CescSystemClient(CescSession *session,QObject *parent=nullptr);
 void getDeviceInfo(); void ping(quint32 token); void getCapabilities(); void getCommunicationStats(); void reset(quint8 mode=0,quint16 delayMs=100);
 [[nodiscard]] DeviceInfo deviceInfo()const{return m_info;}
signals:void deviceInfoChanged();void pingReceived(quint32 token,quint32 uptimeMs);void commandFailed(const QString &message);void communicationStatsReceived(const QList<quint32>&stats);
private:CescSession *m_session{};DeviceInfo m_info;
};
