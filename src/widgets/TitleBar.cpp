#include "TitleBar.h"

#include "theme/IconManager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QToolButton>

TitleBar::TitleBar(IconManager *iconManager, QWidget *parent)
    : QWidget(parent),
      m_iconManager(iconManager)
{
    setObjectName(QStringLiteral("titleBar"));
    setFixedHeight(42);
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_menuButton = createButton(QStringLiteral(":/icons/titlebar/menu.svg"),
                                tr("展开或折叠导航栏"), QSize(42, 42));
    connect(m_menuButton, &QToolButton::clicked,
            this, &TitleBar::navigationToggleRequested);

    m_appIcon = new QLabel(this);
    m_appIcon->setAlignment(Qt::AlignCenter);
    m_appIcon->setFixedSize(34, 42);
    auto *name = new QLabel(QStringLiteral("UpperComputer"), this);
    QFont nameFont = name->font();
    nameFont.setBold(true);
    name->setFont(nameFont);
    m_pageTitle = new QLabel(tr("Plot"), this);
    m_pageTitle->setObjectName(QStringLiteral("titlePageName"));

    m_pinButton = createButton(QStringLiteral(":/icons/titlebar/pin.svg"),
                               tr("窗口置顶"), QSize(46, 42));
    m_pinButton->setCheckable(true);
    connect(m_pinButton, &QToolButton::toggled, this, [this](const bool pinned) {
        m_pinned = pinned;
        refreshIcons();
        emit pinToggleRequested(pinned);
    });
    m_themeButton = createButton(QStringLiteral(":/icons/titlebar/theme.svg"),
                                 tr("切换深浅主题"), QSize(46, 42));
    connect(m_themeButton, &QToolButton::clicked,
            this, &TitleBar::themeToggleRequested);
    m_minimizeButton = createButton(QStringLiteral(":/icons/titlebar/minimize.svg"),
                                    tr("最小化"), QSize(46, 42));
    connect(m_minimizeButton, &QToolButton::clicked,
            this, &TitleBar::minimizeRequested);
    m_maximizeButton = createButton(QStringLiteral(":/icons/titlebar/maximize.svg"),
                                    tr("最大化"), QSize(46, 42));
    connect(m_maximizeButton, &QToolButton::clicked,
            this, &TitleBar::maximizeRestoreRequested);
    m_closeButton = createButton(QStringLiteral(":/icons/titlebar/close.svg"),
                                 tr("关闭"), QSize(46, 42), Qt::white);
    m_closeButton->setObjectName(QStringLiteral("closeButton"));
    connect(m_closeButton, &QToolButton::clicked, this, &TitleBar::closeRequested);

    layout->addWidget(m_menuButton);
    layout->addSpacing(8);
    layout->addWidget(m_appIcon);
    layout->addSpacing(5);
    layout->addWidget(name);
    layout->addSpacing(12);
    layout->addWidget(m_pageTitle);
    layout->addStretch();
    layout->addWidget(m_pinButton);
    layout->addWidget(m_themeButton);
    layout->addWidget(m_minimizeButton);
    layout->addWidget(m_maximizeButton);
    layout->addWidget(m_closeButton);

    connect(m_iconManager, &IconManager::iconsChanged,
            this, &TitleBar::refreshIcons);
    refreshIcons();
}

void TitleBar::setMaximized(const bool maximized)
{
    m_maximized = maximized;
    m_maximizeButton->setToolTip(maximized ? tr("还原") : tr("最大化"));
    refreshIcons();
}

void TitleBar::setCurrentPageTitle(const QString &title)
{
    m_pageTitle->setText(title);
}

QPoint TitleBar::themeButtonCenter(QWidget *target) const
{
    return m_themeButton->mapTo(target, m_themeButton->rect().center());
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

void TitleBar::refreshIcons()
{
    m_menuButton->setIcon(m_iconManager->icon(QStringLiteral(":/icons/titlebar/menu.svg")));
    m_pinButton->setIcon(m_pinned
        ? m_iconManager->rotatedIcon(QStringLiteral(":/icons/titlebar/pin.svg"), 90.0)
        : m_iconManager->icon(QStringLiteral(":/icons/titlebar/pin.svg")));
    m_themeButton->setIcon(m_iconManager->icon(
        m_iconManager->isDarkTheme()
            ? QStringLiteral(":/icons/titlebar/theme_filled.svg")
            : QStringLiteral(":/icons/titlebar/theme.svg")));
    m_minimizeButton->setIcon(m_iconManager->icon(QStringLiteral(":/icons/titlebar/minimize.svg")));
    m_maximizeButton->setIcon(m_iconManager->icon(
        m_maximized ? QStringLiteral(":/icons/titlebar/restore.svg")
                    : QStringLiteral(":/icons/titlebar/maximize.svg")));
    m_closeButton->setIcon(m_iconManager->icon(
        QStringLiteral(":/icons/titlebar/close.svg"), Qt::white));
    m_appIcon->setPixmap(m_iconManager->pixmap(
        QStringLiteral(":/icons/common/app.svg"), QSize(24, 24), QColor("#28A9E0")));
}

QToolButton *TitleBar::createButton(const QString &iconPath,
                                    const QString &toolTip,
                                    const QSize &size,
                                    const QColor &activeColor)
{
    auto *button = new QToolButton(this);
    button->setIcon(m_iconManager->icon(iconPath, activeColor));
    button->setIconSize(QSize(20, 20));
    button->setToolTip(toolTip);
    button->setFixedSize(size);
    button->setAutoRaise(true);
    return button;
}
