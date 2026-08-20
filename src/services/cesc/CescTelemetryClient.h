#pragma once
#include "services/cesc/CescProtocolTypes.h"
#include <QHash>
#include <QObject>
class CescSession; class ChannelDataHub;
class CescTelemetryClient final:public QObject{
 Q_OBJECT
public:
 struct Channel{quint16 id{};Cesc::DataType type{};float scale{1},offset{};QString name,unit;};
 explicit CescTelemetryClient(CescSession*,ChannelDataHub*,QObject *parent=nullptr);
 void enumerate(quint16 first=0,quint8 count=32);void subscribe(const QList<quint16>&,quint32 periodUs=10000,quint8 samples=1);void unsubscribe(quint16);void stopAll();void getStreamStatus(quint16);
 quint16 activeStreamId()const noexcept;
 quint64 droppedFrames()const noexcept{return m_droppedFrames;}quint64 droppedSamples()const noexcept{return m_droppedSamples;}
signals:void channelsChanged();void subscribed(quint16);void streamGapDetected(quint64,quint64);void commandFailed(const QString&);
private:
 struct Stream{QList<quint16> channels;quint16 lastFrame{};quint32 nextSample{};bool seen{};};
 bool value(const QByteArray&,qsizetype&,Cesc::DataType,double&)const;void stream(const Cesc::Packet&);
 CescSession*m_session{};ChannelDataHub*m_hub{};QHash<quint16,Channel>m_channels;QHash<quint16,Stream>m_streams;quint64 m_droppedFrames{},m_droppedSamples{};qint64 m_deviceToHostOffsetUs{};bool m_timestampAligned{};
};
