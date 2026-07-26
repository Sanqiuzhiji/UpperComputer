#include "SendPanel.h"

#include "app/AppContext.h"
#include "app/AppSettings.h"
#include "services/CommunicationCodec.h"
#include "services/ConnectionManager.h"
#include "theme/IconManager.h"
#include "utils/ChecksumUtils.h"
#include "widgets/FocusUnderline.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include <cmath>
#include <limits>

namespace {
template<typename Enum>
void addTypedItem(QComboBox *combo, const QString &text, const Enum value)
{
    combo->addItem(text, QVariant::fromValue(value));
}

QString effectiveFieldId(const FieldDefinition &field, const int index)
{
    return field.id.trimmed().isEmpty()
        ? QStringLiteral("data%1").arg(index + 1) : field.id;
}

QString effectiveFieldName(const FieldDefinition &field, const int index)
{
    if (!field.displayName.trimmed().isEmpty()) return field.displayName;
    if (!field.id.trimmed().isEmpty()) return field.id;
    return QStringLiteral("data%1").arg(index + 1);
}

bool isAutomaticRole(const ProtocolFieldRole role)
{
    return role == ProtocolFieldRole::FrameHeader
        || role == ProtocolFieldRole::Length
        || role == ProtocolFieldRole::MessageId
        || role == ProtocolFieldRole::Checksum;
}
}

SendPanel::SendPanel(AppContext *context, QWidget *parent)
    : QFrame(parent),
      m_context(context),
      m_inputMode(context->settings()->inputMode()),
      m_encoding(context->settings()->textEncoding()),
      m_textDraft(context->settings()->rawTextDraft()),
      m_hexDraft(context->settings()->rawHexDraft())
{
    setProperty("card", true);
    setObjectName(QStringLiteral("sendPanel"));
    setMinimumHeight(76);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 7, 8, 7);
    layout->setSpacing(0);

    m_modeStack = new QStackedWidget(this);
    m_modeStack->setObjectName(QStringLiteral("sendModeStack"));
    m_modeStack->addWidget(createRawPage());
    m_modeStack->addWidget(createCustomBinaryPage());
    layout->addWidget(m_modeStack);

    setProtocols({});
    setSendMode(context->settings()->sendMode());
    refreshIcon();
}

