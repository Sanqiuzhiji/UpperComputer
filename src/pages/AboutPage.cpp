#include "AboutPage.h"

#include <QFrame>
#include <QLabel>
#include <QSysInfo>
#include <QVBoxLayout>

AboutPage::AboutPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 24, 28, 28);
    layout->setSpacing(16);
    auto *title = new QLabel(tr("关于"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *card = new QFrame(this);
    card->setProperty("card", true);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(24, 24, 24, 24);
    auto *name = new QLabel(QStringLiteral("UpperComputer"), card);
    name->setObjectName(QStringLiteral("heroTitle"));
    auto *details = new QLabel(
        tr("开发版本 0.1.0\nQt %1\n编译器：MSVC 2022 x64\n\n"
           "独立、可扩展的桌面仪器界面框架。\n"
           "开源组件说明和官方网站链接将在后续版本补充。")
            .arg(QString::fromLatin1(qVersion())),
        card);
    details->setProperty("muted", true);
    cardLayout->addWidget(name);
    cardLayout->addWidget(details);
    cardLayout->addStretch();
    layout->addWidget(title);
    layout->addWidget(card, 1);
}
