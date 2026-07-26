#include "FieldLibraryPanel.h"

#include "models/ProtocolTypes.h"
#include "pages/protocol/ProtocolMime.h"

#include <QDrag>
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>
#include <QVBoxLayout>

using namespace ProtocolModel;

namespace {

class FieldTemplateWidget final : public QFrame
{
public:
    explicit FieldTemplateWidget(const FieldRole role, QWidget *parent)
        : QFrame(parent),
          m_role(role)
    {
        setProperty("protocolTemplate", true);
        setProperty("fieldRole", roleKey(role));
        setCursor(Qt::OpenHandCursor);
        setToolTip(QStringLiteral("按住并拖到任意协议帧的字段间隙"));
        setMinimumHeight(42);

        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(10, 4, 5, 4);
        layout->setSpacing(8);
        auto *name = new QLabel(roleDisplayName(role), this);
        layout->addWidget(name, 1);
        auto *stripe = new QFrame(this);
        stripe->setProperty("roleStripe", true);
        stripe->setProperty("fieldRole", roleKey(role));
        stripe->setFixedWidth(4);
        layout->addWidget(stripe);
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_startPosition = event->position().toPoint();
            setCursor(Qt::ClosedHandCursor);
        }
        QFrame::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        setCursor(Qt::OpenHandCursor);
        QFrame::mouseReleaseEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (!(event->buttons() & Qt::LeftButton)
            || (event->position().toPoint() - m_startPosition)
                    .manhattanLength()
                < QApplication::startDragDistance()) {
            QFrame::mouseMoveEvent(event);
            return;
        }

        auto *mime = new QMimeData;
        mime->setData(
            QLatin1String(ProtocolMime::FieldTemplate),
            roleKey(m_role).toUtf8());
        auto *drag = new QDrag(this);
        drag->setMimeData(mime);
        drag->setPixmap(grab());
        drag->setHotSpot(m_startPosition);
        drag->exec(Qt::CopyAction);
        setCursor(Qt::OpenHandCursor);
    }

private:
    FieldRole m_role;
    QPoint m_startPosition;
};

} // namespace

FieldLibraryPanel::FieldLibraryPanel(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("fieldLibraryPanel"));
    setProperty("card", true);
    setMinimumWidth(180);
    setMaximumWidth(240);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(7);
    auto *title = new QLabel(QStringLiteral("字段组件库"), this);
    title->setObjectName(QStringLiteral("sectionTitle"));
    layout->addWidget(title);
    auto *hint = new QLabel(QStringLiteral("直接拖到协议帧中的目标位置"), this);
    hint->setProperty("muted", true);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    constexpr FieldRole roles[]{
        FieldRole::Header,
        FieldRole::FrameId,
        FieldRole::Length,
        FieldRole::Data,
        FieldRole::Checksum,
        FieldRole::Tail,
        FieldRole::Skip
    };
    for (const FieldRole role : roles) {
        layout->addWidget(new FieldTemplateWidget(role, this));
    }
    layout->addStretch();
}
