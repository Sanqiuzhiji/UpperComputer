#include "ProtocolPropertyPanel.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <algorithm>

using namespace ProtocolModel;

namespace {

QString uuidText(const QUuid &id)
{
    return id.toString(QUuid::WithoutBraces);
}

QWidget *formPage(QWidget *parent, QFormLayout **form)
{
    auto *page = new QWidget(parent);
    auto *outer = new QVBoxLayout(page);
    outer->setContentsMargins(2, 2, 2, 2);
    auto *layout = new QFormLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(8);
    layout->setVerticalSpacing(8);
    layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    outer->addLayout(layout);
    outer->addStretch();
    *form = layout;
    return page;
}

QWidget *addRow(
    QFormLayout *form, const QString &label, QWidget *field)
{
    auto *row = new QWidget;
    auto *layout = new QVBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(3);
    layout->addWidget(field);
    form->addRow(label, row);
    return row;
}

void setFormRowVisible(QWidget *row, const bool visible)
{
    row->setVisible(visible);
    QWidget *page = row->parentWidget();
    if (!page || !page->layout() || page->layout()->count() == 0) return;
    auto *form = qobject_cast<QFormLayout *>(
        page->layout()->itemAt(0)->layout());
    if (!form) return;
    if (QWidget *label = form->labelForField(row)) {
        label->setVisible(visible);
    }
}

template<typename Enum>
void addEnumItem(QComboBox *combo, const QString &text, const Enum value)
{
    combo->addItem(text, QVariant::fromValue(value));
}

} // namespace

ProtocolPropertyPanel::ProtocolPropertyPanel(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("protocolPropertyPanel"));
    setProperty("card", true);
    setMinimumWidth(260);
    setMaximumWidth(340);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);
    auto *title = new QLabel(QStringLiteral("属性编辑器"), this);
    title->setObjectName(QStringLiteral("sectionTitle"));
    layout->addWidget(title);
    m_stack = new QStackedWidget(this);
    m_stack->setObjectName(QStringLiteral("protocolPropertyStack"));
    m_stack->addWidget(createDocumentPage());
    m_stack->addWidget(createFramePage());
    m_stack->addWidget(createFieldPage());
    layout->addWidget(m_stack, 1);
    connectEditors();
}

void ProtocolPropertyPanel::showDocument(
    const Document &document,
    const QVector<ValidationIssue> &issues)
{
    m_updating = true;
    m_stack->setCurrentIndex(0);
    m_documentName->setText(document.name);
    m_documentFrameCount->setText(QString::number(document.frames.size()));
    m_documentByteCount->setText(
        QStringLiteral("%1 B").arg(documentByteCount(document)));
    const QStringList messages = messagesFor(issues, {}, {});
    m_documentErrors->setText(messages.join(QLatin1Char('\n')));
    m_documentErrors->setVisible(!messages.isEmpty());
    setInvalid(m_documentName, document.name.trimmed().isEmpty());
    m_updating = false;
}

void ProtocolPropertyPanel::showFrame(
    const Document &,
    const Frame &frame,
    const QVector<ValidationIssue> &issues)
{
    m_updating = true;
    m_currentFrame = frame;
    m_currentFrameId = frame.id;
    m_stack->setCurrentIndex(1);
    m_frameName->setText(frame.name);
    m_frameId->setText(uuidText(frame.id));
    m_frameDirection->setCurrentIndex(
        m_frameDirection->findData(
            QVariant::fromValue(frame.direction)));
    m_frameFieldCount->setText(QString::number(frame.fields.size()));
    m_frameByteCount->setText(
        QStringLiteral("%1 B").arg(frameByteCount(frame)));
    const QStringList messages = messagesFor(issues, frame.id);
    m_frameErrors->setText(messages.join(QLatin1Char('\n')));
    m_frameErrors->setVisible(!messages.isEmpty());
    const bool invalidName = frame.name.trimmed().isEmpty()
        || std::ranges::any_of(
            messages, [](const QString &message) {
                return message.contains(QStringLiteral("协议帧名称"));
            });
    setInvalid(m_frameName, invalidName);
    m_updating = false;
}