QWidget *SendPanel::createRawPage()
{
    auto *page = new QWidget(this);
    page->setObjectName(QStringLiteral("rawDataSendPage"));
    auto *layout = new QHBoxLayout(page);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(8);

    m_inputModeButton = new QToolButton(page);
    m_inputModeButton->setObjectName(QStringLiteral("rawInputModeButton"));
    m_inputModeButton->setMinimumSize(56, 38);
    m_inputModeButton->setToolTip(tr("切换文本/HEX 输入"));
    layout->addWidget(m_inputModeButton);

    m_input = new QLineEdit(page);
    m_input->setObjectName(QStringLiteral("rawSendInput"));
    m_input->setClearButtonEnabled(true);
    m_input->setMaxLength(131072);
    m_input->setText(
        m_inputMode == InputMode::Text ? m_textDraft : m_hexDraft);
    layout->addWidget(m_input, 1);
    new FocusUnderline(m_input, m_context->themeManager());

    m_checksumCombo = new QComboBox(page);
    addTypedItem(m_checksumCombo, tr("无校验"), ChecksumMode::None);
    addTypedItem(m_checksumCombo, QStringLiteral("xor8"), ChecksumMode::Xor8);
    addTypedItem(m_checksumCombo, QStringLiteral("crc8"), ChecksumMode::Crc8);
    addTypedItem(m_checksumCombo, QStringLiteral("crc8-maxim"),
                 ChecksumMode::Crc8Maxim);
    m_checksumCombo->setToolTip(tr("校验追加"));
    m_checksumCombo->setMinimumWidth(112);
    layout->addWidget(m_checksumCombo);
    new FocusUnderline(m_checksumCombo, m_context->themeManager());

    m_lineEndingCombo = new QComboBox(page);
    addTypedItem(m_lineEndingCombo, tr("无行尾"), LineEnding::None);
    addTypedItem(m_lineEndingCombo, QStringLiteral("\\n"), LineEnding::LF);
    addTypedItem(m_lineEndingCombo, QStringLiteral("\\r"), LineEnding::CR);
    addTypedItem(m_lineEndingCombo, QStringLiteral("\\n\\r"),
                 LineEnding::LFCR);
    addTypedItem(m_lineEndingCombo, QStringLiteral("\\r\\n"),
                 LineEnding::CRLF);
    m_lineEndingCombo->setToolTip(tr("行尾追加"));
    m_lineEndingCombo->setMinimumWidth(96);
    layout->addWidget(m_lineEndingCombo);
    new FocusUnderline(m_lineEndingCombo, m_context->themeManager());

    m_rawSendButton = new QPushButton(tr("发送"), page);
    m_rawSendButton->setObjectName(QStringLiteral("rawSendButton"));
    m_rawSendButton->setProperty("accent", true);
    m_rawSendButton->setMinimumHeight(38);
    m_rawSendButton->setMinimumWidth(92);
    layout->addWidget(m_rawSendButton);

    const int checksumIndex = m_checksumCombo->findData(
        QVariant::fromValue(m_context->settings()->checksumMode()));
    if (checksumIndex >= 0) m_checksumCombo->setCurrentIndex(checksumIndex);
    const int lineIndex = m_lineEndingCombo->findData(
        QVariant::fromValue(m_context->settings()->lineEnding()));
    if (lineIndex >= 0) m_lineEndingCombo->setCurrentIndex(lineIndex);

    m_inputModeButton->setText(
        m_inputMode == InputMode::Text ? QStringLiteral("ABC")
                                       : QStringLiteral("HEX"));
    m_input->setPlaceholderText(m_inputMode == InputMode::Text
        ? tr("输入要发送的文本，按 Enter 发送")
        : tr("输入 HEX，例如 AA 55 01 02"));

    connect(m_inputModeButton, &QToolButton::clicked,
            this, &SendPanel::toggleInputMode);
    connect(m_rawSendButton, &QPushButton::clicked,
            this, &SendPanel::requestRawSend);
    connect(m_input, &QLineEdit::returnPressed,
            this, &SendPanel::requestRawSend);
    connect(m_input, &QLineEdit::textChanged, this, [this] {
        setInputError(false);
        if (m_inputMode == InputMode::Text) {
            m_textDraft = m_input->text();
            m_context->settings()->setRawTextDraft(m_textDraft);
        } else {
            m_hexDraft = m_input->text();
            m_context->settings()->setRawHexDraft(m_hexDraft);
        }
    });
    connect(m_checksumCombo, &QComboBox::currentIndexChanged, this, [this] {
        m_context->settings()->setChecksumMode(
            m_checksumCombo->currentData().value<ChecksumMode>());
    });
    connect(m_lineEndingCombo, &QComboBox::currentIndexChanged, this, [this] {
        m_context->settings()->setLineEnding(
            m_lineEndingCombo->currentData().value<LineEnding>());
    });
    connect(m_context->iconManager(), &IconManager::iconsChanged,
            this, &SendPanel::refreshIcon);
    return page;
}

