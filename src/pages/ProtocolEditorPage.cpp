#include "ProtocolEditorPage.h"

#include <QFrame>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>

namespace {
QFrame *panel(const QString &title, QWidget *parent)
{
    auto *frame = new QFrame(parent);
    frame->setProperty("card", true);
    auto *layout = new QVBoxLayout(frame);
    layout->addWidget(new QLabel(title, frame));
    layout->addStretch();
    return frame;
}
}

ProtocolEditorPage::ProtocolEditorPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 24, 28, 28);
    layout->setSpacing(16);
    auto *title = new QLabel(tr("Protocol Editor"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(panel(tr("Field Library"), splitter));
    splitter->addWidget(panel(tr("Protocol Frame"), splitter));
    splitter->addWidget(panel(tr("Properties"), splitter));
    splitter->setSizes({220, 700, 280});
    layout->addWidget(title);
    layout->addWidget(splitter, 1);
}
