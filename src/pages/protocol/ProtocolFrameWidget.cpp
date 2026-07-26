#include "ProtocolFrameWidget.h"

#include "pages/protocol/ProtocolFieldCard.h"
#include "pages/protocol/ProtocolMime.h"

#include <QApplication>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include <functional>
#include <utility>

using namespace ProtocolModel;

namespace {

constexpr int kProtocolFrameHeight = 120;

class FrameDragHandle final : public QToolButton
{
public:
    explicit FrameDragHandle(
        QWidget *dragPreview,
        std::function<void(const QPoint &)> startDrag,
        QWidget *parent)
        : QToolButton(parent),
          m_dragPreview(dragPreview),
          m_startDrag(std::move(startDrag))
    {
        setText(QStringLiteral("⋮⋮"));
        setToolTip(QStringLiteral("拖动协议帧"));
        setObjectName(QStringLiteral("protocolFrameDragHandle"));
        setCursor(Qt::OpenHandCursor);
        setFixedWidth(28);
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_start = event->position().toPoint();
            m_dragArmed = true;
        }
        QToolButton::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (!m_dragArmed
            || !(event->buttons() & Qt::LeftButton)
            || (event->position().toPoint() - m_start).manhattanLength()
                < QApplication::startDragDistance()) {
            QToolButton::mouseMoveEvent(event);
            return;
        }
        const QPoint hotSpot = mapTo(m_dragPreview, m_start);
        m_dragArmed = false;
        m_startDrag(hotSpot);
        event->accept();
    }

private:
    QWidget *m_dragPreview{};
    std::function<void(const QPoint &)> m_startDrag;
    QPoint m_start;
    bool m_dragArmed{false};
};

FieldRole roleFromMime(const QByteArray &data)
{
    const QString key = QString::fromUtf8(data);
    constexpr FieldRole roles[]{
        FieldRole::Header, FieldRole::FrameId, FieldRole::Length,
        FieldRole::Data, FieldRole::Checksum, FieldRole::Tail, FieldRole::Skip
    };
    for (const FieldRole role : roles) {
        if (roleKey(role) == key) return role;
    }
    return FieldRole::Data;
}

} // namespace

