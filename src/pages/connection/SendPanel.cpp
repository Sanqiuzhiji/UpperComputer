#include "SendPanel.h"

#include "app/AppContext.h"
#include "app/AppSettings.h"
#include "theme/IconManager.h"
#include "utils/ChecksumUtils.h"
#include "widgets/FocusUnderline.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QStyle>
#include <QToolButton>

namespace {
template<typename Enum>
void addTypedItem(QComboBox *combo, const QString &text, Enum value)
{
    combo->addItem(text, QVariant::fromValue(value));
}
}

SendPanel::SendPanel(AppContext *context, QWidget *parent)
    : QFrame(parent),
      m_context(context),
      m_inputMode(context->settings()->inputMode()),
      m_encoding(context->settings()->textEncoding())
{
    setProperty("card", true);
    setObjectName(QStringLiteral("sendPanel"));
    setMinimumHeight(62);
    setMaximumHeight(70);
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 9, 10, 9);
    layout->setSpacing(8);

    m_inputModeButton = new QToolButton(this);
    m_inputModeButton->setMinimumSize(56, 38);
    m_inputModeButton->setToolTip(tr("切换文本/HEX 输入"));
    layout->addWidget(m_inputModeButton);

    m_input = new QLineEdit(this);
    m_input->setClearButtonEnabled(true);
    m_input->setMaxLength(131072);
    layout->addWidget(m_input, 1);
    new FocusUnderline(m_input, context->themeManager());

    m_checksumCombo = new QComboBox(this);
    addTypedItem(m_checksumCombo, QStringLiteral("none"), ChecksumMode::None);
    addTypedItem(m_checksumCombo, QStringLiteral("xor8"), ChecksumMode::Xor8);
    addTypedItem(m_checksumCombo, QStringLiteral("crc8"), ChecksumMode::Crc8);
    addTypedItem(m_checksumCombo, QStringLiteral("crc8-maxim"), ChecksumMode::Crc8Maxim);
    m_checksumCombo->setToolTip(tr("校验模式"));
    m_checksumCombo->setMinimumWidth(112);
    layout->addWidget(m_checksumCombo);
    new FocusUnderline(m_checksumCombo, context->themeManager());

    m_lineEndingCombo = new QComboBox(this);
    addTypedItem(m_lineEndingCombo, QStringLiteral("No Tail"), LineEnding::None);
    addTypedItem(m_lineEndingCombo, QStringLiteral("\\n"), LineEnding::LF);
    addTypedItem(m_lineEndingCombo, QStringLiteral("\\r"), LineEnding::CR);
    addTypedItem(m_lineEndingCombo, QStringLiteral("\\n\\r"), LineEnding::LFCR);
    addTypedItem(m_lineEndingCombo, QStringLiteral("\\r\\n"), LineEnding::CRLF);
    m_lineEndingCombo->setToolTip(tr("行尾追加"));
    m_lineEndingCombo->setMinimumWidth(96);
    layout->addWidget(m_lineEndingCombo);
    new FocusUnderline(m_lineEndingCombo, context->themeManager());

    m_sendButton = new QPushButton(tr("Send"), this);
    m_sendButton->setProperty("accent", true);
    m_sendButton->setMinimumHeight(38);
    m_sendButton->setMinimumWidth(92);
    layout->addWidget(m_sendButton);

    const int checksumIndex = m_checksumCombo->findData(
        QVariant::fromValue(context->settings()->checksumMode()));
    if (checksumIndex >= 0) m_checksumCombo->setCurrentIndex(checksumIndex);
    const int lineIndex = m_lineEndingCombo->findData(
        QVariant::fromValue(context->settings()->lineEnding()));
    if (lineIndex >= 0) m_lineEndingCombo->setCurrentIndex(lineIndex);
    m_inputModeButton->setText(
        m_inputMode == InputMode::Text ? QStringLiteral("ABC")
                                       : QStringLiteral("HEX"));
    m_input->setPlaceholderText(m_inputMode == InputMode::Text
        ? tr("输入要发送的文本，按 Enter 发送")
        : tr("输入 HEX，例如 AA 55 01 02"));

    connect(m_inputModeButton, &QToolButton::clicked,
            this, &SendPanel::toggleInputMode);
    connect(m_sendButton, &QPushButton::clicked,
            this, &SendPanel::requestSend);
    connect(m_input, &QLineEdit::returnPressed,
            this, &SendPanel::requestSend);
    connect(m_input, &QLineEdit::textChanged,
            this, [this] { setInputError(false); });
    connect(m_checksumCombo, &QComboBox::currentIndexChanged, this, [this] {
        m_context->settings()->setChecksumMode(
            m_checksumCombo->currentData().value<ChecksumMode>());
    });
    connect(m_lineEndingCombo, &QComboBox::currentIndexChanged, this, [this] {
        m_context->settings()->setLineEnding(
            m_lineEndingCombo->currentData().value<LineEnding>());
    });
    connect(context->iconManager(), &IconManager::iconsChanged,
            this, &SendPanel::refreshIcon);
    refreshIcon();
}

