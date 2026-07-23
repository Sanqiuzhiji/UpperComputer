#include "TitleBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QToolButton>

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("titleBar"));
    setFixedHeight(44);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 6, 4);
    layout->setSpacing(3);

    auto *back = createButton(QStringLiteral("‹"), tr("Back"));
    auto *forward = createButton(QStringLiteral("›"), tr("Forward"));
    auto *menu = createButton(QStringLiteral("☰"), tr("Collapse navigation"));
    connect(back, &QToolButton::clicked, this, &TitleBar::backRequested);
    connect(forward, &QToolButton::clicked, this, &TitleBar::forwardRequested);
    connect(menu, &QToolButton::clicked, this, &TitleBar::navigationToggleRequested);

    auto *icon = new QLabel(QStringLiteral("UC"), this);
    icon->setAlignment(Qt::AlignCenter);
    icon->setFixedSize(28, 28);
    icon->setStyleSheet(QStringLiteral(
        "background:#28A9E0;color:white;border-radius:7px;font-weight:700;"));
    auto *name = new QLabel(QStringLiteral("UpperComputer"), this);
    QFont nameFont = name->font();
    nameFont.setBold(true);
    name->setFont(nameFont);

    auto *pin = createButton(QStringLiteral("⌖"), tr("Always on top"));
    pin->setCheckable(true);
    connect(pin, &QToolButton::toggled, this, &TitleBar::pinToggleRequested);
    auto *theme = createButton(QStringLiteral("◐"), tr("Toggle light/dark theme"));
    connect(theme, &QToolButton::clicked, this, &TitleBar::themeToggleRequested);
    auto *minimize = createButton(QStringLiteral("—"), tr("Minimize"));
    connect(minimize, &QToolButton::clicked, this, &TitleBar::minimizeRequested);
    m_maximizeButton = createButton(QStringLiteral("□"), tr("Maximize"));
    connect(m_maximizeButton, &QToolButton::clicked,
            this, &TitleBar::maximizeRestoreRequested);
    auto *close = createButton(QStringLiteral("×"), tr("Close"));
    close->setObjectName(QStringLiteral("closeButton"));
    close->setStyleSheet(QStringLiteral(
        "QToolButton#closeButton:hover{background:#E5484D;color:white;}"));
    connect(close, &QToolButton::clicked, this, &TitleBar::closeRequested);

    layout->addWidget(back);
    layout->addWidget(forward);
    layout->addWidget(menu);
    layout->addSpacing(8);
    layout->addWidget(icon);
    layout->addWidget(name);
    layout->addStretch();
    layout->addWidget(pin);
    layout->addWidget(theme);
    layout->addWidget(minimize);
    layout->addWidget(m_maximizeButton);
    layout->addWidget(close);
}

void TitleBar::setMaximized(const bool maximized)
{
    m_maximizeButton->setText(maximized ? QStringLiteral("❐") : QStringLiteral("□"));
    m_maximizeButton->setToolTip(maximized ? tr("Restore") : tr("Maximize"));
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit maximizeRestoreRequested();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

QToolButton *TitleBar::createButton(const QString &text, const QString &toolTip)
{
    auto *button = new QToolButton(this);
    button->setText(text);
    button->setToolTip(toolTip);
    button->setFixedSize(34, 32);
    button->setAutoRaise(true);
    QFont font = button->font();
    font.setPointSize(13);
    button->setFont(font);
    return button;
}
