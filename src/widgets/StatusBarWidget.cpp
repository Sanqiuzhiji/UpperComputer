#include "StatusBarWidget.h"

#include <QHBoxLayout>
#include <QLabel>

StatusBarWidget::StatusBarWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("statusBar"));
    setFixedHeight(28);
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 12, 0);

    m_connection = new QLabel(this);
    m_source = new QLabel(this);
    m_source->setProperty("muted", true);
    m_rates = new QLabel(this);
    m_rates->setProperty("muted", true);
    m_theme = new QLabel(tr("主题：深色"), this);
    m_theme->setProperty("muted", true);
    m_page = new QLabel(tr("页面：Plot"), this);
    m_page->setProperty("muted", true);

    layout->addWidget(m_connection);
    layout->addSpacing(16);
    layout->addWidget(m_source);
    layout->addWidget(m_rates);
    layout->addStretch();
    layout->addWidget(m_theme);
    layout->addSpacing(16);
    layout->addWidget(m_page);

    setConnectionState(ConnectionState::Disconnected);
    refreshSourceText();
    refreshRateText();
}

void StatusBarWidget::setConnectionState(const ConnectionState state)
{
    QString text;
    QString color;
    switch (state) {
    case ConnectionState::Disconnected:
        text = tr("未连接");
        color = QStringLiteral("#9DA3AE");
        break;
    case ConnectionState::Connecting:
        text = tr("正在连接");
        color = QStringLiteral("#E6A23C");
        break;
    case ConnectionState::Connected:
        text = tr("已连接");
        color = QStringLiteral("#45B97C");
        break;
    case ConnectionState::Disconnecting:
        text = tr("正在断开");
        color = QStringLiteral("#E6A23C");
        break;
    case ConnectionState::Error:
        text = tr("连接异常");
        color = QStringLiteral("#E5484D");
        break;
    }
    m_connection->setText(text);
    m_connection->setStyleSheet(
        QStringLiteral("color:%1;").arg(color));
}

void StatusBarWidget::setDeviceName(const QString &name)
{
    m_deviceName = name;
    refreshSourceText();
}

void StatusBarWidget::setDataSourceName(const QString &name)
{
    m_dataSourceName = name;
    refreshSourceText();
}

void StatusBarWidget::setReceiveRate(const double bytesPerSecond)
{
    m_receiveRate = qMax(0.0, bytesPerSecond);
    refreshRateText();
}

void StatusBarWidget::setTransmitRate(const double bytesPerSecond)
{
    m_transmitRate = qMax(0.0, bytesPerSecond);
    refreshRateText();
}

void StatusBarWidget::setReceiveTotal(const quint64 bytes)
{
    m_receiveTotal = bytes;
    refreshRateText();
}

void StatusBarWidget::setTransmitTotal(const quint64 bytes)
{
    m_transmitTotal = bytes;
    refreshRateText();
}

void StatusBarWidget::setCurrentPageTitle(const QString &title)
{
    m_page->setText(tr("页面：%1").arg(title));
}

void StatusBarWidget::setThemeMode(const ThemeMode mode)
{
    m_theme->setText(tr("主题：%1").arg(
        mode == ThemeMode::Dark ? tr("深色") : tr("浅色")));
}

QString StatusBarWidget::formatRate(const double bytesPerSecond)
{
    if (bytesPerSecond < 1024.0) {
        return tr("%1 B/s").arg(qRound64(bytesPerSecond));
    }
    if (bytesPerSecond < 1024.0 * 1024.0) {
        return tr("%1 KB/s").arg(bytesPerSecond / 1024.0, 0, 'f', 1);
    }
    if (bytesPerSecond < 1024.0 * 1024.0 * 1024.0) {
        return tr("%1 MB/s").arg(
            bytesPerSecond / (1024.0 * 1024.0), 0, 'f', 1);
    }
    return tr("%1 GB/s").arg(
        bytesPerSecond / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
}

QString StatusBarWidget::formatDataSize(const quint64 bytes)
{
    constexpr double kibibyte = 1024.0;
    constexpr double mebibyte = kibibyte * 1024.0;
    constexpr double gibibyte = mebibyte * 1024.0;
    if (bytes < 1024) return tr("%1 B").arg(bytes);
    if (bytes < static_cast<quint64>(mebibyte)) {
        return tr("%1 KB").arg(bytes / kibibyte, 0, 'f', 1);
    }
    if (bytes < static_cast<quint64>(gibibyte)) {
        return tr("%1 MB").arg(bytes / mebibyte, 0, 'f', 1);
    }
    return tr("%1 GB").arg(bytes / gibibyte, 0, 'f', 1);
}

void StatusBarWidget::refreshSourceText()
{
    const QString source = m_dataSourceName.isEmpty() ? tr("无") : m_dataSourceName;
    const QString device = m_deviceName.isEmpty() ? tr("无") : m_deviceName;
    m_source->setText(
        tr("设备：%1    数据源：%2").arg(device, source));
}

void StatusBarWidget::refreshRateText()
{
    m_rates->setText(tr("接收 %1 (%2)    发送 %3 (%4)")
                         .arg(formatRate(m_receiveRate),
                              formatDataSize(m_receiveTotal),
                              formatRate(m_transmitRate),
                              formatDataSize(m_transmitTotal)));
}
