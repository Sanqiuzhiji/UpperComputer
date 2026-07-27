#include "PlotFloatingWindow.h"

#include <QCloseEvent>

PlotFloatingWindow::PlotFloatingWindow(
    QWidget *page, const QString &title, QWidget *parent)
    : QMainWindow(parent, Qt::Window),
      m_page(page)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(title);
    setCentralWidget(page);
    resize(900, 620);
}

QWidget *PlotFloatingWindow::page() const noexcept
{
    return m_page;
}

void PlotFloatingWindow::closeEvent(QCloseEvent *event)
{
    if (!m_reattached && m_page) {
        m_reattached = true;
        QWidget *pageWidget = takeCentralWidget();
        emit reattachRequested(pageWidget, windowTitle());
        m_page = nullptr;
    }
    QMainWindow::closeEvent(event);
}
