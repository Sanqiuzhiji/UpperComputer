#pragma once

#include "services/cesc/CescMotorTypes.h"
#include <QObject>

class CescSession;

class CescMotorClient final : public QObject
{
    Q_OBJECT
public:
    using State = Cesc::MotorState;
    explicit CescMotorClient(CescSession *session, QObject *parent = nullptr);
    void stop();
    void alignEncoder();
    void setIq(qint32 milliAmps);
    void setSpeed(qint32 milliDegreesPerSecond);
    void setPosition(qint32 milliDegrees);
    void setTrajectory(qint32 positionMdeg, qint32 maxSpeedMdps,
                       qint32 accelerationMdps2, qint32 decelerationMdps2);
    void setTorque(qint32 milliNewtonMetres);
    void requestState();
signals:
    void stateReceived(const CescMotorClient::State &state);
    void commandSucceeded(quint8 command);
    void commandFailed(quint8 command, const QString &message);
private:
    void request(quint8 command, const QByteArray &payload = {});
    CescSession *m_session{};
};

Q_DECLARE_METATYPE(CescMotorClient::State)
