#pragma once
#include <QWidget>
#include <QString>
#include "services/cesc/CescConfigurationClient.h"
#include "services/cesc/CescMotorClient.h"
class AppContext; class CescFirmwareUploader; class QComboBox; class QDoubleSpinBox;
class QLabel; class QLineEdit; class QListWidget; class QProgressBar;
class QPushButton; class QSpinBox; class QStackedWidget; class QTimer;
class CescToolPage final:public QWidget{
 Q_OBJECT
public:explicit CescToolPage(AppContext*,QWidget *parent=nullptr);~CescToolPage()override;
private:
 QWidget* devicePage();QWidget* motorPage();QWidget* appPage();QWidget* row(const QString&,const QString&,QWidget*,QWidget*);
 void chooseFirmware();void startUpload();void queryAngle();void toggleAngleStream();
 void updateDevice();void applyFirmwarePath(const QString&,bool notify=false);bool validateFirmware(const QString&,QString*);
 void initializeServices();void availability();void motorState(const CescMotorClient::State&);void updateMotorModeUi();void sendTarget();void stop(bool toast=false);
 CescConfigurationClient::Configuration editorConfig()const;void showConfig(const CescConfigurationClient::Configuration&);void showSave(const CescConfigurationClient::SaveState&);
 AppContext*m_context{};CescFirmwareUploader*m_uploader{};QListWidget*m_nav{};QStackedWidget*m_stack{};QLabel*m_badge{},*m_power{},*m_voltage{},*m_rpm{},*m_iqNow{},*m_fault{},*m_timeout{},*m_saveText{};
 QLabel*m_port{},*m_connection{},*m_version{},*m_angle{},*m_rawAngle{},*m_sensorStatus{},*m_streamStatus{},*m_fileInfo{},*m_uploadStatus{},*m_configOperation{},*m_modeHint{},*m_currentPosition{},*m_alignmentStatus{};
 QLabel*m_boardTemperature{},*m_temperatureStatus{},*m_thermalLimit{},*m_temperatureRaw{},*m_thermalWarning{};
 QLineEdit*m_filePath{};QProgressBar*m_progress{};QPushButton*m_connectionButton{},*m_angleButton{},*m_streamButton{},*m_choose{},*m_upload{},*m_cancel{};
 QComboBox*m_mode{},*m_positionReference{};QDoubleSpinBox*m_iq{},*m_torque{},*m_speed{},*m_position{},*m_trajSpeed{},*m_accel{},*m_decel{};
 QWidget*m_iqRow{},*m_torqueRow{},*m_speedRow{},*m_positionReferenceRow{},*m_positionRow{},*m_trajSpeedRow{},*m_accelRow{},*m_decelRow{};
 QPushButton*m_run{},*m_stop{},*m_align{},*m_read{},*m_write{},*m_save{},*m_reload{},*m_default{};
 QSpinBox*m_poles{},*m_maxIq{},*m_watchdog{};QDoubleSpinBox*m_kt{},*m_resistance{},*m_inductance{},*m_flux{},*m_maxSpeed{},*m_maxPosition{},*m_minVoltage{},*m_maxVoltage{};
 QTimer*m_commandTimer{},*m_stateTimer{},*m_saveTimer{},*m_alignmentTimer{};QString m_pendingConfigOperation;double m_latestPositionTurns{},m_lockedPositionTargetTurns{};qint64 m_alignmentStartedMs{};quint16 m_angleStreamId{};bool m_running{},m_firmwareValid{},m_configurationAvailable{},m_positionValid{},m_alignmentPending{},m_alignmentSawCalibration{},m_motorStartAllowed{true},m_safetyStopSent{};
};
