#include "SideNavigation.h"

#include <QButtonGroup>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

SideNavigation::SideNavigation(QWidget *parent)
    : QWidget(parent),
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
    m_userTitle = new QLabel(tr("UpperComputer"), m_userCard);
    QFont font = m_userTitle->font();
    font.setBold(true);
    m_userTitle->setFont(font);
    auto *subtitle = new QLabel(tr("UI Framework · v0.1"), m_userCard);
    subtitle->setProperty("muted", true);
    userLayout->addWidget(m_userTitle);
    userLayout->addWidget(subtitle);
    m_layout->addWidget(m_userCard);
    m_layout->addSpacing(8);

    addItem(QStringLiteral("⌁"), tr("Plot"), PageId::Plot);
    addItem(QStringLiteral("↔"), tr("Connection"), PageId::Connection);
    addItem(QStringLiteral("{}"), tr("Protocol Editor"), PageId::ProtocolEditor);
    addItem(QStringLiteral("CAN"), tr("CAN Bus"), PageId::CanBus);
    addItem(QStringLiteral("M"), tr("MDF Viewer"), PageId::MdfViewer);
    m_layout->addStretch();
    addItem(QStringLiteral("?"), tr("About"), PageId::About);
    addItem(QStringLiteral("⚙"), tr("Settings"), PageId::Settings);

    m_group->setExclusive(true);
    setCurrentPage(PageId::Plot);
    updatePresentation();
}

bool SideNavigation::isExpanded() const noexcept
{
    return m_expanded;
}

PageId SideNavigation::currentPage() const noexcept
{
    return m_currentPage;
}

void SideNavigation::toggleExpanded()
{
    setExpanded(!m_expanded);
}

void SideNavigation::setExpanded(const bool expanded)
{
    if (m_expanded == expanded) {
        return;
    }
    m_expanded = expanded;
    updatePresentation();
    emit expandedChanged(m_expanded);
}

void SideNavigation::setMode(const Mode mode)
{
    m_mode = mode;
    if (mode == Mode::Expanded) {
        setExpanded(true);
    } else if (mode == Mode::Compact) {
        setExpanded(false);
    }
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
        m_currentPage = page;
        m_buttons.at(index)->setChecked(true);
    }
}

void SideNavigation::addItem(const QString &symbol, const QString &title, const PageId page)
{
    auto *button = new QPushButton(this);
    button->setCheckable(true);
    button->setProperty("nav", true);
    button->setMinimumHeight(42);
    button->setToolTip(title);
    m_group->addButton(button);
    m_buttons.append(button);
    m_symbols.append(symbol);
    m_titles.append(title);
    m_pageIds.append(page);
    connect(button, &QPushButton::clicked, this, [this, page, title] {
        m_currentPage = page;
        emit pageRequested(page, title);
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
                            ? QStringLiteral("  %1    %2").arg(m_symbols.at(index), m_titles.at(index))
                            : m_symbols.at(index));
        button->setStyleSheet(m_expanded
                                  ? QString()
                                  : QStringLiteral("text-align:center;padding:6px;"));
    }
}
