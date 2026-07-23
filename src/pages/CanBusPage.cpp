#include "CanBusPage.h"

#include <QLabel>
#include <QSplitter>
#include <QTableWidget>
#include <QVBoxLayout>

CanBusPage::CanBusPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 24, 28, 28);
    layout->setSpacing(16);
    auto *title = new QLabel(tr("CAN Bus"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *vertical = new QSplitter(Qt::Vertical, this);
    auto *top = new QSplitter(Qt::Horizontal, vertical);
    auto *signalTable = new QTableWidget(5, 4, top);
    signalTable->setHorizontalHeaderLabels({tr("Enable"), tr("Message"), tr("ID"), tr("DLC")});
    top->addWidget(signalTable);
    top->addWidget(new QLabel(tr("Message Tree"), top));
    auto *bottom = new QSplitter(Qt::Horizontal, vertical);
    bottom->addWidget(new QLabel(tr("Periodic Sender"), bottom));
    bottom->addWidget(new QLabel(tr("Manual Send"), bottom));
    vertical->addWidget(top);
    vertical->addWidget(bottom);
    layout->addWidget(title);
    layout->addWidget(vertical, 1);
}
