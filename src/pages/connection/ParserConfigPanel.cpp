#include "ParserConfigPanel.h"

#include "app/AppContext.h"
#include "app/AppSettings.h"
#include "theme/IconManager.h"
#include "widgets/FocusUnderline.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

namespace {
void addParserItem(QComboBox *combo, const QString &text, const ParserMode mode)
{
    combo->addItem(text, QVariant::fromValue(mode));
}
}

ParserConfigPanel::ParserConfigPanel(AppContext *context, QWidget *parent)
    : QFrame(parent),
      m_context(context)
{
    setProperty("card", true);
    setObjectName(QStringLiteral("parserConfigPanel"));
    setMinimumHeight(68);
    setMaximumHeight(78);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(10);
    auto *title = new QLabel(tr("解析配置"), this);
    title->setObjectName(QStringLiteral("sectionTitle"));
    layout->addWidget(title);
    layout->addStretch();

    m_helpButton = new QToolButton(this);
    m_helpButton->setIcon(context->iconManager()->icon(
        QStringLiteral(":/icons/connection/help.svg")));
    m_helpButton->setToolTip(tr("查看当前解析模式说明"));
    layout->addWidget(m_helpButton);

    auto *modeLabel = new QLabel(tr("解析模式"), this);
    modeLabel->setProperty("muted", true);
    layout->addWidget(modeLabel);
    m_parserCombo = new QComboBox(this);
    addParserItem(m_parserCombo, QStringLiteral("RawData"), ParserMode::RawData);
    addParserItem(m_parserCombo, QStringLiteral("JustFloat"), ParserMode::JustFloat);
    addParserItem(m_parserCombo, QStringLiteral("FireWater"), ParserMode::FireWater);
    addParserItem(m_parserCombo, QStringLiteral("CustomBinary"), ParserMode::CustomBinary);
    m_parserCombo->setMinimumWidth(140);
    layout->addWidget(m_parserCombo);

    m_protocolCombo = new QComboBox(this);
    m_protocolCombo->addItem(tr("暂无可用协议"), QString());
    m_protocolCombo->setEnabled(false);
    m_protocolCombo->setMinimumWidth(160);
    layout->addWidget(m_protocolCombo);
    new FocusUnderline(m_parserCombo, context->themeManager());
    new FocusUnderline(m_protocolCombo, context->themeManager());

    const int savedIndex = m_parserCombo->findData(
        QVariant::fromValue(context->settings()->parserMode()));
    if (savedIndex >= 0) m_parserCombo->setCurrentIndex(savedIndex);
    updateCustomProtocolVisibility();

    connect(m_parserCombo, &QComboBox::currentIndexChanged, this, [this] {
        const ParserMode mode = parserMode();
        m_context->settings()->setParserMode(mode);
        updateCustomProtocolVisibility();
        emit parserModeChanged(mode);
        if (mode == ParserMode::CustomBinary) emit requestProtocolLibrary();
    });
    connect(m_protocolCombo, &QComboBox::currentIndexChanged, this, [this] {
        const QString id = m_protocolCombo->currentData().toString();
        m_context->settings()->setCustomProtocolId(id);
        emit customProtocolChanged(id);
    });
    connect(m_helpButton, &QToolButton::clicked, this,
            [this] { emit helpRequested(helpText()); });
    connect(context->iconManager(), &IconManager::iconsChanged, this, [this] {
        m_helpButton->setIcon(m_context->iconManager()->icon(
            QStringLiteral(":/icons/connection/help.svg")));
    });
}

ParserMode ParserConfigPanel::parserMode() const
{
    return m_parserCombo->currentData().value<ParserMode>();
}

void ParserConfigPanel::updateCustomProtocolVisibility()
{
    m_protocolCombo->setVisible(parserMode() == ParserMode::CustomBinary);
}

QString ParserConfigPanel::helpText() const
{
    switch (parserMode()) {
    case ParserMode::RawData:
        return tr("RawData：按原始字节显示，不执行字段解析。");
    case ParserMode::JustFloat:
        return tr("JustFloat：当前项目尚无可确认的协议定义，本阶段按 RawData 显示，解析器接口已预留。");
    case ParserMode::FireWater:
        return tr("FireWater：当前项目尚无可确认的协议定义，本阶段按 RawData 显示，解析器接口已预留。");
    case ParserMode::CustomBinary:
        return tr("CustomBinary：已预留协议库请求接口，后续与 Protocol Editor 字段库连接。");
    }
    return {};
}
