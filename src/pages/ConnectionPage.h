#pragma once

#include <QWidget>

#include "models/AppTypes.h"
#include "models/ConnectionTypes.h"

class AppContext;
class HardwareConfigPanel;
class ParserConfigPanel;
class SendPanel;
class TerminalPanel;

class ConnectionPage final : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectionPage(AppContext *context, QWidget *parent = nullptr);

signals:
    void parserModeChanged(ParserMode mode);
    void customProtocolChanged(const QString &protocolId);
    void requestProtocolLibrary();

private:
    void notify(const QString &message, NotificationType type) const;

    AppContext *m_context{};
    HardwareConfigPanel *m_hardwarePanel{};
    ParserConfigPanel *m_parserPanel{};
    TerminalPanel *m_terminalPanel{};
    SendPanel *m_sendPanel{};
    ConnectionState m_previousState{ConnectionState::Disconnected};
};
