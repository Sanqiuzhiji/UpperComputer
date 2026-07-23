#include "StatusBarWidget.h"

#include <QHBoxLayout>
#include <QLabel>

StatusBarWidget::StatusBarWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("statusBar"));
    setFixedHeight(28);
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 12, 0);

    auto *connection = new QLabel(tr("●  Disconnected"), this);
    connection->setStyleSheet(QStringLiteral("color:#9DA3AE;"));
    auto *source = new QLabel(tr("Source: Virtual Data"), this);
    source->setProperty("muted", true);
    auto *rates = new QLabel(tr("RX 0 B/s    TX 0 B/s"), this);
    rates->setProperty("muted", true);
    m_theme = new QLabel(tr("Theme: Dark"), this);
    m_theme->setProperty("muted", true);
    m_page = new QLabel(tr("Page: Plot"), this);
    m_page->setProperty("muted", true);

    layout->addWidget(connection);
    layout->addSpacing(16);
    layout->addWidget(source);
    layout->addWidget(rates);
    layout->addStretch();
    layout->addWidget(m_theme);
    layout->addSpacing(16);
    layout->addWidget(m_page);
}

void StatusBarWidget::setCurrentPage(const QString &page)
{
    m_page->setText(tr("Page: %1").arg(page));
}

void StatusBarWidget::setTheme(const QString &theme)
{
    m_theme->setText(tr("Theme: %1").arg(theme));
}
