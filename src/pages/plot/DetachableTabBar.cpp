#include "DetachableTabBar.h"

#include <QApplication>
#include <QMouseEvent>

DetachableTabBar::DetachableTabBar(QWidget *parent)
    : QTabBar(parent)
{
    setMovable(true);
}

void DetachableTabBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressedIndex = tabAt(event->position().toPoint());
        m_pressPosition = event->position().toPoint();
        m_detached = false;
    }
    QTabBar::mousePressEvent(event);
}

void DetachableTabBar::mouseMoveEvent(QMouseEvent *event)
{
    QTabBar::mouseMoveEvent(event);
    if (m_pressedIndex < 0 || m_detached
        || !(event->buttons() & Qt::LeftButton)) {
        return;
    }
    const QPoint position = event->position().toPoint();
    const bool outsideVertically =
        position.y() < -60 || position.y() > height() + 60;
    if (outsideVertically
        && (position - m_pressPosition).manhattanLength()
            >= QApplication::startDragDistance() * 3) {
        m_detached = true;
        emit detachRequested(
            m_pressedIndex, event->globalPosition().toPoint());
    }
}

void DetachableTabBar::mouseReleaseEvent(QMouseEvent *event)
{
    m_pressedIndex = -1;
    m_detached = false;
    QTabBar::mouseReleaseEvent(event);
}
