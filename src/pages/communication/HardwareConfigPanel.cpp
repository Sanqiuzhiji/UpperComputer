#include "HardwareConfigPanel.h"

#include "app/AppContext.h"
#include "app/AppSettings.h"
#include "theme/IconManager.h"
#include "theme/ThemeManager.h"
#include "widgets/FocusUnderline.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFutureWatcher>
#include <QFormLayout>
#include <QGridLayout>
#include <QHostAddress>
#include <QLabel>
#include <QLineEdit>
#include <QSerialPortInfo>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStackedWidget>
#include <QToolButton>
#include <QtConcurrent>

namespace {
template<typename Enum>
void addEnumItem(QComboBox *combo, const QString &text, Enum value)
{
    combo->addItem(text, QVariant::fromValue(value));
}

QLabel *fieldLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setProperty("muted", true);
    return label;
}

void setComboValue(QComboBox *combo, const QVariant &value)
{
    const int index = combo->findData(value);
    if (index >= 0) combo->setCurrentIndex(index);
}
}

HardwareConfigPanel::HardwareConfigPanel(AppContext *context, QWidget *parent)
    : QFrame(parent),
      m_context(context)
{
    setProperty("card", true);
    setObjectName(QStringLiteral("hardwareConfigPanel"));
    setMinimumHeight(78);
    setMaximumHeight(92);

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(14, 8, 14, 8);
    root->setSpacing(8);
    auto *title = new QLabel(tr("硬件配置"), this);
    title->setObjectName(QStringLiteral("sectionTitle"));
    root->addWidget(title);
    root->addWidget(fieldLabel(tr("类型"), this));

    m_transportCombo = new QComboBox(this);
    addEnumItem(m_transportCombo, tr("串口"), TransportType::SerialPort);
    addEnumItem(m_transportCombo, QStringLiteral("UDP"), TransportType::Udp);
    addEnumItem(m_transportCombo, tr("TCP 服务端"), TransportType::TcpServer);
    addEnumItem(m_transportCombo, tr("TCP 客户端"), TransportType::TcpClient);
    addEnumItem(m_transportCombo, tr("虚拟数据"), TransportType::VirtualData);
    m_transportCombo->setMinimumWidth(108);
    installUnderline(m_transportCombo);
    root->addWidget(m_transportCombo);

    m_configStack = new QStackedWidget(this);
    m_configStack->setObjectName(QStringLiteral("hardwareConfigStack"));
    m_configStack->setFixedHeight(58);
    m_configStack->addWidget(createSerialPage());
    m_configStack->addWidget(createUdpPage());
    m_configStack->addWidget(createTcpServerPage());
    m_configStack->addWidget(createTcpClientPage());
    m_configStack->addWidget(createVirtualPage());
    root->addWidget(m_configStack, 1);

    m_connectionSwitch = new QCheckBox(tr("连接"), this);
    root->addWidget(m_connectionSwitch);

    loadSettings();
    connect(m_transportCombo, &QComboBox::currentIndexChanged, this,
            [this](const int index) {
                if (m_state != ConnectionState::Disconnected
                    && m_state != ConnectionState::Error) {
                    return;
                }
                m_configStack->setCurrentIndex(index);
                m_context->settings()->setTransportType(transportType());
                if (transportType() == TransportType::SerialPort) {
                    refreshSerialPorts();
                }
            });
    connect(m_connectionSwitch, &QCheckBox::toggled, this,
            [this](const bool checked) {
                if (checked) {
                    QString error;
                    if (!validate(&error)) {
                        const QSignalBlocker blocker(m_connectionSwitch);
                        m_connectionSwitch->setChecked(false);
                        emit notificationRequested(error, NotificationType::Warning);
                        return;
                    }
                    saveCurrentConfig();
                    emit connectRequested(transportType(), transportConfig());
                } else if (m_state != ConnectionState::Disconnected
                           && m_state != ConnectionState::Error) {
                    emit disconnectRequested();
                }
            });
    connect(m_tcpClients, &QComboBox::currentTextChanged,
            this, &HardwareConfigPanel::tcpServerTargetChanged);
    connect(m_refreshButton, &QToolButton::clicked,
            this, &HardwareConfigPanel::refreshSerialPorts);
    connect(m_context->iconManager(), &IconManager::iconsChanged, this, [this] {
        m_refreshButton->setIcon(m_context->iconManager()->icon(
            QStringLiteral(":/icons/connection/refresh.svg")));
    });
    const auto persist = [this] { saveCurrentConfig(); };
    for (QComboBox *combo : {
             m_serialPort, m_baudRate, m_dataBits, m_parity, m_stopBits}) {
        connect(combo, &QComboBox::currentIndexChanged, this, persist);
    }
    connect(m_baudRate, &QComboBox::currentTextChanged, this, persist);
    for (QLineEdit *edit : {
             m_udpRemoteAddress, m_tcpClientHost, m_tcpClientName}) {
        connect(edit, &QLineEdit::editingFinished, this, persist);
    }
    for (QSpinBox *spin : {
             m_udpRemotePort, m_udpLocalPort, m_tcpServerPort,
             m_tcpClientPort, m_virtualChannels}) {
        connect(spin, &QSpinBox::valueChanged, this, persist);
    }
    for (QDoubleSpinBox *spin : {
             m_virtualInterval, m_virtualFrequency, m_virtualAmplitude}) {
        connect(spin, &QDoubleSpinBox::valueChanged, this, persist);
    }
    refreshSerialPorts();
}

