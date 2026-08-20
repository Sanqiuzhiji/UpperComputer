#include "AppContext.h"

#include "app/AppSettings.h"
#include "services/ChannelDataHub.h"
#include "services/ConnectionManager.h"
#include "services/CescFirmwareUploader.h"
#include "services/cesc/CescSession.h"
#include "services/cesc/CescSystemClient.h"
#include "services/cesc/CescSensorClient.h"
#include "services/cesc/CescTelemetryClient.h"
#include "services/ProtocolRepository.h"
#include "services/ReceiveDataPipeline.h"
#include "theme/IconManager.h"
#include "theme/ThemeManager.h"

#include <QDir>

AppContext::AppContext(QObject *parent)
    : QObject(parent),
      m_settings(new AppSettings(this)),
      m_connectionManager(new ConnectionManager(this)),
      m_cescSession(new CescSession(m_connectionManager, this)),
      m_cescFirmwareUploader(new CescFirmwareUploader(
          m_connectionManager, m_settings, m_cescSession, this)),
      m_protocolRepository(new ProtocolRepository(
          this, QDir(m_settings->workspaceDirectory())
                    .filePath(QStringLiteral("protocols")))),
      m_channelDataHub(new ChannelDataHub(this)),
      m_receiveDataPipeline(new ReceiveDataPipeline(
          m_settings,
          m_connectionManager,
          m_protocolRepository,
          m_channelDataHub,
          this)),
      m_themeManager(new ThemeManager(m_settings, this)),
      m_iconManager(new IconManager(m_themeManager, this))
{
    m_cescSystemClient = new CescSystemClient(m_cescSession, this);
    m_cescSensorClient = new CescSensorClient(m_cescSession, this);
    m_cescTelemetryClient = new CescTelemetryClient(
        m_cescSession, m_channelDataHub, this);
    connect(m_protocolRepository,
            &ProtocolRepository::notificationRequested,
            this, &AppContext::notify,
            Qt::QueuedConnection);
    m_protocolRepository->rescan();
    connect(m_settings, &AppSettings::workspaceDirectoryChanged,
            m_protocolRepository, [this](const QString &directory) {
                m_protocolRepository->setDirectoryPath(
                    QDir(directory).filePath(QStringLiteral("protocols")));
            });
}

AppSettings *AppContext::settings() const noexcept
{
    return m_settings;
}

ConnectionManager *AppContext::connectionManager() const noexcept
{
    return m_connectionManager;
}

AppContext::~AppContext()
{
    // The uploader keeps non-owning pointers to settings and connection.
    // Destroy it before QObject tears down the AppContext children.
    delete m_cescFirmwareUploader;
    m_cescFirmwareUploader = nullptr;
}

CescFirmwareUploader *AppContext::cescFirmwareUploader() const noexcept
{
    return m_cescFirmwareUploader;
}

CescSession *AppContext::cescSession() const noexcept { return m_cescSession; }
CescSystemClient *AppContext::cescSystemClient() const noexcept { return m_cescSystemClient; }
CescSensorClient *AppContext::cescSensorClient() const noexcept { return m_cescSensorClient; }
CescTelemetryClient *AppContext::cescTelemetryClient() const noexcept { return m_cescTelemetryClient; }

ThemeManager *AppContext::themeManager() const noexcept
{
    return m_themeManager;
}

IconManager *AppContext::iconManager() const noexcept
{
    return m_iconManager;
}

ProtocolRepository *AppContext::protocolRepository() const noexcept
{
    return m_protocolRepository;
}

ChannelDataHub *AppContext::channelDataHub() const noexcept
{
    return m_channelDataHub;
}

ReceiveDataPipeline *AppContext::receiveDataPipeline() const noexcept
{
    return m_receiveDataPipeline;
}

void AppContext::notify(const QString &message, const NotificationType type)
{
    emit notificationRequested(message, type);
}
