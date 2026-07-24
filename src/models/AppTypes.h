#pragma once

#include <QMetaType>

enum class ThemeMode {
    Light,
    Dark
};

enum class NavigationMode {
    Automatic,
    Compact,
    Expanded
};

enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Disconnecting,
    Error
};

enum class NotificationType {
    Information,
    Success,
    Warning,
    Error
};

Q_DECLARE_METATYPE(ThemeMode)
Q_DECLARE_METATYPE(NavigationMode)
Q_DECLARE_METATYPE(ConnectionState)
Q_DECLARE_METATYPE(NotificationType)
