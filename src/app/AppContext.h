#pragma once

#include <QObject>
#include <QString>

#include "models/AppTypes.h"

class AppSettings;
class ChannelDataHub;
class ConnectionManager;
class CescFirmwareUploader;
class CescSession;
class CescSystemClient;
class CescSensorClient;
class CescTelemetryClient;
class IconManager;
class ProtocolRepository;
class ReceiveDataPipeline;
class ThemeManager;

class AppContext final : public QObject
{
    Q_OBJECT

public:
    explicit AppContext(QObject *parent = nullptr);
    ~AppContext() override;

    [[nodiscard]] AppSettings *settings() const noexcept;
    [[nodiscard]] ConnectionManager *connectionManager() const noexcept;
    [[nodiscard]] CescFirmwareUploader *cescFirmwareUploader() const noexcept;
    [[nodiscard]] CescSession *cescSession() const noexcept;
    [[nodiscard]] CescSystemClient *cescSystemClient() const noexcept;
    [[nodiscard]] CescSensorClient *cescSensorClient() const noexcept;
    [[nodiscard]] CescTelemetryClient *cescTelemetryClient() const noexcept;
    [[nodiscard]] ThemeManager *themeManager() const noexcept;
    [[nodiscard]] IconManager *iconManager() const noexcept;
    [[nodiscard]] ProtocolRepository *protocolRepository() const noexcept;
    [[nodiscard]] ChannelDataHub *channelDataHub() const noexcept;
    [[nodiscard]] ReceiveDataPipeline *receiveDataPipeline() const noexcept;

    void notify(const QString &message,
                NotificationType type = NotificationType::Information);

signals:
    void notificationRequested(const QString &message, NotificationType type);

private:
    AppSettings *m_settings{};
    ConnectionManager *m_connectionManager{};
    CescSession *m_cescSession{};
    CescSystemClient *m_cescSystemClient{};
    CescSensorClient *m_cescSensorClient{};
    CescTelemetryClient *m_cescTelemetryClient{};
    CescFirmwareUploader *m_cescFirmwareUploader{};
    ProtocolRepository *m_protocolRepository{};
    ChannelDataHub *m_channelDataHub{};
    ReceiveDataPipeline *m_receiveDataPipeline{};
    ThemeManager *m_themeManager{};
    IconManager *m_iconManager{};
};