void ProtocolPropertyPanel::showField(
    const Frame &frame,
    const Field &field,
    const QVector<ValidationIssue> &issues)
{
    m_updating = true;
    m_currentFrameId = frame.id;
    m_currentField = field;
    m_stack->setCurrentIndex(2);
    m_fieldName->setText(field.name);
    m_fieldRole->setCurrentIndex(
        m_fieldRole->findData(QVariant::fromValue(field.role)));
    m_byteCount->setValue(field.byteCount);
    m_dataType->setCurrentIndex(
        m_dataType->findData(QVariant::fromValue(field.dataType)));
    m_byteOrder->setCurrentIndex(
        m_byteOrder->findData(QVariant::fromValue(field.byteOrder)));
    m_scale->setValue(field.scale);
    m_offset->setValue(field.offset);
    m_fixedHex->setText(formatHex(field.fixedBytes));
    m_checksum->setCurrentIndex(
        m_checksum->findData(QVariant::fromValue(
            field.checksumAlgorithm)));
    const QStringList messages = messagesFor(issues, frame.id, field.id);
    m_fieldErrors->setText(messages.join(QLatin1Char('\n')));
    m_fieldErrors->setVisible(!messages.isEmpty());
    const bool invalidName = field.name.trimmed().isEmpty()
        || std::ranges::any_of(
            messages, [](const QString &message) {
                return message.contains(QStringLiteral("字段名称"))
                    || message.contains(QStringLiteral("名称重复"));
            });
    setInvalid(m_fieldName, invalidName);
    setInvalid(m_byteCount, field.byteCount <= 0);
    updateFieldVisibility();
    updateHexValidation();
    m_updating = false;
}

QWidget *ProtocolPropertyPanel::createDocumentPage()
{
    QFormLayout *form = nullptr;
    QWidget *page = formPage(m_stack, &form);
    auto *heading = new QLabel(QStringLiteral("协议属性"), page);
    heading->setObjectName(QStringLiteral("protocolPropertyHeading"));
    form->addRow(heading);
    m_documentName = new QLineEdit(page);
    m_documentName->setObjectName(QStringLiteral("protocolNameEditor"));
    form->addRow(QStringLiteral("协议名称"), m_documentName);
    m_documentFrameCount = new QLabel(page);
    form->addRow(QStringLiteral("协议帧数量"), m_documentFrameCount);
    m_documentByteCount = new QLabel(page);
    form->addRow(QStringLiteral("文档总字节数"), m_documentByteCount);
    auto *future = new QLabel(QStringLiteral("代码生成：后续功能"), page);
    future->setProperty("muted", true);
    form->addRow(future);
    m_documentErrors = new QLabel(page);
    m_documentErrors->setProperty("validationMessage", true);
    m_documentErrors->setWordWrap(true);
    form->addRow(m_documentErrors);
    return page;
}

QWidget *ProtocolPropertyPanel::createFramePage()
{
    QFormLayout *form = nullptr;
    QWidget *page = formPage(m_stack, &form);
    auto *heading = new QLabel(QStringLiteral("协议帧属性"), page);
    heading->setObjectName(QStringLiteral("protocolPropertyHeading"));
    form->addRow(heading);
    m_frameName = new QLineEdit(page);
    m_frameName->setObjectName(QStringLiteral("protocolFrameNameEditor"));
    form->addRow(QStringLiteral("协议帧名称"), m_frameName);
    m_frameId = new QLineEdit(page);
    m_frameId->setReadOnly(true);
    form->addRow(QStringLiteral("协议帧 ID"), m_frameId);
    m_frameDirection = new QComboBox(page);
    m_frameDirection->setObjectName(
        QStringLiteral("protocolFrameDirectionEditor"));
    addEnumItem(
        m_frameDirection, QStringLiteral("双向"),
        FrameDirection::Bidirectional);
    addEnumItem(
        m_frameDirection, QStringLiteral("仅发送 (Tx)"),
        FrameDirection::TransmitOnly);
    addEnumItem(
        m_frameDirection, QStringLiteral("仅接收 (Rx)"),
        FrameDirection::ReceiveOnly);
    form->addRow(QStringLiteral("通信方向"), m_frameDirection);
    m_frameFieldCount = new QLabel(page);
    form->addRow(QStringLiteral("字段数量"), m_frameFieldCount);
    m_frameByteCount = new QLabel(page);
    form->addRow(QStringLiteral("总字节数"), m_frameByteCount);
    m_frameErrors = new QLabel(page);
    m_frameErrors->setProperty("validationMessage", true);
    m_frameErrors->setWordWrap(true);
    form->addRow(m_frameErrors);
    return page;
}