TransportType HardwareConfigPanel::transportType() const
{
    return m_transportCombo->currentData().value<TransportType>();
}

TransportConfig HardwareConfigPanel::transportConfig() const
{
    switch (transportType()) {
    case TransportType::SerialPort:
        return SerialConfig{
            m_serialPort->currentData().toString(),
            m_baudRate->currentText().toInt(),
            m_dataBits->currentData().value<QSerialPort::DataBits>(),
            m_parity->currentData().value<QSerialPort::Parity>(),
            m_stopBits->currentData().value<QSerialPort::StopBits>()};
    case TransportType::Udp:
        return UdpConfig{m_udpRemoteAddress->text().trimmed(),
                         static_cast<quint16>(m_udpRemotePort->value()),
                         static_cast<quint16>(m_udpLocalPort->value())};
    case TransportType::TcpServer:
        return TcpServerConfig{
            static_cast<quint16>(m_tcpServerPort->value())};
    case TransportType::TcpClient:
        return TcpClientConfig{m_tcpClientHost->text().trimmed(),
                               static_cast<quint16>(m_tcpClientPort->value()),
                               m_tcpClientName->text().trimmed()};
    case TransportType::VirtualData:
        return VirtualDataConfig{m_virtualInterval->value(),
                                 m_virtualFrequency->value(),
                                 m_virtualAmplitude->value(),
                                 m_virtualChannels->value()};
    }
    return SerialConfig{};
}

void HardwareConfigPanel::setConnectionState(const ConnectionState state)
{
    m_state = state;
    updateUiForState();
}

void HardwareConfigPanel::setTcpClients(const QStringList &clients)
{
    const QString selected = m_tcpClients->currentText();
    const QSignalBlocker blocker(m_tcpClients);
    m_tcpClients->clear();
    m_tcpClients->addItems(clients);
    const int previous = m_tcpClients->findText(selected);
    if (previous >= 0) m_tcpClients->setCurrentIndex(previous);
    m_tcpClientCount->setText(tr("%1 个").arg(clients.size()));
    if (m_tcpClients->count() > 0) {
        emit tcpServerTargetChanged(m_tcpClients->currentText());
    }
}

void HardwareConfigPanel::refreshSerialPorts()
{
    if (m_refreshing) return;
    m_refreshing = true;
    m_refreshButton->setEnabled(false);
    QString previous = m_serialPort->currentData().toString();
    if (previous.isEmpty()) {
        previous = m_context->settings()->serialConfig().portName;
    }
    auto *watcher = new QFutureWatcher<QList<QSerialPortInfo>>(this);
    connect(watcher, &QFutureWatcher<QList<QSerialPortInfo>>::finished,
            this, [this, watcher, previous] {
        const QList<QSerialPortInfo> ports = watcher->result();
        watcher->deleteLater();
        const QSignalBlocker blocker(m_serialPort);
        m_serialPort->clear();
        for (const QSerialPortInfo &port : ports) {
            QString label = port.portName();
            if (!port.description().trimmed().isEmpty()) {
                label += QStringLiteral(" - ") + port.description();
            }
            m_serialPort->addItem(label, port.portName());
        }
        if (ports.isEmpty()) {
            m_serialPort->addItem(tr("未发现串口"), QString());
        } else {
            const int index = m_serialPort->findData(previous);
            if (index >= 0) m_serialPort->setCurrentIndex(index);
        }
        m_refreshing = false;
        m_refreshButton->setEnabled(true);
        saveCurrentConfig();
        if (m_firstRefresh) {
            m_firstRefresh = false;
        } else {
            emit notificationRequested(
                ports.isEmpty() ? tr("未发现可用串口")
                                : tr("已发现 %1 个串口").arg(ports.size()),
                ports.isEmpty() ? NotificationType::Warning
                                : NotificationType::Information);
        }
    });
    watcher->setFuture(QtConcurrent::run([] {
        return QSerialPortInfo::availablePorts();
    }));
}

