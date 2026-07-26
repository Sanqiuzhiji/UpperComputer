#include "AppContext.h"

#include "app/AppSettings.h"
#include "services/ConnectionManager.h"
#include "services/ProtocolRepository.h"
#include "theme/IconManager.h"
#include "theme/ThemeManager.h"

AppContext::AppContext(QObject *parent)
    : QObject(parent),
      m_settings(new AppSettings(this)),
      m_connectionManager(new ConnectionManager(this)),
      m_themeManager(new ThemeManager(m_settings, this)),
      m_iconManager(new IconManager(m_themeManager, this)),
      m_protocolRepository(new ProtocolRepository(this))
{
    connect(m_protocolRepository,
            &ProtocolRepository::notificationRequested,
            this, &AppContext::notify,
            Qt::QueuedConnection);
    m_protocolRepository->rescan();
}

AppSettings *AppContext::settings() const noexcept
{
    return m_settings;
}

ConnectionManager *AppContext::connectionManager() const noexcept
{
    return m_connectionManager;
}

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

void AppContext::notify(const QString &message, const NotificationType type)
{
    emit notificationRequested(message, type);
}
