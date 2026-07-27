#pragma once

#include <QFrame>

class QMouseEvent;

class DashboardItem final : public QFrame
{
    Q_OBJECT

public:
    explicit DashboardItem(QWidget *content, QWidget *parent = nullptr);
    void setEditMode(bool enabled);
    [[nodiscard]] QWidget *content() const noexcept;

signals:
    void geometryEdited(const QRect &geometry);
    void deleteRequested(DashboardItem *item);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void beginInteraction(
        const QPoint &globalPosition, const QPoint &localPosition);
    void continueInteraction(const QPoint &globalPosition);
    void endInteraction();
    [[nodiscard]] bool onResizeHandle(const QPoint &point) const;
    [[nodiscard]] QRect boundedGeometry(const QRect &candidate) const;
    [[nodiscard]] int snap(int value) const;

    QWidget *m_content{};
    bool m_editMode{};
    bool m_dragging{};
    bool m_resizing{};
    QPoint m_pressGlobal;
    QRect m_startGeometry;
};
