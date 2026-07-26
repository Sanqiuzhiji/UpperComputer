#include "ProtocolCanvas.h"

#include "pages/protocol/ProtocolFrameWidget.h"
#include "pages/protocol/ProtocolMime.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>

ProtocolCanvas::ProtocolCanvas(QWidget *parent)
    : QScrollArea(parent)
{
    setObjectName(QStringLiteral("protocolCanvas"));
    setWidgetResizable(true);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
    viewport()->installEventFilter(this);

    m_content = new QWidget(this);
    m_content->setObjectName(QStringLiteral("protocolCanvasContent"));
    m_content->setMinimumWidth(560);
    m_layout = new QVBoxLayout(m_content);
    m_layout->setContentsMargins(8, 8, 8, 8);
    m_layout->setSpacing(7);
    setWidget(m_content);

    m_frameInsertionLine = new QFrame(viewport());
    m_frameInsertionLine->setObjectName(
        QStringLiteral("protocolFrameInsertionLine"));
    m_frameInsertionLine->setFixedHeight(3);
    m_frameInsertionLine->hide();
}

void ProtocolCanvas::setDocument(
    const ProtocolModel::Document &document,
    const QVector<ProtocolModel::ValidationIssue> &issues)
{
    m_document = document;
    m_issues = issues;
    rebuild();
}

void ProtocolCanvas::setSelection(
    const QUuid &frameId, const QUuid &fieldId)
{
    m_selectedFrameId = frameId;
    m_selectedFieldId = fieldId;
    for (ProtocolFrameWidget *widget : std::as_const(m_frameWidgets)) {
        widget->setSelection(frameId, fieldId);
    }
}

bool ProtocolCanvas::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == viewport() && event->type() == QEvent::MouseButtonPress) {
        const auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() == Qt::LeftButton
            && viewport()->childAt(mouse->position().toPoint()) == m_content) {
            emit documentSelected();
        }
    }
    return QScrollArea::eventFilter(watched, event);
}

void ProtocolCanvas::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(
            QLatin1String(ProtocolMime::Frame))) {
        event->setDropAction(Qt::MoveAction);
        event->accept();
        return;
    }
    event->ignore();
}

void ProtocolCanvas::dragMoveEvent(QDragMoveEvent *event)
{
    if (!event->mimeData()->hasFormat(
            QLatin1String(ProtocolMime::Frame))) {
        event->ignore();
        return;
    }
    showFrameInsertionLine(
        frameInsertionIndex(event->position().toPoint().y()));
    autoScroll(viewport()->mapToGlobal(event->position().toPoint()));
    event->accept();
}

void ProtocolCanvas::dragLeaveEvent(QDragLeaveEvent *event)
{
    hideFrameInsertionLine();
    QScrollArea::dragLeaveEvent(event);
}

void ProtocolCanvas::dropEvent(QDropEvent *event)
{
    hideFrameInsertionLine();
    if (!event->mimeData()->hasFormat(
            QLatin1String(ProtocolMime::Frame))) {
        event->ignore();
        return;
    }
    const QUuid frameId(QString::fromUtf8(
        event->mimeData()->data(QLatin1String(ProtocolMime::Frame))));
    if (frameId.isNull()) {
        event->ignore();
        return;
    }
    emit frameMoveRequested(
        frameId, frameInsertionIndex(event->position().toPoint().y()));
    event->setDropAction(Qt::MoveAction);
    event->accept();
}

void ProtocolCanvas::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) emit documentSelected();
    QScrollArea::mousePressEvent(event);
}

