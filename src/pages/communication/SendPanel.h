#pragma once

#include <QFrame>
#include <QList>

#include <map>
#include <memory>

#include "models/AppTypes.h"
#include "models/ConnectionTypes.h"
#include "models/ProtocolDefinition.h"

class AppContext;
class CustomBinaryEncoder;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QStackedWidget;
class QToolButton;
class QVBoxLayout;

class SendPanel final : public QFrame
{
    Q_OBJECT

public:
    explicit SendPanel(AppContext *context, QWidget *parent = nullptr);

    void setProtocols(const QList<ProtocolDefinition> &protocols);
    void setCurrentProtocolId(const QString &protocolId);
    void setCurrentCommandId(const QString &commandId);
    void setEncoder(
        const QString &protocolId,
        std::shared_ptr<const CustomBinaryEncoder> encoder);

    [[nodiscard]] int dynamicFieldCount() const noexcept;
    [[nodiscard]] bool collectCustomValues(
        ProtocolFieldValues *values, QString *errorMessage);

public slots:
    void setTextEncoding(TextEncoding encoding);
    void setSendMode(SendMode mode);

signals:
    void sendRequested(const QByteArray &data);
    void notificationRequested(const QString &message, NotificationType type);

private:
    struct FieldEditor {
        FieldDefinition definition;
        QWidget *editor{};
        QWidget *row{};
    };

    QWidget *createRawPage();
    QWidget *createCustomBinaryPage();
    QWidget *createFieldEditor(
        const FieldDefinition &field, int fieldIndex, QWidget *parent);
    void toggleInputMode();
    void requestRawSend();
    void requestCustomSend();
    void previewCustomPayload();
    void populateCommands();
    void rebuildDynamicFields();
    void clearDynamicFields();
    void saveCustomDrafts();
    void updateCustomState();
    [[nodiscard]] bool buildRawPayload(
        QByteArray *payload, QString *error) const;
    [[nodiscard]] bool buildCustomPayload(
        QByteArray *payload, QString *error);
    [[nodiscard]] bool fieldValue(
        FieldEditor &editor, QVariant *value, QString *error);
    [[nodiscard]] bool normalizeIntegerEditor(
        FieldEditor &editor, QVariant *value, QString *error);
    [[nodiscard]] const ProtocolDefinition *currentProtocol() const;
    [[nodiscard]] const MessageDefinition *currentMessage() const;
    [[nodiscard]] QByteArray lineEndingBytes() const;
    [[nodiscard]] QString fieldDraftKey(const QString &fieldId) const;
    [[nodiscard]] QString fieldTypeText(const FieldDefinition &field) const;
    [[nodiscard]] QString fieldMetaText(const FieldDefinition &field) const;
    void setInvalid(QWidget *widget, bool invalid);
    void setInputError(bool error);
    void refreshIcon();

    AppContext *m_context{};
    QStackedWidget *m_modeStack{};

    QToolButton *m_inputModeButton{};
    QLineEdit *m_input{};
    QComboBox *m_checksumCombo{};
    QComboBox *m_lineEndingCombo{};
    QPushButton *m_rawSendButton{};

    QScrollArea *m_fieldScroll{};
    QWidget *m_fieldContainer{};
    QVBoxLayout *m_fieldLayout{};
    QLabel *m_customStatus{};
    QLabel *m_previewLabel{};
    QPushButton *m_previewButton{};
    QPushButton *m_customSendButton{};

    QList<ProtocolDefinition> m_protocols;
    QList<FieldEditor> m_fieldEditors;
    std::map<QString, std::shared_ptr<const CustomBinaryEncoder>> m_encoders;
    QString m_currentProtocolId;
    QString m_currentCommandId;
    InputMode m_inputMode{InputMode::Text};
    TextEncoding m_encoding{TextEncoding::Utf8};
    QString m_textDraft;
    QString m_hexDraft;
};
