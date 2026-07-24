#include "AppContext.h"

#include "app/AppSettings.h"
#include "services/ConnectionManager.h"
#include "theme/IconManager.h"
#include "theme/ThemeManager.h"

AppContext::AppContext(QObject *parent)
    : QObject(parent),
      m_settings(new AppSettings(this)),
      m_connectionManager(new ConnectionManager(this)),
      m_themeManager(new ThemeManager(m_settings, this)),
      m_iconManager(new IconManager(m_themeManager, this))
{
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

void AppContext::notify(const QString &message, const NotificationType type)
{
    emit notificationRequested(message, type);
}