void ProtocolCanvas::rebuild()
{
    while (QLayoutItem *item = m_layout->takeAt(0)) {
        if (QWidget *widget = item->widget()) widget->deleteLater();
        delete item;
    }
    m_frameWidgets.clear();
    m_content->setMinimumWidth(560);
    if (m_document.frames.isEmpty()) {
        auto *empty = new QFrame(m_content);
        empty->setProperty("protocolEmptyState", true);
        auto *layout = new QVBoxLayout(empty);
        layout->setContentsMargins(30, 50, 30, 50);
        auto *label = new QLabel(
            QStringLiteral("当前协议中还没有协议帧"), empty);
        label->setAlignment(Qt::AlignCenter);
        label->setProperty("muted", true);
        layout->addWidget(label);
        auto *button = new QPushButton(
            QStringLiteral("+ 添加协议帧"), empty);
        button->setProperty("accent", true);
        button->setMaximumWidth(160);
        layout->addWidget(button, 0, Qt::AlignHCenter);
        connect(button, &QPushButton::clicked,
                this, &ProtocolCanvas::addFrameRequested);
        m_layout->addWidget(empty);
    } else {
        int maximumFields = 0;
        for (const ProtocolModel::Frame &frame : m_document.frames) {
            maximumFields = qMax(maximumFields, frame.fields.size());
            auto *widget = new ProtocolFrameWidget(
                frame, m_issues, m_content);
            m_frameWidgets.append(widget);
            m_layout->addWidget(widget);
            connect(widget, &ProtocolFrameWidget::frameSelected,
                    this, &ProtocolCanvas::frameSelected);
            connect(widget, &ProtocolFrameWidget::frameDeleteRequested,
                    this, &ProtocolCanvas::frameDeleteRequested);
            connect(widget, &ProtocolFrameWidget::fieldSelected,
                    this, &ProtocolCanvas::fieldSelected);
            connect(widget, &ProtocolFrameWidget::fieldDeleteRequested,
                    this, &ProtocolCanvas::fieldDeleteRequested);
            connect(widget, &ProtocolFrameWidget::fieldTemplateDropped,
                    this, &ProtocolCanvas::fieldTemplateDropped);
            connect(widget, &ProtocolFrameWidget::fieldMoveRequested,
                    this, &ProtocolCanvas::fieldMoveRequested);
            connect(widget, &ProtocolFrameWidget::dragMoved,
                    this, &ProtocolCanvas::autoScroll);
            connect(
                widget, &ProtocolFrameWidget::frameDragMoved,
                this, [this](const QPoint &globalPosition) {
                    const QPoint viewportPosition =
                        viewport()->mapFromGlobal(globalPosition);
                    showFrameInsertionLine(
                        frameInsertionIndex(viewportPosition.y()));
                    autoScroll(globalPosition);
                });
            connect(
                widget, &ProtocolFrameWidget::frameDragFinished,
                this, &ProtocolCanvas::hideFrameInsertionLine);
            connect(
                widget, &ProtocolFrameWidget::frameDropRequested,
                this,
                [this](
                    const QUuid &sourceFrameId,
                    const QUuid &targetFrameId,
                    const bool insertAfter) {
                    int targetIndex = -1;
                    for (int index = 0;
                         index < m_frameWidgets.size(); ++index) {
                        if (m_frameWidgets.at(index)->frameId()
                            == targetFrameId) {
                            targetIndex = index;
                            break;
                        }
                    }
                    hideFrameInsertionLine();
                    if (targetIndex >= 0) {
                        emit frameMoveRequested(
                            sourceFrameId,
                            targetIndex + (insertAfter ? 1 : 0));
                    }
                });
        }
        m_content->setMinimumWidth(
            qMax(560, 210 + maximumFields * 162));
    }
    m_layout->addStretch();
    setSelection(m_selectedFrameId, m_selectedFieldId);
}

int ProtocolCanvas::frameInsertionIndex(const int viewportY) const
{
    for (int index = 0; index < m_frameWidgets.size(); ++index) {
        const QPoint point = m_frameWidgets.at(index)->mapTo(
            viewport(), QPoint{});
        if (viewportY < point.y() + m_frameWidgets.at(index)->height() / 2) {
            return index;
        }
    }
    return m_frameWidgets.size();
}

void ProtocolCanvas::showFrameInsertionLine(const int index)
{
    int y = 8;
    if (!m_frameWidgets.isEmpty()) {
        if (index < m_frameWidgets.size()) {
            y = m_frameWidgets.at(index)->mapTo(
                    viewport(), QPoint{}).y()
                - 6;
        } else {
            ProtocolFrameWidget *last = m_frameWidgets.constLast();
            y = last->mapTo(viewport(), QPoint{}).y()
                + last->height() + 4;
        }
    }
    m_frameInsertionLine->setGeometry(
        8, y, qMax(40, viewport()->width() - 16), 3);
    m_frameInsertionLine->show();
    m_frameInsertionLine->raise();
}

void ProtocolCanvas::hideFrameInsertionLine()
{
    m_frameInsertionLine->hide();
}

void ProtocolCanvas::autoScroll(const QPoint &globalPosition)
{
    const QPoint local = viewport()->mapFromGlobal(globalPosition);
    constexpr int edge = 36;
    constexpr int step = 24;
    if (local.y() < edge) {
        verticalScrollBar()->setValue(
            verticalScrollBar()->value() - step);
    } else if (local.y() > viewport()->height() - edge) {
        verticalScrollBar()->setValue(
            verticalScrollBar()->value() + step);
    }
    if (local.x() < edge) {
        horizontalScrollBar()->setValue(
            horizontalScrollBar()->value() - step);
    } else if (local.x() > viewport()->width() - edge) {
        horizontalScrollBar()->setValue(
            horizontalScrollBar()->value() + step);
    }
}
