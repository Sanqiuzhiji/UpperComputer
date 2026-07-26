#include "ProtocolEditorPage.h"

#include "app/AppContext.h"
#include "pages/protocol/FieldLibraryPanel.h"
#include "pages/protocol/ProtocolCanvas.h"
#include "pages/protocol/ProtocolPropertyPanel.h"
#include "services/ProtocolRepository.h"
#include "theme/IconManager.h"

#include <QAction>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QSignalBlocker>
#include <QSize>
#include <QSplitter>
#include <QToolButton>
#include <QTimer>
#include <QUndoCommand>
#include <QUndoStack>
#include <QVBoxLayout>

#include <functional>
#include <utility>

using namespace ProtocolModel;

namespace {

class DocumentSnapshotCommand final : public QUndoCommand
{
public:
    using Apply = std::function<void(
        const Document &, const QUuid &, const QUuid &)>;

    DocumentSnapshotCommand(
        const Document &before,
        const Document &after,
        const QUuid &beforeFrame,
        const QUuid &beforeField,
        const QUuid &afterFrame,
        const QUuid &afterField,
        const QString &description,
        Apply apply)
        : QUndoCommand(description),
          m_before(before),
          m_after(after),
          m_beforeFrame(beforeFrame),
          m_beforeField(beforeField),
          m_afterFrame(afterFrame),
          m_afterField(afterField),
          m_apply(std::move(apply))
    {
    }

    void undo() override
    {
        m_apply(m_before, m_beforeFrame, m_beforeField);
    }

    void redo() override
    {
        m_apply(m_after, m_afterFrame, m_afterField);
    }

private:
    Document m_before;
    Document m_after;
    QUuid m_beforeFrame;
    QUuid m_beforeField;
    QUuid m_afterFrame;
    QUuid m_afterField;
    Apply m_apply;
};

QString normalizedId(const QUuid &id)
{
    return id.toString(QUuid::WithoutBraces);
}

QString copiedName(const QString &name)
{
    return name + QStringLiteral("_copy");
}

QString workspaceNameFromPath(const QString &filePath)
{
    QString fileName = QFileInfo(filePath).fileName();
    constexpr auto workspaceSuffix = ".ucproto.json";
    if (fileName.endsWith(
            QLatin1String(workspaceSuffix),
            Qt::CaseInsensitive)) {
        fileName.chop(
            static_cast<int>(
                QLatin1String(workspaceSuffix).size()));
    }
    return fileName;
}

} // namespace

ProtocolEditorPage::ProtocolEditorPage(
    AppContext *context, QWidget *parent)
    : QWidget(parent),
      m_context(context),
      m_repository(context->protocolRepository()),
      m_undoStack(new QUndoStack(this))
{
    setObjectName(QStringLiteral("protocolEditorPage"));
    createActions();
    createUi();

    connect(m_repository, &ProtocolRepository::protocolLibraryChanged,
            this, &ProtocolEditorPage::refreshProtocolCombo);
    connect(m_undoStack, &QUndoStack::cleanChanged,
            this, [this] {
                refreshProtocolCombo();
                updateActionStates();
            });
    connect(m_undoStack, &QUndoStack::canUndoChanged,
            this, &ProtocolEditorPage::updateActionStates);
    connect(m_undoStack, &QUndoStack::canRedoChanged,
            this, &ProtocolEditorPage::updateActionStates);
    connect(m_context->iconManager(), &IconManager::iconsChanged,
            this, &ProtocolEditorPage::refreshIcons);

    const QList<ProtocolSummary> workspaces =
        m_repository->availableProtocols();
    if (workspaces.isEmpty()) {
        newTemporaryDocument();
    } else if (const auto document =
                   m_repository->protocolById(
                       workspaces.constFirst().id)) {
        setDocument(
            *document, workspaces.constFirst().filePath, false);
    }
}

ProtocolEditorPage::~ProtocolEditorPage()
{
    // QUndoStack clears its commands while QObject is tearing down children.
    // Disconnect before the derived page type is gone, otherwise its state
    // signals could target ProtocolEditorPage slots during base destruction.
    disconnect(m_undoStack, nullptr, this, nullptr);
}

const Document &ProtocolEditorPage::document() const
{
    return m_document;
}

bool ProtocolEditorPage::hasUnsavedChanges() const
{
    return m_forceDirty || !m_undoStack->isClean();
}

