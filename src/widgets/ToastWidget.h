#pragma once

#include <QWidget>

class IconManager;
class QLabel;
class QProgressBar;
class QPropertyAnimation;

class ToastWidget final : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)

public:
    enum class Type { Information, Success, Warning, Error };

    ToastWidget(IconManager *iconManager,
                const QString &title,
                const QString &message,
                Type type = Type::Information,
                QWidget *parent = nullptr);

    [[nodiscard]] qreal progress() const noexcept;
    void setProgress(qreal progress);
    void startCountdown(int durationMs = 3200);

signals:
    void closeRequested(ToastWidget *toast);

private:
    [[nodiscard]] QColor accentColor() const;
    [[nodiscard]] QColor backgroundColor() const;
    [[nodiscard]] QColor borderColor() const;
    [[nodiscard]] QColor titleColor() const;
    [[nodiscard]] QColor messageColor() const;

    Type m_type;
    bool m_darkTheme{};
    QProgressBar *m_progressBar{};
    QPropertyAnimation *m_countdown{};
    qreal m_progress{1.0};
};