QWidget *SendPanel::createCustomBinaryPage()
{
    auto *page = new QWidget(this);
    page->setObjectName(QStringLiteral("customBinarySendPage"));
    auto *root = new QVBoxLayout(page);
    root->setContentsMargins(2, 2, 2, 2);
    root->setSpacing(6);

    auto *header = new QHBoxLayout;
    header->setSpacing(8);
    auto *commandLabel = new QLabel(tr("发送命令"), page);
    commandLabel->setProperty("muted", true);
    header->addWidget(commandLabel);
    m_commandCombo = new QComboBox(page);
    m_commandCombo->setObjectName(QStringLiteral("customCommandCombo"));
    m_commandCombo->setMinimumWidth(220);
    header->addWidget(m_commandCombo);
    new FocusUnderline(m_commandCombo, m_context->themeManager());
    m_customStatus = new QLabel(page);
    m_customStatus->setObjectName(QStringLiteral("customSendStatus"));
    m_customStatus->setProperty("muted", true);
    header->addWidget(m_customStatus, 1);
    root->addLayout(header);

    m_fieldScroll = new QScrollArea(page);
    m_fieldScroll->setObjectName(QStringLiteral("customFieldScroll"));
    m_fieldScroll->setWidgetResizable(true);
    m_fieldScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_fieldContainer = new QWidget(m_fieldScroll);
    m_fieldContainer->setObjectName(QStringLiteral("customFieldContainer"));
    m_fieldLayout = new QVBoxLayout(m_fieldContainer);
    m_fieldLayout->setContentsMargins(0, 0, 4, 0);
    m_fieldLayout->setSpacing(5);
    m_fieldLayout->addStretch();
    m_fieldScroll->setWidget(m_fieldContainer);
    root->addWidget(m_fieldScroll, 1);

    auto *footer = new QHBoxLayout;
    m_previewLabel = new QLabel(page);
    m_previewLabel->setObjectName(QStringLiteral("customEncodingPreview"));
    m_previewLabel->setProperty("muted", true);
    m_previewLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    footer->addWidget(m_previewLabel, 1);
    m_previewButton = new QPushButton(tr("查看编码预览"), page);
    m_previewButton->setObjectName(QStringLiteral("customPreviewButton"));
    footer->addWidget(m_previewButton);
    m_customSendButton = new QPushButton(tr("发送"), page);
    m_customSendButton->setObjectName(QStringLiteral("customSendButton"));
    m_customSendButton->setProperty("accent", true);
    m_customSendButton->setMinimumWidth(92);
    footer->addWidget(m_customSendButton);
    root->addLayout(footer);

    connect(m_commandCombo, &QComboBox::currentIndexChanged, this, [this] {
        saveCustomDrafts();
        m_currentCommandId = m_commandCombo->currentData().toString();
        m_context->settings()->setCustomCommandId(m_currentCommandId);
        rebuildDynamicFields();
    });
    connect(m_previewButton, &QPushButton::clicked,
            this, &SendPanel::previewCustomPayload);
    connect(m_customSendButton, &QPushButton::clicked,
            this, &SendPanel::requestCustomSend);
    return page;
}

void SendPanel::setProtocols(const QList<ProtocolDefinition> &protocols)
{
    saveCustomDrafts();
    m_protocols = protocols;
    QString selected = m_currentProtocolId;
    if (selected.isEmpty()) {
        selected = m_context->settings()->customProtocolId();
    }
    bool found = false;
    for (const ProtocolDefinition &protocol : m_protocols) {
        if (protocol.id == selected) {
            found = true;
            break;
        }
    }
    m_currentProtocolId = found ? selected : QString();
    populateCommands();
}

void SendPanel::setCurrentProtocolId(const QString &protocolId)
{
    if (m_currentProtocolId == protocolId) return;
    saveCustomDrafts();
    m_currentProtocolId = protocolId;
    populateCommands();
}

void SendPanel::setEncoder(
    const QString &protocolId,
    std::shared_ptr<const CustomBinaryEncoder> encoder)
{
    if (encoder) {
        m_encoders[protocolId] = std::move(encoder);
    } else {
        m_encoders.erase(protocolId);
    }
    updateCustomState();
}

int SendPanel::dynamicFieldCount() const noexcept
{
    return static_cast<int>(m_fieldEditors.size());
}

void SendPanel::setTextEncoding(const TextEncoding encoding)
{
    m_encoding = encoding;
}

void SendPanel::setSendMode(const SendMode mode)
{
    if (!m_modeStack) return;
    saveCustomDrafts();
    m_modeStack->setCurrentIndex(
        mode == SendMode::RawData ? 0 : 1);
}

