#pragma once

#include <QList>
#include <QHash>
#include <QWidget>

#include <memory>

#include "models/AppTypes.h"
#include "models/ConnectionTypes.h"
#include "models/ProtocolDefinition.h"

class AppContext;
class CommunicationModePanel;
class CommunicationMonitorPanel;
class CustomBinaryCodec;
class CustomBinaryEncoder;
class HardwareConfigPanel;
class QSplitter;
class SendPanel;

class CommunicationPage final : public QWidget
{
    Q_OBJECT

public:
    explicit CommunicationPage(
        AppContext *context, QWidget *parent = nullptr);

    void setProtocols(const QList<ProtocolDefinition> &protocols);
    void setCustomBinaryEncoder(
        const QString &protocolId,
        std::shared_ptr<const CustomBinaryEncoder> encoder);

signals:
    void receiveModeChanged(ParserMode mode);
    void sendModeChanged(SendMode mode);
    void customProtocolChanged(const QString &protocolId);
    void requestProtocolLibrary();

private:
    void updateReceivePipeline();
    void notify(const QString &message, NotificationType type) const;

    AppContext *m_context{};
    HardwareConfigPanel *m_hardwarePanel{};
    CommunicationModePanel *m_modePanel{};
    CommunicationMonitorPanel *m_monitorPanel{};
    SendPanel *m_sendPanel{};
    QSplitter *m_dataSplitter{};
    QList<ProtocolDefinition> m_protocols;
    QHash<QString, std::shared_ptr<CustomBinaryCodec>> m_customCodecs;
    ConnectionState m_previousState{ConnectionState::Disconnected};
};
