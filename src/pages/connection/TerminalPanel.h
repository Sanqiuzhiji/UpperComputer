#pragma once

#include <QFile>
#include <QFrame>
#include <QList>

#include "models/AppTypes.h"
#include "models/ConnectionTypes.h"

class AppContext;
class QComboBox;
class QTextCharFormat;
class QTextEdit;
class QTimer;
class QToolButton;

class TerminalPanel final : public QFrame
{
    Q_OBJECT

public:
    explicit TerminalPanel(AppContext *context, QWidget *parent = nullptr);
    ~TerminalPanel() override;

    [[nodiscard]] TextEncoding textEncoding() const;
    [[nodiscard]] QString decode(const QByteArray &bytes) const;

public slots:
    void addEntry(DataDirection direction, const QByteArray &bytes);
    void clearEntries();

signals:
    void textEncodingChanged(TextEncoding encoding);
    void notificationRequested(const QString &message, NotificationType type);

private:
    QToolButton *createToolButton(const QString &text,
                                  const QString &toolTip,
                                  const QString &iconPath = {});
    void refreshIcons();
    void flushPending();
    void renderAll();
    void appendEntry(const TerminalEntry &entry);
    void appendAnsiText(const QString &text);
    void applySgrCode(const QString &code, QTextCharFormat *format) const;
    [[nodiscard]] QString displayText(const QByteArray &bytes) const;
    [[nodiscard]] QString logLine(const TerminalEntry &entry) const;
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
    QList<TerminalEntry> m_entries;
    QList<TerminalEntry> m_pendingEntries;
    qsizetype m_entryBytes{};
    qsizetype m_pendingBytes{};
    QFile m_logFile;
    QByteArray m_pendingLog;
    TerminalDisplayMode m_displayMode{TerminalDisplayMode::Text};
};
