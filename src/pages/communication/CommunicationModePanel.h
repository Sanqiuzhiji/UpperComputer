#pragma once

#include <QFrame>
#include <QList>

#include "models/ConnectionTypes.h"
#include "models/ProtocolDefinition.h"

class AppContext;
class QComboBox;
class QLabel;
class QPushButton;
class QToolButton;

class CommunicationModePanel final : public QFrame
{
    Q_OBJECT

public:
    explicit CommunicationModePanel(
        AppContext *context, QWidget *parent = nullptr);

    [[nodiscard]] ParserMode receiveMode() const;
    [[nodiscard]] SendMode sendMode() const;
    [[nodiscard]] QString currentProtocolId() const;
    [[nodiscard]] QString currentReceiveCommandId() const;
    [[nodiscard]] QString currentSendCommandId() const;

    void setProtocols(const QList<ProtocolDefinition> &protocols);

signals:
    void receiveModeChanged(ParserMode mode);
    void sendModeChanged(SendMode mode);
    void customProtocolChanged(const QString &protocolId);
    void receiveCommandChanged(const QString &commandId);
    void sendCommandChanged(const QString &commandId);
    void requestProtocolLibrary();
    void helpRequested(const QString &message);

private:
    void updateCustomProtocolVisibility();
    void updateCommandChoices();
    [[nodiscard]] QString helpText() const;

    AppContext *m_context{};
    QToolButton *m_helpButton{};
    QWidget *m_customOptionsRow{};
    QLabel *m_protocolLabel{};
    QLabel *m_receiveCommandLabel{};
    QLabel *m_sendCommandLabel{};
    QComboBox *m_receiveCombo{};
    QComboBox *m_sendCombo{};
    QComboBox *m_protocolCombo{};
    QComboBox *m_receiveCommandCombo{};
    QComboBox *m_sendCommandCombo{};
    QPushButton *m_openLibraryButton{};
    QList<ProtocolDefinition> m_protocols;
};
