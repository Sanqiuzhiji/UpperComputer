#pragma once

#include <QObject>
#include <QString>

enum class ThemeMode {
    Dark,
    Light
};

class ThemeManager final : public QObject
{
    Q_OBJECT

public:
    explicit ThemeManager(QObject *parent = nullptr);

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

    ThemeMode m_mode{ThemeMode::Dark};
};

Q_DECLARE_METATYPE(ThemeMode)
