#include "ProtocolEditorPage.h"

#include <QVBoxLayout>

ProtocolEditorPage::ProtocolEditorPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->addStretch();
}
