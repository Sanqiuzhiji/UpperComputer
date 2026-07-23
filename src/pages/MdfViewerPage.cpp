#include "MdfViewerPage.h"

#include <QLabel>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>

MdfViewerPage::MdfViewerPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 24, 28, 28);
    layout->setSpacing(16);
    auto *title = new QLabel(tr("MDF Viewer"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *mdi = new QMdiArea(this);
    auto *content = new QSplitter(Qt::Horizontal, mdi);
    auto *tree = new QTreeWidget(content);
    tree->setHeaderLabel(tr("Channels"));
    tree->addTopLevelItem(new QTreeWidgetItem({tr("Demo group")}));
    content->addWidget(tree);
    auto *plot = new QLabel(tr("MDF curve preview"), content);
    plot->setAlignment(Qt::AlignCenter);
    content->addWidget(plot);
    auto *subWindow = mdi->addSubWindow(content);
    subWindow->setWindowTitle(tr("MDF Viewer · Demo"));
    subWindow->resize(760, 480);
    subWindow->show();
    layout->addWidget(title);
    layout->addWidget(mdi, 1);
}