void SendPanel::toggleInputMode()
{
    if (m_inputMode == InputMode::Text) {
        m_textDraft = m_input->text();
        m_context->settings()->setRawTextDraft(m_textDraft);
        m_inputMode = InputMode::Hex;
        m_input->setText(m_hexDraft);
    } else {
        m_hexDraft = m_input->text();
        m_context->settings()->setRawHexDraft(m_hexDraft);
        m_inputMode = InputMode::Text;
        m_input->setText(m_textDraft);
    }
    m_inputModeButton->setText(
        m_inputMode == InputMode::Text ? QStringLiteral("ABC")
                                       : QStringLiteral("HEX"));
    m_input->setPlaceholderText(m_inputMode == InputMode::Text
        ? tr("输入要发送的文本，按 Enter 发送")
        : tr("输入 HEX，例如 AA 55 01 02"));
    m_context->settings()->setInputMode(m_inputMode);
    setInputError(false);
}

void SendPanel::requestRawSend()
{
    if (m_context->connectionManager()->state()
        != ConnectionState::Connected) {
        emit notificationRequested(
            tr("设备未连接，无法发送"), NotificationType::Warning);
        return;
    }
    QByteArray payload;
    QString error;
    if (!buildRawPayload(&payload, &error)) {
        setInputError(true);
        emit notificationRequested(error, NotificationType::Warning);
        return;
    }
    emit sendRequested(payload);
}

void SendPanel::requestCustomSend()
{
    if (m_context->connectionManager()->state()
        != ConnectionState::Connected) {
        emit notificationRequested(
            tr("设备未连接，无法发送"), NotificationType::Warning);
        return;
    }
    QByteArray payload;
    QString error;
    if (!buildCustomPayload(&payload, &error)) {
        emit notificationRequested(error, NotificationType::Warning);
        return;
    }
    emit sendRequested(payload);
}

void SendPanel::previewCustomPayload()
{
    QByteArray payload;
    QString error;
    if (!buildCustomPayload(&payload, &error)) {
        emit notificationRequested(error, NotificationType::Warning);
        return;
    }
    m_previewLabel->setText(
        QString::fromLatin1(payload.toHex(' ').toUpper()));
}

void SendPanel::populateCommands()
{
    const QSignalBlocker blocker(m_commandCombo);
    m_commandCombo->clear();
    const ProtocolDefinition *protocol = currentProtocol();
    if (!protocol) {
        m_commandCombo->addItem(tr("请先选择自定义协议"), QString());
        m_commandCombo->setEnabled(false);
        m_currentCommandId.clear();
        rebuildDynamicFields();
        return;
    }
    for (const MessageDefinition &message : protocol->sendMessages) {
        const QString name = message.displayName.trimmed().isEmpty()
            ? message.id : message.displayName;
        m_commandCombo->addItem(name, message.id);
    }
    m_commandCombo->setEnabled(!protocol->sendMessages.isEmpty());
    const int savedIndex = m_commandCombo->findData(
        m_context->settings()->customCommandId());
    if (savedIndex >= 0) m_commandCombo->setCurrentIndex(savedIndex);
    m_currentCommandId = m_commandCombo->currentData().toString();
    rebuildDynamicFields();
}

