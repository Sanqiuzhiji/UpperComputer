#include "ConnectionPage.h"

#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

ConnectionPage::ConnectionPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 24, 28, 28);
    layout->setSpacing(16);
    auto *title = new QLabel(tr("Connection"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *card = new QFrame(this);
    card->setProperty("card", true);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 18, 20, 18);
    auto *source = new QComboBox(card);
    source->addItems({tr("虚拟数据"), tr("串口"), tr("UDP"), tr("TCP")});
    auto *connectButton = new QPushButton(tr("连接虚拟数据源"), card);
    connectButton->setProperty("accent", true);
    cardLayout->addWidget(new QLabel(tr("数据源"), card));
    cardLayout->addWidget(source);
    cardLayout->addWidget(connectButton, 0, Qt::AlignLeft);
    cardLayout->addStretch();
    layout->addWidget(title);
    layout->addWidget(card, 1);
}
