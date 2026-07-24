#pragma once

#include <QObject>

class AppSettings;
class ConnectionManager;
class ThemeManager;

class AppContext final : public QObject
{
    Q_OBJECT

public:
    explicit AppContext(QObject *parent = nullptr);

    [[nodiscard]] AppSettings *settings() const noexcept;
    [[nodiscard]] ConnectionManager *connectionManager() const noexcept;
    [[nodiscard]] ThemeManager *themeManager() const noexcept;

private:
    AppSettings *m_settings{};
    ConnectionManager *m_connectionManager{};
    ThemeManager *m_themeManager{};
};
