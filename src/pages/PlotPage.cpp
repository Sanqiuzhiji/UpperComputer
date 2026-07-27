#include "PlotPage.h"

#include "pages/plot/DetachableTabWidget.h"
#include "pages/plot/PlotCanvas.h"
#include "pages/plot/PlotToolBar.h"
#include "pages/plot/PlotWorkspacePage.h"

#include <QVBoxLayout>

PlotPage::PlotPage(AppContext *context, QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("plotPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 12, 20, 12);
    layout->setSpacing(8);
    m_toolBar = new PlotToolBar(context, this);
    m_tabs = new DetachableTabWidget(context, this);
    layout->addWidget(m_toolBar);
    layout->addWidget(m_tabs, 1);
    addPage();

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

PlotWorkspacePage *PlotPage::currentWorkspace() const
{
    return m_tabs->currentWorkspace();
}
