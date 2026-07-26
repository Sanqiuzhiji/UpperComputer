#pragma once

#include <QScrollArea>

#include "models/ProtocolTypes.h"

class QFrame;
class QVBoxLayout;
class ProtocolFrameWidget;

class ProtocolCanvas final : public QScrollArea
{
    Q_OBJECT

public:
    explicit ProtocolCanvas(QWidget *parent = nullptr);

    void setDocument(
        const ProtocolModel::Document &document,
        const QVector<ProtocolModel::ValidationIssue> &issues);
    void setSelection(
        const QUuid &frameId, const QUuid &fieldId = {});

signals:
    void documentSelected();
    void frameSelected(const QUuid &frameId);
    void frameDeleteRequested(const QUuid &frameId);
    void fieldSelected(const QUuid &frameId, const QUuid &fieldId);
    void fieldDeleteRequested(
        const QUuid &frameId, const QUuid &fieldId);
    void addFrameRequested();
    void fieldTemplateDropped(
        ProtocolModel::FieldRole role,
        const QUuid &targetFrameId,
        int targetIndex);
    void fieldMoveRequested(
        const QUuid &sourceFrameId,
        const QUuid &fieldId,
        const QUuid &targetFrameId,
        int targetIndex);
    void frameMoveRequested(const QUuid &frameId, int targetIndex);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void rebuild();
    [[nodiscard]] int frameInsertionIndex(int viewportY) const;
    void showFrameInsertionLine(int index);
    void hideFrameInsertionLine();
    void autoScroll(const QPoint &globalPosition);

    QWidget *m_content{};
    QVBoxLayout *m_layout{};
    QFrame *m_frameInsertionLine{};
    QList<ProtocolFrameWidget *> m_frameWidgets;
    ProtocolModel::Document m_document;
    QVector<ProtocolModel::ValidationIssue> m_issues;
    QUuid m_selectedFrameId;
    QUuid m_selectedFieldId;
};