QWidget *HardwareConfigPanel::createSerialPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QGridLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(8);
    layout->setVerticalSpacing(4);

    m_serialPort = new QComboBox(page);
    m_serialPort->setMinimumWidth(185);
    m_baudRate = new QComboBox(page);
    m_baudRate->setEditable(true);
    for (const int rate : {9600, 19200, 38400, 57600, 115200, 230400,
                           460800, 921600, 1000000, 2000000, 4000000}) {
        m_baudRate->addItem(QString::number(rate), rate);
    }
    m_dataBits = new QComboBox(page);
    addEnumItem(m_dataBits, QStringLiteral("5"), QSerialPort::Data5);
    addEnumItem(m_dataBits, QStringLiteral("6"), QSerialPort::Data6);
    addEnumItem(m_dataBits, QStringLiteral("7"), QSerialPort::Data7);
    addEnumItem(m_dataBits, QStringLiteral("8"), QSerialPort::Data8);
    m_parity = new QComboBox(page);
    addEnumItem(m_parity, tr("无"), QSerialPort::NoParity);
    addEnumItem(m_parity, tr("奇校验"), QSerialPort::OddParity);
    addEnumItem(m_parity, tr("偶校验"), QSerialPort::EvenParity);
    addEnumItem(m_parity, QStringLiteral("Mark"), QSerialPort::MarkParity);
    addEnumItem(m_parity, QStringLiteral("Space"), QSerialPort::SpaceParity);
    m_stopBits = new QComboBox(page);
    addEnumItem(m_stopBits, QStringLiteral("1"), QSerialPort::OneStop);
    addEnumItem(m_stopBits, QStringLiteral("1.5"), QSerialPort::OneAndHalfStop);
    addEnumItem(m_stopBits, QStringLiteral("2"), QSerialPort::TwoStop);
    m_refreshButton = new QToolButton(page);
    m_refreshButton->setIcon(m_context->iconManager()->icon(
        QStringLiteral(":/icons/connection/refresh.svg")));
    m_refreshButton->setToolTip(tr("刷新串口列表"));
    m_refreshButton->setFixedSize(36, 34);

    const QList<QPair<QString, QWidget *>> fields{
        {tr("串口设备"), m_serialPort}, {tr("波特率"), m_baudRate},
        {tr("数据位"), m_dataBits}, {tr("校验位"), m_parity},
        {tr("停止位"), m_stopBits}};
    for (int i = 0; i < fields.size(); ++i) {
        layout->addWidget(fieldLabel(fields[i].first, page), 0, i);
        layout->addWidget(fields[i].second, 1, i);
        installUnderline(fields[i].second);
    }
    layout->addWidget(m_refreshButton, 1, fields.size());
    layout->setColumnStretch(0, 2);
    return page;
}

QWidget *HardwareConfigPanel::createUdpPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QGridLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(10);
    m_udpRemoteAddress = new QLineEdit(page);
    m_udpRemotePort = new QSpinBox(page);
    m_udpLocalPort = new QSpinBox(page);
    for (QSpinBox *port : {m_udpRemotePort, m_udpLocalPort}) {
        port->setRange(1, 65535);
    }
    const QList<QPair<QString, QWidget *>> fields{
        {tr("远程 IP"), m_udpRemoteAddress},
        {tr("远程端口"), m_udpRemotePort},
        {tr("本地端口"), m_udpLocalPort}};
    for (int i = 0; i < fields.size(); ++i) {
        layout->addWidget(fieldLabel(fields[i].first, page), 0, i);
        layout->addWidget(fields[i].second, 1, i);
        installUnderline(fields[i].second);
    }
    layout->setColumnStretch(3, 1);
    return page;
}

