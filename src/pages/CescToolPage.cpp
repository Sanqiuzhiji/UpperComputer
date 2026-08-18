#include "CescToolPage.h"

#include "app/AppContext.h"
#include "models/AppTypes.h"
#include "services/CescFirmwareUploader.h"
#include "services/ConnectionManager.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

CescToolPage::CescToolPage(AppContext *context, QWidget *parent)
    : QWidget(parent), m_context(context),
      m_uploader(new CescFirmwareUploader(context->connectionManager(), this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 24, 28, 28);
    layout->setSpacing(16);
    auto *title = new QLabel(QStringLiteral("CESC Tool"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(tr("通过当前通信连接为下位机刷写 VESC/CESC 固件"), this);
    subtitle->setProperty("muted", true);
    layout->addWidget(title);
    layout->addWidget(subtitle);

    auto *card = new QFrame(this);
    card->setProperty("card", true);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(24, 24, 24, 24);
    cardLayout->setSpacing(14);
    auto *section = new QLabel(tr("固件刷写"), card);
    section->setObjectName(QStringLiteral("sectionTitle"));
    cardLayout->addWidget(section);

    auto *fileRow = new QHBoxLayout;
    m_filePath = new QLineEdit(card);
    m_filePath->setReadOnly(true);
    m_filePath->setPlaceholderText(tr("请选择 .bin 固件文件"));
    m_chooseButton = new QPushButton(tr("选择文件"), card);
    fileRow->addWidget(m_filePath, 1);
    fileRow->addWidget(m_chooseButton);
    cardLayout->addLayout(fileRow);
    m_fileInfo = new QLabel(tr("尚未选择固件"), card);
    m_fileInfo->setProperty("muted", true);
    cardLayout->addWidget(m_fileInfo);

    m_progress = new QProgressBar(card);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_status = new QLabel(tr("等待操作"), card);
    m_status->setProperty("muted", true);
    cardLayout->addWidget(m_progress);
    cardLayout->addWidget(m_status);

    auto *buttons = new QHBoxLayout;
    buttons->addStretch();
    m_cancelButton = new QPushButton(tr("取消"), card);
    m_uploadButton = new QPushButton(tr("开始刷写"), card);
    m_uploadButton->setProperty("accent", true);
    buttons->addWidget(m_cancelButton);
    buttons->addWidget(m_uploadButton);
    cardLayout->addLayout(buttons);
    auto *warning = new QLabel(
        tr("刷写期间请勿断电或断开连接。选择错误的硬件固件可能导致设备无法启动。"), card);
    warning->setWordWrap(true);
    warning->setProperty("muted", true);
    cardLayout->addWidget(warning);
    layout->addWidget(card);
    layout->addStretch();

    connect(m_chooseButton, &QPushButton::clicked, this, &CescToolPage::chooseFirmware);
    connect(m_uploadButton, &QPushButton::clicked, this, &CescToolPage::startUpload);
    connect(m_cancelButton, &QPushButton::clicked, m_uploader, &CescFirmwareUploader::cancel);
    connect(context->connectionManager(), &ConnectionManager::stateChanged,
            this, [this] { updateActions(); });
    connect(m_uploader, &CescFirmwareUploader::progressChanged, this,
            [this](int progress, const QString &status) {
                m_progress->setValue(progress); m_status->setText(status);
            });
    connect(m_uploader, &CescFirmwareUploader::finished, this,
            [this](bool success, const QString &message) {
                m_status->setText(message);
                if (success) m_progress->setValue(100);
                m_context->notify(message, success ? NotificationType::Success
                                                   : NotificationType::Error);
                updateActions();
            });
    updateActions();
}

void CescToolPage::chooseFirmware()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("选择固件文件"), QString(), tr("固件文件 (*.bin);;所有文件 (*)"));
    if (path.isEmpty()) return;
    const QFileInfo info(path);
    m_filePath->setText(path);
    m_fileInfo->setText(tr("%1 · %2 KB").arg(info.fileName())
                            .arg((info.size() + 1023) / 1024));
    m_progress->setValue(0);
    m_status->setText(tr("固件已就绪"));
    updateActions();
}

void CescToolPage::startUpload()
{
    if (QMessageBox::warning(
            this, tr("确认刷写固件"),
            tr("刷写会覆盖下位机当前固件。请确认文件与硬件型号匹配，并确保刷写期间设备持续供电。"),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel)
        != QMessageBox::Yes) {
        return;
    }
    QFile file(m_filePath->text());
    if (!file.open(QIODevice::ReadOnly)) {
        m_context->notify(tr("无法读取所选固件文件"), NotificationType::Error);
        return;
    }
    m_uploader->start(file.readAll());
    updateActions();
}

void CescToolPage::updateActions()
{
    const bool busy = m_uploader->isBusy();
    m_chooseButton->setEnabled(!busy);
    m_cancelButton->setEnabled(busy);
    m_uploadButton->setEnabled(!busy && !m_filePath->text().isEmpty()
                                && m_context->connectionManager()->canSend());
}
