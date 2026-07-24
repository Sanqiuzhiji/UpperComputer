#include "IconManager.h"

#include <QPainter>
#include <QSvgRenderer>

IconManager::IconManager(ThemeManager *themeManager, QObject *parent)
    : QObject(parent),
      m_themeManager(themeManager)
{
    connect(m_themeManager, &ThemeManager::themeChanged, this,
            [this] {
                clearCache();
                emit iconsChanged();
            });
}

QIcon IconManager::icon(const QString &resourcePath, const QColor &activeColor) const
{
    return buildIcon(resourcePath, 0.0, activeColor);
}

QIcon IconManager::rotatedIcon(const QString &resourcePath,
                               const qreal degrees,
                               const QColor &activeColor) const
{
    return buildIcon(resourcePath, degrees, activeColor);
}

QPixmap IconManager::pixmap(const QString &resourcePath,
                            const QSize &size,
                            const QColor &color) const
{
    return renderedPixmap(resourcePath, size, color, 0.0);
}

QIcon IconManager::buildIcon(const QString &resourcePath,
                             const qreal degrees,
                             const QColor &activeColor) const
{
    const QColor normal = normalColor();
    const QColor disabled = disabledColor();
    const QString key = QStringLiteral("%1|%2|%3|%4|%5")
        .arg(resourcePath)
        .arg(normal.rgba())
        .arg(activeColor.rgba())
        .arg(disabled.rgba())
        .arg(degrees, 0, 'g', 12);
    if (const auto iterator = m_iconCache.constFind(key);
        iterator != m_iconCache.cend()) {
        return iterator.value();
    }

    constexpr QSize size(24, 24);
    QIcon result;
    result.addPixmap(
        renderedPixmap(resourcePath, size, normal, degrees), QIcon::Normal);
    result.addPixmap(
        renderedPixmap(resourcePath, size, activeColor, degrees), QIcon::Active);
    result.addPixmap(
        renderedPixmap(resourcePath, size, activeColor, degrees), QIcon::Selected);
    result.addPixmap(
        renderedPixmap(resourcePath, size, activeColor, degrees),
        QIcon::Normal, QIcon::On);
    result.addPixmap(
        renderedPixmap(resourcePath, size, activeColor, degrees),
        QIcon::Active, QIcon::On);
    result.addPixmap(
        renderedPixmap(resourcePath, size, disabled, degrees), QIcon::Disabled);
    m_iconCache.insert(key, result);
    return result;
}

QPixmap IconManager::renderedPixmap(const QString &resourcePath,
                                    const QSize &size,
                                    const QColor &color,
                                    const qreal degrees) const
{
    const QString key = QStringLiteral("%1|%2x%3|%4|%5")
        .arg(resourcePath)
        .arg(size.width())
        .arg(size.height())
        .arg(color.rgba())
        .arg(degrees, 0, 'g', 12);
    if (const auto iterator = m_pixmapCache.constFind(key);
        iterator != m_pixmapCache.cend()) {
        return iterator.value();
    }

    QPixmap tinted(size);
    tinted.fill(Qt::transparent);
    QPainter painter(&tinted);
    QSvgRenderer renderer(resourcePath);
    renderer.render(&painter, QRectF(QPointF(0, 0), QSizeF(size)));
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(tinted.rect(), color);
    painter.end();

    if (!qFuzzyIsNull(degrees)) {
        QTransform transform;
        transform.rotate(degrees);
        tinted = tinted.transformed(transform, Qt::SmoothTransformation);
    }
    m_pixmapCache.insert(key, tinted);
    return tinted;
}

void IconManager::clearCache()
{
    m_iconCache.clear();
    m_pixmapCache.clear();
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
