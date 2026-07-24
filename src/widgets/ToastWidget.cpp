#include "ToastWidget.h"

#include "theme/IconManager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
constexpr int kMinimumToastWidth = 280;
constexpr int kMaximumToastWidth = 420;
constexpr int kMinimumToastHeight = 82;
}

ToastWidget::ToastWidget(IconManager *iconManager,
                         const QString &title,
                         const QString &message,
                         const Type type,
                         QWidget *parent)
    : QWidget(parent),
      m_type(type),
      m_darkTheme(iconManager->isDarkTheme())
{
    setObjectName(QStringLiteral("toastWidget"));
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);
    setMinimumSize(kMinimumToastWidth, kMinimumToastHeight);
    setMaximumWidth(kMaximumToastWidth);
    setMaximumHeight(parent
        ? qMax(kMinimumToastHeight, parent->height() - 100)
        : 240);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    setStyleSheet(QStringLiteral(
        "QWidget#toastWidget{background:%1;border:1px solid %2;border-radius:9px;}"
        "QLabel#toastTitle{color:%3;background:transparent;}"
        "QLabel#toastMessage{color:%4;background:transparent;}"
        "QToolButton{background:transparent;border:none;border-radius:5px;}"
        "QToolButton:hover{background:rgba(128,128,128,32);}")
        .arg(backgroundColor().name(), borderColor().name(),
             titleColor().name(), messageColor().name()));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(14, 10, 10, 7);
    root->setSpacing(5);
    auto *header = new QHBoxLayout;
    header->setSpacing(8);

    auto *mark = new QLabel(this);
    mark->setFixedSize(9, 9);
    mark->setStyleSheet(QStringLiteral(
        "background:%1;border-radius:4px;").arg(accentColor().name()));
    auto *titleLabel = new QLabel(title, this);
    titleLabel->setObjectName(QStringLiteral("toastTitle"));
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    auto *closeButton = new QToolButton(this);
    closeButton->setIcon(QIcon(iconManager->pixmap(
        QStringLiteral(":/icons/titlebar/close.svg"),
        QSize(24, 24), titleColor())));
    closeButton->setIconSize(QSize(24, 24));
    closeButton->setFixedSize(40, 36);
    closeButton->setToolTip(tr("关闭通知"));

    header->addWidget(mark);
    header->addWidget(titleLabel);
    header->addStretch();
    header->addWidget(closeButton);
    root->addLayout(header);

    auto *messageLabel = new QLabel(message, this);
    messageLabel->setObjectName(QStringLiteral("toastMessage"));
    messageLabel->setWordWrap(true);
    messageLabel->setMaximumWidth(kMaximumToastWidth - 28);
    messageLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    root->addWidget(messageLabel, 1);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setObjectName(QStringLiteral("toastProgress"));
    m_progressBar->setRange(0, 1000);
    m_progressBar->setValue(1000);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(3);
    m_progressBar->setStyleSheet(QStringLiteral(
        "QProgressBar{border:none;background:transparent;}"
        "QProgressBar::chunk{background:%1;border-radius:1px;}")
        .arg(accentColor().name()));
    root->addWidget(m_progressBar);

    const int preferredWidth = qBound(
        kMinimumToastWidth, messageLabel->sizeHint().width() + 28,
        kMaximumToastWidth);
    messageLabel->setMaximumWidth(preferredWidth - 28);
    root->activate();
    resize(preferredWidth,
           qBound(kMinimumToastHeight, root->sizeHint().height(),
                  maximumHeight()));

    connect(closeButton, &QToolButton::clicked, this, [this] {
        if (m_countdown) {
            m_countdown->stop();
        }
        emit closeRequested(this);
    });
}

qreal ToastWidget::progress() const noexcept
{
    return m_progress;
}

void ToastWidget::setProgress(const qreal progress)
{
    m_progress = qBound(0.0, progress, 1.0);
    m_progressBar->setValue(qRound(m_progress * 1000.0));
}

void ToastWidget::startCountdown(const int durationMs)
{
    if (m_countdown) {
        m_countdown->stop();
        m_countdown->deleteLater();
    }
    m_countdown = new QPropertyAnimation(this, "progress", this);
    m_countdown->setDuration(durationMs);
    m_countdown->setStartValue(1.0);
    m_countdown->setEndValue(0.0);
    m_countdown->setEasingCurve(QEasingCurve::Linear);
    connect(m_countdown, &QPropertyAnimation::finished, this, [this] {
        emit closeRequested(this);
    });
    m_countdown->start();
}

QColor ToastWidget::accentColor() const
{
    switch (m_type) {
    case Type::Success: return QColor("#45B97C");
    case Type::Warning: return QColor("#E6A23C");
    case Type::Error: return QColor("#E5484D");
    case Type::Information: return QColor("#28A9E0");
    }
    return QColor("#28A9E0");
}

QColor ToastWidget::backgroundColor() const
{
    if (m_darkTheme) {
        switch (m_type) {
        case Type::Success: return QColor("#183529");
        case Type::Warning: return QColor("#3A301D");
        case Type::Error: return QColor("#3B2225");
        case Type::Information: return QColor("#1D3040");
        }
    }
    switch (m_type) {
    case Type::Success: return QColor("#E8F8EF");
    case Type::Warning: return QColor("#FFF7E6");
    case Type::Error: return QColor("#FDECEC");
    case Type::Information: return QColor("#EAF5FD");
    }
    return QColor("#EAF5FD");
}

QColor ToastWidget::borderColor() const
{
    if (m_darkTheme) {
        switch (m_type) {
        case Type::Success: return QColor("#2F7651");
        case Type::Warning: return QColor("#80652B");
        case Type::Error: return QColor("#884047");
        case Type::Information: return QColor("#356C8C");
        }
    }
    switch (m_type) {
    case Type::Success: return QColor("#A9DFC0");
    case Type::Warning: return QColor("#F2CF83");
    case Type::Error: return QColor("#F0B2B5");
    case Type::Information: return QColor("#A8D8F0");
    }
    return QColor("#A8D8F0");
}

QColor ToastWidget::titleColor() const
{
    return m_darkTheme ? QColor("#F1F5F9") : QColor("#172033");
}

QColor ToastWidget::messageColor() const
{
    return m_darkTheme ? QColor("#CBD5E1") : QColor("#52606D");
}
