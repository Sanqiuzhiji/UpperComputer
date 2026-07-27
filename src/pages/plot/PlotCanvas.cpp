#include "PlotCanvas.h"

#include "app/AppContext.h"
#include "pages/plot/DashboardItem.h"
#include "pages/plot/PlotTheme.h"
#include "pages/plot/RealtimePlotWidget.h"
#include "theme/ThemeManager.h"

#include <QContextMenuEvent>
#include <QMenu>
#include <QPainter>
#include <QResizeEvent>
#include <QUuid>

PlotCanvas::PlotCanvas(AppContext *context, QWidget *parent)
    : QWidget(parent),
      m_context(context)
{
    setObjectName(QStringLiteral("plotCanvas"));
    setMinimumSize(360, 260);
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setAutoFillBackground(true);
    setPalette(PlotTheme::palette(context->themeManager()->mode()));
    connect(context->themeManager(), &ThemeManager::themeChanged,
            this, [this](const ThemeMode mode) {
                setPalette(PlotTheme::palette(mode));
                update();
            });
}

QList<RealtimePlotWidget *> PlotCanvas::plots() const
{
    QList<RealtimePlotWidget *> result;
    result.reserve(m_items.size());
    for (DashboardItem *item : m_items) {
        if (auto *plot = qobject_cast<RealtimePlotWidget *>(item->content())) {
            result.append(plot);
        }
    }
    return result;
}

bool PlotCanvas::editMode() const noexcept
{
    return m_editMode;
}

RealtimePlotWidget *PlotCanvas::addRealtimePlot()
{
    auto *plot = new RealtimePlotWidget(m_context, this);
    auto *item = new DashboardItem(plot, this);
    item->setProperty(
        "itemId", QUuid::createUuid().toString(QUuid::WithoutBraces));
    item->setGeometry(nextItemGeometry());
    item->setEditMode(m_editMode);
    plot->setPaused(m_paused);
    item->show();
    m_items.append(item);
    connect(item, &DashboardItem::deleteRequested, this,
            [this](DashboardItem *target) {
                if (!m_items.removeOne(target)) return;
                target->deleteLater();
            });
    connect(plot, &RealtimePlotWidget::deleteRequested, item,
            [item] { emit item->deleteRequested(item); });
    emit plotAdded(plot);
    return plot;
}

void PlotCanvas::setEditMode(const bool enabled)
{
    if (m_editMode == enabled) return;
    m_editMode = enabled;
    for (DashboardItem *item : m_items) item->setEditMode(enabled);
    update();
}

void PlotCanvas::setPaused(const bool paused)
{
    m_paused = paused;
    for (RealtimePlotWidget *plot : plots()) plot->setPaused(paused);
}

void PlotCanvas::resetPlots()
{
    for (RealtimePlotWidget *plot : plots()) plot->resetView();
}

void PlotCanvas::fitPlotsY()
{
    for (RealtimePlotWidget *plot : plots()) plot->fitYOnce();
}

void PlotCanvas::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    if (!m_editMode) return;
    QPainter painter(this);
    QColor grid = palette().color(QPalette::Mid);
    grid.setAlpha(70);
    painter.setPen(grid);
    constexpr int spacing = 20;
    for (int x = spacing; x < width(); x += spacing) {
        painter.drawLine(x, 0, x, height());
    }
    for (int y = spacing; y < height(); y += spacing) {
        painter.drawLine(0, y, width(), y);
    }
}

void PlotCanvas::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    for (DashboardItem *item : m_items) {
        QRect geometry = item->geometry();
        geometry.setWidth(qMin(geometry.width(), width()));
        geometry.setHeight(qMin(geometry.height(), height()));
        geometry.moveLeft(qBound(0, geometry.left(), width() - geometry.width()));
        geometry.moveTop(qBound(0, geometry.top(), height() - geometry.height()));
        item->setGeometry(geometry);
    }
}

void PlotCanvas::contextMenuEvent(QContextMenuEvent *event)
{
    if (childAt(event->pos()) != nullptr) {
        QWidget::contextMenuEvent(event);
        return;
    }
    QMenu menu(this);
    menu.setObjectName(QStringLiteral("plotCanvasContextMenu"));
    menu.setPalette(PlotTheme::palette(
        m_context->themeManager()->mode()));
    QAction *add = menu.addAction(tr("添加实时绘图"));
    if (menu.exec(event->globalPos()) == add) addRealtimePlot();
}

QRect PlotCanvas::nextItemGeometry() const
{
    constexpr QSize size(520, 320);
    for (int y = 20; y + size.height() <= qMax(height(), size.height());
         y += 40) {
        for (int x = 20; x + size.width() <= qMax(width(), size.width());
             x += 40) {
            const QRect candidate(x, y, size.width(), size.height());
            bool overlaps = false;
            for (DashboardItem *item : m_items) {
                if (item->geometry().intersects(candidate)) {
                    overlaps = true;
                    break;
                }
            }
            if (!overlaps) return candidate;
        }
    }
    const int offset = (m_items.size() * 40) % 200;
    return QRect(
        qMin(offset, qMax(0, width() - size.width())),
        qMin(offset, qMax(0, height() - size.height())),
        qMin(size.width(), qMax(320, width())),
        qMin(size.height(), qMax(220, height())));
}