QWidget *HardwareConfigPanel::createTcpServerPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QGridLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(10);
    m_tcpServerPort = new QSpinBox(page);
    m_tcpServerPort->setRange(1, 65535);
    m_tcpClientCount = new QLabel(tr("0 个"), page);
    m_tcpClients = new QComboBox(page);
    m_tcpClients->setMinimumWidth(220);
    layout->addWidget(fieldLabel(tr("监听端口"), page), 0, 0);
    layout->addWidget(m_tcpServerPort, 1, 0);
    layout->addWidget(fieldLabel(tr("当前连接数"), page), 0, 1);
    layout->addWidget(m_tcpClientCount, 1, 1);
    layout->addWidget(fieldLabel(tr("已连接客户端"), page), 0, 2);
    layout->addWidget(m_tcpClients, 1, 2);
    layout->setColumnStretch(3, 1);
    installUnderline(m_tcpServerPort);
    installUnderline(m_tcpClients);
    return page;
}

QWidget *HardwareConfigPanel::createTcpClientPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QGridLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(10);
    m_tcpClientHost = new QLineEdit(page);
    m_tcpClientPort = new QSpinBox(page);
    m_tcpClientPort->setRange(1, 65535);
    m_tcpClientName = new QLineEdit(page);
    m_tcpClientName->setPlaceholderText(tr("可选"));
    const QList<QPair<QString, QWidget *>> fields{
        {tr("服务端 IP"), m_tcpClientHost},
        {tr("服务端端口"), m_tcpClientPort},
        {tr("连接名称"), m_tcpClientName}};
    for (int i = 0; i < fields.size(); ++i) {
        layout->addWidget(fieldLabel(fields[i].first, page), 0, i);
        layout->addWidget(fields[i].second, 1, i);
        installUnderline(fields[i].second);
    }
    layout->setColumnStretch(3, 1);
    return page;
}

QWidget *HardwareConfigPanel::createVirtualPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QGridLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(10);
    m_virtualInterval = new QDoubleSpinBox(page);
    m_virtualInterval->setRange(0.2, 60000.0);
    m_virtualInterval->setDecimals(3);
    m_virtualInterval->setSuffix(QStringLiteral(" ms"));
    m_virtualFrequency = new QDoubleSpinBox(page);
    m_virtualFrequency->setRange(0.0, 100000.0);
    m_virtualFrequency->setDecimals(3);
    m_virtualFrequency->setSuffix(QStringLiteral(" Hz"));
    m_virtualAmplitude = new QDoubleSpinBox(page);
    m_virtualAmplitude->setRange(0.0, 1000000.0);
    m_virtualAmplitude->setDecimals(2);
    m_virtualChannels = new QSpinBox(page);
    m_virtualChannels->setRange(1, 128);
    m_virtualChannels->setSuffix(QStringLiteral(" Ch"));
    const QList<QPair<QString, QWidget *>> fields{
        {tr("采样周期"), m_virtualInterval},
        {tr("信号频率"), m_virtualFrequency},
        {tr("幅值"), m_virtualAmplitude},
        {tr("通道数量"), m_virtualChannels}};
    for (int i = 0; i < fields.size(); ++i) {
        layout->addWidget(fieldLabel(fields[i].first, page), 0, i);
        layout->addWidget(fields[i].second, 1, i);
        installUnderline(fields[i].second);
    }
    layout->setColumnStretch(4, 1);
    return page;
}

void HardwareConfigPanel::loadSettings()
{
    AppSettings *settings = m_context->settings();
    setComboValue(m_transportCombo, QVariant::fromValue(settings->transportType()));
    m_configStack->setCurrentIndex(m_transportCombo->currentIndex());

    const SerialConfig serial = settings->serialConfig();
    m_baudRate->setCurrentText(QString::number(serial.baudRate));
    setComboValue(m_dataBits, QVariant::fromValue(serial.dataBits));
    setComboValue(m_parity, QVariant::fromValue(serial.parity));
    setComboValue(m_stopBits, QVariant::fromValue(serial.stopBits));

    const UdpConfig udp = settings->udpConfig();
    m_udpRemoteAddress->setText(udp.remoteAddress);
    m_udpRemotePort->setValue(udp.remotePort);
    m_udpLocalPort->setValue(udp.localPort);
    const TcpServerConfig server = settings->tcpServerConfig();
    m_tcpServerPort->setValue(server.listenPort);
    const TcpClientConfig client = settings->tcpClientConfig();
    m_tcpClientHost->setText(client.host);
    m_tcpClientPort->setValue(client.port);
    m_tcpClientName->setText(client.connectionName);
    const VirtualDataConfig virtualData = settings->virtualDataConfig();
    m_virtualInterval->setValue(virtualData.sampleIntervalMs);
    m_virtualFrequency->setValue(virtualData.signalFrequencyHz);
    m_virtualAmplitude->setValue(virtualData.amplitude);
    m_virtualChannels->setValue(virtualData.channelCount);
}

