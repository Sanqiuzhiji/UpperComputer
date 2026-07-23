#pragma once

#include <QWidget>

class QLabel;
class QToolButton;

class TitleBar final : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = nullptr);
    void setMaximized(bool maximized);

signals:
    void backRequested();
    void forwardRequested();
    void navigationToggleRequested();
    void pinToggleRequested(bool pinned);
    void themeToggleRequested();
    void minimizeRequested();
    void maximizeRestoreRequested();
    void closeRequested();

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QToolButton *createButton(const QString &text, const QString &toolTip);

    QToolButton *m_maximizeButton{};
};
