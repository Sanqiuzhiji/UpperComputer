#pragma once

#include <QFile>
#include <QFrame>
#include <QList>

#include "models/AppTypes.h"
#include "models/ConnectionTypes.h"
#include "models/ProtocolDefinition.h"

class AppContext;
class QComboBox;
class QTextCharFormat;
class QTextEdit;
class QTimer;
class QToolButton;

class CommunicationMonitorPanel final : public QFrame
{
    Q_OBJECT

public:
    explicit CommunicationMonitorPanel(
        AppContext *context, QWidget *parent = nullptr);
    ~CommunicationMonitorPanel() override;

    [[nodiscard]] TextEncoding textEncoding() const;
    [[nodiscard]] QString decode(const QByteArray &bytes) const;
    [[nodiscard]] ParserMode receiveMode() const noexcept;

public slots:
    void setReceiveMode(ParserMode mode);
    void addEntry(DataDirection direction, const QByteArray &bytes);
    void addReceivedData(qint64 timestampUs, const QByteArray &bytes);
    void addParsedMessages(
        qint64 timestampUs, const QList<ParsedMessage> &messages);
    void clearEntries();

signals:
    void textEncodingChanged(TextEncoding encoding);
    void notificationRequested(const QString &message, NotificationType type);

private:
    struct MonitorEntry {
        QDateTime timestamp;
        DataDirection direction{DataDirection::Receive};
        QByteArray rawData;
        QString messageName;
        QList<ParsedField> fields;
        bool structured{};
    };

    QToolButton *createToolButton(const QString &text,
                                  const QString &toolTip,
                                  const QString &iconPath = {});
    void refreshIcons();
    void flushPending();
    void renderAll();
    void appendEntry(const MonitorEntry &entry);
    void appendAnsiText(
        const QString &text, const QTextCharFormat &baseFormat);
    void applySgrCode(
        const QString &code,
        const QTextCharFormat &baseFormat,
        QTextCharFormat *format) const;
    void showModeEmptyState();
    void updateToolbarForMode();
    [[nodiscard]] QString displayText(const QByteArray &bytes) const;
    [[nodiscard]] QString logLine(const TerminalEntry &entry) const;
    [[nodiscard]] QString parserUnavailableText() const;
    void updateToolTips();
    void toggleLogging(bool enabled);
    void stopLogging(bool notify);

    static constexpr int kMaximumEntries = 5000;
    static constexpr qsizetype kMaximumBytes = 5 * 1024 * 1024;

    AppContext *m_context{};
    QToolButton *m_displayModeButton{};
    QToolButton *m_timestampButton{};
    QComboBox *m_encodingCombo{};
    QToolButton *m_receiveButton{};
    QToolButton *m_transmitButton{};
    QToolButton *m_ansiButton{};
    QToolButton *m_logButton{};
    QToolButton *m_clearButton{};
    QTextEdit *m_terminal{};
    QTimer *m_flushTimer{};
    QList<MonitorEntry> m_entries;
    QList<MonitorEntry> m_pendingEntries;
    qsizetype m_entryBytes{};
    qsizetype m_pendingBytes{};
    QFile m_logFile;
    QByteArray m_pendingLog;
    TerminalDisplayMode m_displayMode{TerminalDisplayMode::Text};
    ParserMode m_receiveMode{ParserMode::RawData};
    bool m_emptyStateVisible{};
    bool m_rawTextLineOpen{};
    DataDirection m_rawTextLineDirection{DataDirection::Receive};
};
