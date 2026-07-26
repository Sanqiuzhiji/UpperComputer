#include "ProtocolFieldCard.h"

#include "pages/protocol/ProtocolMime.h"

#include <QApplication>
#include <QDrag>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

using namespace ProtocolModel;

ProtocolFieldCard::ProtocolFieldCard(
    const QUuid &frameId,
    const Field &field,
    const int fieldIndex,
    const int byteOffset,
    QWidget *parent)
    : QFrame(parent),
      m_frameId(frameId),
      m_field(field)
{
    setObjectName(QStringLiteral("protocolFieldCard"));
    setProperty("protocolCard", true);
    setProperty("selected", false);
    setProperty("fieldRole", roleKey(field.role));
    setCursor(Qt::OpenHandCursor);
    setFixedWidth(154);
    setMinimumHeight(48);
    setToolTip(QStringLiteral("单击编辑字段，拖动可调整顺序"));

    auto *outer = new QHBoxLayout(this);
    outer->setContentsMargins(6, 3, 3, 3);
    outer->setSpacing(3);
    auto *textLayout = new QVBoxLayout;
    textLayout->setSpacing(2);

    auto *heading = new QHBoxLayout;
    heading->setSpacing(4);
    auto *name = new QLabel(field.name, this);
    name->setObjectName(QStringLiteral("protocolFieldName"));
    name->setTextInteractionFlags(Qt::NoTextInteraction);
    heading->addWidget(name, 1);
    auto *sequence = new QLabel(
        QStringLiteral("#%1").arg(fieldIndex + 1), this);
    sequence->setProperty("muted", true);
    heading->addWidget(sequence);
    textLayout->addLayout(heading);

    const int lastByte = byteOffset + qMax(0, field.byteCount) - 1;
    const QString byteRange =
        field.byteCount > 0
            ? (field.byteCount == 1
                   ? QStringLiteral("byte %1").arg(byteOffset)
                   : QStringLiteral("byte %1..%2")
                         .arg(byteOffset)
                         .arg(lastByte))
            : QStringLiteral("byte —");
    auto *meta = new QLabel(
        QStringLiteral("%1 · %2B · %3")
            .arg(dataTypeDisplayName(field.dataType))
            .arg(field.byteCount)
            .arg(byteRange),
        this);
    meta->setProperty("muted", true);
    textLayout->addWidget(meta);
    outer->addLayout(textLayout, 1);

    auto *deleteButton = new QToolButton(this);
    deleteButton->setObjectName(QStringLiteral("protocolFieldDeleteButton"));
    deleteButton->setProperty("protocolDeleteButton", true);
    deleteButton->setText(QStringLiteral("×"));
    deleteButton->setToolTip(QStringLiteral("删除字段"));
    deleteButton->setCursor(Qt::ArrowCursor);
    deleteButton->setAutoRaise(true);
    deleteButton->setFixedSize(22, 22);
    connect(deleteButton, &QToolButton::clicked, this, [this] {
        emit deleteRequested(m_field.id);
    });
    outer->addWidget(deleteButton, 0, Qt::AlignTop);

    auto *stripe = new QFrame(this);
    stripe->setProperty("roleStripe", true);
    stripe->setProperty("fieldRole", roleKey(field.role));
    stripe->setFixedWidth(4);
    outer->addWidget(stripe);
}

QUuid ProtocolFieldCard::fieldId() const
{
    return m_field.id;
}

void ProtocolFieldCard::setSelected(const bool selected)
{
    setProperty("selected", selected);
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void ProtocolFieldCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStart = event->position().toPoint();
        emit selected(m_field.id);
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void ProtocolFieldCard::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)
        || (event->position().toPoint() - m_dragStart).manhattanLength()
            < QApplication::startDragDistance()) {
        QFrame::mouseMoveEvent(event);
        return;
    }
    auto *mime = new QMimeData;
    const QByteArray payload =
        m_frameId.toString(QUuid::WithoutBraces).toUtf8()
        + '|' + m_field.id.toString(QUuid::WithoutBraces).toUtf8();
    mime->setData(QLatin1String(ProtocolMime::Field), payload);
    auto *drag = new QDrag(this);
    drag->setMimeData(mime);
    QPixmap preview = grab();
    preview.setDevicePixelRatio(devicePixelRatioF());
    drag->setPixmap(preview);
    drag->setHotSpot(m_dragStart);
    setProperty("dragging", true);
    style()->unpolish(this);
    style()->polish(this);
    drag->exec(Qt::MoveAction);
    setProperty("dragging", false);
    style()->unpolish(this);
    style()->polish(this);
}
