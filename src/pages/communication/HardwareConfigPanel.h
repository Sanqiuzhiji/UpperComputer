#pragma once

#include <QFrame>

#include "models/AppTypes.h"
#include "models/ConnectionTypes.h"

class AppContext;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QSpinBox;
class QStackedWidget;
class QToolButton;

class HardwareConfigPanel final : public QFrame
{
    Q_OBJECT

public:
    explicit HardwareConfigPanel(AppContext *context, QWidget *parent = nullptr);

    [[nodiscard]] TransportType transportType() const;
    [[nodiscard]] TransportConfig transportConfig() const;

public slots:
    void setConnectionState(ConnectionState state);
    void setFirmwareOperationActive(bool active);
    void setTcpClients(const QStringList &clients);
    void refreshSerialPorts();

signals:
    void connectRequested(TransportType type, const TransportConfig &config);
    void disconnectRequested();
    void tcpServerTargetChanged(const QString &clientId);
    void notificationRequested(const QString &message, NotificationType type);

private:
    QWidget *createSerialPage();
    QWidget *createUdpPage();
    QWidget *createTcpServerPage();
    QWidget *createTcpClientPage();
    QWidget *createVirtualPage();
    void loadSettings();
    void saveCurrentConfig();
    void updateUiForState();
    void installUnderline(QWidget *widget) const;
    [[nodiscard]] bool validate(QString *errorMessage) const;

    AppContext *m_context{};
    QComboBox *m_transportCombo{};
    QStackedWidget *m_configStack{};
    QCheckBox *m_connectionSwitch{};
    QToolButton *m_refreshButton{};

    QComboBox *m_serialPort{};
    QComboBox *m_baudRate{};
    QComboBox *m_dataBits{};
    QComboBox *m_parity{};
    QComboBox *m_stopBits{};

    QLineEdit *m_udpRemoteAddress{};
    QSpinBox *m_udpRemotePort{};
    QSpinBox *m_udpLocalPort{};

    QSpinBox *m_tcpServerPort{};
    QLabel *m_tcpClientCount{};
    QComboBox *m_tcpClients{};

    QLineEdit *m_tcpClientHost{};
    QSpinBox *m_tcpClientPort{};
    QLineEdit *m_tcpClientName{};

    QDoubleSpinBox *m_virtualInterval{};
    QDoubleSpinBox *m_virtualFrequency{};
    QDoubleSpinBox *m_virtualAmplitude{};
    QSpinBox *m_virtualChannels{};

    ConnectionState m_state{ConnectionState::Disconnected};
    bool m_refreshing{};
    bool m_firstRefresh{true};
    bool m_firmwareOperationActive{};
};