ProtocolFrameWidget::ProtocolFrameWidget(
    const Frame &frame,
    const QVector<ValidationIssue> &issues,
    QWidget *parent)
    : QFrame(parent),
      m_frame(frame)
{
    setObjectName(QStringLiteral("protocolFrameWidget"));
    setProperty("protocolFrame", true);
    setProperty("selected", false);
    setAcceptDrops(true);
    setFixedHeight(kProtocolFrameHeight);
    QSizePolicy frameSizePolicy = sizePolicy();
    frameSizePolicy.setVerticalPolicy(QSizePolicy::Fixed);
    setSizePolicy(frameSizePolicy);

    bool hasError = false;
    bool hasWarning = false;
    QStringList issueTexts;
    for (const ValidationIssue &issue : issues) {
        if (issue.frameId != frame.id) continue;
        issueTexts.append(issue.message);
        hasError |= issue.severity == ValidationSeverity::Error;
        hasWarning |= issue.severity == ValidationSeverity::Warning;
    }
    setProperty("validationState",
                hasError ? "error" : hasWarning ? "warning" : "valid");
    if (!issueTexts.isEmpty()) setToolTip(issueTexts.join(QLatin1Char('\n')));

    auto *outer = new QHBoxLayout(this);
    outer->setContentsMargins(4, 4, 4, 4);
    outer->setSpacing(4);

    auto *info = new QWidget(this);
    info->setObjectName(QStringLiteral("protocolFrameInfo"));
    info->setFixedWidth(154);
    auto *infoLayout = new QHBoxLayout(info);
    infoLayout->setContentsMargins(0, 0, 2, 0);
    infoLayout->setSpacing(3);
    infoLayout->addWidget(new FrameDragHandle(
        this,
        [this](const QPoint &hotSpot) {
            startFrameDrag(hotSpot);
        },
        info));
    auto *details = new QVBoxLayout;
    details->setSpacing(1);
    auto *statusLine = new QHBoxLayout;
    statusLine->setSpacing(3);
    QString directionText;
    switch (frame.direction) {
    case FrameDirection::Bidirectional:
        directionText = QStringLiteral("Tx / Rx");
        break;
    case FrameDirection::TransmitOnly:
        directionText = QStringLiteral("Tx");
        break;
    case FrameDirection::ReceiveOnly:
        directionText = QStringLiteral("Rx");
        break;
    }
    auto *direction = new QLabel(directionText, info);
    direction->setProperty("muted", true);
    direction->setToolTip(
        frame.direction == FrameDirection::Bidirectional
            ? QStringLiteral("双向 Frame")
        : frame.direction == FrameDirection::TransmitOnly
            ? QStringLiteral("仅发送 Frame")
            : QStringLiteral("仅接收 Frame"));
    statusLine->addWidget(direction);
    if (hasError || hasWarning) {
        auto *status = new QLabel(
            hasError ? QStringLiteral("● 错误")
                     : QStringLiteral("● 警告"),
            info);
        status->setProperty(
            "validationState",
            hasError ? "error" : "warning");
        statusLine->addWidget(status);
    }
    statusLine->addStretch();
    auto *deleteButton = new QToolButton(info);
    deleteButton->setObjectName(QStringLiteral("protocolFrameDeleteButton"));
    deleteButton->setProperty("protocolDeleteButton", true);
    deleteButton->setText(QStringLiteral("×"));
    deleteButton->setToolTip(QStringLiteral("删除此协议"));
    deleteButton->setCursor(Qt::ArrowCursor);
    deleteButton->setAutoRaise(true);
    deleteButton->setFixedSize(22, 22);
    connect(deleteButton, &QToolButton::clicked, this, [this] {
        emit frameDeleteRequested(m_frame.id);
    });
    statusLine->addWidget(deleteButton);
    details->addLayout(statusLine);
    auto *name = new QLabel(frame.name, info);
    name->setObjectName(QStringLiteral("protocolFrameName"));
    name->setWordWrap(true);
    details->addWidget(name);
    auto *bytes = new QLabel(
        QStringLiteral("%1 个字段 · %2 B")
            .arg(frame.fields.size())
            .arg(frameByteCount(frame)),
        info);
    bytes->setProperty("muted", true);
    details->addWidget(bytes);
    details->addStretch();
    infoLayout->addLayout(details, 1);
    outer->addWidget(info);

    m_fieldsHost = new QWidget(this);
    m_fieldsHost->setObjectName(QStringLiteral("protocolFieldsHost"));
    m_fieldsLayout = new QHBoxLayout(m_fieldsHost);
    m_fieldsLayout->setContentsMargins(4, 2, 4, 2);
    m_fieldsLayout->setSpacing(4);

    if (frame.fields.isEmpty()) {
        auto *empty = new QLabel(
            QStringLiteral("将字段从左侧拖到此处"), m_fieldsHost);
        empty->setObjectName(QStringLiteral("protocolFrameEmpty"));
        empty->setAlignment(Qt::AlignCenter);
        empty->setProperty("muted", true);
        empty->setMinimumWidth(260);
        m_fieldsLayout->addWidget(empty);
    } else {
        const QVector<int> offsets = fieldOffsets(frame);
        for (int index = 0; index < frame.fields.size(); ++index) {
            auto *card = new ProtocolFieldCard(
                frame.id, frame.fields.at(index), index,
                offsets.at(index), m_fieldsHost);
            m_cards.append(card);
            m_fieldsLayout->addWidget(card);
            connect(card, &ProtocolFieldCard::selected, this,
                    [this](const QUuid &fieldId) {
                        emit fieldSelected(m_frame.id, fieldId);
                    });
            connect(card, &ProtocolFieldCard::deleteRequested, this,
                    [this](const QUuid &fieldId) {
                        emit fieldDeleteRequested(m_frame.id, fieldId);
                    });
        }
    }
    m_fieldsLayout->addStretch();
    outer->addWidget(m_fieldsHost, 1);

    m_insertionLine = new QFrame(m_fieldsHost);
    m_insertionLine->setObjectName(QStringLiteral("protocolInsertionLine"));
    m_insertionLine->setFixedWidth(3);
    m_insertionLine->hide();
}

QUuid ProtocolFrameWidget::frameId() const
{
    return m_frame.id;
}

void ProtocolFrameWidget::setSelection(
    const QUuid &selectedFrameId, const QUuid &selectedFieldId)
{
    const bool frameSelected =
        selectedFrameId == m_frame.id && selectedFieldId.isNull();
    setProperty("selected", frameSelected);
    style()->unpolish(this);
    style()->polish(this);
    for (ProtocolFieldCard *card : std::as_const(m_cards)) {
        card->setSelected(card->fieldId() == selectedFieldId);
    }
}

void ProtocolFrameWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_frameDragStart = event->position().toPoint();
        m_frameDragArmed = true;
        emit frameSelected(m_frame.id);
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void ProtocolFrameWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_frameDragArmed
        || !(event->buttons() & Qt::LeftButton)
        || (event->position().toPoint() - m_frameDragStart).manhattanLength()
            < QApplication::startDragDistance()) {
        QFrame::mouseMoveEvent(event);
        return;
    }
    const QPoint hotSpot = m_frameDragStart;
    m_frameDragArmed = false;
    startFrameDrag(hotSpot);
    event->accept();
}

