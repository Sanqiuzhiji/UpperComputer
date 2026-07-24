#include "TerminalPanel.h"

#include "app/AppContext.h"
#include "app/AppSettings.h"
#include "theme/IconManager.h"
#include "widgets/FocusUnderline.h"

#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <utility>

namespace {
QColor ansiColor(const int color, const bool bright)
{
    static const QColor normal[]{
        QColor("#2B2B2B"), QColor("#D14D41"), QColor("#49A65A"),
        QColor("#C9A227"), QColor("#3B82D0"), QColor("#A65FD0"),
        QColor("#2AA6A6"), QColor("#D8D8D8")};
    static const QColor high[]{
        QColor("#737373"), QColor("#FF6257"), QColor("#63D176"),
        QColor("#F0CC4B"), QColor("#64A9F2"), QColor("#D58AF2"),
        QColor("#57D6D6"), QColor("#FFFFFF")};
    return (bright ? high : normal)[qBound(0, color, 7)];
}
}

TerminalPanel::TerminalPanel(AppContext *context, QWidget *parent)
    : QFrame(parent),
      m_context(context),
      m_flushTimer(new QTimer(this)),
      m_displayMode(context->settings()->terminalDisplayMode())
{
    setProperty("card", true);
    setObjectName(QStringLiteral("terminalPanel"));
    setMinimumHeight(220);
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(10, 8, 10, 10);
    root->setSpacing(7);
    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(4);

    m_displayModeButton = createToolButton(
        m_displayMode == TerminalDisplayMode::Hex
            ? QStringLiteral("HEX") : QStringLiteral("ABC"),
        tr("切换 HEX/文本显示"));
    m_timestampButton = createToolButton(QStringLiteral("Time"), tr("显示时间戳"),
                                         QStringLiteral(":/icons/connection/clock.svg"));
    m_timestampButton->setCheckable(true);
    m_timestampButton->setChecked(context->settings()->terminalTimestampEnabled());
    m_encodingCombo = new QComboBox(this);
    m_encodingCombo->addItem(QStringLiteral("UTF-8"),
                             QVariant::fromValue(TextEncoding::Utf8));
    m_encodingCombo->addItem(QStringLiteral("Local 8-bit"),
                             QVariant::fromValue(TextEncoding::Local8Bit));
    m_encodingCombo->addItem(QStringLiteral("Latin-1"),
                             QVariant::fromValue(TextEncoding::Latin1));
    const int encodingIndex = m_encodingCombo->findData(
        QVariant::fromValue(context->settings()->textEncoding()));
    if (encodingIndex >= 0) m_encodingCombo->setCurrentIndex(encodingIndex);
    m_encodingCombo->setToolTip(tr("文本编码"));
    m_encodingCombo->setMinimumWidth(112);
    new FocusUnderline(m_encodingCombo, context->themeManager());

    m_receiveButton = createToolButton(QStringLiteral("RX"), {},
                                       QStringLiteral(":/icons/connection/receive.svg"));
    m_transmitButton = createToolButton(QStringLiteral("TX"), {},
                                        QStringLiteral(":/icons/connection/transmit.svg"));
    m_ansiButton = createToolButton(QStringLiteral("ANSI"), tr("解析 ANSI 转义序列"),
                                    QStringLiteral(":/icons/connection/ansi.svg"));
    m_logButton = createToolButton(QStringLiteral("Log"), {},
                                   QStringLiteral(":/icons/connection/log.svg"));
    for (QToolButton *button : {
             m_receiveButton, m_transmitButton, m_ansiButton, m_logButton}) {
        button->setCheckable(true);
    }
    m_receiveButton->setChecked(context->settings()->receiveVisible());
    m_transmitButton->setChecked(context->settings()->transmitVisible());
    m_ansiButton->setChecked(context->settings()->ansiEnabled());
    m_clearButton = createToolButton({}, tr("清空显示数据"),
                                     QStringLiteral(":/icons/connection/clear.svg"));

    toolbar->addWidget(m_displayModeButton);
    toolbar->addWidget(m_timestampButton);
    toolbar->addWidget(m_encodingCombo);
    toolbar->addWidget(m_receiveButton);
    toolbar->addWidget(m_transmitButton);
    toolbar->addWidget(m_ansiButton);
    toolbar->addWidget(m_logButton);
    toolbar->addStretch();
    toolbar->addWidget(m_clearButton);
    root->addLayout(toolbar);

    m_terminal = new QTextEdit(this);
    m_terminal->setObjectName(QStringLiteral("connectionTerminal"));
    m_terminal->setReadOnly(true);
    m_terminal->setUndoRedoEnabled(false);
    m_terminal->setAcceptRichText(false);
    m_terminal->setPlaceholderText(tr("接收和发送的数据将在这里显示"));
    m_terminal->document()->setMaximumBlockCount(10000);
    new FocusUnderline(m_terminal, context->themeManager());
    root->addWidget(m_terminal, 1);

    m_flushTimer->setInterval(33);
    connect(m_flushTimer, &QTimer::timeout, this, &TerminalPanel::flushPending);
    m_flushTimer->start();
    connect(m_displayModeButton, &QToolButton::clicked, this, [this] {
        m_displayMode = m_displayMode == TerminalDisplayMode::Text
            ? TerminalDisplayMode::Hex : TerminalDisplayMode::Text;
        m_context->settings()->setTerminalDisplayMode(m_displayMode);
        renderAll();
        refreshIcons();
        updateToolTips();
    });
    connect(m_timestampButton, &QToolButton::toggled, this, [this](bool checked) {
        m_context->settings()->setTerminalTimestampEnabled(checked);
        renderAll();
    });
    connect(m_encodingCombo, &QComboBox::currentIndexChanged, this, [this] {
        const TextEncoding encoding = textEncoding();
        m_context->settings()->setTextEncoding(encoding);
        renderAll();
        emit textEncodingChanged(encoding);
    });
    connect(m_receiveButton, &QToolButton::toggled, this, [this](bool checked) {
        m_context->settings()->setReceiveVisible(checked);
        renderAll();
        updateToolTips();
    });
    connect(m_transmitButton, &QToolButton::toggled, this, [this](bool checked) {
        m_context->settings()->setTransmitVisible(checked);
        renderAll();
        updateToolTips();
    });
    connect(m_ansiButton, &QToolButton::toggled, this, [this](bool checked) {
        m_context->settings()->setAnsiEnabled(checked);
        renderAll();
    });
    connect(m_logButton, &QToolButton::toggled,
            this, &TerminalPanel::toggleLogging);
    connect(m_clearButton, &QToolButton::clicked,
            this, &TerminalPanel::clearEntries);
    connect(context->iconManager(), &IconManager::iconsChanged,
            this, &TerminalPanel::refreshIcons);
    refreshIcons();
    updateToolTips();
}

