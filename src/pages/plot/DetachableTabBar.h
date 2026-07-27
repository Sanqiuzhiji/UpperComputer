#pragma once

#include <QTabBar>

class DetachableTabBar final : public QTabBar
{
    Q_OBJECT

public:
    explicit DetachableTabBar(QWidget *parent = nullptr);

signals:
    void detachRequested(int index, const QPoint &globalPosition);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int m_pressedIndex{-1};
    QPoint m_pressPosition;
    bool m_detached{};
};