void SendPanel::rebuildDynamicFields()
{
    clearDynamicFields();
    const MessageDefinition *message = currentMessage();
    if (!message) {
        updateCustomState();
        return;
    }

    int displayIndex = 0;
    for (const FieldDefinition &field : message->fields) {
        if (isAutomaticRole(field.role)) continue;
        QWidget *row = new QFrame(m_fieldContainer);
        row->setProperty("channelRow", true);
        auto *layout = new QHBoxLayout(row);
        layout->setContentsMargins(6, 3, 6, 3);
        layout->setSpacing(8);

        auto *name = new QLabel(
            effectiveFieldName(field, displayIndex), row);
        name->setMinimumWidth(120);
        layout->addWidget(name);

        auto *meta = new QLabel(fieldMetaText(field), row);
        meta->setProperty("muted", true);
        meta->setMinimumWidth(150);
        meta->setToolTip(field.description);
        layout->addWidget(meta);

        QWidget *editor =
            createFieldEditor(field, displayIndex, row);
        editor->setObjectName(
            QStringLiteral("customField_%1")
                .arg(effectiveFieldId(field, displayIndex)));
        editor->setProperty("fieldId", effectiveFieldId(field, displayIndex));
        if (!field.description.isEmpty()) {
            editor->setToolTip(field.description);
        }
        layout->addWidget(editor, 1);

        m_fieldLayout->insertWidget(m_fieldLayout->count() - 1, row);
        m_fieldEditors.append(FieldEditor{field, editor, row});

        if (auto *line = qobject_cast<QLineEdit *>(editor)) {
            connect(line, &QLineEdit::textChanged,
                    this, &SendPanel::saveCustomDrafts);
            if (field.editable
                && (field.type == ProtocolFieldType::Int
                    || field.type == ProtocolFieldType::UInt)) {
                connect(line, &QLineEdit::editingFinished, this,
                        [this, editor] {
                            for (FieldEditor &candidate : m_fieldEditors) {
                                if (candidate.editor != editor) continue;
                                QVariant value;
                                QString error;
                                if (!normalizeIntegerEditor(
                                        candidate, &value, &error)) {
                                    emit notificationRequested(
                                        error, NotificationType::Warning);
                                }
                                saveCustomDrafts();
                                break;
                            }
                        });
            }
        } else if (auto *check = qobject_cast<QCheckBox *>(editor)) {
            connect(check, &QCheckBox::toggled,
                    this, &SendPanel::saveCustomDrafts);
        } else if (auto *combo = qobject_cast<QComboBox *>(editor)) {
            connect(combo, &QComboBox::currentIndexChanged,
                    this, &SendPanel::saveCustomDrafts);
        }
        ++displayIndex;
    }
    updateCustomState();
}

void SendPanel::clearDynamicFields()
{
    m_fieldEditors.clear();
    while (m_fieldLayout->count() > 1) {
        QLayoutItem *item = m_fieldLayout->takeAt(0);
        if (QWidget *widget = item->widget()) {
            widget->hide();
            widget->setParent(nullptr);
            widget->deleteLater();
        }
        delete item;
    }
}

QWidget *SendPanel::createFieldEditor(
    const FieldDefinition &field, const int fieldIndex, QWidget *parent)
{
    const QVariantMap drafts = m_context->settings()->customFieldDrafts();
    QVariant value = drafts.value(
        fieldDraftKey(effectiveFieldId(field, fieldIndex)),
        field.defaultValue);
    if (field.role == ProtocolFieldRole::Constant
        && field.fixedValue.isValid()) {
        value = field.fixedValue;
    }

    if (field.type == ProtocolFieldType::Bool) {
        auto *check = new QCheckBox(parent);
        check->setChecked(value.toBool());
        check->setEnabled(field.editable
                          && field.role == ProtocolFieldRole::Value);
        return check;
    }
    if (field.type == ProtocolFieldType::Enum) {
        auto *combo = new QComboBox(parent);
        for (const EnumOption &option : field.enumOptions) {
            combo->addItem(option.displayName, option.value);
        }
        const int index = combo->findData(value);
        if (index >= 0) combo->setCurrentIndex(index);
        combo->setEnabled(field.editable
                          && field.role == ProtocolFieldRole::Value);
        new FocusUnderline(combo, m_context->themeManager());
        return combo;
    }

    auto *line = new QLineEdit(parent);
    line->setText(value.toString());
    line->setReadOnly(!field.editable
                      || field.role != ProtocolFieldRole::Value);
    if (field.type == ProtocolFieldType::ByteArray) {
        line->setPlaceholderText(tr("HEX，例如 AA 55 01 02"));
    }
    new FocusUnderline(line, m_context->themeManager());
    return line;
}

void SendPanel::saveCustomDrafts()
{
    if (!m_commandCombo || m_fieldEditors.isEmpty()) return;
    QVariantMap drafts = m_context->settings()->customFieldDrafts();
    for (const FieldEditor &entry : std::as_const(m_fieldEditors)) {
        QVariant value;
        if (const auto *line = qobject_cast<QLineEdit *>(entry.editor)) {
            value = line->text();
        } else if (const auto *check =
                       qobject_cast<QCheckBox *>(entry.editor)) {
            value = check->isChecked();
        } else if (const auto *combo =
                       qobject_cast<QComboBox *>(entry.editor)) {
            value = combo->currentData();
        }
        drafts.insert(fieldDraftKey(
            entry.editor->property("fieldId").toString()), value);
    }
    m_context->settings()->setCustomFieldDrafts(drafts);
}

