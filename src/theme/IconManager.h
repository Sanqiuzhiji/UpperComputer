#pragma once

#include <QIcon>
#include <QObject>

#include "ThemeManager.h"

class IconManager final : public QObject
{
    Q_OBJECT

public:
    explicit IconManager(ThemeManager *themeManager, QObject *parent = nullptr);

    [[nodiscard]] QIcon icon(const QString &resourcePath,
                             const QColor &activeColor = QColor("#28A9E0")) const;
    [[nodiscard]] QIcon rotatedIcon(const QString &resourcePath,
                                    qreal degrees,
                                    const QColor &activeColor = QColor("#28A9E0")) const;
    [[nodiscard]] QPixmap pixmap(const QString &resourcePath,
                                 const QSize &size,
                                 const QColor &color) const;
    [[nodiscard]] bool isDarkTheme() const noexcept;

signals:
    void iconsChanged();

private:
    [[nodiscard]] QColor normalColor() const;
    [[nodiscard]] QColor disabledColor() const;

    ThemeManager *m_themeManager;
};
