#pragma once

#include <QWidget>

#include "models/AppTypes.h"

class AppContext;
class AppSettings;
class QCheckBox;
class QComboBox;
class ThemeManager;

class SettingsPage final : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(AppContext *context, QWidget *parent = nullptr);

signals:
    void themeModeRequested(ThemeMode mode);
    void userCardVisibilityChanged(bool visible);
    void navigationModeChanged(NavigationMode mode);
    void unavailableSettingRequested(const QString &setting);

private:
    QWidget *createSettingCard(const QString &title,
                               const QString &description,
                               QWidget *control);
    QComboBox *createOptions(const QStringList &options);

    AppSettings *m_settings;
    ThemeManager *m_themeManager;
    QComboBox *m_themeCombo{};
};
