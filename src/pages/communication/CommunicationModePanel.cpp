#include "CommunicationModePanel.h"

#include "app/AppContext.h"
#include "app/AppSettings.h"
#include "theme/IconManager.h"
#include "widgets/FocusUnderline.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QToolButton>

namespace {
template<typename Enum>
void addModeItem(QComboBox *combo, const QString &text, const Enum mode)
{
    combo->addItem(text, QVariant::fromValue(mode));
}

QLabel *mutedLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setProperty("muted", true);
    return label;
}
}

CommunicationModePanel::CommunicationModePanel(
    AppContext *context, QWidget *parent)
    : QFrame(parent),
      m_context(context)
{
    setProperty("card", true);
    setObjectName(QStringLiteral("communicationModePanel"));
    setMinimumHeight(68);
    setMaximumHeight(78);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 10, 16, 10);
    layout->setSpacing(8);

    auto *title = new QLabel(tr("收发模式"), this);
    title->setObjectName(QStringLiteral("sectionTitle"));
    layout->addWidget(title);

    m_helpButton = new QToolButton(this);
    m_helpButton->setIcon(context->iconManager()->icon(
        QStringLiteral(":/icons/connection/help.svg")));
    m_helpButton->setToolTip(tr("查看收发模式说明"));
    layout->addWidget(m_helpButton);
    layout->addStretch();

    layout->addWidget(mutedLabel(tr("接收解析"), this));
    m_receiveCombo = new QComboBox(this);
    m_receiveCombo->setObjectName(QStringLiteral("receiveModeCombo"));
    addModeItem(m_receiveCombo, QStringLiteral("RawData"),
                ParserMode::RawData);
    addModeItem(m_receiveCombo, QStringLiteral("FireWater"),
                ParserMode::FireWater);
    addModeItem(m_receiveCombo, QStringLiteral("JustFloat"),
                ParserMode::JustFloat);
    addModeItem(m_receiveCombo, QStringLiteral("CustomBinary"),
                ParserMode::CustomBinary);
    m_receiveCombo->setMinimumWidth(132);
    layout->addWidget(m_receiveCombo);

    layout->addWidget(mutedLabel(tr("发送模式"), this));
    m_sendCombo = new QComboBox(this);
    m_sendCombo->setObjectName(QStringLiteral("sendModeCombo"));
    addModeItem(m_sendCombo, QStringLiteral("RawData"), SendMode::RawData);
    addModeItem(m_sendCombo, QStringLiteral("CustomBinary"),
                SendMode::CustomBinary);
    m_sendCombo->setMinimumWidth(132);
    layout->addWidget(m_sendCombo);

    m_protocolLabel = mutedLabel(tr("自定义协议"), this);
    layout->addWidget(m_protocolLabel);
    m_protocolCombo = new QComboBox(this);
    m_protocolCombo->setObjectName(QStringLiteral("customProtocolCombo"));
    m_protocolCombo->setMinimumWidth(154);
    layout->addWidget(m_protocolCombo);

    m_openLibraryButton = new QPushButton(tr("打开协议库"), this);
    m_openLibraryButton->setObjectName(
        QStringLiteral("openProtocolLibraryButton"));
    m_openLibraryButton->setToolTip(tr("打开协议编辑器/协议库"));
    layout->addWidget(m_openLibraryButton);

    new FocusUnderline(m_receiveCombo, context->themeManager());
    new FocusUnderline(m_sendCombo, context->themeManager());
    new FocusUnderline(m_protocolCombo, context->themeManager());

    const int receiveIndex = m_receiveCombo->findData(
        QVariant::fromValue(context->settings()->parserMode()));
    if (receiveIndex >= 0) m_receiveCombo->setCurrentIndex(receiveIndex);
    const int sendIndex = m_sendCombo->findData(
        QVariant::fromValue(context->settings()->sendMode()));
    if (sendIndex >= 0) m_sendCombo->setCurrentIndex(sendIndex);
    setProtocols({});
    updateCustomProtocolVisibility();

    connect(m_receiveCombo, &QComboBox::currentIndexChanged, this, [this] {
        const ParserMode mode = receiveMode();
        m_context->settings()->setParserMode(mode);
        updateCustomProtocolVisibility();
        emit receiveModeChanged(mode);
    });
    connect(m_sendCombo, &QComboBox::currentIndexChanged, this, [this] {
        const SendMode mode = sendMode();
        m_context->settings()->setSendMode(mode);
        updateCustomProtocolVisibility();
        emit sendModeChanged(mode);
    });
    connect(m_protocolCombo, &QComboBox::currentIndexChanged, this, [this] {
        const QString id = currentProtocolId();
        m_context->settings()->setCustomProtocolId(id);
        emit customProtocolChanged(id);
    });
    connect(m_openLibraryButton, &QPushButton::clicked,
            this, &CommunicationModePanel::requestProtocolLibrary);
    connect(m_helpButton, &QToolButton::clicked, this,
            [this] { emit helpRequested(helpText()); });
    connect(context->iconManager(), &IconManager::iconsChanged, this, [this] {
        m_helpButton->setIcon(m_context->iconManager()->icon(
            QStringLiteral(":/icons/connection/help.svg")));
    });
}

ParserMode CommunicationModePanel::receiveMode() const
{
    return m_receiveCombo->currentData().value<ParserMode>();
}

SendMode CommunicationModePanel::sendMode() const
{
    return m_sendCombo->currentData().value<SendMode>();
}

QString CommunicationModePanel::currentProtocolId() const
{
    return m_protocolCombo->currentData().toString();
}

void CommunicationModePanel::setProtocols(
    const QList<ProtocolDefinition> &protocols)
{
    m_protocols = protocols;
    const QString selected = m_context->settings()->customProtocolId();
    const QSignalBlocker blocker(m_protocolCombo);
    m_protocolCombo->clear();
    if (protocols.isEmpty()) {
        m_protocolCombo->addItem(tr("暂无可用协议"), QString());
        m_protocolCombo->setEnabled(false);
        return;
    }

    for (const ProtocolDefinition &protocol : protocols) {
        const QString name = protocol.displayName.trimmed().isEmpty()
            ? protocol.id : protocol.displayName;
        m_protocolCombo->addItem(name, protocol.id);
    }
    m_protocolCombo->setEnabled(true);
    const int savedIndex = m_protocolCombo->findData(selected);
    if (savedIndex >= 0) {
        m_protocolCombo->setCurrentIndex(savedIndex);
    }
}

void CommunicationModePanel::updateCustomProtocolVisibility()
{
    const bool visible = receiveMode() == ParserMode::CustomBinary
        || sendMode() == SendMode::CustomBinary;
    m_protocolLabel->setVisible(visible);
    m_protocolCombo->setVisible(visible);
    m_openLibraryButton->setVisible(visible);
}

QString CommunicationModePanel::helpText() const
{
    switch (receiveMode()) {
    case ParserMode::RawData:
        return tr("RawData 接收模式显示设备实际收到的原始字节或文本。"
                  "接收模式与发送模式彼此独立。");
    case ParserMode::JustFloat:
        return tr("JustFloat 解析入口已经预留，但当前仓库没有经过确认的"
                  "解析器实现，因此不会把原始字节伪装成解析结果。");
    case ParserMode::FireWater:
        return tr("FireWater 解析入口已经预留，但当前仓库没有经过确认的"
                  "解析器实现，因此不会猜测协议算法。");
    case ParserMode::CustomBinary:
        return tr("CustomBinary 使用共享协议选择。协议库与解析器尚未接入时，"
                  "监视区会显示明确的空状态。");
    }
    return {};
}