TerminalPanel::~TerminalPanel()
{
    stopLogging(false);
}

TextEncoding TerminalPanel::textEncoding() const
{
    return m_encodingCombo->currentData().value<TextEncoding>();
}

QString TerminalPanel::decode(const QByteArray &bytes) const
{
    switch (textEncoding()) {
    case TextEncoding::Utf8: return QString::fromUtf8(bytes);
    case TextEncoding::Local8Bit: return QString::fromLocal8Bit(bytes);
    case TextEncoding::Latin1: return QString::fromLatin1(bytes);
    }
    return QString::fromUtf8(bytes);
}

void TerminalPanel::addEntry(
    const DataDirection direction, const QByteArray &bytes)
{
    if (bytes.isEmpty()) return;
    const QDateTime timestamp = QDateTime::currentDateTime();
    if (m_logFile.isOpen()) {
        m_pendingLog += logLine(
            TerminalEntry{timestamp, direction, bytes}).toUtf8();
    }
    TerminalEntry entry{
        timestamp, direction,
        bytes.size() > kMaximumBytes ? bytes.right(kMaximumBytes) : bytes};
    m_entries.append(entry);
    m_pendingEntries.append(entry);
    m_entryBytes += entry.rawData.size();
    m_pendingBytes += entry.rawData.size();
    while (m_entries.size() > kMaximumEntries
           || m_entryBytes > kMaximumBytes) {
        m_entryBytes -= m_entries.front().rawData.size();
        m_entries.removeFirst();
    }
    while (m_pendingEntries.size() > kMaximumEntries
           || m_pendingBytes > kMaximumBytes) {
        m_pendingBytes -= m_pendingEntries.front().rawData.size();
        m_pendingEntries.removeFirst();
    }
}

void TerminalPanel::clearEntries()
{
    m_entries.clear();
    m_pendingEntries.clear();
    m_entryBytes = 0;
    m_pendingBytes = 0;
    m_terminal->clear();
}

