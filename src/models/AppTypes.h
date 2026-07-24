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
    Error
};

Q_DECLARE_METATYPE(ThemeMode)
Q_DECLARE_METATYPE(NavigationMode)
Q_DECLARE_METATYPE(ConnectionState)
