#include "CescToolPage.h"

#include "app/AppContext.h"
#include "app/AppSettings.h"
#include "models/AppTypes.h"
#include "services/CescFirmwareUploader.h"
#include "services/ConnectionManager.h"
#include "services/cesc/CescSession.h"
#include "services/cesc/CescSensorClient.h"
#include "services/cesc/CescTelemetryClient.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

CescToolPage::CescToolPage(AppContext *context, QWidget *parent)
    : QWidget(parent), m_context(context),
      m_uploader(context->cescFirmwareUploader())
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 24, 28, 28);
    layout->setSpacing(16);
    auto *title = new QLabel(QStringLiteral("CESC Tool"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(tr("通过当前共享串口连接为下位机烧录 CESC 固件"), this);
    subtitle->setProperty("muted", true);
    layout->addWidget(title);
    layout->addWidget(subtitle);

    auto *deviceCard = new QFrame(this);
    deviceCard->setProperty("card", true);
    auto *deviceLayout = new QGridLayout(deviceCard);
    deviceLayout->setContentsMargins(24, 18, 24, 18);
    deviceLayout->setHorizontalSpacing(20);
    deviceLayout->addWidget(new QLabel(tr("当前设备"), deviceCard), 0, 0, 1, 4);
    deviceLayout->addWidget(new QLabel(tr("串口"), deviceCard), 1, 0);
    deviceLayout->addWidget(new QLabel(tr("波特率"), deviceCard), 1, 1);
    deviceLayout->addWidget(new QLabel(tr("连接状态"), deviceCard), 1, 2);
    deviceLayout->addWidget(new QLabel(tr("固件版本"), deviceCard), 1, 3);
    m_portValue = new QLabel(deviceCard);
    m_baudValue = new QLabel(deviceCard);
    m_connectionValue = new QLabel(deviceCard);
    m_versionValue = new QLabel(deviceCard);
    for (QLabel *label : {m_portValue, m_baudValue, m_connectionValue,
                          m_versionValue}) {
        QFont font = label->font(); font.setBold(true); label->setFont(font);
    }
    deviceLayout->addWidget(m_portValue, 2, 0);
    deviceLayout->addWidget(m_baudValue, 2, 1);
    deviceLayout->addWidget(m_connectionValue, 2, 2);
    deviceLayout->addWidget(m_versionValue, 2, 3);
    m_connectionButton = new QPushButton(deviceCard);
    deviceLayout->addWidget(m_connectionButton, 1, 4, 2, 1);
    layout->addWidget(deviceCard);

    auto *angleCard = new QFrame(this);
    angleCard->setProperty("card", true);
    auto *angleLayout = new QGridLayout(angleCard);
    angleLayout->setContentsMargins(24, 18, 24, 18);
    angleLayout->setHorizontalSpacing(18);
    auto *angleTitle = new QLabel(tr("Angle sensor and telemetry"), angleCard);
    angleTitle->setObjectName(QStringLiteral("sectionTitle"));
    angleLayout->addWidget(angleTitle, 0, 0, 1, 6);
    angleLayout->addWidget(new QLabel(tr("Angle"), angleCard), 1, 0);
    angleLayout->addWidget(new QLabel(tr("Raw"), angleCard), 1, 1);
    angleLayout->addWidget(new QLabel(tr("Sensor status"), angleCard), 1, 2);
    m_angleValue = new QLabel(QStringLiteral("-- deg"), angleCard);
    m_rawAngleValue = new QLabel(QStringLiteral("--"), angleCard);
    m_sensorStatusValue = new QLabel(tr("Unknown"), angleCard);
    m_streamStatus = new QLabel(tr("Stopped"), angleCard);
    angleLayout->addWidget(m_angleValue, 2, 0);
    angleLayout->addWidget(m_rawAngleValue, 2, 1);
    angleLayout->addWidget(m_sensorStatusValue, 2, 2);
    angleLayout->addWidget(m_streamStatus, 2, 3);
    m_queryAngleButton = new QPushButton(tr("Read once"), angleCard);
    m_telemetryButton = new QPushButton(tr("Start telemetry"), angleCard);
    m_telemetryButton->setProperty("accent", true);
    angleLayout->addWidget(m_queryAngleButton, 1, 4, 2, 1);
    angleLayout->addWidget(m_telemetryButton, 1, 5, 2, 1);
    auto *plotHint = new QLabel(
        tr("After telemetry starts, open Plot and select the CESC V1 channels "
           "shaft_angle, shaft_angle_raw or shaft_sensor_status."), angleCard);
    plotHint->setWordWrap(true);
    plotHint->setProperty("muted", true);
    angleLayout->addWidget(plotHint, 3, 0, 1, 6);
    layout->addWidget(angleCard);

    auto *card = new QFrame(this);
    card->setProperty("card", true);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(24, 24, 24, 24);
    cardLayout->setSpacing(14);
    auto *section = new QLabel(tr("固件烧录"), card);
    section->setObjectName(QStringLiteral("sectionTitle"));
    cardLayout->addWidget(section);
    auto *fileRow = new QHBoxLayout;
    m_filePath = new QLineEdit(card);
    m_filePath->setReadOnly(true);
    m_filePath->setPlaceholderText(tr("请选择 .bin 固件文件"));
    m_chooseButton = new QPushButton(tr("浏览..."), card);
    fileRow->addWidget(m_filePath, 1);
    fileRow->addWidget(m_chooseButton);
    cardLayout->addLayout(fileRow);
    m_fileInfo = new QLabel(tr("尚未选择固件"), card);
    m_fileInfo->setProperty("muted", true);
    cardLayout->addWidget(m_fileInfo);
    m_progress = new QProgressBar(card);
    m_progress->setRange(0, 100);
    m_progress->setValue(m_uploader->progress());
    m_status = new QLabel(m_uploader->statusText().isEmpty()
                              ? tr("等待烧录") : m_uploader->statusText(), card);
    m_status->setProperty("muted", true);
    cardLayout->addWidget(m_progress);
    cardLayout->addWidget(m_status);
    auto *buttons = new QHBoxLayout;
    buttons->addStretch();
    m_cancelButton = new QPushButton(tr("取消"), card);
    m_uploadButton = new QPushButton(tr("开始烧录"), card);
    m_uploadButton->setProperty("accent", true);
    buttons->addWidget(m_cancelButton);
    buttons->addWidget(m_uploadButton);
    cardLayout->addLayout(buttons);
    auto *warning = new QLabel(tr("烧录期间请勿断开设备或关闭串口。"), card);
    warning->setProperty("muted", true);
    cardLayout->addWidget(warning);
    layout->addWidget(card);
    layout->addStretch();

    auto *manager = context->connectionManager();
    connect(m_chooseButton, &QPushButton::clicked, this, &CescToolPage::chooseFirmware);
    connect(m_uploadButton, &QPushButton::clicked, this, &CescToolPage::startUpload);
    connect(m_cancelButton, &QPushButton::clicked, m_uploader, &CescFirmwareUploader::cancel);
    connect(m_queryAngleButton, &QPushButton::clicked,
            this, &CescToolPage::queryAngle);
    connect(m_telemetryButton, &QPushButton::clicked, this, [this] {
        if (m_angleStreamId != 0U
            || m_context->cescTelemetryClient()->activeStreamId() != 0U) {
            stopAngleStream();
        } else {
            startAngleStream();
        }
    });
    connect(context->cescSensorClient(), &CescSensorClient::sampleReceived,
            this, [this](const CescSensorClient::Sample &sample) {
                m_angleValue->setText(QStringLiteral("%1 deg").arg(sample.degrees, 0, 'f', 2));
                m_rawAngleValue->setText(QString::number(sample.rawAngle));
                m_sensorStatusValue->setText(QString::number(sample.status));
            });
    connect(context->cescSensorClient(), &CescSensorClient::commandFailed,
            this, [this](const QString &message) {
                m_context->notify(message, NotificationType::Error);
            });
    connect(context->cescTelemetryClient(), &CescTelemetryClient::subscribed,
            this, [this](quint16 streamId) {
                m_angleStreamId = streamId;
                m_streamStatus->setText(tr("Streaming (ID %1, 100 Hz)").arg(streamId));
                updateActions();
            });
    connect(context->cescTelemetryClient(), &CescTelemetryClient::commandFailed,
            this, [this](const QString &message) {
                m_streamStatus->setText(message);
                m_context->notify(message, NotificationType::Error);
                updateActions();
            });
    connect(m_connectionButton, &QPushButton::clicked, this, [this, manager] {
        if (manager->state() == ConnectionState::Connected
            || manager->state() == ConnectionState::Connecting) {
            manager->disconnectTransport();
            return;
        }
        const SerialConfig config = m_context->settings()->serialConfig();
        if (config.portName.trimmed().isEmpty()) {
            m_context->notify(tr("请先在 Communication 页面选择串口"), NotificationType::Warning);
            return;
        }
        manager->connectTransport(TransportType::SerialPort, config);
    });
    connect(manager, &ConnectionManager::stateChanged, this, [this] {
        updateConnectionUi(); updateActions();
        if (m_context->connectionManager()->state()
            == ConnectionState::Connected) {
            m_context->cescSession()->setEnabled(true);
        } else if (m_context->connectionManager()->state()
                   == ConnectionState::Disconnected) {
            m_angleStreamId = 0U;
            m_streamStatus->setText(tr("Stopped"));
        }
    });
    connect(manager, &ConnectionManager::deviceNameChanged,
            this, [this] { updateConnectionUi(); });
    connect(context->cescSession(), &CescSession::stateChanged, this,
            [this](CescSession::State state) {
                updateConnectionUi();
                updateActions();
                if (state == CescSession::State::Ready
                    && !m_uploader->isBusy()) {
                    m_uploader->requestFirmwareVersion();
                }
            });
    connect(m_uploader, &CescFirmwareUploader::stateChanged, this,
            [this](CescFirmwareUploader::State state, int progress,
                   const QString &status) {
                const bool recovering =
                    state == CescFirmwareUploader::State::Rebooting
                    || state == CescFirmwareUploader::State::ReadingVersion;
                m_progress->setRange(0, recovering ? 0 : 100);
                if (!recovering) m_progress->setValue(progress);
                m_status->setText(status);
                updateActions();
            });
    connect(m_uploader, &CescFirmwareUploader::finished, this,
            [this](bool success, const QString &message) {
                m_context->notify(message, success ? NotificationType::Success
                                                   : NotificationType::Error);
            });
    connect(m_uploader, &CescFirmwareUploader::firmwareVersionChanged,
            m_versionValue, &QLabel::setText);
    connect(context->settings(), &AppSettings::cescFirmwarePathChanged,
            this, [this](const QString &path) {
                if (!m_uploader->isBusy()) applyFirmwarePath(path);
            });
    applyFirmwarePath(context->settings()->cescFirmwarePath());
    const auto initialState = m_uploader->state();
    if (initialState == CescFirmwareUploader::State::Rebooting
        || initialState == CescFirmwareUploader::State::ReadingVersion) {
        m_progress->setRange(0, 0);
    }
    updateConnectionUi();
    updateActions();
    if (manager->state() == ConnectionState::Connected) {
        context->cescSession()->setEnabled(true);
        if (context->cescSession()->isReady()) {
            m_uploader->requestFirmwareVersion();
            m_angleStreamId = context->cescTelemetryClient()->activeStreamId();
        }
    }
}

void CescToolPage::queryAngle()
{
    if (!m_context->cescSession()->isReady()) return;
    m_context->cescSensorClient()->getSample();
}

void CescToolPage::startAngleStream()
{
    if (!m_context->cescSession()->isReady()) return;
    m_angleStreamId = m_context->cescTelemetryClient()->activeStreamId();
    if (m_angleStreamId != 0U) return;
    m_streamStatus->setText(tr("Starting..."));
    m_context->cescTelemetryClient()->enumerate();
    m_context->cescTelemetryClient()->subscribe({0U, 1U, 2U}, 10000U, 1U);
    updateActions();
}

void CescToolPage::stopAngleStream()
{
    if (m_angleStreamId != 0U) {
        m_context->cescTelemetryClient()->unsubscribe(m_angleStreamId);
        m_angleStreamId = 0U;
    } else if (m_context->cescSession()->isReady()) {
        m_context->cescTelemetryClient()->stopAll();
    }
    m_streamStatus->setText(tr("Stopped"));
    updateActions();
}

void CescToolPage::chooseFirmware()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("选择固件文件"), m_context->settings()->cescFirmwareDirectory(),
        tr("固件文件 (*.bin);;所有文件 (*)"));
    if (path.isEmpty()) return;
    m_context->settings()->setCescFirmwareDirectory(
        QFileInfo(path).absolutePath());
    applyFirmwarePath(path, true);
}

