#pragma once

#include <QWidget>

#include "models/AppTypes.h"
#include "pages/PageId.h"

class IconManager;
class QAbstractAnimation;
class QButtonGroup;
class QFrame;
class QLabel;
class QPushButton;
class QVBoxLayout;

class SideNavigation final : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal indicatorTop READ indicatorTop WRITE setIndicatorTop)
    Q_PROPERTY(qreal indicatorBottom READ indicatorBottom WRITE setIndicatorBottom)

public:
    explicit SideNavigation(IconManager *iconManager, QWidget *parent = nullptr);
    [[nodiscard]] bool isExpanded() const noexcept;
    [[nodiscard]] NavigationMode mode() const noexcept;
    [[nodiscard]] PageId currentPage() const noexcept;
    [[nodiscard]] qreal indicatorTop() const noexcept;
    [[nodiscard]] qreal indicatorBottom() const noexcept;
    void setIndicatorTop(qreal value);
    void setIndicatorBottom(qreal value);

public slots:
    void toggleExpanded();
    void setExpanded(bool expanded);
    void setMode(NavigationMode mode);
    void setUserCardVisible(bool visible);
    void setCurrentPage(PageId page);

signals:
    void pageRequested(PageId page);
    void expandedChanged(bool expanded);

private:
    void addItem(const QString &iconPath, const QString &title, PageId page);
    void updatePresentation();
    void refreshIcons();
    void animateIndicatorTo(PageId page);
    void syncIndicatorToCurrent();

protected:
    void paintEvent(QPaintEvent *event) override;

    IconManager *m_iconManager;
    QVBoxLayout *m_layout{};
    QFrame *m_userCard{};
    QLabel *m_userTitle{};
    QButtonGroup *m_group{};
    QList<QPushButton *> m_buttons;
    QStringList m_iconPaths;
    QStringList m_titles;
    QList<PageId> m_pageIds;
    PageId m_currentPage{PageId::Plot};
    bool m_expanded{true};
    bool m_userCardEnabled{true};
    NavigationMode m_mode{NavigationMode::Expanded};
    qreal m_indicatorTop{};
    qreal m_indicatorBottom{};
    bool m_indicatorInitialized{};
    QAbstractAnimation *m_indicatorAnimation{};
};
