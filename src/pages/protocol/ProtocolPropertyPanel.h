#pragma once

#include <QFrame>

#include "models/ProtocolTypes.h"

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QSpinBox;
class QStackedWidget;

class ProtocolPropertyPanel final : public QFrame
{
    Q_OBJECT

public:
    explicit ProtocolPropertyPanel(QWidget *parent = nullptr);

    void showDocument(
        const ProtocolModel::Document &document,
        const QVector<ProtocolModel::ValidationIssue> &issues);
    void showFrame(
        const ProtocolModel::Document &document,
        const ProtocolModel::Frame &frame,
        const QVector<ProtocolModel::ValidationIssue> &issues);
    void showField(
        const ProtocolModel::Frame &frame,
        const ProtocolModel::Field &field,
        const QVector<ProtocolModel::ValidationIssue> &issues);

signals:
    void documentNameEdited(const QString &name);
    void frameEdited(const ProtocolModel::Frame &frame);
    void fieldEdited(
        const QUuid &frameId, const ProtocolModel::Field &field);

private:
    QWidget *createDocumentPage();
    QWidget *createFramePage();
    QWidget *createFieldPage();
    void connectEditors();
    void updateFieldVisibility();
    void submitFrame();
    void submitField();
    void setInvalid(QWidget *widget, bool invalid);
    void updateHexValidation();
    [[nodiscard]] QStringList messagesFor(
        const QVector<ProtocolModel::ValidationIssue> &issues,
        const QUuid &frameId,
        const QUuid &fieldId = {}) const;

    QStackedWidget *m_stack{};

    QLineEdit *m_documentName{};
    QLabel *m_documentFrameCount{};
    QLabel *m_documentByteCount{};
    QLabel *m_documentErrors{};

    QLineEdit *m_frameName{};
    QLineEdit *m_frameId{};
    QComboBox *m_frameDirection{};
    QLabel *m_frameFieldCount{};
    QLabel *m_frameByteCount{};
    QLabel *m_frameErrors{};

    QLineEdit *m_fieldName{};
    QComboBox *m_fieldRole{};
    QSpinBox *m_byteCount{};
    QWidget *m_typeRow{};
    QComboBox *m_dataType{};
    QWidget *m_byteOrderRow{};
    QComboBox *m_byteOrder{};
    QWidget *m_scaleRow{};
    QDoubleSpinBox *m_scale{};
    QWidget *m_offsetRow{};
    QDoubleSpinBox *m_offset{};
    QWidget *m_fixedHexRow{};
    QLineEdit *m_fixedHex{};
    QLabel *m_fixedHexError{};
    QWidget *m_checksumRow{};
    QComboBox *m_checksum{};
    QLabel *m_fieldErrors{};

    bool m_updating{};
    QUuid m_currentFrameId;
    ProtocolModel::Frame m_currentFrame;
    ProtocolModel::Field m_currentField;
};