QWidget *ProtocolPropertyPanel::createFieldPage()
{
    QFormLayout *form = nullptr;
    QWidget *page = formPage(m_stack, &form);
    auto *heading = new QLabel(QStringLiteral("字段属性"), page);
    heading->setObjectName(QStringLiteral("protocolPropertyHeading"));
    form->addRow(heading);
    m_fieldName = new QLineEdit(page);
    m_fieldName->setObjectName(QStringLiteral("protocolFieldNameEditor"));
    form->addRow(QStringLiteral("字段名"), m_fieldName);

    m_fieldRole = new QComboBox(page);
    addEnumItem(m_fieldRole, QStringLiteral("Header"), FieldRole::Header);
    addEnumItem(m_fieldRole, QStringLiteral("FrameId"), FieldRole::FrameId);
    addEnumItem(m_fieldRole, QStringLiteral("Length"), FieldRole::Length);
    addEnumItem(m_fieldRole, QStringLiteral("Data"), FieldRole::Data);
    addEnumItem(m_fieldRole, QStringLiteral("Checksum"), FieldRole::Checksum);
    addEnumItem(m_fieldRole, QStringLiteral("Tail"), FieldRole::Tail);
    addEnumItem(m_fieldRole, QStringLiteral("Skip"), FieldRole::Skip);
    form->addRow(QStringLiteral("字段角色"), m_fieldRole);

    m_byteCount = new QSpinBox(page);
    m_byteCount->setRange(1, 65535);
    m_byteCount->setSuffix(QStringLiteral(" B"));
    form->addRow(QStringLiteral("字节数"), m_byteCount);

    m_dataType = new QComboBox(page);
    constexpr DataType types[]{
        DataType::UInt8, DataType::Int8, DataType::UInt16, DataType::Int16,
        DataType::UInt32, DataType::Int32, DataType::UInt64, DataType::Int64,
        DataType::Float32, DataType::Float64, DataType::Bytes
    };
    for (const DataType type : types) {
        addEnumItem(m_dataType, dataTypeDisplayName(type), type);
    }
    m_typeRow = addRow(form, QStringLiteral("类型"), m_dataType);

    m_byteOrder = new QComboBox(page);
    addEnumItem(
        m_byteOrder, QStringLiteral("Little Endian"),
        ByteOrder::LittleEndian);
    addEnumItem(
        m_byteOrder, QStringLiteral("Big Endian"),
        ByteOrder::BigEndian);
    m_byteOrderRow = addRow(
        form, QStringLiteral("字节序"), m_byteOrder);

    m_scale = new QDoubleSpinBox(page);
    m_scale->setDecimals(8);
    m_scale->setRange(-1e12, 1e12);
    m_scaleRow = addRow(form, QStringLiteral("缩放值"), m_scale);
    m_offset = new QDoubleSpinBox(page);
    m_offset->setDecimals(8);
    m_offset->setRange(-1e12, 1e12);
    m_offsetRow = addRow(form, QStringLiteral("偏移值"), m_offset);

    auto *hexContainer = new QWidget(page);
    auto *hexLayout = new QVBoxLayout(hexContainer);
    hexLayout->setContentsMargins(0, 0, 0, 0);
    hexLayout->setSpacing(3);
    m_fixedHex = new QLineEdit(hexContainer);
    m_fixedHex->setObjectName(QStringLiteral("protocolFixedHexEditor"));
    m_fixedHex->setPlaceholderText(QStringLiteral("AA 55"));
    hexLayout->addWidget(m_fixedHex);
    m_fixedHexError = new QLabel(hexContainer);
    m_fixedHexError->setProperty("validationMessage", true);
    m_fixedHexError->setWordWrap(true);
    hexLayout->addWidget(m_fixedHexError);
    m_fixedHexRow = addRow(
        form, QStringLiteral("固定 Hex"), hexContainer);

    m_checksum = new QComboBox(page);
    addEnumItem(m_checksum, QStringLiteral("Sum8"), ChecksumAlgorithm::Sum8);
    addEnumItem(m_checksum, QStringLiteral("XOR8"), ChecksumAlgorithm::Xor8);
    addEnumItem(m_checksum, QStringLiteral("CRC8"), ChecksumAlgorithm::Crc8);
    m_checksumRow = addRow(
        form, QStringLiteral("校验算法"), m_checksum);

    m_fieldErrors = new QLabel(page);
    m_fieldErrors->setProperty("validationMessage", true);
    m_fieldErrors->setWordWrap(true);
    form->addRow(m_fieldErrors);
    return page;
}

