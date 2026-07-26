#pragma once

#include <QFrame>

#include "models/ProtocolTypes.h"

class QLabel;
class QMouseEvent;

class ProtocolFieldCard final : public QFrame
{
    Q_OBJECT

public:
    explicit ProtocolFieldCard(
        const QUuid &frameId,
        const ProtocolModel::Field &field,
        int fieldIndex,
        int byteOffset,
        QWidget *parent = nullptr);

    [[nodiscard]] QUuid fieldId() const;
    void setSelected(bool selected);

signals:
    void selected(const QUuid &fieldId);
    void deleteRequested(const QUuid &fieldId);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QUuid m_frameId;
    ProtocolModel::Field m_field;
    QPoint m_dragStart;
};
