#pragma once

#include <QWidget>

#include "models/AppTypes.h"

class QLabel;

class StatusBarWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit StatusBarWidget(QWidget *parent = nullptr);

public slots:
    void setConnectionState(ConnectionState state);
    void setDeviceName(const QString &name);
    void setDataSourceName(const QString &name);
    void setReceiveRate(double bytesPerSecond);
    void setTransmitRate(double bytesPerSecond);
    void setReceiveTotal(quint64 bytes);
    void setTransmitTotal(quint64 bytes);
    void setCurrentPageTitle(const QString &title);
    void setThemeMode(ThemeMode mode);

private:
    [[nodiscard]] static QString formatRate(double bytesPerSecond);
    [[nodiscard]] static QString formatDataSize(quint64 bytes);
    void refreshSourceText();
    void refreshRateText();

    QLabel *m_connection{};
    QLabel *m_source{};
    QLabel *m_rates{};
    QLabel *m_page{};
    QLabel *m_theme{};
    QString m_deviceName;
    QString m_dataSourceName;
    double m_receiveRate{};
    double m_transmitRate{};
    quint64 m_receiveTotal{};
    quint64 m_transmitTotal{};
};
