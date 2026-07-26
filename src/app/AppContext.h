#pragma once

#include <QObject>
#include <QString>

#include "models/AppTypes.h"

class AppSettings;
class ConnectionManager;
class IconManager;
class ProtocolRepository;
class ThemeManager;

class AppContext final : public QObject
{
    Q_OBJECT

public:
    explicit AppContext(QObject *parent = nullptr);

    [[nodiscard]] AppSettings *settings() const noexcept;
    [[nodiscard]] ConnectionManager *connectionManager() const noexcept;
    [[nodiscard]] ThemeManager *themeManager() const noexcept;
    [[nodiscard]] IconManager *iconManager() const noexcept;
    [[nodiscard]] ProtocolRepository *protocolRepository() const noexcept;

    void notify(const QString &message,
                NotificationType type = NotificationType::Information);

signals:
    void notificationRequested(const QString &message, NotificationType type);

private:
    AppSettings *m_settings{};
    ConnectionManager *m_connectionManager{};
    ThemeManager *m_themeManager{};
    IconManager *m_iconManager{};
    ProtocolRepository *m_protocolRepository{};
};
