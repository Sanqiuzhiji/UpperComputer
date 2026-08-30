#pragma once

#include "services/cesc/CescProtocolTypes.h"
#include <QObject>

class CescSession;

class CescConfigurationClient final : public QObject
{
    Q_OBJECT
public:
    struct Configuration {
        quint8 polePairs{11}; float torqueConstant{0.23F}, phaseResistance{2.2F};
        float phaseInductance{0.00116F}, fluxLinkage{0.023F};
        qint32 maximumIqMa{300}, maximumSpeedMdps{3000000}, maximumPositionMdeg{3600000};
        quint32 minimumBusVoltageMv{6000}, maximumBusVoltageMv{15000}, watchdogMs{1000};
    };
    struct SaveState {
        quint8 source{}, dirty{}, slotAValid{}, slotBValid{}, savePending{}, lastSaveOk{};
        quint32 sequence{};
    };
    explicit CescConfigurationClient(CescSession *session, QObject *parent = nullptr);
    static QByteArray encode(const Configuration &configuration);
    static bool decode(const QByteArray &data, Configuration *configuration);
    void readConfiguration(); void writeRam(const Configuration &configuration);
    void saveFlash(); void reloadFlash(); void restoreDefaults(); void requestSaveState();
signals:
    void configurationReceived(const CescConfigurationClient::Configuration &configuration);
    void saveStateReceived(const CescConfigurationClient::SaveState &state);
    void commandSucceeded(quint8 command);
    void commandFailed(quint8 command, const QString &message);
private:
    void request(quint8 command, const QByteArray &payload = {});
    CescSession *m_session{};
};

Q_DECLARE_METATYPE(CescConfigurationClient::Configuration)
Q_DECLARE_METATYPE(CescConfigurationClient::SaveState)