QToolButton *TerminalPanel::createToolButton(
    const QString &text, const QString &toolTip, const QString &iconPath)
{
    auto *button = new QToolButton(this);
    button->setProperty("terminalTool", true);
    button->setText(text);
    button->setToolTip(toolTip);
    button->setToolButtonStyle(text.isEmpty()
        ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon);
    button->setProperty("iconPath", iconPath);
    button->setMinimumHeight(34);
    return button;
}

void TerminalPanel::refreshIcons()
{
    m_displayModeButton->setIcon(m_context->iconManager()->icon(
        m_displayMode == TerminalDisplayMode::Hex
            ? QStringLiteral(":/icons/connection/hex.svg")
            : QStringLiteral(":/icons/connection/text.svg")));
    m_displayModeButton->setIconSize(QSize(17, 17));
    for (QToolButton *button : {
             m_timestampButton, m_receiveButton, m_transmitButton,
             m_ansiButton, m_logButton, m_clearButton}) {
        const QString path = button->property("iconPath").toString();
        if (!path.isEmpty()) {
            button->setIcon(m_context->iconManager()->icon(path));
            button->setIconSize(QSize(17, 17));
        }
    }
}

void TerminalPanel::flushPending()
{
    if (!m_pendingEntries.isEmpty()) {
        const QList<TerminalEntry> batch = std::exchange(
            m_pendingEntries, QList<TerminalEntry>{});
        m_pendingBytes = 0;
        for (const TerminalEntry &entry : batch) {
            if ((entry.direction == DataDirection::Receive
                 && !m_receiveButton->isChecked())
                || (entry.direction == DataDirection::Transmit
                    && !m_transmitButton->isChecked())) {
                continue;
            }
            appendEntry(entry);
        }
        m_terminal->moveCursor(QTextCursor::End);
    }
    if (m_logFile.isOpen() && !m_pendingLog.isEmpty()) {
        if (m_logFile.write(m_pendingLog) < 0 || !m_logFile.flush()) {
            const QString error = m_logFile.errorString();
            stopLogging(false);
            emit notificationRequested(
                tr("通信日志写入失败：%1").arg(error),
                NotificationType::Error);
        }
        m_pendingLog.clear();
    }
}

void TerminalPanel::renderAll()
{
    m_pendingEntries.clear();
    m_pendingBytes = 0;
    m_terminal->clear();
    for (const TerminalEntry &entry : std::as_const(m_entries)) {
        if ((entry.direction == DataDirection::Receive
             && !m_receiveButton->isChecked())
            || (entry.direction == DataDirection::Transmit
                && !m_transmitButton->isChecked())) {
            continue;
        }
        appendEntry(entry);
    }
    m_terminal->moveCursor(QTextCursor::End);
}

void TerminalPanel::appendEntry(const TerminalEntry &entry)
{
    QTextCursor cursor = m_terminal->textCursor();
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat prefixFormat;
    prefixFormat.setForeground(entry.direction == DataDirection::Receive
        ? QColor(QStringLiteral("#45B97C"))
        : m_context->themeManager()->accentColor());
    QString prefix;
    if (m_timestampButton->isChecked()) {
        prefix += QStringLiteral("[%1] ")
            .arg(entry.timestamp.toString(QStringLiteral("HH:mm:ss.zzz")));
    }
    prefix += entry.direction == DataDirection::Receive
        ? QStringLiteral("RX ") : QStringLiteral("TX ");
    cursor.insertText(prefix, prefixFormat);
    m_terminal->setTextCursor(cursor);
    QString text = displayText(entry.rawData);
    if (m_displayMode == TerminalDisplayMode::Text
        && m_ansiButton->isChecked()) {
        appendAnsiText(text);
    } else {
        static const QRegularExpression ansi(
            QStringLiteral("\\x1B\\[[0-9;?]*[ -/]*[@-~]"));
        cursor = m_terminal->textCursor();
        cursor.insertText(text.remove(ansi));
    }
    cursor = m_terminal->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(QStringLiteral("\n"));
    m_terminal->setTextCursor(cursor);
}