void ProtocolPropertyPanel::connectEditors()
{
    connect(m_documentName, &QLineEdit::editingFinished, this, [this] {
        if (!m_updating) emit documentNameEdited(m_documentName->text());
    });
    connect(m_frameName, &QLineEdit::editingFinished,
            this, &ProtocolPropertyPanel::submitFrame);
    connect(m_frameDirection, &QComboBox::currentIndexChanged,
            this, &ProtocolPropertyPanel::submitFrame);
    connect(m_fieldName, &QLineEdit::editingFinished,
            this, &ProtocolPropertyPanel::submitField);
    connect(m_byteCount, &QSpinBox::editingFinished,
            this, &ProtocolPropertyPanel::submitField);
    connect(m_scale, &QDoubleSpinBox::editingFinished,
            this, &ProtocolPropertyPanel::submitField);
    connect(m_offset, &QDoubleSpinBox::editingFinished,
            this, &ProtocolPropertyPanel::submitField);
    connect(m_fixedHex, &QLineEdit::editingFinished, this, [this] {
        if (m_updating) return;
        QString normalized = m_fixedHex->text().toUpper();
        bool valid = false;
        const QByteArray bytes = parseHex(normalized, &valid);
        if (valid) {
            normalized = formatHex(bytes);
            m_fixedHex->setText(normalized);
        }
        updateHexValidation();
        submitField();
    });
    const auto comboChanged = [this] {
        if (m_updating) return;
        submitField();
    };
    connect(m_fieldRole, &QComboBox::currentIndexChanged,
            this, comboChanged);
    connect(m_dataType, &QComboBox::currentIndexChanged,
            this, comboChanged);
    connect(m_byteOrder, &QComboBox::currentIndexChanged,
            this, comboChanged);
    connect(m_checksum, &QComboBox::currentIndexChanged,
            this, comboChanged);
}

void ProtocolPropertyPanel::updateFieldVisibility()
{
    const FieldRole role =
        m_fieldRole->currentData().value<FieldRole>();
    const bool typed = role == FieldRole::Data
        || role == FieldRole::FrameId || role == FieldRole::Length;
    setFormRowVisible(m_typeRow, typed);
    setFormRowVisible(m_byteOrderRow, typed);
    setFormRowVisible(m_scaleRow, typed);
    setFormRowVisible(m_offsetRow, typed);
    setFormRowVisible(m_fixedHexRow,
        role == FieldRole::Header || role == FieldRole::Tail);
    setFormRowVisible(m_checksumRow, role == FieldRole::Checksum);
}

void ProtocolPropertyPanel::submitFrame()
{
    if (m_updating) return;
    Frame updated = m_currentFrame;
    updated.name = m_frameName->text();
    updated.direction =
        m_frameDirection->currentData().value<FrameDirection>();
    if (updated == m_currentFrame) return;
    m_currentFrame = updated;
    emit frameEdited(updated);
}

void ProtocolPropertyPanel::submitField()
{
    if (m_updating) return;
    Field updated = m_currentField;
    updated.name = m_fieldName->text();
    updated.role = m_fieldRole->currentData().value<FieldRole>();
    updated.byteCount = m_byteCount->value();
    updated.dataType = m_dataType->currentData().value<DataType>();
    updated.byteOrder = m_byteOrder->currentData().value<ByteOrder>();
    updated.scale = m_scale->value();
    updated.offset = m_offset->value();
    bool validHex = false;
    const QByteArray fixed = parseHex(m_fixedHex->text(), &validHex);
    if (validHex) updated.fixedBytes = fixed;
    updated.checksumAlgorithm =
        m_checksum->currentData().value<ChecksumAlgorithm>();
    updateFieldVisibility();
    updateHexValidation();
    if (updated == m_currentField) return;
    m_currentField = updated;
    emit fieldEdited(m_currentFrameId, updated);
}

void ProtocolPropertyPanel::setInvalid(
    QWidget *widget, const bool invalid)
{
    widget->setProperty("invalid", invalid);
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

void ProtocolPropertyPanel::updateHexValidation()
{
    const FieldRole role =
        m_fieldRole->currentData().value<FieldRole>();
    if (role != FieldRole::Header && role != FieldRole::Tail) {
        setInvalid(m_fixedHex, false);
        m_fixedHexError->clear();
        m_fixedHexError->hide();
        return;
    }
    bool valid = false;
    const QByteArray bytes = parseHex(m_fixedHex->text(), &valid);
    QString error;
    if (!valid) {
        error = QStringLiteral("请输入成对的十六进制字节");
    } else if (bytes.size() != m_byteCount->value()) {
        error = QStringLiteral("实际为 %1 字节，应为 %2 字节")
                    .arg(bytes.size())
                    .arg(m_byteCount->value());
    }
    setInvalid(m_fixedHex, !error.isEmpty());
    m_fixedHexError->setText(error);
    m_fixedHexError->setVisible(!error.isEmpty());
}

QStringList ProtocolPropertyPanel::messagesFor(
    const QVector<ValidationIssue> &issues,
    const QUuid &frameId,
    const QUuid &fieldId) const
{
    QStringList messages;
    for (const ValidationIssue &issue : issues) {
        const bool matches = fieldId.isNull()
            ? frameId.isNull()
                ? issue.frameId.isNull()
                : issue.frameId == frameId
            : issue.frameId == frameId && issue.fieldId == fieldId;
        if (!matches) continue;
        messages.append(
            (issue.severity == ValidationSeverity::Error
                 ? QStringLiteral("错误：") : QStringLiteral("警告："))
            + issue.message);
    }
    return messages;
}
