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
    setMinimumHeight(58);
    setMaximumHeight(66);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 7, 12, 7);
    layout->setSpacing(8);

    auto *title = new QLabel(tr("收发模式"), this);
    title->setObjectName(QStringLiteral("sectionTitle"));
    layout->addWidget(title);

    m_helpButton = new QToolButton(this);
    m_helpButton->setIcon(context->iconManager()->icon(
        QStringLiteral(":/icons/connection/help.svg")));
    m_helpButton->setToolTip(tr("查看收发模式说明"));
    layout->addWidget(m_helpButton);

    layout->addWidget(mutedLabel(tr("接收"), this));
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
    m_receiveCombo->setMinimumWidth(105);
    layout->addWidget(m_receiveCombo);

    layout->addWidget(mutedLabel(tr("发送"), this));
    m_sendCombo = new QComboBox(this);
    m_sendCombo->setObjectName(QStringLiteral("sendModeCombo"));
    addModeItem(m_sendCombo, QStringLiteral("RawData"), SendMode::RawData);
    addModeItem(m_sendCombo, QStringLiteral("CustomBinary"),
                SendMode::CustomBinary);
    m_sendCombo->setMinimumWidth(105);
    layout->addWidget(m_sendCombo);

    m_customOptionsRow = new QWidget(this);
    auto *customLayout = new QHBoxLayout(m_customOptionsRow);
    customLayout->setContentsMargins(0, 0, 0, 0);
    customLayout->setSpacing(8);

    m_protocolLabel = mutedLabel(tr("协议"), this);
    customLayout->addWidget(m_protocolLabel);
    m_protocolCombo = new QComboBox(this);
    m_protocolCombo->setObjectName(QStringLiteral("customProtocolCombo"));
    m_protocolCombo->setMinimumWidth(110);
    customLayout->addWidget(m_protocolCombo);

    m_receiveCommandLabel = mutedLabel(tr("接收命令"), this);
    customLayout->addWidget(m_receiveCommandLabel);
    m_receiveCommandCombo = new QComboBox(this);
    m_receiveCommandCombo->setObjectName(
        QStringLiteral("customReceiveCommandCombo"));
    m_receiveCommandCombo->setMinimumWidth(110);
    customLayout->addWidget(m_receiveCommandCombo);

    m_sendCommandLabel = mutedLabel(tr("发送命令"), this);
    customLayout->addWidget(m_sendCommandLabel);
    m_sendCommandCombo = new QComboBox(this);
    m_sendCommandCombo->setObjectName(
        QStringLiteral("customCommandCombo"));
    m_sendCommandCombo->setMinimumWidth(110);
    customLayout->addWidget(m_sendCommandCombo);

    m_openLibraryButton = new QPushButton(tr("打开协议库"), this);
    m_openLibraryButton->setObjectName(
        QStringLiteral("openProtocolLibraryButton"));
    m_openLibraryButton->setToolTip(tr("打开协议编辑器/协议库"));
    customLayout->addWidget(m_openLibraryButton);
    layout->addWidget(m_customOptionsRow);
    layout->addStretch();

    new FocusUnderline(m_receiveCombo, context->themeManager());
    new FocusUnderline(m_sendCombo, context->themeManager());
    new FocusUnderline(m_protocolCombo, context->themeManager());
    new FocusUnderline(
        m_receiveCommandCombo, context->themeManager());
    new FocusUnderline(
        m_sendCommandCombo, context->themeManager());

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
        updateCommandChoices();
        emit customProtocolChanged(id);
        emit receiveCommandChanged(currentReceiveCommandId());
        emit sendCommandChanged(currentSendCommandId());
    });
    connect(m_receiveCommandCombo,
            &QComboBox::currentIndexChanged, this, [this] {
                const QString id = currentReceiveCommandId();
                m_context->settings()->setCustomReceiveCommandId(id);
                emit receiveCommandChanged(id);
    });
    connect(m_sendCommandCombo,
            &QComboBox::currentIndexChanged, this, [this] {
                const QString id = currentSendCommandId();
                m_context->settings()->setCustomCommandId(id);
                emit sendCommandChanged(id);
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

QString CommunicationModePanel::currentReceiveCommandId() const
{
    return m_receiveCommandCombo->currentData().toString();
}

QString CommunicationModePanel::currentSendCommandId() const
{
    return m_sendCommandCombo->currentData().toString();
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
        updateCommandChoices();
        updateCustomProtocolVisibility();
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
    updateCommandChoices();
    updateCustomProtocolVisibility();
}

void CommunicationModePanel::updateCustomProtocolVisibility()
{
    const bool visible = receiveMode() == ParserMode::CustomBinary
        || sendMode() == SendMode::CustomBinary;
    m_customOptionsRow->setVisible(visible);
    const bool receiveCommandVisible =
        receiveMode() == ParserMode::CustomBinary;
    m_receiveCommandLabel->setVisible(receiveCommandVisible);
    m_receiveCommandCombo->setVisible(receiveCommandVisible);
    const bool sendCommandVisible =
        sendMode() == SendMode::CustomBinary;
    m_sendCommandLabel->setVisible(sendCommandVisible);
    m_sendCommandCombo->setVisible(sendCommandVisible);
}

void CommunicationModePanel::updateCommandChoices()
{
    const QString savedReceive =
        m_context->settings()->customReceiveCommandId();
    const QString savedSend =
        m_context->settings()->customCommandId();
    const QSignalBlocker receiveBlocker(
        m_receiveCommandCombo);
    const QSignalBlocker sendBlocker(m_sendCommandCombo);
    m_receiveCommandCombo->clear();
    m_sendCommandCombo->clear();
    const QString protocolId = currentProtocolId();
    for (const ProtocolDefinition &protocol : m_protocols) {
        if (protocol.id != protocolId) continue;
        for (const MessageDefinition &message :
             protocol.receiveMessages) {
            const QString name = message.displayName.trimmed().isEmpty()
                ? message.id : message.displayName;
            m_receiveCommandCombo->addItem(name, message.id);
        }
        for (const MessageDefinition &message :
             protocol.sendMessages) {
            const QString name = message.displayName.trimmed().isEmpty()
                ? message.id : message.displayName;
            m_sendCommandCombo->addItem(name, message.id);
        }
        break;
    }
    if (m_receiveCommandCombo->count() == 0) {
        m_receiveCommandCombo->addItem(
            tr("无可接收命令"), QString());
        m_receiveCommandCombo->setEnabled(false);
    } else {
        m_receiveCommandCombo->setEnabled(true);
        const int savedIndex =
            m_receiveCommandCombo->findData(savedReceive);
        if (savedIndex >= 0) {
            m_receiveCommandCombo->setCurrentIndex(savedIndex);
        }
        m_context->settings()->setCustomReceiveCommandId(
            currentReceiveCommandId());
    }
    if (m_sendCommandCombo->count() == 0) {
        m_sendCommandCombo->addItem(
            tr("无可发送命令"), QString());
        m_sendCommandCombo->setEnabled(false);
    } else {
        m_sendCommandCombo->setEnabled(true);
        const int savedIndex =
            m_sendCommandCombo->findData(savedSend);
        if (savedIndex >= 0) {
            m_sendCommandCombo->setCurrentIndex(savedIndex);
        }
        m_context->settings()->setCustomCommandId(
            currentSendCommandId());
    }
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
        return tr("CustomBinary 使用共享协议选择，按协议中的帧头、长度、"
                  "字段类型、字节序和校验规则自动拆帧并显示字段值。");
    }
    return {};
}
