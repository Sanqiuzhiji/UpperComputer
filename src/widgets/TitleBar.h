#pragma once

#include <QWidget>

#include "models/AppTypes.h"

class IconManager;
class QLabel;
class QToolButton;

class TitleBar final : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(IconManager *iconManager, QWidget *parent = nullptr);
    void setMaximized(bool maximized);
    void setConnectionState(ConnectionState state);
    [[nodiscard]] QPoint themeButtonCenter(QWidget *target) const;

signals:
    void navigationToggleRequested();
    void connectionToggleRequested();
    void pinToggleRequested(bool pinned);
    void themeToggleRequested();
    void minimizeRequested();
    void maximizeRestoreRequested();
    void closeRequested();

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void refreshIcons();
    QToolButton *createButton(const QString &iconPath,
                              const QString &toolTip,
                              const QSize &size,
                              const QColor &activeColor = QColor("#28A9E0"));

    IconManager *m_iconManager;
    QLabel *m_appIcon{};
    QToolButton *m_menuButton{};
    QToolButton *m_connectionButton{};
    QToolButton *m_pinButton{};
    QToolButton *m_themeButton{};
    QToolButton *m_minimizeButton{};
    QToolButton *m_maximizeButton{};
    QToolButton *m_closeButton{};
    bool m_maximized{};
    bool m_pinned{};
};
