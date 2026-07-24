#pragma once

#include <QByteArray>
#include <QObject>
#include <QSettings>

#include "models/AppTypes.h"
#include "pages/PageId.h"

class AppSettings final : public QObject
{
    Q_OBJECT

public:
    explicit AppSettings(QObject *parent = nullptr);

    [[nodiscard]] ThemeMode themeMode() const noexcept;
    [[nodiscard]] NavigationMode navigationMode() const noexcept;
    [[nodiscard]] bool userCardVisible() const noexcept;
    [[nodiscard]] PageId lastPage() const noexcept;
    [[nodiscard]] QByteArray windowGeometry() const;
    [[nodiscard]] bool windowMaximized() const noexcept;

public slots:
    void setThemeMode(ThemeMode mode);
    void setNavigationMode(NavigationMode mode);
    void setUserCardVisible(bool visible);
    void setLastPage(PageId page);
    void setWindowGeometry(const QByteArray &geometry);
    void setWindowMaximized(bool maximized);

private:
    void load();

    QSettings m_store;
    ThemeMode m_themeMode{ThemeMode::Dark};
    NavigationMode m_navigationMode{NavigationMode::Expanded};
    bool m_userCardVisible{true};
    PageId m_lastPage{PageId::Plot};
    QByteArray m_windowGeometry;
    bool m_windowMaximized{true};
};