bool SendPanel::collectCustomValues(
    ProtocolFieldValues *values, QString *errorMessage)
{
    if (!values || !errorMessage) return false;
    values->clear();
    const MessageDefinition *message = currentMessage();
    if (!message) {
        *errorMessage = tr("当前协议没有可发送命令");
        return false;
    }

    int editorIndex = 0;
    int displayIndex = 0;
    for (int fieldIndex = 0;
         fieldIndex < static_cast<int>(message->fields.size());
         ++fieldIndex) {
        const FieldDefinition &field = message->fields.at(fieldIndex);
        if (isAutomaticRole(field.role)) {
            const QString key = effectiveFieldId(field, fieldIndex);
            const QVariant value = field.fixedValue.isValid()
                ? field.fixedValue : field.defaultValue;
            values->insert(key, value);
            continue;
        }
        const QString key = effectiveFieldId(field, displayIndex++);
        if (editorIndex >= m_fieldEditors.size()) {
            *errorMessage = tr("字段表单不完整");
            return false;
        }
        FieldEditor &editor = m_fieldEditors[editorIndex++];
        if (field.role == ProtocolFieldRole::Constant || !field.editable) {
            const QVariant value = field.fixedValue.isValid()
                ? field.fixedValue : field.defaultValue;
            values->insert(key, value);
            continue;
        }
        QVariant value;
        if (!fieldValue(editor, &value, errorMessage)) return false;
        values->insert(key, value);
    }
    saveCustomDrafts();
    return true;
}

bool SendPanel::fieldValue(
    FieldEditor &editor, QVariant *value, QString *error)
{
    const FieldDefinition &field = editor.definition;
    if (field.type == ProtocolFieldType::Int
        || field.type == ProtocolFieldType::UInt) {
        return normalizeIntegerEditor(editor, value, error);
    }
    if (field.type == ProtocolFieldType::Bool) {
        *value = qobject_cast<QCheckBox *>(editor.editor)->isChecked();
        setInvalid(editor.editor, false);
        return true;
    }
    if (field.type == ProtocolFieldType::Enum) {
        auto *combo = qobject_cast<QComboBox *>(editor.editor);
        if (combo->currentIndex() < 0) {
            *error = tr("字段“%1”没有可选枚举值")
                         .arg(field.displayName);
            setInvalid(combo, true);
            return false;
        }
        *value = combo->currentData();
        setInvalid(combo, false);
        return true;
    }

    auto *line = qobject_cast<QLineEdit *>(editor.editor);
    const QString text = line->text().trimmed();
    if (field.type == ProtocolFieldType::Float
        || field.type == ProtocolFieldType::Double) {
        bool ok = false;
        const double number = text.toDouble(&ok);
        if (!ok || !std::isfinite(number)) {
            *error = tr("字段“%1”需要有效的小数")
                         .arg(field.displayName);
            setInvalid(line, true);
            return false;
        }
        const double minimum = field.minimum.isValid()
            ? field.minimum.toDouble()
            : -std::numeric_limits<double>::max();
        const double maximum = field.maximum.isValid()
            ? field.maximum.toDouble()
            : std::numeric_limits<double>::max();
        if (number < minimum || number > maximum) {
            *error = tr("字段“%1”的允许范围为 %2～%3")
                         .arg(field.displayName,
                              QString::number(minimum, 'g', 12),
                              QString::number(maximum, 'g', 12));
            setInvalid(line, true);
            return false;
        }
        *value = number;
        setInvalid(line, false);
        return true;
    }
    if (field.type == ProtocolFieldType::ByteArray) {
        QString normalized = text;
        normalized.replace(
            QRegularExpression(QStringLiteral("(?i)\\b0x")), QString());
        normalized.remove(QRegularExpression(QStringLiteral("[\\s,]+")));
        if (normalized.isEmpty()
            || normalized.size() % 2 != 0
            || !QRegularExpression(QStringLiteral("^[0-9A-Fa-f]+$"))
                    .match(normalized).hasMatch()) {
            *error = tr("字段“%1”需要偶数个合法 HEX 字符")
                         .arg(field.displayName);
            setInvalid(line, true);
            return false;
        }
        *value = QByteArray::fromHex(normalized.toLatin1());
        setInvalid(line, false);
        return true;
    }

    *value = line->text();
    setInvalid(line, false);
    return true;
}

