#pragma once

#include <QFrame>

#include "models/ProtocolTypes.h"

class QLabel;
class QHBoxLayout;
class ProtocolFieldCard;

class ProtocolFrameWidget final : public QFrame
{
    Q_OBJECT

public:
    explicit ProtocolFrameWidget(
        const ProtocolModel::Frame &frame,
        const QVector<ProtocolModel::ValidationIssue> &issues,
        QWidget *parent = nullptr);

    [[nodiscard]] QUuid frameId() const;
    void setSelection(
        const QUuid &selectedFrameId, const QUuid &selectedFieldId);

signals:
    void frameSelected(const QUuid &frameId);
    void frameDeleteRequested(const QUuid &frameId);
    void fieldSelected(const QUuid &frameId, const QUuid &fieldId);
    void fieldDeleteRequested(
        const QUuid &frameId, const QUuid &fieldId);
    void fieldTemplateDropped(
        ProtocolModel::FieldRole role,
        const QUuid &targetFrameId,
        int targetIndex);
    void fieldMoveRequested(
        const QUuid &sourceFrameId,
        const QUuid &fieldId,
        const QUuid &targetFrameId,
        int targetIndex);
    void dragMoved(const QPoint &globalPosition);
    void frameDragMoved(const QPoint &globalPosition);
    void frameDragFinished();
    void frameDropRequested(
        const QUuid &sourceFrameId,
        const QUuid &targetFrameId,
        bool insertAfter);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void startFrameDrag(const QPoint &hotSpot);
    [[nodiscard]] int insertionIndex(int localX) const;
    void showInsertionLine(int index);
    void hideInsertionLine();

    ProtocolModel::Frame m_frame;
    QWidget *m_fieldsHost{};
    QHBoxLayout *m_fieldsLayout{};
    QFrame *m_insertionLine{};
    QList<ProtocolFieldCard *> m_cards;
    QPoint m_frameDragStart;
    bool m_frameDragArmed{false};
};