void SendPanel::setTextEncoding(const TextEncoding encoding)
{
    m_encoding = encoding;
}

void SendPanel::toggleInputMode()
{
    if (m_inputMode == InputMode::Text) {
        m_textDraft = m_input ? m_input->text() : QString();
        m_inputMode = InputMode::Hex;
        if (m_input) m_input->setText(m_hexDraft);
    } else {
        m_hexDraft = m_input ? m_input->text() : QString();
        m_inputMode = InputMode::Text;
        if (m_input) m_input->setText(m_textDraft);
    }
    if (!m_inputModeButton || !m_input) return;
    m_inputModeButton->setText(
        m_inputMode == InputMode::Text ? QStringLiteral("ABC")
                                       : QStringLiteral("HEX"));
    m_input->setPlaceholderText(m_inputMode == InputMode::Text
        ? tr("输入要发送的文本，按 Enter 发送")
        : tr("输入 HEX，例如 AA 55 01 02"));
    m_context->settings()->setInputMode(m_inputMode);
    setInputError(false);
}

void SendPanel::requestSend()
{
    QByteArray payload;
    QString error;
    if (!buildPayload(&payload, &error)) {
        setInputError(true);
        emit notificationRequested(error, NotificationType::Warning);
        return;
    }
    emit sendRequested(payload);
}

bool SendPanel::buildPayload(QByteArray *payload, QString *error) const
{
    const QString input = m_input->text();
    if (input.isEmpty()) {
        *error = tr("发送内容不能为空");
        return false;
    }
    if (m_inputMode == InputMode::Text) {
        switch (m_encoding) {
        case TextEncoding::Utf8: *payload = input.toUtf8(); break;
        case TextEncoding::Local8Bit: *payload = input.toLocal8Bit(); break;
        case TextEncoding::Latin1: *payload = input.toLatin1(); break;
        }
    } else {
        QString normalized = input.trimmed();
        normalized.replace(
            QRegularExpression(QStringLiteral("(?i)\\b0x")), QString());
        normalized.remove(QRegularExpression(QStringLiteral("[\\s,]+")));
        if (normalized.isEmpty()) {
            *error = tr("HEX 输入不能为空");
            return false;
        }
        if (!QRegularExpression(QStringLiteral("^[0-9A-Fa-f]+$"))
                 .match(normalized).hasMatch()) {
            *error = tr("HEX 输入包含非法字符");
            return false;
        }
        if (normalized.size() % 2 != 0) {
            *error = tr("HEX 输入必须包含偶数个十六进制字符");
            return false;
        }
        if (normalized.size() > 131072) {
            *error = tr("发送内容过长，最大允许 64 KiB");
            return false;
        }
        *payload = QByteArray::fromHex(normalized.toLatin1());
    }

    *payload = ChecksumUtils::append(
        *payload, m_checksumCombo->currentData().value<ChecksumMode>());
    payload->append(lineEndingBytes());
    return !payload->isEmpty();
}

QByteArray SendPanel::lineEndingBytes() const
{
    switch (m_lineEndingCombo->currentData().value<LineEnding>()) {
    case LineEnding::None: return {};
    case LineEnding::LF: return QByteArrayLiteral("\n");
    case LineEnding::CR: return QByteArrayLiteral("\r");
    case LineEnding::LFCR: return QByteArrayLiteral("\n\r");
    case LineEnding::CRLF: return QByteArrayLiteral("\r\n");
    }
    return {};
}

void SendPanel::setInputError(const bool error)
{
    if (m_input->property("invalid").toBool() == error) return;
    m_input->setProperty("invalid", error);
    m_input->style()->unpolish(m_input);
    m_input->style()->polish(m_input);
}

void SendPanel::refreshIcon()
{
    m_sendButton->setIcon(QIcon(m_context->iconManager()->pixmap(
        QStringLiteral(":/icons/connection/send.svg"),
        QSize(18, 18), QColor(Qt::white))));
    m_sendButton->setIconSize(QSize(18, 18));
}
