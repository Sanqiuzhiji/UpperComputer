#include "PlotPage.h"

#include "app/AppContext.h"
#include "app/AppSettings.h"
#include "models/AppTypes.h"
#include "pages/plot/DetachableTabWidget.h"
#include "pages/plot/PlotCanvas.h"
#include "pages/plot/PlotToolBar.h"
#include "pages/plot/PlotWorkspacePage.h"

#include <QVBoxLayout>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QShowEvent>

PlotPage::PlotPage(AppContext *context, QWidget *parent)
    : QWidget(parent),
      m_context(context)
{
    setObjectName(QStringLiteral("plotPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 12, 20, 12);
    layout->setSpacing(8);
    m_toolBar = new PlotToolBar(context, this);
    m_tabs = new DetachableTabWidget(context, this);
    layout->addWidget(m_toolBar);
    layout->addWidget(m_tabs, 1);

    connect(m_toolBar, &PlotToolBar::addPageRequested,
            this, &PlotPage::addPage);
    connect(m_toolBar, &PlotToolBar::addPlotRequested,
            this, &PlotPage::addRealtimePlot);
    connect(m_toolBar, &PlotToolBar::editModeChanged,
            this, [this](const bool enabled) {
                if (PlotWorkspacePage *page = currentWorkspace()) {
                    page->canvas()->setEditMode(enabled);
                }
            });
    connect(m_toolBar, &PlotToolBar::pauseChanged,
            this, [this](const bool paused) {
                if (PlotWorkspacePage *page = currentWorkspace()) {
                    page->setPaused(paused);
                }
            });
    connect(m_toolBar, &PlotToolBar::resetRequested, this, [this] {
        if (PlotWorkspacePage *page = currentWorkspace()) {
            page->canvas()->resetPlots();
        }
    });
    connect(m_toolBar, &PlotToolBar::autoYRequested, this, [this] {
        if (PlotWorkspacePage *page = currentWorkspace()) {
            page->canvas()->fitPlotsY();
        }
    });
    connect(m_toolBar, &PlotToolBar::savePageRequested,
            this, &PlotPage::saveCurrentPage);
    connect(m_toolBar, &PlotToolBar::importPageRequested,
            this, &PlotPage::importPage);
    connect(m_tabs, &DetachableTabWidget::currentWorkspaceChanged,
            this, [this](PlotWorkspacePage *page) {
                if (!page) return;
                page->canvas()->setEditMode(m_toolBar->editMode());
                m_toolBar->setPagePaused(page->paused());
            });
}

DetachableTabWidget *PlotPage::tabWidget() const noexcept
{
    return m_tabs;
}

PlotWorkspacePage *PlotPage::addPage()
{
    PlotWorkspacePage *page = m_tabs->addWorkspace();
    page->canvas()->setEditMode(m_toolBar->editMode());
    return page;
}

void PlotPage::addRealtimePlot()
{
    PlotWorkspacePage *page = currentWorkspace();
    if (!page) page = addPage();
    page->canvas()->addRealtimePlot();
}

void PlotPage::saveCurrentPage()
{
    PlotWorkspacePage *page = currentWorkspace();
    if (!page) return;
    const int index = m_tabs->currentIndex();
    const QString title = m_tabs->tabText(index);
    QString safeTitle = title;
    safeTitle.replace(QRegularExpression(QStringLiteral(R"([\\/:*?"<>|])")),
                      QStringLiteral("_"));
    const QString directory = QDir(m_context->settings()->workspaceDirectory())
        .filePath(QStringLiteral("plot"));
    if (!QDir().mkpath(directory)) {
        m_context->notify(tr("无法创建 Plot 工作空间目录"),
                          NotificationType::Error);
        return;
    }
    const QString path = QFileDialog::getSaveFileName(
        this, tr("保存 Plot 页面"),
        QDir(directory).filePath(safeTitle + QStringLiteral(".plot.json")),
        tr("Plot 页面 (*.plot.json)"));
    if (path.isEmpty()) return;
    const QJsonObject document{
        {QStringLiteral("format"), QStringLiteral("uppercomputer.plot-page")},
        {QStringLiteral("version"), 1},
        {QStringLiteral("title"), title},
        {QStringLiteral("canvas"), page->canvas()->saveLayout()}};
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)
        || file.write(QJsonDocument(document).toJson(QJsonDocument::Indented)) < 0
        || !file.commit()) {
        m_context->notify(tr("Plot 页面保存失败"), NotificationType::Error);
        return;
    }
    m_context->notify(tr("Plot 页面已保存"), NotificationType::Success);
}

void PlotPage::importPage()
{
    const QString directory = QDir(m_context->settings()->workspaceDirectory())
        .filePath(QStringLiteral("plot"));
    QDir().mkpath(directory);
    const QString path = QFileDialog::getOpenFileName(
        this, tr("导入 Plot 页面"), directory,
        tr("Plot 页面 (*.plot.json);;JSON 文件 (*.json)"));
    if (path.isEmpty()) return;
    loadPageFile(path, true);
}

void PlotPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!m_defaultPagesLoaded) loadDefaultPages();
}

void PlotPage::loadDefaultPages()
{
    m_defaultPagesLoaded = true;
    const QString directory = QDir(m_context->settings()->workspaceDirectory())
        .filePath(QStringLiteral("plot"));
    QDir plotDirectory(directory);
    if (!plotDirectory.exists()) {
        QDir().mkpath(directory);
        return;
    }
    const QFileInfoList files = plotDirectory.entryInfoList(
        {QStringLiteral("*.plot.json")},
        QDir::Files | QDir::Readable,
        QDir::Name | QDir::IgnoreCase);
    int loaded = 0;
    for (const QFileInfo &file : files) {
        if (loadPageFile(file.absoluteFilePath(), false)) ++loaded;
    }
    if (loaded > 0) {
        m_context->notify(
            tr("已加载 %1 个 Plot 页面").arg(loaded),
            NotificationType::Success);
    }
}

bool PlotPage::loadPageFile(
    const QString &path, const bool notifySuccess)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        m_context->notify(tr("无法读取 Plot 页面文件"), NotificationType::Error);
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument json = QJsonDocument::fromJson(file.readAll(), &parseError);
    const QJsonObject root = json.object();
    if (parseError.error != QJsonParseError::NoError || !json.isObject()
        || root.value(QStringLiteral("format")).toString()
            != QStringLiteral("uppercomputer.plot-page")
        || root.value(QStringLiteral("version")).toInt() != 1
        || !root.value(QStringLiteral("canvas")).isObject()) {
        m_context->notify(tr("不是有效或受支持的 Plot 页面文件"),
                          NotificationType::Error);
        return false;
    }
    QString title = root.value(QStringLiteral("title")).toString().trimmed();
    if (title.isEmpty()) title = QFileInfo(path).completeBaseName();
    PlotWorkspacePage *page = m_tabs->addWorkspace(title);
    if (!page) return false;
    QString error;
    if (!page->canvas()->restoreLayout(
            root.value(QStringLiteral("canvas")).toObject(), &error)) {
        const int failedIndex = m_tabs->indexOf(page);
        m_tabs->removeTab(failedIndex);
        page->deleteLater();
        m_context->notify(error, NotificationType::Error);
        return false;
    }
    page->canvas()->setEditMode(m_toolBar->editMode());
    if (notifySuccess) {
        m_context->notify(tr("Plot 页面已导入"), NotificationType::Success);
    }
    return true;
}

PlotWorkspacePage *PlotPage::currentWorkspace() const
{
    return m_tabs->currentWorkspace();
}
