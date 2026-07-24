#include "AppSettings.h"

namespace {
constexpr auto kThemeModeKey = "appearance/themeMode";
constexpr auto kNavigationModeKey = "navigation/mode";
constexpr auto kUserCardVisibleKey = "navigation/userCardVisible";
constexpr auto kLastPageKey = "navigation/lastPage";
constexpr auto kWindowGeometryKey = "window/geometry";
constexpr auto kWindowMaximizedKey = "window/maximized";

template<typename Enum>
int enumValue(const Enum value)
{
    return static_cast<int>(value);
}
}

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
{
    load();
}

ThemeMode AppSettings::themeMode() const noexcept
{
    return m_themeMode;
}

NavigationMode AppSettings::navigationMode() const noexcept
{
    return m_navigationMode;
}

bool AppSettings::userCardVisible() const noexcept
{
    return m_userCardVisible;
}

PageId AppSettings::lastPage() const noexcept
{
    return m_lastPage;
}

QByteArray AppSettings::windowGeometry() const
{
    return m_windowGeometry;
}

bool AppSettings::windowMaximized() const noexcept
{
    return m_windowMaximized;
}

void AppSettings::setThemeMode(const ThemeMode mode)
{
    if (m_themeMode == mode) {
        return;
    }
    m_themeMode = mode;
    m_store.setValue(QLatin1String(kThemeModeKey), enumValue(mode));
}

void AppSettings::setNavigationMode(const NavigationMode mode)
{
    if (m_navigationMode == mode) {
        return;
    }
    m_navigationMode = mode;
    m_store.setValue(QLatin1String(kNavigationModeKey), enumValue(mode));
}

void AppSettings::setUserCardVisible(const bool visible)
{
    if (m_userCardVisible == visible) {
        return;
    }
    m_userCardVisible = visible;
    m_store.setValue(QLatin1String(kUserCardVisibleKey), visible);
}

void AppSettings::setLastPage(const PageId page)
{
    if (m_lastPage == page) {
        return;
    }
    m_lastPage = page;
    m_store.setValue(QLatin1String(kLastPageKey), enumValue(page));
}

void AppSettings::setWindowGeometry(const QByteArray &geometry)
{
    if (m_windowGeometry == geometry) {
        return;
    }
    m_windowGeometry = geometry;
    m_store.setValue(QLatin1String(kWindowGeometryKey), geometry);
}

void AppSettings::setWindowMaximized(const bool maximized)
{
    if (m_windowMaximized == maximized) {
        return;
    }
    m_windowMaximized = maximized;
    m_store.setValue(QLatin1String(kWindowMaximizedKey), maximized);
}

void AppSettings::load()
{
    const int theme = m_store.value(
        QLatin1String(kThemeModeKey), enumValue(ThemeMode::Dark)).toInt();
    if (theme == enumValue(ThemeMode::Light)
        || theme == enumValue(ThemeMode::Dark)) {
        m_themeMode = static_cast<ThemeMode>(theme);
    }

    const int navigation = m_store.value(
        QLatin1String(kNavigationModeKey),
        enumValue(NavigationMode::Expanded)).toInt();
    if (navigation >= enumValue(NavigationMode::Automatic)
        && navigation <= enumValue(NavigationMode::Expanded)) {
        m_navigationMode = static_cast<NavigationMode>(navigation);
    }

    m_userCardVisible = m_store.value(
        QLatin1String(kUserCardVisibleKey), true).toBool();

    const int page = m_store.value(
        QLatin1String(kLastPageKey), enumValue(PageId::Plot)).toInt();
    if (page >= enumValue(PageId::Plot)
        && page <= enumValue(PageId::Settings)) {
        m_lastPage = static_cast<PageId>(page);
    }

    m_windowGeometry =
        m_store.value(QLatin1String(kWindowGeometryKey)).toByteArray();
    m_windowMaximized = m_store.value(
        QLatin1String(kWindowMaximizedKey), true).toBool();
}
