#include "PlotWorkspacePage.h"

#include "pages/plot/PlotCanvas.h"

#include <QVBoxLayout>

PlotWorkspacePage::PlotWorkspacePage(AppContext *context, QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_canvas = new PlotCanvas(context, this);
    layout->addWidget(m_canvas);
}

PlotCanvas *PlotWorkspacePage::canvas() const noexcept
{
    return m_canvas;
}

bool PlotWorkspacePage::paused() const noexcept
{
    return m_paused;
}

void PlotWorkspacePage::setPaused(const bool paused)
{
    m_paused = paused;
    m_canvas->setPaused(paused);
}