void HardwareConfigPanel::saveCurrentConfig()
{
    AppSettings *settings = m_context->settings();
    settings->setTransportType(transportType());
    settings->setSerialConfig(SerialConfig{
        m_serialPort->currentData().toString(),
        m_baudRate->currentText().toInt(),
        m_dataBits->currentData().value<QSerialPort::DataBits>(),
        m_parity->currentData().value<QSerialPort::Parity>(),
        m_stopBits->currentData().value<QSerialPort::StopBits>()});
    settings->setUdpConfig(UdpConfig{
        m_udpRemoteAddress->text().trimmed(),
        static_cast<quint16>(m_udpRemotePort->value()),
        static_cast<quint16>(m_udpLocalPort->value())});
    settings->setTcpServerConfig(TcpServerConfig{
        static_cast<quint16>(m_tcpServerPort->value())});
    settings->setTcpClientConfig(TcpClientConfig{
        m_tcpClientHost->text().trimmed(),
        static_cast<quint16>(m_tcpClientPort->value()),
        m_tcpClientName->text().trimmed()});
    settings->setVirtualDataConfig(VirtualDataConfig{
        m_virtualInterval->value(), m_virtualFrequency->value(),
        m_virtualAmplitude->value(), m_virtualChannels->value()});
}

void HardwareConfigPanel::updateUiForState()
{
    const bool busy = m_state == ConnectionState::Connecting
        || m_state == ConnectionState::Connected
        || m_state == ConnectionState::Disconnecting;
    m_transportCombo->setEnabled(!busy);
    // Keep the stack enabled so its descriptive labels retain their normal
    // palette and font rendering. Lock only the actual configuration editors.
    m_configStack->setEnabled(true);
    const QList<QWidget *> configurationEditors{
        m_serialPort, m_baudRate, m_dataBits, m_parity, m_stopBits,
        m_refreshButton,
        m_udpRemoteAddress, m_udpRemotePort, m_udpLocalPort,
        m_tcpServerPort,
        m_tcpClientHost, m_tcpClientPort, m_tcpClientName,
        m_virtualInterval, m_virtualFrequency, m_virtualAmplitude,
        m_virtualChannels
    };
    for (QWidget *editor : configurationEditors) {
        if (editor) editor->setEnabled(!busy);
    }
    if (m_tcpClients) {
        m_tcpClients->setEnabled(
            m_state == ConnectionState::Connected
            && transportType() == TransportType::TcpServer);
    }
    // Connecting stays cancellable; Disconnecting is locked to prevent races.
    m_connectionSwitch->setEnabled(
        m_state != ConnectionState::Disconnecting);
    {
        const QSignalBlocker blocker(m_connectionSwitch);
        m_connectionSwitch->setChecked(
            m_state == ConnectionState::Connecting
            || m_state == ConnectionState::Connected
            || m_state == ConnectionState::Disconnecting);
    }
    m_connectionSwitch->setText(
        m_state == ConnectionState::Connecting ? tr("连接中")
        : m_state == ConnectionState::Disconnecting ? tr("断开中")
        : tr("连接"));
}

void HardwareConfigPanel::installUnderline(QWidget *widget) const
{
    new FocusUnderline(widget, m_context->themeManager());
}

bool HardwareConfigPanel::validate(QString *errorMessage) const
{
    const TransportConfig config = transportConfig();
    if (const auto *serial = std::get_if<SerialConfig>(&config)) {
        if (serial->portName.isEmpty() || serial->baudRate <= 0) {
            *errorMessage = tr("请选择有效串口并检查波特率");
            return false;
        }
    } else if (const auto *udp = std::get_if<UdpConfig>(&config)) {
        QHostAddress address;
        if (!address.setAddress(udp->remoteAddress)
            || udp->remotePort == 0 || udp->localPort == 0) {
            *errorMessage = tr("请检查 UDP 地址和端口");
            return false;
        }
    } else if (const auto *client = std::get_if<TcpClientConfig>(&config)) {
        if (client->host.isEmpty() || client->port == 0) {
            *errorMessage = tr("请检查 TCP 服务端地址和端口");
            return false;
        }
    }
    return true;
}
