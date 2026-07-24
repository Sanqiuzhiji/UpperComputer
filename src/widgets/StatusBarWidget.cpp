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
    return tr("%1 MB/s").arg(
        bytesPerSecond / (1024.0 * 1024.0), 0, 'f', 1);
}

void StatusBarWidget::refreshSourceText()
{
    const QString source = m_dataSourceName.isEmpty()
        ? tr("虚拟数据") : m_dataSourceName;
    if (m_deviceName.isEmpty()) {
        m_source->setText(tr("数据源：%1").arg(source));
        return;
    }
    m_source->setText(
        tr("设备：%1    数据源：%2").arg(m_deviceName, source));
}

void StatusBarWidget::refreshRateText()
{
    m_rates->setText(tr("接收 %1    发送 %2")
                         .arg(formatRate(m_receiveRate),
                              formatRate(m_transmitRate)));
}
