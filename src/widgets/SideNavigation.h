#pragma once

#include <QWidget>

#include "pages/PageId.h"

class QButtonGroup;
class QFrame;
class QLabel;
class QPushButton;
class QVBoxLayout;

class SideNavigation final : public QWidget
{
    Q_OBJECT

public:
    enum class Mode {
        Expanded,
        Compact,
        Auto
    };

    explicit SideNavigation(QWidget *parent = nullptr);
    [[nodiscard]] bool isExpanded() const noexcept;
    [[nodiscard]] PageId currentPage() const noexcept;

public slots:
    void toggleExpanded();
    void setExpanded(bool expanded);
    void setMode(Mode mode);
    void setUserCardVisible(bool visible);
    void setCurrentPage(PageId page);

signals:
    void pageRequested(PageId page, const QString &title);
    void expandedChanged(bool expanded);

private:
    void addItem(const QString &symbol, const QString &title, PageId page);
    void updatePresentation();

    QVBoxLayout *m_layout{};
    QFrame *m_userCard{};
    QLabel *m_userTitle{};
    QButtonGroup *m_group{};
    QList<QPushButton *> m_buttons;
    QStringList m_symbols;
    QStringList m_titles;
    QList<PageId> m_pageIds;
    PageId m_currentPage{PageId::Plot};
    bool m_expanded{true};
    bool m_userCardEnabled{true};
    Mode m_mode{Mode::Expanded};
};
