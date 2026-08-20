#pragma once
#include <QByteArray>
#include <QObject>
#include "models/ConnectionTypes.h"
#include "services/cesc/CescProtocolTypes.h"
class ConnectionManager; class AppSettings; class CescSession; class QTimer;
class CescFirmwareUploader final : public QObject {
 Q_OBJECT
public:
 enum class State { Idle,Preparing,Uploading,Verifying,Rebooting,ReadingVersion,Completed,Failed }; Q_ENUM(State)
 CescFirmwareUploader(ConnectionManager*,AppSettings*,CescSession*,QObject *parent=nullptr);
 bool isBusy()const noexcept; State state()const noexcept; int progress()const noexcept; QString statusText()const; QString firmwareVersion()const;
public slots: void start(const QByteArray&); void cancel(); void requestFirmwareVersion();
signals: void progressChanged(int,const QString&); void finished(bool,const QString&); void stateChanged(CescFirmwareUploader::State,int,const QString&); void firmwareVersionChanged(const QString&);
private:
 enum class Stage { Idle,Stop,Begin,Write,Finish,Activate,WaitPort,Connecting,Info };
 void response(quint8,quint8,Cesc::Status,const QByteArray&); void begin(); void write(); void finishImage(); void activate(); void recover(); void reconnect(); QString findPort()const; void done(bool,const QString&); void update(State,int,const QString&); static quint32 crc32(const QByteArray&);
 ConnectionManager*m_connection{}; AppSettings*m_settings{}; CescSession*m_session{}; QTimer*m_reconnect{}; Stage m_stage{Stage::Idle}; QByteArray m_image; quint32 m_crc{},m_updateId{},m_offset{},m_oldSession{}; quint16 m_chunk{512}; State m_state{State::Idle}; int m_progress{}; QString m_text,m_version; SerialConfig m_config; QString m_serial; quint16 m_vid{},m_pid{}; int m_attempt{};
};