void ProtocolEditorPage::createUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(10);

    auto *topBar = new QFrame(this);
    topBar->setObjectName(QStringLiteral("protocolTopBar"));
    topBar->setProperty("card", true);
    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(10, 7, 10, 7);
    topLayout->setSpacing(4);
    auto *label = new QLabel(QStringLiteral("工作空间"), topBar);
    label->setProperty("muted", true);
    topLayout->addWidget(label);
    m_protocolCombo = new QComboBox(topBar);
    m_protocolCombo->setObjectName(QStringLiteral("protocolFileCombo"));
    m_protocolCombo->setMinimumWidth(210);
    m_protocolCombo->setSizeAdjustPolicy(
        QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_protocolCombo->setMinimumContentsLength(22);
    topLayout->addWidget(m_protocolCombo, 1);
    addToolButton(
        m_deleteWorkspaceAction,
        QStringLiteral("workspaceDeleteButton"));
    topLayout->addSpacing(4);

    addToolButton(m_rescanAction, QStringLiteral("protocolRescanButton"));
    addToolButton(m_newDocumentAction, QStringLiteral("protocolNewButton"));
    addToolButton(m_addFrameAction, QStringLiteral("protocolAddFrameButton"));
    topLayout->addSpacing(6);
    addToolButton(m_undoAction, QStringLiteral("protocolUndoButton"));
    addToolButton(m_redoAction, QStringLiteral("protocolRedoButton"));
    addToolButton(m_copyAction, QStringLiteral("protocolCopyButton"));
    addToolButton(m_pasteAction, QStringLiteral("protocolPasteButton"));
    topLayout->addSpacing(6);
    addToolButton(m_importAction, QStringLiteral("protocolImportButton"));
    addToolButton(m_saveAction, QStringLiteral("protocolSaveButton"));
    addToolButton(m_saveAsAction, QStringLiteral("protocolSaveAsButton"));
    layout->addWidget(topBar);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName(QStringLiteral("protocolEditorSplitter"));
    splitter->setChildrenCollapsible(false);
    m_libraryPanel = new FieldLibraryPanel(splitter);
    m_canvas = new ProtocolCanvas(splitter);
    m_propertyPanel = new ProtocolPropertyPanel(splitter);
    splitter->addWidget(m_libraryPanel);
    splitter->addWidget(m_canvas);
    splitter->addWidget(m_propertyPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    splitter->setSizes({200, 760, 290});
    layout->addWidget(splitter, 1);

    connect(m_protocolCombo, &QComboBox::currentIndexChanged,
            this, &ProtocolEditorPage::switchProtocol);
    connect(m_canvas, &ProtocolCanvas::documentSelected,
            this, &ProtocolEditorPage::selectDocument);
    connect(m_canvas, &ProtocolCanvas::frameSelected,
            this, &ProtocolEditorPage::selectFrame);
    connect(m_canvas, &ProtocolCanvas::frameDeleteRequested,
            this, &ProtocolEditorPage::deleteFrame);
    connect(m_canvas, &ProtocolCanvas::fieldSelected,
            this, &ProtocolEditorPage::selectField);
    connect(m_canvas, &ProtocolCanvas::fieldDeleteRequested,
            this, &ProtocolEditorPage::deleteField);
    connect(m_canvas, &ProtocolCanvas::addFrameRequested,
            this, &ProtocolEditorPage::addFrame);
    connect(m_canvas, &ProtocolCanvas::fieldTemplateDropped,
            this, &ProtocolEditorPage::addFieldTemplate);
    connect(m_canvas, &ProtocolCanvas::fieldMoveRequested,
            this, &ProtocolEditorPage::moveField);
    connect(m_canvas, &ProtocolCanvas::frameMoveRequested,
            this, &ProtocolEditorPage::moveFrame);
    connect(m_propertyPanel, &ProtocolPropertyPanel::documentNameEdited,
            this, &ProtocolEditorPage::updateDocumentName);
    connect(m_propertyPanel, &ProtocolPropertyPanel::frameEdited,
            this, &ProtocolEditorPage::updateFrame);
    connect(m_propertyPanel, &ProtocolPropertyPanel::fieldEdited,
            this, &ProtocolEditorPage::updateField);
}

void ProtocolEditorPage::createActions()
{
    const auto action = [this](
        const QString &text,
        const QString &tooltip,
        const QKeySequence &shortcut) {
        auto *result = new QAction(text, this);
        result->setToolTip(tooltip);
        result->setShortcut(shortcut);
        result->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        addAction(result);
        return result;
    };

    m_rescanAction = action(
        QStringLiteral("重新扫描"), QStringLiteral("重新扫描协议目录"),
        QKeySequence(QStringLiteral("F5")));
    m_newDocumentAction = action(
        QStringLiteral("新建"), QStringLiteral("新建临时协议 (Ctrl+N)"),
        QKeySequence::New);
    m_addFrameAction = action(
        QStringLiteral("新建帧"), QStringLiteral("新建协议帧 (Ctrl+Shift+N)"),
        QKeySequence(QStringLiteral("Ctrl+Shift+N")));
    m_undoAction = action(
        QStringLiteral("撤销"), QStringLiteral("撤销 (Ctrl+Z)"),
        QKeySequence::Undo);
    m_redoAction = action(
        QStringLiteral("重做"), QStringLiteral("重做 (Ctrl+Y)"),
        QKeySequence::Redo);
    m_copyAction = action(
        QStringLiteral("复制"), QStringLiteral("复制字段或协议帧 (Ctrl+C)"),
        QKeySequence::Copy);
    m_pasteAction = action(
        QStringLiteral("粘贴"), QStringLiteral("粘贴 (Ctrl+V)"),
        QKeySequence::Paste);
    m_deleteWorkspaceAction = action(
        QStringLiteral("删除工作空间"),
        QStringLiteral("删除当前工作空间文件"),
        {});
    m_importAction = action(
        QStringLiteral("打开"), QStringLiteral("打开工作空间文件 (Ctrl+O)"),
        QKeySequence::Open);
    m_saveAction = action(
        QStringLiteral("保存"), QStringLiteral("保存 (Ctrl+S)"),
        QKeySequence::Save);
    m_saveAsAction = action(
        QStringLiteral("另存为"),
        QStringLiteral("另存为 (Ctrl+Shift+S)"),
        QKeySequence::SaveAs);
    m_duplicateAction = action(
        QStringLiteral("创建副本"),
        QStringLiteral("复制当前字段或协议帧 (Ctrl+D)"),
        QKeySequence(QStringLiteral("Ctrl+D")));

    connect(m_rescanAction, &QAction::triggered,
            this, &ProtocolEditorPage::rescanProtocols);
    connect(m_newDocumentAction, &QAction::triggered, this, [this] {
        if (maybeSaveChanges()) newTemporaryDocument();
    });
    connect(m_addFrameAction, &QAction::triggered,
            this, &ProtocolEditorPage::addFrame);
    connect(m_undoAction, &QAction::triggered,
            m_undoStack, &QUndoStack::undo);
    connect(m_redoAction, &QAction::triggered,
            m_undoStack, &QUndoStack::redo);
    connect(m_copyAction, &QAction::triggered,
            this, &ProtocolEditorPage::copySelection);
    connect(m_pasteAction, &QAction::triggered,
            this, &ProtocolEditorPage::pasteSelection);
    connect(m_deleteWorkspaceAction, &QAction::triggered,
            this, &ProtocolEditorPage::deleteCurrentWorkspace);
    connect(m_importAction, &QAction::triggered,
            this, &ProtocolEditorPage::importProtocol);
    connect(m_saveAction, &QAction::triggered, this, [this] {
        (void)saveDocument(false);
    });
    connect(m_saveAsAction, &QAction::triggered, this, [this] {
        (void)saveDocument(true);
    });
    connect(m_duplicateAction, &QAction::triggered,
            this, &ProtocolEditorPage::duplicateSelection);
    refreshIcons();
}

QToolButton *ProtocolEditorPage::addToolButton(
    QAction *action, const QString &objectName)
{
    auto *topBar = findChild<QFrame *>(
        QStringLiteral("protocolTopBar"));
    QWidget *buttonParent = topBar
        ? static_cast<QWidget *>(topBar)
        : static_cast<QWidget *>(this);
    auto *button = new QToolButton(buttonParent);
    button->setObjectName(objectName);
    button->setDefaultAction(action);
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setAutoRaise(true);
    button->setIconSize(QSize(24, 24));
    button->setFixedSize(40, 40);
    if (topBar) {
        if (auto *layout =
                qobject_cast<QHBoxLayout *>(topBar->layout())) {
            layout->addWidget(button);
        }
    }
    return button;
}

void ProtocolEditorPage::refreshIcons()
{
    IconManager *icons = m_context->iconManager();
    if (!icons) return;
    const auto set = [icons](QAction *action, const QString &path) {
        if (action) action->setIcon(icons->icon(path));
    };
    set(m_rescanAction, QStringLiteral(":/icons/protocol/refresh.svg"));
    set(m_newDocumentAction, QStringLiteral(":/icons/protocol/new.svg"));
    set(m_addFrameAction, QStringLiteral(":/icons/protocol/add_frame.svg"));
    set(m_undoAction, QStringLiteral(":/icons/protocol/undo.svg"));
    set(m_redoAction, QStringLiteral(":/icons/protocol/redo.svg"));
    set(m_copyAction, QStringLiteral(":/icons/protocol/copy.svg"));
    set(m_pasteAction, QStringLiteral(":/icons/protocol/paste.svg"));
    set(m_deleteWorkspaceAction,
        QStringLiteral(":/icons/protocol/delete.svg"));
    set(m_importAction, QStringLiteral(":/icons/protocol/import.svg"));
    set(m_saveAction, QStringLiteral(":/icons/protocol/save.svg"));
    set(m_saveAsAction, QStringLiteral(":/icons/protocol/save_as.svg"));
}

void ProtocolEditorPage::refreshProtocolCombo()
{
    if (!m_protocolCombo) return;
    m_refreshingCombo = true;
    const QSignalBlocker blocker(m_protocolCombo);
    m_protocolCombo->clear();
    const QString currentId = normalizedId(m_document.id);
    bool currentAdded = false;
    const QList<ProtocolSummary> summaries =
        m_repository->availableProtocols();
    for (const ProtocolSummary &summary : summaries) {
        QString name = summary.displayName;
        if (summary.id == currentId) {
            if (hasUnsavedChanges()) name += QStringLiteral(" *");
            currentAdded = true;
        }
        m_protocolCombo->addItem(name, summary.id);
        m_protocolCombo->setItemData(
            m_protocolCombo->count() - 1,
            summary.filePath,
            Qt::ToolTipRole);
    }
    if (!currentAdded && !m_document.id.isNull()) {
        QString name = m_document.name;
        if (hasUnsavedChanges()) name += QStringLiteral(" *");
        m_protocolCombo->insertItem(0, name, currentId);
    }
    const int index = m_protocolCombo->findData(currentId);
    if (index >= 0) m_protocolCombo->setCurrentIndex(index);
    m_refreshingCombo = false;
}

void ProtocolEditorPage::refreshEditor()
{
    m_issues = validate(m_document);
    m_canvas->setDocument(m_document, m_issues);
    refreshSelection();
    refreshProtocolCombo();
    updateActionStates();
}

void ProtocolEditorPage::refreshSelection()
{
    const int framePosition = frameIndex(m_selectedFrameId);
    if (framePosition < 0) {
        m_selectedFrameId = {};
        m_selectedFieldId = {};
        m_canvas->setSelection({}, {});
        m_propertyPanel->showDocument(m_document, m_issues);
        return;
    }
    const Frame &frame = m_document.frames.at(framePosition);
    const int fieldPosition = fieldIndex(
        framePosition, m_selectedFieldId);
    if (fieldPosition < 0) {
        m_selectedFieldId = {};
        m_canvas->setSelection(m_selectedFrameId, {});
        m_propertyPanel->showFrame(m_document, frame, m_issues);
        return;
    }
    m_canvas->setSelection(m_selectedFrameId, m_selectedFieldId);
    m_propertyPanel->showField(
        frame, frame.fields.at(fieldPosition), m_issues);
}

void ProtocolEditorPage::updateActionStates()
{
    if (!m_undoAction) return;
    m_undoAction->setEnabled(m_undoStack->canUndo());
    m_redoAction->setEnabled(m_undoStack->canRedo());
    const bool hasSelection = !m_selectedFrameId.isNull();
    m_copyAction->setEnabled(hasSelection);
    m_deleteWorkspaceAction->setEnabled(
        !m_temporary && !m_document.id.isNull());
    const bool validFieldTarget =
        m_clipboardKind == ClipboardKind::Field && hasSelection;
    const bool validFrameTarget =
        m_clipboardKind == ClipboardKind::Frame;
    m_pasteAction->setEnabled(validFieldTarget || validFrameTarget);
    m_duplicateAction->setEnabled(hasSelection);
    m_saveAction->setEnabled(
        !m_document.id.isNull() && hasUnsavedChanges());
    m_saveAsAction->setEnabled(!m_document.id.isNull());
}

void ProtocolEditorPage::setDocument(
    const Document &document,
    const QString &filePath,
    const bool temporary)
{
    resetDeleteWorkspaceConfirmation();
    m_document = document;
    m_filePath = filePath;
    m_temporary = temporary;
    m_forceDirty = temporary;
    m_selectedFrameId = {};
    m_selectedFieldId = {};
    m_undoStack->clear();
    if (!temporary) m_undoStack->setClean();
    refreshEditor();
}

void ProtocolEditorPage::applyDocumentChange(
    const Document &after,
    const QString &description,
    const QUuid &selectedFrameId,
    const QUuid &selectedFieldId)
{
    if (after == m_document) return;
    m_undoStack->push(new DocumentSnapshotCommand(
        m_document, after,
        m_selectedFrameId, m_selectedFieldId,
        selectedFrameId, selectedFieldId,
        description,
        [this](const Document &snapshot,
               const QUuid &frameId,
               const QUuid &fieldId) {
            applySnapshot(snapshot, frameId, fieldId);
        }));
}

void ProtocolEditorPage::applySnapshot(
    const Document &snapshot,
    const QUuid &selectedFrameId,
    const QUuid &selectedFieldId)
{
    m_document = snapshot;
    m_selectedFrameId = selectedFrameId;
    m_selectedFieldId = selectedFieldId;
    refreshEditor();
}

bool ProtocolEditorPage::maybeSaveChanges()
{
    return true;
}

bool ProtocolEditorPage::saveDocument(const bool saveAs)
{
    if (!validateBeforeSave()) return false;
    Document documentToSave = m_document;
    if (saveAs) {
        documentToSave.id = QUuid::createUuid();
    }
    QString target;
    if (saveAs) {
        target = QFileDialog::getSaveFileName(
            this,
            QStringLiteral("工作空间另存为"),
            QDir(m_repository->directoryPath()).filePath(
                m_document.name + QStringLiteral(".ucproto.json")),
            QStringLiteral("UpperComputer Workspace (*.ucproto.json)"));
        if (target.isEmpty()) return false;
    }
    QString savedPath;
    QString error;
    if (!m_repository->save(
            documentToSave, target, &savedPath, &error)) {
        notify(
            QStringLiteral("保存失败：%1").arg(error),
            NotificationType::Error);
        return false;
    }
    documentToSave.name =
        workspaceNameFromPath(savedPath);
    m_document = documentToSave;
    if (saveAs) {
        m_undoStack->clear();
    }
    m_filePath = savedPath;
    m_temporary = false;
    m_forceDirty = false;
    m_undoStack->setClean();
    refreshProtocolCombo();
    return true;
}

bool ProtocolEditorPage::validateBeforeSave()
{
    const QList<QWidget *> editors = findChildren<QWidget *>();
    for (QWidget *editor : editors) {
        if (!editor->property("invalid").toBool()
            || !editor->isVisibleTo(this)) {
            continue;
        }
        editor->setFocus(Qt::OtherFocusReason);
        notify(
            QStringLiteral("无法保存：请先修正属性编辑器中的错误"),
            NotificationType::Error);
        return false;
    }
    m_issues = validate(m_document);
    refreshEditor();
    for (const ValidationIssue &issue : std::as_const(m_issues)) {
        if (issue.severity != ValidationSeverity::Error) continue;
        selectIssue(issue);
        notify(
            QStringLiteral("无法保存：%1").arg(issue.message),
            NotificationType::Error);
        return false;
    }
    return true;
}

void ProtocolEditorPage::selectIssue(const ValidationIssue &issue)
{
    if (!issue.fieldId.isNull()) {
        selectField(issue.frameId, issue.fieldId);
    } else if (!issue.frameId.isNull()) {
        selectFrame(issue.frameId);
    } else {
        selectDocument();
    }
}

void ProtocolEditorPage::newTemporaryDocument()
{
    ++m_untitledCounter;
    setDocument(
        makeDocument(
            QStringLiteral("Untitled Workspace %1")
                .arg(m_untitledCounter)),
        {}, true);
}

void ProtocolEditorPage::switchProtocol(const int comboIndex)
{
    if (m_refreshingCombo || comboIndex < 0) return;
    const QString id =
        m_protocolCombo->itemData(comboIndex).toString();
    if (id == normalizedId(m_document.id)) return;
    if (!maybeSaveChanges()) {
        refreshProtocolCombo();
        return;
    }
    const auto document = m_repository->protocolById(id);
    if (!document) {
        notify(
            QStringLiteral("找不到所选协议，可能已被移动或删除"),
            NotificationType::Warning);
        refreshProtocolCombo();
        return;
    }
    setDocument(*document, m_repository->filePathForId(id), false);
}

void ProtocolEditorPage::rescanProtocols()
{
    QStringList errors;
    m_repository->rescan(&errors);
    refreshProtocolCombo();
    notify(
        errors.isEmpty()
            ? QStringLiteral("协议目录扫描完成")
            : QStringLiteral("协议目录扫描完成，%1 个文件加载失败")
                  .arg(errors.size()),
        errors.isEmpty()
            ? NotificationType::Success : NotificationType::Warning);
}

void ProtocolEditorPage::importProtocol()
{
    if (!maybeSaveChanges()) return;
    const QString source = QFileDialog::getOpenFileName(
        this, QStringLiteral("打开工作空间文件"), {},
        QStringLiteral("UpperComputer Workspace (*.ucproto.json);;"
                       "JSON (*.json)"));
    if (source.isEmpty()) return;
    Document document;
    QString error;
    if (!m_repository->openFile(source, &document, &error)) {
        notify(
            QStringLiteral("打开失败：%1").arg(error),
            NotificationType::Error);
        return;
    }
    setDocument(document, QFileInfo(source).absoluteFilePath(), false);
}

void ProtocolEditorPage::deleteCurrentWorkspace()
{
    if (m_temporary || m_document.id.isNull()) return;

    if (!m_deleteWorkspaceArmed) {
        m_deleteWorkspaceArmed = true;
        const quint64 token =
            ++m_deleteWorkspaceConfirmationToken;
        m_deleteWorkspaceAction->setText(
            QStringLiteral("确认删除"));
        m_deleteWorkspaceAction->setToolTip(
            QStringLiteral("再次点击，永久删除当前工作空间"));
        notify(
            QStringLiteral(
                "再次点击删除按钮以永久删除工作空间“%1”")
                .arg(m_document.name),
            NotificationType::Warning);
        QTimer::singleShot(5000, this, [this, token] {
            if (token == m_deleteWorkspaceConfirmationToken) {
                resetDeleteWorkspaceConfirmation();
            }
        });
        return;
    }
    resetDeleteWorkspaceConfirmation();
    QString error;
    if (!m_repository->remove(
            normalizedId(m_document.id), &error)) {
        notify(
            QStringLiteral("删除失败：%1").arg(error),
            NotificationType::Error);
        return;
    }

    const QList<ProtocolSummary> remaining =
        m_repository->availableProtocols();
    if (remaining.isEmpty()) {
        newTemporaryDocument();
        return;
    }
    const ProtocolSummary &next = remaining.constFirst();
    if (const auto document =
            m_repository->protocolById(next.id)) {
        setDocument(*document, next.filePath, false);
    } else {
        newTemporaryDocument();
    }
}

void ProtocolEditorPage::resetDeleteWorkspaceConfirmation()
{
    ++m_deleteWorkspaceConfirmationToken;
    m_deleteWorkspaceArmed = false;
    if (!m_deleteWorkspaceAction) return;
    m_deleteWorkspaceAction->setText(
        QStringLiteral("删除工作空间"));
    m_deleteWorkspaceAction->setToolTip(
        QStringLiteral("删除当前工作空间文件"));
}

void ProtocolEditorPage::deleteFrame(const QUuid &frameId)
{
    const int framePosition = frameIndex(frameId);
    if (framePosition < 0) return;
    Document after = m_document;
    after.frames.removeAt(framePosition);
    applyDocumentChange(
        after, QStringLiteral("删除协议"), {}, {});
}

void ProtocolEditorPage::deleteField(
    const QUuid &frameId, const QUuid &fieldId)
{
    const int framePosition = frameIndex(frameId);
    const int fieldPosition = fieldIndex(framePosition, fieldId);
    if (fieldPosition < 0) return;
    Document after = m_document;
    after.frames[framePosition].fields.removeAt(fieldPosition);
    applyDocumentChange(
        after, QStringLiteral("删除字段"), frameId, {});
}

void ProtocolEditorPage::addFrame()
{
    Document after = m_document;
    Frame frame = makeFrame(uniqueFrameName(
        QStringLiteral("Frame%1").arg(after.frames.size() + 1),
        after));
    after.frames.append(frame);
    applyDocumentChange(
        after, QStringLiteral("新增协议帧"), frame.id, {});
}

void ProtocolEditorPage::copySelection()
{
    const int framePosition = frameIndex(m_selectedFrameId);
    if (framePosition < 0) return;
    const int fieldPosition = fieldIndex(
        framePosition, m_selectedFieldId);
    if (fieldPosition >= 0) {
        m_copiedField =
            m_document.frames.at(framePosition).fields.at(fieldPosition);
        m_clipboardKind = ClipboardKind::Field;
        notify(QStringLiteral("字段已复制"), NotificationType::Information);
    } else {
        m_copiedFrame = m_document.frames.at(framePosition);
        m_clipboardKind = ClipboardKind::Frame;
        notify(
            QStringLiteral("协议帧已复制"), NotificationType::Information);
    }
    updateActionStates();
}

void ProtocolEditorPage::pasteSelection()
{
    if (m_clipboardKind == ClipboardKind::Field) {
        const int targetFrame = frameIndex(m_selectedFrameId);
        if (targetFrame < 0) {
            notify(
                QStringLiteral("请先选择目标协议帧"),
                NotificationType::Warning);
            return;
        }
        Document after = m_document;
        Frame &frame = after.frames[targetFrame];
        Field copy = duplicatedField(m_copiedField);
        copy.name = uniqueFieldName(copiedName(copy.name), frame);
        const int selectedField =
            fieldIndex(targetFrame, m_selectedFieldId);
        const int insertAt = selectedField >= 0
            ? selectedField + 1 : frame.fields.size();
        frame.fields.insert(insertAt, copy);
        applyDocumentChange(
            after, QStringLiteral("粘贴字段"), frame.id, copy.id);
        return;
    }
    if (m_clipboardKind == ClipboardKind::Frame) {
        Document after = m_document;
        Frame copy = duplicatedFrame(m_copiedFrame);
        copy.name = uniqueFrameName(copiedName(copy.name), after);
        for (int index = 0; index < copy.fields.size(); ++index) {
            copy.fields[index].name = uniqueFieldName(
                copy.fields.at(index).name,
                Frame{copy.id, copy.name, copy.direction,
                      copy.fields.mid(0, index)});
        }
        const int selectedFrame = frameIndex(m_selectedFrameId);
        const int insertAt = selectedFrame >= 0
            ? selectedFrame + 1 : after.frames.size();
        after.frames.insert(insertAt, copy);
        applyDocumentChange(
            after, QStringLiteral("粘贴协议帧"), copy.id, {});
    }
}

void ProtocolEditorPage::duplicateSelection()
{
    copySelection();
    pasteSelection();
}

void ProtocolEditorPage::selectDocument()
{
    m_selectedFrameId = {};
    m_selectedFieldId = {};
    refreshSelection();
    updateActionStates();
}

void ProtocolEditorPage::selectFrame(const QUuid &frameId)
{
    m_selectedFrameId = frameId;
    m_selectedFieldId = {};
    refreshSelection();
    updateActionStates();
}

void ProtocolEditorPage::selectField(
    const QUuid &frameId, const QUuid &fieldId)
{
    m_selectedFrameId = frameId;
    m_selectedFieldId = fieldId;
    refreshSelection();
    updateActionStates();
}

void ProtocolEditorPage::addFieldTemplate(
    const FieldRole role,
    const QUuid &targetFrameId,
    int targetIndex)
{
    const int framePosition = frameIndex(targetFrameId);
    if (framePosition < 0) return;
    Document after = m_document;
    Frame &frame = after.frames[framePosition];
    Field field = makeField(role);
    field.name = uniqueFieldName(field.name, frame);
    targetIndex = qBound(0, targetIndex, frame.fields.size());
    frame.fields.insert(targetIndex, field);
    applyDocumentChange(
        after, QStringLiteral("新增字段"), frame.id, field.id);
}

void ProtocolEditorPage::moveField(
    const QUuid &sourceFrameId,
    const QUuid &fieldId,
    const QUuid &targetFrameId,
    int targetIndex)
{
    const int sourceFrame = frameIndex(sourceFrameId);
    const int targetFrame = frameIndex(targetFrameId);
    if (sourceFrame < 0 || targetFrame < 0) return;
    const int sourceField = fieldIndex(sourceFrame, fieldId);
    if (sourceField < 0) return;
    Document after = m_document;
    Field field = after.frames[sourceFrame].fields.takeAt(sourceField);
    if (sourceFrame == targetFrame && targetIndex > sourceField) {
        --targetIndex;
    }
    Frame &target = after.frames[targetFrame];
    targetIndex = qBound(0, targetIndex, target.fields.size());
    if (sourceFrame != targetFrame) {
        field.name = uniqueFieldName(field.name, target);
    }
    target.fields.insert(targetIndex, field);
    applyDocumentChange(
        after, QStringLiteral("移动字段"), target.id, field.id);
}

void ProtocolEditorPage::moveFrame(
    const QUuid &frameId, int targetIndex)
{
    const int sourceIndex = frameIndex(frameId);
    if (sourceIndex < 0) return;
    Document after = m_document;
    Frame frame = after.frames.takeAt(sourceIndex);
    if (targetIndex > sourceIndex) --targetIndex;
    targetIndex = qBound(0, targetIndex, after.frames.size());
    after.frames.insert(targetIndex, frame);
    applyDocumentChange(
        after, QStringLiteral("调整协议帧顺序"), frame.id, {});
}

void ProtocolEditorPage::updateDocumentName(const QString &name)
{
    Document after = m_document;
    after.name = name;
    applyDocumentChange(
        after, QStringLiteral("修改工作空间名称"), {}, {});
}

void ProtocolEditorPage::updateFrame(const Frame &frame)
{
    const int position = frameIndex(frame.id);
    if (position < 0) return;
    Document after = m_document;
    after.frames[position] = frame;
    applyDocumentChange(
        after, QStringLiteral("修改协议帧属性"), frame.id, {});
}

void ProtocolEditorPage::updateField(
    const QUuid &frameId, const Field &field)
{
    const int framePosition = frameIndex(frameId);
    if (framePosition < 0) return;
    const int fieldPosition = fieldIndex(framePosition, field.id);
    if (fieldPosition < 0) return;
    Document after = m_document;
    after.frames[framePosition].fields[fieldPosition] = field;
    applyDocumentChange(
        after, QStringLiteral("修改字段属性"), frameId, field.id);
}

int ProtocolEditorPage::frameIndex(const QUuid &frameId) const
{
    if (frameId.isNull()) return -1;
    for (int index = 0; index < m_document.frames.size(); ++index) {
        if (m_document.frames.at(index).id == frameId) return index;
    }
    return -1;
}

int ProtocolEditorPage::fieldIndex(
    const int framePosition, const QUuid &fieldId) const
{
    if (fieldId.isNull()
        || framePosition < 0
        || framePosition >= m_document.frames.size()) {
        return -1;
    }
    const Frame &frame = m_document.frames.at(framePosition);
    for (int index = 0; index < frame.fields.size(); ++index) {
        if (frame.fields.at(index).id == fieldId) return index;
    }
    return -1;
}

QString ProtocolEditorPage::uniqueFrameName(
    const QString &base,
    const Document &document) const
{
    const auto exists = [&document](const QString &candidate) {
        for (const Frame &frame : document.frames) {
            if (frame.name.compare(candidate, Qt::CaseInsensitive) == 0) {
                return true;
            }
        }
        return false;
    };
    if (!exists(base)) return base;
    int suffix = 2;
    while (exists(base + QString::number(suffix))) ++suffix;
    return base + QString::number(suffix);
}

QString ProtocolEditorPage::uniqueFieldName(
    const QString &base,
    const Frame &frame) const
{
    const auto exists = [&frame](const QString &candidate) {
        for (const Field &field : frame.fields) {
            if (field.name.compare(candidate, Qt::CaseInsensitive) == 0) {
                return true;
            }
        }
        return false;
    };
    if (!exists(base)) return base;
    int suffix = 2;
    while (exists(base + QString::number(suffix))) ++suffix;
    return base + QString::number(suffix);
}

void ProtocolEditorPage::notify(
    const QString &message, const NotificationType type) const
{
    m_context->notify(message, type);
}