bool SendPanel::normalizeIntegerEditor(
    FieldEditor &editor, QVariant *value, QString *error)
{
    auto *line = qobject_cast<QLineEdit *>(editor.editor);
    const FieldDefinition &field = editor.definition;
    bool ok = false;
    const double entered = line->text().trimmed().toDouble(&ok);
    const QString name = field.displayName.trimmed().isEmpty()
        ? editor.editor->property("fieldId").toString()
        : field.displayName;
    if (!ok || !std::isfinite(entered)) {
        *error = tr("字段“%1”需要有效的整数").arg(name);
        setInvalid(line, true);
        return false;
    }
    if (field.type == ProtocolFieldType::UInt && entered < 0.0) {
        *error = tr("无符号字段“%1”不允许负值").arg(name);
        setInvalid(line, true);
        return false;
    }

    const int bits = qBound(1, field.bitWidth, 64);
    double typeMinimum = 0.0;
    double typeMaximum = 0.0;
    if (field.type == ProtocolFieldType::UInt) {
        typeMaximum = bits == 64
            ? std::nextafter(std::ldexp(1.0, 64), 0.0)
            : std::ldexp(1.0, bits) - 1.0;
    } else {
        typeMinimum = bits == 64
            ? static_cast<double>((std::numeric_limits<qint64>::min)())
            : -std::ldexp(1.0, bits - 1);
        typeMaximum = bits == 64
            ? std::nextafter(std::ldexp(1.0, 63), 0.0)
            : std::ldexp(1.0, bits - 1) - 1.0;
    }
    const double minimum = field.minimum.isValid()
        ? (std::max)(typeMinimum, field.minimum.toDouble()) : typeMinimum;
    const double maximum = field.maximum.isValid()
        ? (std::min)(typeMaximum, field.maximum.toDouble()) : typeMaximum;
    const double truncated = std::trunc(entered);
    if (truncated < minimum || truncated > maximum) {
        *error = tr("字段“%1”的允许范围为 %2～%3")
                     .arg(name,
                          QString::number(minimum, 'g', 15),
                          QString::number(maximum, 'g', 15));
        setInvalid(line, true);
        return false;
    }

    if (field.type == ProtocolFieldType::UInt) {
        const quint64 finalValue = static_cast<quint64>(truncated);
        line->setText(QString::number(finalValue));
        *value = QVariant::fromValue(finalValue);
    } else {
        const qint64 finalValue = static_cast<qint64>(truncated);
        line->setText(QString::number(finalValue));
        *value = QVariant::fromValue(finalValue);
    }
    setInvalid(line, false);
    return true;
}

bool SendPanel::buildRawPayload(QByteArray *payload, QString *error) const
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

bool SendPanel::buildCustomPayload(QByteArray *payload, QString *error)
{
    const ProtocolDefinition *protocol = currentProtocol();
    if (!protocol) {
        *error = tr("请先选择自定义协议");
        return false;
    }
    const MessageDefinition *message = currentMessage();
    if (!message) {
        *error = tr("当前协议没有可发送命令");
        return false;
    }
    const auto encoder = m_encoders.find(protocol->id);
    if (encoder == m_encoders.end() || !encoder->second) {
        *error = tr("当前协议尚无编码器");
        return false;
    }
    ProtocolFieldValues values;
    if (!collectCustomValues(&values, error)) return false;
    if (!encoder->second->encode(
            *protocol, *message, values, payload, error)) {
        if (error->isEmpty()) *error = tr("自定义协议编码失败");
        return false;
    }
    if (payload->isEmpty()) {
        *error = tr("编码器没有生成可发送数据");
        return false;
    }
    return true;
}

