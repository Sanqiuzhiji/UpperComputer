#include "FocusUnderline.h"

#include "theme/ThemeManager.h"

#include <QEvent>
#include <QFocusEvent>
#include <QPropertyAnimation>
#include <QWidget>

FocusUnderline::FocusUnderline(QWidget *target, ThemeManager *themeManager)
    : QObject(target),
      m_target(target),
      m_line(new QWidget(target)),
      m_themeManager(themeManager),
      m_animation(new QPropertyAnimation(this, "progress", this))
{
    m_line->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_line->hide();
    m_animation->setDuration(160);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_animation, &QPropertyAnimation::finished, this, [this] {
        if (qFuzzyIsNull(m_progress)) {
            m_line->hide();
        }
    });
    target->installEventFilter(this);
    connect(themeManager, &ThemeManager::themeChanged,
            this, &FocusUnderline::updateColor);
    updateColor();
}

qreal FocusUnderline::progress() const noexcept
{
    return m_progress;
}

void FocusUnderline::setProgress(const qreal progress)
{
    m_progress = qBound(0.0, progress, 1.0);
    updateGeometry();
}

bool FocusUnderline::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != m_target) {
        return QObject::eventFilter(watched, event);
    }
    switch (event->type()) {
    case QEvent::FocusIn:
        m_line->show();
        m_line->raise();
        animateTo(1.0);
        break;
    case QEvent::FocusOut:
        if (static_cast<QFocusEvent *>(event)->reason()
            == Qt::PopupFocusReason) {
            break;
        }
        animateTo(0.0);
        break;
    case QEvent::Resize:
    case QEvent::Move:
    case QEvent::Show:
        updateGeometry();
        break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

void FocusUnderline::animateTo(const qreal target)
{
    m_animation->stop();
    m_animation->setStartValue(m_progress);
    m_animation->setEndValue(target);
    m_animation->start();
}

void FocusUnderline::updateGeometry()
{
    const int width = qRound(m_target->width() * m_progress);
    const int x = (m_target->width() - width) / 2;
    m_line->setGeometry(x, qMax(0, m_target->height() - 2), width, 2);
}

void FocusUnderline::updateColor()
{
    m_line->setStyleSheet(QStringLiteral("background:%1;")
                              .arg(m_themeManager->accentColor().name()));
}