void CescToolPage::applyFirmwarePath(
    const QString &path, const bool notifyOnError)
{
    m_filePath->setText(path);
    if (path.trimmed().isEmpty()) {
        m_firmwareValid = false;
        m_firmwareSize = 0;
        m_fileInfo->setText(tr("尚未选择固件"));
        updateActions();
        return;
    }
    QString error;
    m_firmwareValid = validateFirmware(path, &error);
    if (m_firmwareValid) {
        const QFileInfo info(path);
        m_firmwareSize = info.size();
        m_fileInfo->setText(tr("%1 · %2 KB").arg(info.fileName())
                            .arg((info.size() + 1023) / 1024));
        m_progress->setRange(0, 100);
        m_progress->setValue(0);
        m_status->setText(tr("固件已就绪"));
    } else {
        m_firmwareSize = 0;
        m_fileInfo->setText(error);
        m_status->setText(tr("默认固件无效：%1").arg(error));
        if (notifyOnError) m_context->notify(error, NotificationType::Error);
    }
    updateActions();
}

bool CescToolPage::validateFirmware(const QString &path, QString *errorMessage)
{
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile()) { *errorMessage = tr("文件不存在"); return false; }
    if (info.suffix().compare(QStringLiteral("bin"), Qt::CaseInsensitive) != 0) {
        *errorMessage = tr("文件后缀必须为 .bin"); return false;
    }
    if (info.size() <= 0) { *errorMessage = tr("固件文件为空"); return false; }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        *errorMessage = tr("文件无法读取：%1").arg(file.errorString()); return false;
    }
    return true;
}

