#pragma once

#include <QJsonObject>
#include <QWidget>

#include "models/AppTypes.h"
#include "models/ProtocolTypes.h"

class QAction;
class AppContext;
class FieldLibraryPanel;
class ProtocolCanvas;
class ProtocolPropertyPanel;
class ProtocolRepository;
class QComboBox;
class QToolButton;
class QUndoStack;

class ProtocolEditorPage final : public QWidget
{
    Q_OBJECT

public:
    explicit ProtocolEditorPage(
        AppContext *context, QWidget *parent = nullptr);
    ~ProtocolEditorPage() override;

    [[nodiscard]] const ProtocolModel::Document &document() const;
    [[nodiscard]] bool hasUnsavedChanges() const;

private:
    enum class ClipboardKind {
        None,
        Field,
        Frame
    };

    void createUi();
    void createActions();
    QToolButton *addToolButton(
        QAction *action, const QString &objectName);
    void refreshIcons();
    void refreshProtocolCombo();
    void refreshEditor();
    void refreshSelection();
    void updateActionStates();
    void setDocument(
        const ProtocolModel::Document &document,
        const QString &filePath,
        bool temporary);
    void applyDocumentChange(
        const ProtocolModel::Document &after,
        const QString &description,
        const QUuid &selectedFrameId = {},
        const QUuid &selectedFieldId = {});
    void applySnapshot(
        const ProtocolModel::Document &snapshot,
        const QUuid &selectedFrameId,
        const QUuid &selectedFieldId);

    [[nodiscard]] bool maybeSaveChanges();
    [[nodiscard]] bool saveDocument(bool saveAs);
    [[nodiscard]] bool validateBeforeSave();
    void selectIssue(const ProtocolModel::ValidationIssue &issue);
    void newTemporaryDocument();
    void switchProtocol(int comboIndex);
    void rescanProtocols();
    void importProtocol();
    void deleteSelection();
    void deleteFrame(const QUuid &frameId);
    void deleteField(
        const QUuid &frameId, const QUuid &fieldId);
    void addFrame();
    void copySelection();
    void pasteSelection();
    void duplicateSelection();

    void selectDocument();
    void selectFrame(const QUuid &frameId);
    void selectField(const QUuid &frameId, const QUuid &fieldId);
    void addFieldTemplate(
        ProtocolModel::FieldRole role,
        const QUuid &targetFrameId,
        int targetIndex);
    void moveField(
        const QUuid &sourceFrameId,
        const QUuid &fieldId,
        const QUuid &targetFrameId,
        int targetIndex);
    void moveFrame(const QUuid &frameId, int targetIndex);
    void updateDocumentName(const QString &name);
    void updateFrame(const ProtocolModel::Frame &frame);
    void updateField(
        const QUuid &frameId, const ProtocolModel::Field &field);

    [[nodiscard]] int frameIndex(const QUuid &frameId) const;
    [[nodiscard]] int fieldIndex(
        int frameIndex, const QUuid &fieldId) const;
    [[nodiscard]] QString uniqueFrameName(
        const QString &base,
        const ProtocolModel::Document &document) const;
    [[nodiscard]] QString uniqueFieldName(
        const QString &base,
        const ProtocolModel::Frame &frame) const;
    void notify(
        const QString &message, NotificationType type) const;

    AppContext *m_context{};
    ProtocolRepository *m_repository{};
    QComboBox *m_protocolCombo{};
    FieldLibraryPanel *m_libraryPanel{};
    ProtocolCanvas *m_canvas{};
    ProtocolPropertyPanel *m_propertyPanel{};
    QUndoStack *m_undoStack{};

    QAction *m_rescanAction{};
    QAction *m_newDocumentAction{};
    QAction *m_addFrameAction{};
    QAction *m_undoAction{};
    QAction *m_redoAction{};
    QAction *m_copyAction{};
    QAction *m_pasteAction{};
    QAction *m_deleteAction{};
    QAction *m_importAction{};
    QAction *m_saveAction{};
    QAction *m_saveAsAction{};
    QAction *m_duplicateAction{};

    ProtocolModel::Document m_document;
    QVector<ProtocolModel::ValidationIssue> m_issues;
    QString m_filePath;
    bool m_temporary{true};
    bool m_forceDirty{true};
    bool m_refreshingCombo{};
    int m_untitledCounter{};
    QUuid m_selectedFrameId;
    QUuid m_selectedFieldId;
    ClipboardKind m_clipboardKind{ClipboardKind::None};
    ProtocolModel::Field m_copiedField;
    ProtocolModel::Frame m_copiedFrame;
};