void ProtocolFrameWidget::startFrameDrag(const QPoint &hotSpot)
{
    auto *mime = new QMimeData;
    mime->setData(
        QLatin1String(ProtocolMime::Frame),
        m_frame.id.toString(QUuid::WithoutBraces).toUtf8());
    auto *drag = new QDrag(this);
    drag->setMimeData(mime);
    QPixmap preview = grab();
    preview.setDevicePixelRatio(devicePixelRatioF());
    drag->setPixmap(preview);
    drag->setHotSpot(hotSpot);
    setCursor(Qt::ClosedHandCursor);
    drag->exec(Qt::MoveAction, Qt::MoveAction);
    unsetCursor();
    emit frameDragFinished();
}

void ProtocolFrameWidget::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData *mime = event->mimeData();
    if (mime->hasFormat(QLatin1String(ProtocolMime::Frame))) {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    } else if (mime->hasFormat(QLatin1String(ProtocolMime::FieldTemplate))) {
        event->setDropAction(Qt::CopyAction);
        event->accept();
    } else if (mime->hasFormat(QLatin1String(ProtocolMime::Field))) {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    } else {
        event->ignore();
    }
}

void ProtocolFrameWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat(
            QLatin1String(ProtocolMime::Frame))) {
        emit frameDragMoved(mapToGlobal(event->position().toPoint()));
        event->setDropAction(Qt::MoveAction);
        event->accept();
        return;
    }
    if (!event->mimeData()->hasFormat(
            QLatin1String(ProtocolMime::FieldTemplate))
        && !event->mimeData()->hasFormat(
            QLatin1String(ProtocolMime::Field))) {
        event->ignore();
        return;
    }
    const QPoint hostPoint = m_fieldsHost->mapFrom(
        this, event->position().toPoint());
    showInsertionLine(insertionIndex(hostPoint.x()));
    emit dragMoved(mapToGlobal(event->position().toPoint()));
    event->accept();
}

void ProtocolFrameWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    hideInsertionLine();
    emit frameDragFinished();
    QFrame::dragLeaveEvent(event);
}

void ProtocolFrameWidget::dropEvent(QDropEvent *event)
{
    const QPoint hostPoint = m_fieldsHost->mapFrom(
        this, event->position().toPoint());
    const int targetIndex = insertionIndex(hostPoint.x());
    hideInsertionLine();
    const QMimeData *mime = event->mimeData();
    if (mime->hasFormat(QLatin1String(ProtocolMime::Frame))) {
        const QUuid sourceFrameId(QString::fromUtf8(
            mime->data(QLatin1String(ProtocolMime::Frame))));
        if (!sourceFrameId.isNull()) {
            emit frameDropRequested(
                sourceFrameId,
                m_frame.id,
                event->position().toPoint().y() >= height() / 2);
            event->setDropAction(Qt::MoveAction);
            event->accept();
            return;
        }
    }
    if (mime->hasFormat(QLatin1String(ProtocolMime::FieldTemplate))) {
        emit fieldTemplateDropped(
            roleFromMime(
                mime->data(QLatin1String(ProtocolMime::FieldTemplate))),
            m_frame.id, targetIndex);
        event->setDropAction(Qt::CopyAction);
        event->accept();
        return;
    }
    if (mime->hasFormat(QLatin1String(ProtocolMime::Field))) {
        const QList<QByteArray> parts =
            mime->data(QLatin1String(ProtocolMime::Field)).split('|');
        if (parts.size() == 2) {
            emit fieldMoveRequested(
                QUuid(QString::fromUtf8(parts.at(0))),
                QUuid(QString::fromUtf8(parts.at(1))),
                m_frame.id, targetIndex);
            event->setDropAction(Qt::MoveAction);
            event->accept();
            return;
        }
    }
    event->ignore();
}

int ProtocolFrameWidget::insertionIndex(const int localX) const
{
    for (int index = 0; index < m_cards.size(); ++index) {
        const ProtocolFieldCard *card = m_cards.at(index);
        if (localX < card->geometry().center().x()) return index;
    }
    return m_cards.size();
}

void ProtocolFrameWidget::showInsertionLine(const int index)
{
    int x = 7;
    if (!m_cards.isEmpty()) {
        if (index < m_cards.size()) {
            x = m_cards.at(index)->geometry().left() - 5;
        } else {
            x = m_cards.constLast()->geometry().right() + 5;
        }
    }
    m_insertionLine->setGeometry(
        x, 3, 3, qMax(20, m_fieldsHost->height() - 6));
    m_insertionLine->show();
    m_insertionLine->raise();
}

void ProtocolFrameWidget::hideInsertionLine()
{
    m_insertionLine->hide();
}
