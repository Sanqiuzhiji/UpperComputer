#include "DetachableTabWidget.h"

#include "app/AppContext.h"
#include "pages/plot/DetachableTabBar.h"
#include "pages/plot/PlotFloatingWindow.h"
#include "pages/plot/PlotTheme.h"
#include "pages/plot/PlotWorkspacePage.h"
#include "theme/ThemeManager.h"

#include <QInputDialog>

DetachableTabWidget::DetachableTabWidget(
    AppContext *context, QWidget *parent)
    : QTabWidget(parent),
      m_context(context)
{
    setObjectName(QStringLiteral("plotTabWidget"));
    auto *bar = new DetachableTabBar(this);
    bar->setObjectName(QStringLiteral("plotTabBar"));
    setTabBar(bar);
    setPalette(PlotTheme::palette(context->themeManager()->mode()));
    setTabsClosable(true);
    setMovable(true);
    setDocumentMode(true);
    connect(this, &QTabWidget::tabCloseRequested,
            this, &DetachableTabWidget::closeWorkspace);
    connect(this, &QTabWidget::tabBarDoubleClicked,
            this, &DetachableTabWidget::renameWorkspace);
    connect(bar, &DetachableTabBar::detachRequested,
            this, &DetachableTabWidget::detachWorkspace);
    connect(context->themeManager(), &ThemeManager::themeChanged,
            this, [this, bar](const ThemeMode mode) {
                const QPalette themePalette = PlotTheme::palette(mode);
                setPalette(themePalette);
                bar->setPalette(themePalette);
                update();
                bar->update();
            });
    connect(this, &QTabWidget::currentChanged, this, [this] {
        emit currentWorkspaceChanged(currentWorkspace());
    });
}

DetachableTabWidget::~DetachableTabWidget()
{
    m_shuttingDown = true;
    const auto windows = m_windows.values();
    for (PlotFloatingWindow *window : windows) {
        if (window) {
            window->setAttribute(Qt::WA_DeleteOnClose, false);
            delete window;
        }
    }
}

PlotWorkspacePage *DetachableTabWidget::addWorkspace(const QString &title)
{
    auto *page = new PlotWorkspacePage(m_context, this);
    const QString pageTitle = title.trimmed().isEmpty()
        ? tr("页面 %1").arg(++m_pageCounter) : title;
    const int index = addTab(page, pageTitle);
    setCurrentIndex(index);
    return page;
}

PlotWorkspacePage *DetachableTabWidget::currentWorkspace() const
{
    return qobject_cast<PlotWorkspacePage *>(currentWidget());
}

int DetachableTabWidget::workspaceCount() const
{
    return count() + m_windows.size();
}

void DetachableTabWidget::closeWorkspace(const int index)
{
    if (index < 0 || index >= count()) return;
    QWidget *page = widget(index);
    removeTab(index);
    page->deleteLater();
}

void DetachableTabWidget::renameWorkspace(const int index)
{
    if (index < 0 || index >= count()) return;
    bool accepted = false;
    const QString title = QInputDialog::getText(
        this, tr("重命名页面"), tr("页面名称"),
        QLineEdit::Normal, tabText(index), &accepted).trimmed();
    if (accepted && !title.isEmpty()) setTabText(index, title);
}

void DetachableTabWidget::detachWorkspace(
    const int index, const QPoint &globalPosition)
{
    if (index < 0 || index >= count()) return;
    QWidget *page = widget(index);
    const QString title = tabText(index);
    removeTab(index);
    auto *window = new PlotFloatingWindow(page, title, this);
    m_windows.insert(page, window);
    connect(window, &PlotFloatingWindow::reattachRequested,
            this, &DetachableTabWidget::attachWorkspace);
    connect(window, &QObject::destroyed, this, [this, page] {
        m_windows.remove(page);
    });
    window->move(globalPosition - QPoint(120, 20));
    window->show();
}

void DetachableTabWidget::attachWorkspace(
    QWidget *page, const QString &title)
{
    if (!page || m_shuttingDown) return;
    m_windows.remove(page);
    page->setParent(this);
    const int index = addTab(page, title);
    setCurrentIndex(index);
}
