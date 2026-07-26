#pragma once

#include <QList>
#include <QWidget>

#include <memory>

#include "models/AppTypes.h"
#include "models/ConnectionTypes.h"
#include "models/ProtocolDefinition.h"

class AppContext;
class CommunicationModePanel;
class CommunicationMonitorPanel;
class CommunicationParser;
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
    void setReceiveParser(
        ParserMode mode,
        std::shared_ptr<const CommunicationParser> parser);
    void setCustomBinaryEncoder(
        const QString &protocolId,
        std::shared_ptr<const CustomBinaryEncoder> encoder);

signals:
    void receiveModeChanged(ParserMode mode);
    void sendModeChanged(SendMode mode);
    void customProtocolChanged(const QString &protocolId);
    void requestProtocolLibrary();

private:
    void notify(const QString &message, NotificationType type) const;

    AppContext *m_context{};
    HardwareConfigPanel *m_hardwarePanel{};
    CommunicationModePanel *m_modePanel{};
    CommunicationMonitorPanel *m_monitorPanel{};
    SendPanel *m_sendPanel{};
    QSplitter *m_dataSplitter{};
    ConnectionState m_previousState{ConnectionState::Disconnected};
};

