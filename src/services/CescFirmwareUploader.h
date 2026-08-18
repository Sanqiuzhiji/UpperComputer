#pragma once

#include <QByteArray>
#include <QObject>

#include "models/ConnectionTypes.h"

class ConnectionManager;
class AppSettings;
class QTimer;

class CescFirmwareUploader final : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Idle, Preparing, Uploading, Rebooting, ReadingVersion,
        Completed, Failed
    };
    Q_ENUM(State)

    explicit CescFirmwareUploader(ConnectionManager *connection,
                                  AppSettings *settings,
                                  QObject *parent = nullptr);
    [[nodiscard]] bool isBusy() const noexcept;
    [[nodiscard]] State state() const noexcept;
    [[nodiscard]] int progress() const noexcept;
    [[nodiscard]] QString statusText() const;
    [[nodiscard]] QString firmwareVersion() const;

public slots:
    void start(const QByteArray &firmware);
    void cancel();
    void requestFirmwareVersion();

signals:
    void progressChanged(int percent, const QString &status);
    void finished(bool success, const QString &message);
    void stateChanged(CescFirmwareUploader::State state, int percent,
                      const QString &status);
    void firmwareVersionChanged(const QString &version);

private:
    enum class Stage {
        Idle, Erasing, Writing, WaitingForPort, Connecting, ReadingVersion
    };

    void sendErase();
    void sendCurrentChunk();
    void handlePacket(const QByteArray &packet);
    void processReceived(const QByteArray &data);
    void handleTimeout();
    void beginPostUploadRecovery();
    void tryReconnect();
    void sendFirmwareVersionRequest(bool postUpload);
    void handleFirmwareVersion(const QByteArray &packet);
    [[nodiscard]] QString findReappearedPort() const;
    void complete(bool success, const QString &message);
    void updateState(State state, int percent, const QString &status);
    [[nodiscard]] QByteArray frame(const QByteArray &payload) const;
    [[nodiscard]] static quint16 crc16(const QByteArray &data);
    static void appendUint32(QByteArray &data, quint32 value);

    ConnectionManager *m_connection{};
    AppSettings *m_settings{};
    QTimer *m_timeout{};
    QTimer *m_reconnectTimer{};
    Stage m_stage{Stage::Idle};
    QByteArray m_image;
    QByteArray m_rxBuffer;
    qsizetype m_offset{};
    qsizetype m_chunkLength{};
    int m_attempt{};
    State m_publicState{State::Idle};
    int m_progress{};
    QString m_statusText;
    QString m_firmwareVersion;
    SerialConfig m_reconnectConfig;
    QString m_usbSerialNumber;
    quint16 m_usbVendorId{};
    quint16 m_usbProductId{};
    int m_reconnectAttempts{};
    bool m_postUploadVersionRequest{};
};
