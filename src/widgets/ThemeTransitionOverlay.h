#pragma once

#include <QPixmap>
#include <QWidget>

class QPropertyAnimation;

class ThemeTransitionOverlay final : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal radius READ radius WRITE setRadius)

public:
    ThemeTransitionOverlay(const QPixmap &oldFrame,
                           const QPixmap &newFrame,
                           const QPoint &center,
                           QWidget *parent = nullptr);

    [[nodiscard]] qreal radius() const noexcept;
    void setRadius(qreal radius);
    void setFrames(const QPixmap &oldFrame, const QPixmap &newFrame);
    void start(qreal maximumRadius, int duration = 620);
    void cancel();

signals:
    void transitionFinished();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap m_oldFrame;
    QPixmap m_newFrame;
    QPoint m_center;
    qreal m_radius{};
    QPropertyAnimation *m_animation{};
};
