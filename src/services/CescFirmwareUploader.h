#pragma once

#include <QByteArray>
#include <QObject>

class ConnectionManager;
class QTimer;

class CescFirmwareUploader final : public QObject
{
    Q_OBJECT

public:
    explicit CescFirmwareUploader(ConnectionManager *connection,
                                  QObject *parent = nullptr);
    [[nodiscard]] bool isBusy() const noexcept;

public slots:
    void start(const QByteArray &firmware);
    void cancel();

signals:
    void progressChanged(int percent, const QString &status);
    void finished(bool success, const QString &message);

private:
    enum class Stage { Idle, Erasing, Writing };

    void sendErase();
    void sendCurrentChunk();
    void handlePacket(const QByteArray &packet);
    void processReceived(const QByteArray &data);
    void handleTimeout();
    void complete(bool success, const QString &message);
    [[nodiscard]] QByteArray frame(const QByteArray &payload) const;
    [[nodiscard]] static quint16 crc16(const QByteArray &data);
    static void appendUint32(QByteArray &data, quint32 value);

    ConnectionManager *m_connection{};
    QTimer *m_timeout{};
    Stage m_stage{Stage::Idle};
    QByteArray m_image;
    QByteArray m_rxBuffer;
    qsizetype m_offset{};
    qsizetype m_chunkLength{};
    int m_attempt{};
};
