#include "PlotToolBar.h"

#include "app/AppContext.h"
#include "theme/IconManager.h"

#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QToolButton>

PlotToolBar::PlotToolBar(AppContext *context, QWidget *parent)
    : QFrame(parent),
      m_context(context)
{
    setObjectName(QStringLiteral("plotToolBar"));
    setProperty("card", true);
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 5, 8, 5);
    layout->setSpacing(4);
    m_addPage = button(
        QStringLiteral("plotAddPageButton"), tr("新建页面"),
        tr("新建绘图页面"), QStringLiteral(":/icons/protocol/new.svg"));
    m_addPlot = button(
        QStringLiteral("plotAddWidgetButton"), tr("添加绘图"),
        tr("在当前页面添加实时绘图控件"),
        QStringLiteral(":/icons/protocol/add_frame.svg"));
    m_edit = button(
        QStringLiteral("plotEditLayoutButton"), tr("编辑布局"),
        tr("切换布局编辑与锁定"), QStringLiteral(":/icons/protocol/redo.svg"));
    m_pause = button(
        QStringLiteral("plotPauseButton"), tr("暂停"),
        tr("暂停或继续当前页面"), QStringLiteral(":/icons/connection/ansi.svg"));
    m_reset = button(
        QStringLiteral("plotResetButton"), tr("恢复视图"),
        tr("恢复当前页面所有绘图的实时视图"),
        QStringLiteral(":/icons/protocol/refresh.svg"));
    m_autoY = button(
        QStringLiteral("plotAutoYButton"), tr("适配 Y 轴"),
        tr("对当前页面所有绘图执行一次 Y 轴适配"),
        QStringLiteral(":/icons/protocol/save.svg"));
    m_savePage = button(
        QStringLiteral("plotSavePageButton"), tr("保存页面"),
        tr("保存当前 Plot 页面布局"),
        QStringLiteral(":/icons/protocol/save_as.svg"));
    m_importPage = button(
        QStringLiteral("plotImportPageButton"), tr("导入页面"),
        tr("从工作空间文件导入 Plot 页面"),
        QStringLiteral(":/icons/protocol/import.svg"));
    m_edit->setCheckable(true);
    m_pause->setCheckable(true);
    for (QToolButton *item :
         {m_addPage, m_addPlot, m_edit, m_pause, m_reset, m_autoY,
          m_savePage, m_importPage}) {
        layout->addWidget(item);
    }
    layout->addStretch();
    connect(m_addPage, &QToolButton::clicked,
            this, &PlotToolBar::addPageRequested);
    connect(m_addPlot, &QToolButton::clicked,
            this, &PlotToolBar::addPlotRequested);
    connect(m_edit, &QToolButton::toggled,
            this, &PlotToolBar::editModeChanged);
    connect(m_pause, &QToolButton::toggled,
            this, &PlotToolBar::pauseChanged);
    connect(m_reset, &QToolButton::clicked,
            this, &PlotToolBar::resetRequested);
    connect(m_autoY, &QToolButton::clicked,
            this, &PlotToolBar::autoYRequested);
    connect(m_savePage, &QToolButton::clicked,
            this, &PlotToolBar::savePageRequested);
    connect(m_importPage, &QToolButton::clicked,
            this, &PlotToolBar::importPageRequested);
    connect(context->iconManager(), &IconManager::iconsChanged,
            this, &PlotToolBar::refreshIcons);
    refreshIcons();
}

bool PlotToolBar::editMode() const
{
    return m_edit->isChecked();
}

void PlotToolBar::setPagePaused(const bool paused)
{
    const QSignalBlocker blocker(m_pause);
    m_pause->setChecked(paused);
}

QToolButton *PlotToolBar::button(
    const QString &objectName,
    const QString &text,
    const QString &toolTip,
    const QString &iconPath)
{
    auto *result = new QToolButton(this);
    result->setObjectName(objectName);
    result->setText(text);
    result->setToolTip(toolTip);
    result->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    result->setProperty("terminalTool", true);
    result->setProperty("iconPath", iconPath);
    result->setMinimumHeight(34);
    return result;
}

void PlotToolBar::refreshIcons()
{
    for (QToolButton *item :
         {m_addPage, m_addPlot, m_edit, m_pause, m_reset, m_autoY,
          m_savePage, m_importPage}) {
        item->setIcon(m_context->iconManager()->icon(
            item->property("iconPath").toString()));
        item->setIconSize(QSize(17, 17));
    }
}
