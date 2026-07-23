#include "ThemeTransitionOverlay.h"

#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>

ThemeTransitionOverlay::ThemeTransitionOverlay(const QPixmap &oldFrame,
                                               const QPixmap &newFrame,
                                               const QPoint &center,
                                               QWidget *parent)
    : QWidget(parent),
      m_oldFrame(oldFrame),
      m_newFrame(newFrame),
      m_center(center)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setMouseTracking(true);
}

qreal ThemeTransitionOverlay::radius() const noexcept
{
    return m_radius;
}

void ThemeTransitionOverlay::setRadius(const qreal radius)
{
    if (qFuzzyCompare(m_radius, radius)) {
        return;
    }
    m_radius = radius;
    update();
}

void ThemeTransitionOverlay::setFrames(const QPixmap &oldFrame,
                                       const QPixmap &newFrame)
{
    m_oldFrame = oldFrame;
    m_newFrame = newFrame;
    update();
}

void ThemeTransitionOverlay::start(const qreal maximumRadius, const int duration)
{
    if (m_animation) {
        m_animation->stop();
        m_animation->deleteLater();
    }
    m_animation = new QPropertyAnimation(this, "radius", this);
    m_animation->setDuration(duration);
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(maximumRadius);
    m_animation->setEasingCurve(QEasingCurve::InOutCubic);
    connect(m_animation, &QPropertyAnimation::finished,
            this, &ThemeTransitionOverlay::transitionFinished);
    m_animation->start();
}

void ThemeTransitionOverlay::cancel()
{
    if (m_animation) {
        m_animation->stop();
    }
    emit transitionFinished();
}

void ThemeTransitionOverlay::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawPixmap(rect(), m_oldFrame, m_oldFrame.rect());

    QPainterPath reveal;
    reveal.addEllipse(QPointF(m_center), m_radius, m_radius);
    painter.save();
    painter.setClipPath(reveal);
    painter.drawPixmap(rect(), m_newFrame, m_newFrame.rect());
    painter.restore();
}
