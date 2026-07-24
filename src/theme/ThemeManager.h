#pragma once

#include <QObject>
#include <QString>

#include "models/AppTypes.h"

class AppSettings;

class ThemeManager final : public QObject
{
    Q_OBJECT

public:
    explicit ThemeManager(AppSettings *settings, QObject *parent = nullptr);

    [[nodiscard]] ThemeMode mode() const noexcept;
    [[nodiscard]] QString modeName() const;

public slots:
    void setMode(ThemeMode mode);
    void toggleMode();

signals:
    void themeChanged(ThemeMode mode);

private:
    void apply() const;
    [[nodiscard]] QString styleSheet() const;

    AppSettings *m_settings;
    ThemeMode m_mode{ThemeMode::Dark};
};