const ProtocolDefinition *SendPanel::currentProtocol() const
{
    for (const ProtocolDefinition &protocol : m_protocols) {
        if (protocol.id == m_currentProtocolId) return &protocol;
    }
    return nullptr;
}

const MessageDefinition *SendPanel::currentMessage() const
{
    const ProtocolDefinition *protocol = currentProtocol();
    if (!protocol || !m_commandCombo) return nullptr;
    for (const MessageDefinition &message : protocol->sendMessages) {
        if (message.id == m_currentCommandId) return &message;
    }
    return nullptr;
}

void SendPanel::updateCustomState()
{
    QString status;
    bool ready = false;
    const ProtocolDefinition *protocol = currentProtocol();
    if (!protocol) {
        status = tr("请先选择自定义协议");
    } else if (protocol->sendMessages.isEmpty() || !currentMessage()) {
        status = tr("当前协议没有可发送命令");
    } else if (!m_encoders.contains(protocol->id)
               || !m_encoders.at(protocol->id)) {
        status = tr("当前协议尚无编码器");
    } else {
        status = tr("填写字段后可预览或发送");
        ready = true;
    }
    m_customStatus->setText(status);
    m_previewButton->setEnabled(ready);
    m_customSendButton->setEnabled(ready);
    if (!ready) m_previewLabel->clear();
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

QString SendPanel::fieldDraftKey(const QString &fieldId) const
{
    return QStringLiteral("%1/%2/%3")
        .arg(m_currentProtocolId, m_currentCommandId, fieldId);
}

QString SendPanel::fieldTypeText(const FieldDefinition &field) const
{
    switch (field.type) {
    case ProtocolFieldType::Int:
        return QStringLiteral("int%1").arg(field.bitWidth);
    case ProtocolFieldType::UInt:
        return QStringLiteral("uint%1").arg(field.bitWidth);
    case ProtocolFieldType::Float: return QStringLiteral("float");
    case ProtocolFieldType::Double: return QStringLiteral("double");
    case ProtocolFieldType::Bool: return QStringLiteral("bool");
    case ProtocolFieldType::Enum: return QStringLiteral("enum");
    case ProtocolFieldType::String: return QStringLiteral("string");
    case ProtocolFieldType::ByteArray: return QStringLiteral("byte[]");
    }
    return {};
}

QString SendPanel::fieldMetaText(const FieldDefinition &field) const
{
    QStringList parts{fieldTypeText(field)};
    if (!field.unit.trimmed().isEmpty()) parts.append(field.unit);
    if (field.minimum.isValid() || field.maximum.isValid()) {
        parts.append(QStringLiteral("%1～%2")
            .arg(field.minimum.isValid() ? field.minimum.toString()
                                         : QStringLiteral("−∞"),
                 field.maximum.isValid() ? field.maximum.toString()
                                         : QStringLiteral("+∞")));
    }
    if (field.role == ProtocolFieldRole::Constant || !field.editable) {
        parts.append(tr("只读"));
    }
    return parts.join(QStringLiteral(" · "));
}

void SendPanel::setInvalid(QWidget *widget, const bool invalid)
{
    if (!widget || widget->property("invalid").toBool() == invalid) return;
    widget->setProperty("invalid", invalid);
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
}

void SendPanel::setInputError(const bool error)
{
    setInvalid(m_input, error);
}

void SendPanel::refreshIcon()
{
    if (!m_rawSendButton) return;
    m_rawSendButton->setIcon(QIcon(m_context->iconManager()->pixmap(
        QStringLiteral(":/icons/connection/send.svg"),
        QSize(18, 18), QColor(Qt::white))));
    m_rawSendButton->setIconSize(QSize(18, 18));
    if (m_customSendButton) {
        m_customSendButton->setIcon(m_rawSendButton->icon());
        m_customSendButton->setIconSize(QSize(18, 18));
    }
}
