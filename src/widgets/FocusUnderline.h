#pragma once

#include <QObject>

class QPropertyAnimation;
class ThemeManager;
class QWidget;

class FocusUnderline final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)

public:
    explicit FocusUnderline(QWidget *target,
                            ThemeManager *themeManager);

    [[nodiscard]] qreal progress() const noexcept;
    void setProgress(qreal progress);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void animateTo(qreal target);
    void updateGeometry();
    void updateColor();

    QWidget *m_target;
    QWidget *m_line;
    ThemeManager *m_themeManager;
    QPropertyAnimation *m_animation;
    qreal m_progress{};
};