void TerminalPanel::appendAnsiText(const QString &text)
{
    static const QRegularExpression controlSequence(
        QStringLiteral("\\x1B\\[([0-9;?]*)([ -/]*)([@-~])"));
    QTextCursor cursor = m_terminal->textCursor();
    QTextCharFormat format;
    qsizetype offset = 0;
    auto matchIterator = controlSequence.globalMatch(text);
    while (matchIterator.hasNext()) {
        const QRegularExpressionMatch match = matchIterator.next();
        cursor.insertText(text.mid(offset, match.capturedStart() - offset), format);
        if (match.captured(3) == QStringLiteral("m")) {
            const QStringList codes = match.captured(1).isEmpty()
                ? QStringList{QStringLiteral("0")}
                : match.captured(1).split(u';');
            for (const QString &code : codes) applySgrCode(code, &format);
        }
        // Unsupported control sequences are consumed without executing them.
        offset = match.capturedEnd();
    }
    cursor.insertText(text.mid(offset), format);
    m_terminal->setTextCursor(cursor);
}

void TerminalPanel::applySgrCode(
    const QString &codeText, QTextCharFormat *format) const
{
    bool ok = false;
    const int code = codeText.toInt(&ok);
    if (!ok || code == 0) {
        *format = QTextCharFormat();
    } else if (code == 1) {
        format->setFontWeight(QFont::Bold);
    } else if (code == 22) {
        format->setFontWeight(QFont::Normal);
    } else if (code >= 30 && code <= 37) {
        format->setForeground(ansiColor(code - 30, false));
    } else if (code >= 90 && code <= 97) {
        format->setForeground(ansiColor(code - 90, true));
    } else if (code == 39) {
        format->clearForeground();
    }
}

QString TerminalPanel::displayText(const QByteArray &bytes) const
{
    return m_displayMode == TerminalDisplayMode::Hex
        ? QString::fromLatin1(bytes.toHex(' ').toUpper())
        : decode(bytes);
}

QString TerminalPanel::logLine(const TerminalEntry &entry) const
{
    const QString direction = entry.direction == DataDirection::Receive
        ? QStringLiteral("RX") : QStringLiteral("TX");
    QString readable = decode(entry.rawData);
    readable.replace(u'\r', QStringLiteral("\\r"));
    readable.replace(u'\n', QStringLiteral("\\n"));
    return QStringLiteral("[%1] %2 | HEX: %3 | TEXT: %4\n")
        .arg(entry.timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")),
             direction,
             QString::fromLatin1(entry.rawData.toHex(' ').toUpper()),
             readable);
}

void TerminalPanel::updateToolTips()
{
    m_displayModeButton->setText(
        m_displayMode == TerminalDisplayMode::Hex
            ? QStringLiteral("HEX") : QStringLiteral("ABC"));
    m_receiveButton->setToolTip(m_receiveButton->isChecked()
        ? tr("隐藏 RX 数据") : tr("显示 RX 数据"));
    m_transmitButton->setToolTip(m_transmitButton->isChecked()
        ? tr("隐藏 TX 数据") : tr("显示 TX 数据"));
    m_logButton->setToolTip(m_logFile.isOpen()
        ? tr("停止通信日志记录") : tr("开始通信日志记录"));
}

void TerminalPanel::toggleLogging(const bool enabled)
{
    if (!enabled) {
        stopLogging(true);
        return;
    }
    const QString path = QFileDialog::getSaveFileName(
        this, tr("保存通信日志"), QString(),
        tr("文本日志 (*.log *.txt);;所有文件 (*)"));
    if (path.isEmpty()) {
        const QSignalBlocker blocker(m_logButton);
        m_logButton->setChecked(false);
        return;
    }
    m_logFile.setFileName(path);
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        const QSignalBlocker blocker(m_logButton);
        m_logButton->setChecked(false);
        emit notificationRequested(
            tr("无法开始通信日志：%1").arg(m_logFile.errorString()),
            NotificationType::Error);
        return;
    }
    updateToolTips();
    emit notificationRequested(tr("通信日志记录已开始"),
                               NotificationType::Success);
}

void TerminalPanel::stopLogging(const bool notify)
{
    if (!m_logFile.isOpen()) {
        updateToolTips();
        return;
    }
    if (!m_pendingLog.isEmpty()) {
        m_logFile.write(m_pendingLog);
        m_pendingLog.clear();
    }
    m_logFile.flush();
    m_logFile.close();
    {
        const QSignalBlocker blocker(m_logButton);
        m_logButton->setChecked(false);
    }
    updateToolTips();
    if (notify) {
        emit notificationRequested(tr("通信日志记录已停止"),
                                   NotificationType::Information);
    }
}
