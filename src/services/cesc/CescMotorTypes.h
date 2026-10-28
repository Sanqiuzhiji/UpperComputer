#pragma once

#include "services/cesc/CescProtocolTypes.h"
#include <limits>

namespace Cesc {
enum class BoardTemperatureStatus : quint8 { Uninitialized=0, Ok=1, NtcOpen=2, NtcShort=3, Invalid=4, Overtemperature=5 };

struct BoardTemperature {
    bool supported{}, flagValid{};
    quint16 raw{};
    qint16 deciC{std::numeric_limits<qint16>::min()};
    BoardTemperatureStatus status{BoardTemperatureStatus::Uninitialized};
    quint8 thermalFaults{};
    qint32 currentLimitMa{};
    [[nodiscard]] bool hasValue() const noexcept { return supported && flagValid && deciC!=std::numeric_limits<qint16>::min() && (status==BoardTemperatureStatus::Ok || status==BoardTemperatureStatus::Overtemperature); }
    [[nodiscard]] bool sensorFault() const noexcept { return supported && (!flagValid || status==BoardTemperatureStatus::Uninitialized || status==BoardTemperatureStatus::NtcOpen || status==BoardTemperatureStatus::NtcShort || status==BoardTemperatureStatus::Invalid || (thermalFaults&1)); }
    [[nodiscard]] bool thermalFault() const noexcept { return supported && (thermalFaults!=0 || status==BoardTemperatureStatus::Overtemperature); }
};

struct MotorState {
    quint8 powerState{}, flags{}, controlMode{}, speedCurrentFoc{};
    quint16 drvFaults{}, predictedElectricalRaw{};
    quint32 busVoltageMv{}, encoderAgeMs{}, timeoutRemainingMs{};
    qint32 speedTargetMdps{}, speedReferenceMdps{}, speedActualMdps{}, idMa{}, iqMa{}, iqTargetMa{};
    qint16 predictionErrorRaw{};
    BoardTemperature boardTemperature;
};

inline bool parseBoardTemperatureTail(const QByteArray&data,qsizetype offset,bool flagValid,BoardTemperature&out){
    if(offset<0||offset+10>data.size())return false; BoardTemperature t;t.supported=true;t.flagValid=flagValid;quint8 status{};qsizetype p=offset;
    if(!readU16(data,p,t.raw)||!readI16(data,p,t.deciC)||!readU8(data,p,status)||!readU8(data,p,t.thermalFaults)||!readI32(data,p,t.currentLimitMa))return false;
    t.status=BoardTemperatureStatus(status);out=t;return true;
}
inline bool parseMotorState(const QByteArray&data,MotorState&s){
    if(data.size()<96)return false;qsizetype p=0;
    if(!readU8(data,p,s.powerState)||!readU8(data,p,s.flags)||!readU16(data,p,s.drvFaults)||!readU32(data,p,s.busVoltageMv)||!readU8(data,p,s.controlMode)||!readU8(data,p,s.speedCurrentFoc)||!readI32(data,p,s.speedTargetMdps)||!readI32(data,p,s.speedReferenceMdps)||!readI32(data,p,s.speedActualMdps)||!readI32(data,p,s.idMa)||!readI32(data,p,s.iqMa)||!readI32(data,p,s.iqTargetMa)||!readU16(data,p,s.predictedElectricalRaw)||!readU32(data,p,s.encoderAgeMs)||!readI16(data,p,s.predictionErrorRaw)||!readU32(data,p,s.timeoutRemainingMs))return false;
    s.boardTemperature={};return data.size()<106||parseBoardTemperatureTail(data,96,(s.flags&0x10)!=0,s.boardTemperature);
}
// The complete diagnostic may grow in the middle; locate this block from the response end.
inline bool parsePowerStageTemperature(const QByteArray&data,bool flagValid,BoardTemperature&out){return data.size()>=10&&parseBoardTemperatureTail(data,data.size()-10,flagValid,out);}
}
