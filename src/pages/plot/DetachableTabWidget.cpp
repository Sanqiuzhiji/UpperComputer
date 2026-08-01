#include "DetachableTabWidget.h"

#include "app/AppContext.h"
#include "pages/plot/DetachableTabBar.h"
#include "pages/plot/PlotFloatingWindow.h"
#include "pages/plot/PlotTheme.h"
#include "pages/plot/PlotWorkspacePage.h"
#include "theme/ThemeManager.h"

#include <QInputDialog>
#include <QToolButton>

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
    setTabsClosable(false);
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
    QString pageTitle = title.trimmed();
    if (pageTitle.isEmpty()) {
        do {
            pageTitle = tr("页面 %1").arg(++m_pageCounter);
        } while (hasWorkspaceTitle(pageTitle));
    } else if (hasWorkspaceTitle(pageTitle)) {
        m_context->notify(
            tr("页面名称“%1”已存在").arg(pageTitle),
            NotificationType::Warning);
        return nullptr;
    }

    auto *page = new PlotWorkspacePage(m_context, this);
    const int index = addTab(page, pageTitle);
    installCloseButton(index, page);
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

bool DetachableTabWidget::hasWorkspaceTitle(
    const QString &title, const QWidget *excludedPage) const
{
    const QString candidate = title.trimmed();
    for (int index = 0; index < count(); ++index) {
        if (widget(index) != excludedPage
            && tabText(index).trimmed().compare(
                candidate, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    for (auto it = m_windows.cbegin(); it != m_windows.cend(); ++it) {
        if (it.key() != excludedPage && it.value()
            && it.value()->windowTitle().trimmed().compare(
                candidate, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

void DetachableTabWidget::installCloseButton(
    const int index, QWidget *page)
{
    auto *button = new QToolButton(tabBar());
    button->setProperty("protocolDeleteButton", true);
    button->setText(QStringLiteral("×"));
    button->setToolTip(tr("关闭页面"));
    button->setCursor(Qt::ArrowCursor);
    button->setAutoRaise(true);
    button->setFixedSize(22, 22);
    tabBar()->setTabButton(index, QTabBar::RightSide, button);
    connect(button, &QToolButton::clicked, this, [this, page] {
        const int currentIndex = indexOf(page);
        if (currentIndex >= 0) closeWorkspace(currentIndex);
    });
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
    if (!accepted || title.isEmpty()) return;
    if (hasWorkspaceTitle(title, widget(index))) {
        m_context->notify(
            tr("页面名称“%1”已存在").arg(title),
            NotificationType::Warning);
        return;
    }
    setTabText(index, title);
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
    installCloseButton(index, page);
    setCurrentIndex(index);
}
