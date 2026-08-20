#pragma once
#include <QObject>
class CescSession;
class CescSensorClient final:public QObject
{
    Q_OBJECT
public:
    struct Sample{quint8 sensorId{},type{},status{};quint16 rawAngle{};float degrees{};quint64 timestampUs{};};
    explicit CescSensorClient(CescSession*,QObject *parent=nullptr);
    void enumerate();void getSample(quint8 id=0);void getStatus(quint8 id=0);
signals:
    void sampleReceived(const CescSensorClient::Sample&);void sensorsEnumerated(const QStringList&);void commandFailed(const QString&);
private:
    CescSession*m_session{};
};
Q_DECLARE_METATYPE(CescSensorClient::Sample)
