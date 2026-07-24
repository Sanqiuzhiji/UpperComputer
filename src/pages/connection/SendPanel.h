#pragma once

#include <QFrame>

#include "models/AppTypes.h"
#include "models/ConnectionTypes.h"

class AppContext;
class QComboBox;
class QLineEdit;
class QPushButton;
class QToolButton;

class SendPanel final : public QFrame
{
    Q_OBJECT

public:
    explicit SendPanel(AppContext *context, QWidget *parent = nullptr);

public slots:
    void setTextEncoding(TextEncoding encoding);

signals:
    void sendRequested(const QByteArray &data);
    void notificationRequested(const QString &message, NotificationType type);

private:
    void toggleInputMode();
    void requestSend();
    [[nodiscard]] bool buildPayload(QByteArray *payload, QString *error) const;
    [[nodiscard]] QByteArray lineEndingBytes() const;
    void setInputError(bool error);
    void refreshIcon();

    AppContext *m_context{};
    QToolButton *m_inputModeButton{};
    QLineEdit *m_input{};
    QComboBox *m_checksumCombo{};
    QComboBox *m_lineEndingCombo{};
    QPushButton *m_sendButton{};
    InputMode m_inputMode{InputMode::Text};
    TextEncoding m_encoding{TextEncoding::Utf8};
    QString m_textDraft;
    QString m_hexDraft;
};