void CescToolPage::startUpload()
{
    QString error;
    if (!validateFirmware(m_filePath->text(), &error)) {
        m_firmwareValid = false; m_status->setText(error); updateActions(); return;
    }
    auto *manager = m_context->connectionManager();
    const QFileInfo info(m_filePath->text());
    const QString prompt = tr("即将通过 %1 烧录 %2（%3 KB）。\n烧录过程中请勿断开设备或关闭串口。\n是否继续？")
        .arg(manager->deviceName(), info.fileName())
        .arg((info.size() + 1023) / 1024);
    if (QMessageBox::question(this, tr("确认烧录固件"), prompt,
                              QMessageBox::Yes | QMessageBox::Cancel,
                              QMessageBox::Cancel) != QMessageBox::Yes) return;
    QFile file(m_filePath->text());
    if (!file.open(QIODevice::ReadOnly)) {
        m_context->notify(tr("文件读取失败：%1").arg(file.errorString()), NotificationType::Error);
        return;
    }
    m_uploader->start(file.readAll());
    updateActions();
}

void CescToolPage::updateConnectionUi()
{
    auto *manager = m_context->connectionManager();
    const SerialConfig serial = m_context->settings()->serialConfig();
    const bool serialConnected = manager->state() == ConnectionState::Connected
        && manager->transportType() == TransportType::SerialPort;
    m_portValue->setText(serialConnected ? manager->deviceName()
                                         : (serial.portName.isEmpty() ? tr("未选择") : serial.portName));
    m_baudValue->setText(QString::number(serial.baudRate));
    const auto sessionState = m_context->cescSession()->state();
    m_connectionValue->setText(!serialConnected ? tr("传输未连接")
        : sessionState == CescSession::State::Ready ? tr("CESC 已就绪")
        : sessionState == CescSession::State::Negotiating ? tr("CESC 协商中")
        : tr("传输已连接 / CESC 未就绪"));
    m_versionValue->setText(m_uploader->firmwareVersion().isEmpty()
        ? tr("未知") : m_uploader->firmwareVersion());
    m_connectionButton->setText(serialConnected ? tr("断开") : tr("连接"));
}

void CescToolPage::updateActions()
{
    auto *manager = m_context->connectionManager();
    const bool busy = m_uploader->isBusy();
    const bool serialReady = manager->canSend()
        && manager->transportType() == TransportType::SerialPort
        && m_context->cescSession()->isReady()
        && (m_context->cescSession()->capabilities() & Cesc::FirmwareUpdate);
    m_chooseButton->setEnabled(!busy);
    m_cancelButton->setEnabled(busy);
    m_uploadButton->setEnabled(!busy && m_firmwareValid && serialReady);
    m_connectionButton->setEnabled(!busy
        && manager->state() != ConnectionState::Disconnecting);
    const bool telemetryReady = !busy && m_context->cescSession()->isReady()
        && (m_context->cescSession()->capabilities() & Cesc::TelemetryStreaming);
    m_queryAngleButton->setEnabled(!busy && m_context->cescSession()->isReady()
        && (m_context->cescSession()->capabilities() & Cesc::SensorService));
    m_telemetryButton->setEnabled(telemetryReady);
    m_telemetryButton->setText(m_angleStreamId == 0U
        ? tr("Start telemetry") : tr("Stop telemetry"));
}
