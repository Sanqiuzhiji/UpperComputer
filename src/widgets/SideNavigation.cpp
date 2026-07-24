#include "SideNavigation.h"

#include "pages/PageRegistry.h"
#include "theme/IconManager.h"

#include <QButtonGroup>
#include <QFrame>
#include <QLabel>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

SideNavigation::SideNavigation(IconManager *iconManager, QWidget *parent)
    : QWidget(parent),
      m_iconManager(iconManager),
      m_layout(new QVBoxLayout(this)),
      m_group(new QButtonGroup(this))
{
    setObjectName(QStringLiteral("sideNavigation"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_layout->setContentsMargins(8, 10, 8, 10);
    m_layout->setSpacing(5);

    m_userCard = new QFrame(this);
    m_userCard->setProperty("card", true);
    m_userCard->setFixedHeight(74);
    auto *userLayout = new QVBoxLayout(m_userCard);
    userLayout->setContentsMargins(12, 10, 12, 10);
    m_userTitle = new QLabel(QStringLiteral("UpperComputer"), m_userCard);
    QFont font = m_userTitle->font();
    font.setBold(true);
    m_userTitle->setFont(font);
    auto *subtitle = new QLabel(tr("界面框架 · v0.1"), m_userCard);
    subtitle->setProperty("muted", true);
    userLayout->addWidget(m_userTitle);
    userLayout->addWidget(subtitle);
    m_layout->addWidget(m_userCard);
    m_layout->addSpacing(8);

    bool bottomSectionStarted = false;
    for (const PageDescriptor &descriptor : pageDescriptors()) {
        if (descriptor.alignBottom && !bottomSectionStarted) {
            m_layout->addStretch();
            bottomSectionStarted = true;
        }
        addItem(descriptor.iconPath, descriptor.title, descriptor.id);
    }

    m_group->setExclusive(true);
    setCurrentPage(PageId::Plot);
    updatePresentation();
    connect(m_iconManager, &IconManager::iconsChanged,
            this, &SideNavigation::refreshIcons);
    QTimer::singleShot(0, this, &SideNavigation::syncIndicatorToCurrent);
}

bool SideNavigation::isExpanded() const noexcept { return m_expanded; }
NavigationMode SideNavigation::mode() const noexcept { return m_mode; }
PageId SideNavigation::currentPage() const noexcept { return m_currentPage; }
qreal SideNavigation::indicatorTop() const noexcept { return m_indicatorTop; }
qreal SideNavigation::indicatorBottom() const noexcept { return m_indicatorBottom; }

void SideNavigation::setIndicatorTop(const qreal value)
{
    m_indicatorTop = value;
    update();
}

void SideNavigation::setIndicatorBottom(const qreal value)
{
    m_indicatorBottom = value;
    update();
}

void SideNavigation::toggleExpanded() { setExpanded(!m_expanded); }

void SideNavigation::setExpanded(const bool expanded)
{
    if (m_expanded == expanded) return;
    m_expanded = expanded;
    updatePresentation();
    emit expandedChanged(m_expanded);
}

void SideNavigation::setMode(const NavigationMode mode)
{
    m_mode = mode;
    if (mode == NavigationMode::Expanded) setExpanded(true);
    else if (mode == NavigationMode::Compact) setExpanded(false);
}

void SideNavigation::setUserCardVisible(const bool visible)
{
    m_userCardEnabled = visible;
    m_userCard->setVisible(visible && m_expanded);
}

void SideNavigation::setCurrentPage(const PageId page)
{
    const qsizetype index = m_pageIds.indexOf(page);
    if (index >= 0) {
        if (m_indicatorInitialized && page != m_currentPage) {
            animateIndicatorTo(page);
        }
        m_currentPage = page;
        m_buttons.at(index)->setChecked(true);
    }
}

void SideNavigation::addItem(const QString &iconPath,
                             const QString &title,
                             const PageId page)
{
    auto *button = new QPushButton(this);
    button->setCheckable(true);
    button->setProperty("nav", true);
    button->setFixedHeight(46);
    button->setIconSize(QSize(22, 22));
    button->setToolTip(title);
    m_group->addButton(button);
    m_buttons.append(button);
    m_iconPaths.append(iconPath);
    m_titles.append(title);
    m_pageIds.append(page);
    connect(button, &QPushButton::clicked, this, [this, page] {
        setCurrentPage(page);
        emit pageRequested(page);
    });
    m_layout->addWidget(button);
}

void SideNavigation::updatePresentation()
{
    setFixedWidth(m_expanded ? 220 : 64);
    m_userCard->setVisible(m_userCardEnabled && m_expanded);
    for (qsizetype index = 0; index < m_buttons.size(); ++index) {
        auto *button = m_buttons.at(index);
        button->setText(m_expanded
                            ? QStringLiteral("   %1").arg(m_titles.at(index))
                            : QString());
        button->setStyleSheet(m_expanded
                                  ? QString()
                                  : QStringLiteral("text-align:center;padding:6px;"));
    }
    refreshIcons();
}

void SideNavigation::refreshIcons()
{
    for (qsizetype index = 0; index < m_buttons.size(); ++index) {
        m_buttons.at(index)->setIcon(m_iconManager->icon(m_iconPaths.at(index)));
    }
}

void SideNavigation::animateIndicatorTo(const PageId page)
{
    const qsizetype targetIndex = m_pageIds.indexOf(page);
    const qsizetype currentIndex = m_pageIds.indexOf(m_currentPage);
    if (targetIndex < 0 || currentIndex < 0) {
        return;
    }

    m_layout->activate();
    const QRect targetRect = m_buttons.at(targetIndex)->geometry();
    constexpr qreal indicatorHeight = 26.0;
    const qreal targetTop = targetRect.center().y() - indicatorHeight / 2.0;
    const qreal targetBottom = targetTop + indicatorHeight;
    const int distance = qAbs(static_cast<int>(targetIndex - currentIndex));
    const bool movingDown = targetTop > m_indicatorTop;

    if (m_indicatorAnimation) {
        m_indicatorAnimation->stop();
        m_indicatorAnimation->deleteLater();
    }

    auto *group = new QParallelAnimationGroup(this);
    auto *topAnimation = new QPropertyAnimation(this, "indicatorTop", group);
    auto *bottomAnimation = new QPropertyAnimation(this, "indicatorBottom", group);
    topAnimation->setStartValue(m_indicatorTop);
    topAnimation->setEndValue(targetTop);
    bottomAnimation->setStartValue(m_indicatorBottom);
    bottomAnimation->setEndValue(targetBottom);

    // The leading edge arrives first and stretches the indicator; the trailing
    // edge then catches up. Long jumps gain distance faster than duration.
    const int leadDuration = 135 + distance * 16;
    const int trailDuration = 235 + distance * 18;
    if (movingDown) {
        bottomAnimation->setDuration(leadDuration);
        topAnimation->setDuration(trailDuration);
        bottomAnimation->setEasingCurve(QEasingCurve::OutCubic);
        topAnimation->setEasingCurve(QEasingCurve::InOutCubic);
    } else {
        topAnimation->setDuration(leadDuration);
        bottomAnimation->setDuration(trailDuration);
        topAnimation->setEasingCurve(QEasingCurve::OutCubic);
        bottomAnimation->setEasingCurve(QEasingCurve::InOutCubic);
    }
    connect(group, &QParallelAnimationGroup::finished, this, [this, group] {
        if (m_indicatorAnimation == group) {
            m_indicatorAnimation = nullptr;
        }
        group->deleteLater();
    });
    m_indicatorAnimation = group;
    group->start();
}

void SideNavigation::syncIndicatorToCurrent()
{
    const qsizetype index = m_pageIds.indexOf(m_currentPage);
    if (index < 0) {
        return;
    }
    m_layout->activate();
    const QRect buttonRect = m_buttons.at(index)->geometry();
    constexpr qreal indicatorHeight = 26.0;
    m_indicatorTop = buttonRect.center().y() - indicatorHeight / 2.0;
    m_indicatorBottom = m_indicatorTop + indicatorHeight;
    m_indicatorInitialized = true;
    update();
}

void SideNavigation::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    if (!m_indicatorInitialized) {
        return;
    }
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#28A9E0"));
    painter.drawRoundedRect(
        QRectF(3.0, m_indicatorTop, 4.0,
               qMax<qreal>(4.0, m_indicatorBottom - m_indicatorTop)),
        2.0, 2.0);
}
