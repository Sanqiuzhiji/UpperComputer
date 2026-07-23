#include "IconManager.h"

#include <QPainter>
#include <QSvgRenderer>

IconManager::IconManager(ThemeManager *themeManager, QObject *parent)
    : QObject(parent),
      m_themeManager(themeManager)
{
    connect(m_themeManager, &ThemeManager::themeChanged,
            this, &IconManager::iconsChanged);
}

QIcon IconManager::icon(const QString &resourcePath, const QColor &activeColor) const
{
    constexpr QSize size(24, 24);
    QIcon result;
    result.addPixmap(pixmap(resourcePath, size, normalColor()), QIcon::Normal);
    result.addPixmap(pixmap(resourcePath, size, activeColor), QIcon::Active);
    result.addPixmap(pixmap(resourcePath, size, activeColor), QIcon::Selected);
    result.addPixmap(pixmap(resourcePath, size, disabledColor()), QIcon::Disabled);
    return result;
}

QIcon IconManager::rotatedIcon(const QString &resourcePath,
                               const qreal degrees,
                               const QColor &activeColor) const
{
    const auto rotated = [degrees](const QPixmap &source) {
        QTransform transform;
        transform.rotate(degrees);
        return source.transformed(transform, Qt::SmoothTransformation);
    };
    constexpr QSize size(24, 24);
    QIcon result;
    result.addPixmap(rotated(pixmap(resourcePath, size, normalColor())), QIcon::Normal);
    result.addPixmap(rotated(pixmap(resourcePath, size, activeColor)), QIcon::Active);
    result.addPixmap(rotated(pixmap(resourcePath, size, activeColor)), QIcon::Selected);
    result.addPixmap(rotated(pixmap(resourcePath, size, disabledColor())), QIcon::Disabled);
    return result;
}

QPixmap IconManager::pixmap(const QString &resourcePath,
                            const QSize &size,
                            const QColor &color) const
{
    QPixmap tinted(size);
    tinted.fill(Qt::transparent);
    QPainter painter(&tinted);
    QSvgRenderer renderer(resourcePath);
    renderer.render(&painter, QRectF(QPointF(0, 0), QSizeF(size)));
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(tinted.rect(), color);
    return tinted;
}

QColor IconManager::normalColor() const
{
    return m_themeManager->mode() == ThemeMode::Dark
        ? QColor("#E6E8EB") : QColor("#202329");
}

bool IconManager::isDarkTheme() const noexcept
{
    return m_themeManager->mode() == ThemeMode::Dark;
}

QColor IconManager::disabledColor() const
{
    return m_themeManager->mode() == ThemeMode::Dark
        ? QColor("#656A73") : QColor("#A8ADB5");
}
