#pragma once

#include <QWidget>

#include "theme/ThemeManager.h"

class QCheckBox;
class QComboBox;

class SettingsPage final : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(ThemeManager *themeManager, QWidget *parent = nullptr);

signals:
    void userCardVisibilityChanged(bool visible);
    void navigationModeChanged(const QString &mode);
    void unavailableSettingRequested(const QString &setting);

private:
    QWidget *createSettingCard(const QString &title,
                               const QString &description,
                               QWidget *control);
    QComboBox *createOptions(const QStringList &options);

    ThemeManager *m_themeManager;
    QComboBox *m_themeCombo{};
};
