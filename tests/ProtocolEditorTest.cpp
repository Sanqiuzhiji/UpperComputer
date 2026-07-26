#include <QAction>
#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QKeySequence>
#include <QLineEdit>
#include <QMouseEvent>
#include <QSet>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QToolButton>

#include "app/AppContext.h"
#include "models/ProtocolTypes.h"
#include "pages/ProtocolEditorPage.h"
#include "pages/protocol/ProtocolCanvas.h"
#include "pages/protocol/ProtocolFrameWidget.h"
#include "services/ProtocolRepository.h"
#include "theme/ThemeManager.h"

namespace {

int failures = 0;

void verify(const bool condition, const char *message)
{
    if (condition) return;
    qCritical("FAIL: %s", message);
    ++failures;
}

QAction *actionForShortcut(
    ProtocolEditorPage &page, const QKeySequence &shortcut)
{
    const QList<QAction *> actions = page.actions();
    for (QAction *action : actions) {
        if (action->shortcut() == shortcut) return action;
    }
    return nullptr;
}

void testModelAndJson()
{
    using namespace ProtocolModel;
    Document document = makeDocument(QStringLiteral("MotorTelemetry"));
    Field header = makeField(FieldRole::Header);
    Field speed = makeField(FieldRole::Data);
    speed.name = QStringLiteral("speed");
    speed.dataType = DataType::Float32;
    speed.byteCount = 4;
    document.frames[0].fields = {header, speed};

    verify(
        fieldOffsets(document.frames[0]) == QVector<int>({0, 2}),
        "field byte offsets are recalculated");
    verify(
        frameByteCount(document.frames[0]) == 6,
        "frame byte count is calculated");
    verify(
        !hasValidationErrors(validate(document)),
        "valid document has no errors");

    Document roundTrip;
    QString error;
    verify(
        fromJson(toJson(document), &roundTrip, &error),
        "JSON round trip parses");
    verify(roundTrip == document, "JSON round trip preserves model");

    roundTrip.frames[0].fields[0].fixedBytes =
        QByteArray::fromHex("AA");
    verify(
        hasValidationErrors(validate(roundTrip)),
        "fixed Hex length mismatch is rejected");
}

void testRepository()
{
    using namespace ProtocolModel;
    QTemporaryDir temporary;
    verify(temporary.isValid(), "temporary repository directory exists");
    ProtocolRepository repository(nullptr, temporary.path());
    Document document = makeDocument(QStringLiteral("RepositoryProtocol"));
    document.frames[0].fields.append(makeField(FieldRole::Data));

    QString path;
    QString error;
    verify(
        repository.save(document, {}, &path, &error),
        "repository saves with QSaveFile");
    verify(QFile::exists(path), "saved protocol file exists");
    verify(
        repository.availableProtocols().size() == 1,
        "saved protocol is indexed");

    ProtocolRepository reloaded(nullptr, temporary.path());
    reloaded.rescan();
    verify(
        reloaded.availableProtocols().size() == 1,
        "saved protocol reloads after rescan");
    verify(
        reloaded.protocolById(
            document.id.toString(QUuid::WithoutBraces))
            .has_value(),
        "protocol can be found by stable id");

    QFile corrupt(
        temporary.filePath(QStringLiteral("broken.ucproto.json")));
    verify(
        corrupt.open(QIODevice::WriteOnly | QIODevice::Text),
        "corrupt test file can be created");
    corrupt.write("{ definitely broken");
    corrupt.close();
    QStringList errors;
    verify(reloaded.rescan(&errors), "rescan survives corrupt file");
    verify(errors.size() == 1, "corrupt file is reported individually");
    verify(
        reloaded.availableProtocols().size() == 1,
        "healthy protocol remains available");

    const QString importSource =
        temporary.filePath(QStringLiteral("same-name-source.json"));
    QFile importFile(importSource);
    verify(
        importFile.open(QIODevice::WriteOnly | QIODevice::Text),
        "same-name import source can be created");
    importFile.write(
        QJsonDocument(toJson(document)).toJson(QJsonDocument::Indented));
    importFile.close();
    QString firstImportedId;
    QString secondImportedId;
    verify(
        reloaded.importFile(importSource, &firstImportedId, &error),
        "same-name protocol can be imported once");
    verify(
        reloaded.importFile(importSource, &secondImportedId, &error),
        "same-name protocol can be imported twice");
    const QList<ProtocolSummary> imported = reloaded.availableProtocols();
    QSet<QString> importedLabels;
    for (const ProtocolSummary &summary : imported) {
        importedLabels.insert(summary.displayName);
    }
    verify(
        imported.size() == 3
            && importedLabels.size() == imported.size(),
        "same-name imports have distinct protocol picker labels");
}

void testPage()
{
    QStandardPaths::setTestModeEnabled(true);
    AppContext context;
    ProtocolEditorPage page(&context);
    page.resize(960, 640);
    page.show();
    QApplication::processEvents();

    verify(
        page.findChild<QWidget *>(
            QStringLiteral("fieldLibraryPanel")) != nullptr,
        "field library panel exists");
    auto *canvas = page.findChild<ProtocolCanvas *>(
        QStringLiteral("protocolCanvas"));
    verify(canvas != nullptr, "protocol canvas exists");
    verify(
        page.findChild<QWidget *>(
            QStringLiteral("protocolPropertyPanel")) != nullptr,
        "property panel exists");
    verify(
        page.findChild<QWidget *>(
            QStringLiteral("protocolEditorSplitter")) != nullptr,
        "three-column splitter exists");
    auto *newButton = page.findChild<QToolButton *>(
        QStringLiteral("protocolNewButton"));
    verify(
        newButton != nullptr && newButton->iconSize().width() >= 24,
        "protocol editor toolbar icons are enlarged");

    const ProtocolModel::Document initial = page.document();
    verify(
        initial.frames.size() == 1,
        "temporary protocol starts with one frame");
    const QUuid frameId = initial.frames.constFirst().id;
    const bool invoked = QMetaObject::invokeMethod(
        canvas, "fieldTemplateDropped", Qt::DirectConnection,
        Q_ARG(ProtocolModel::FieldRole,
              ProtocolModel::FieldRole::Data),
        Q_ARG(QUuid, frameId),
        Q_ARG(int, 0));
    verify(invoked, "field drop signal can be delivered");
    verify(
        page.document().frames.constFirst().fields.size() == 1,
        "field template drop changes the document");

    QAction *undo = actionForShortcut(page, QKeySequence::Undo);
    verify(undo != nullptr && undo->isEnabled(), "undo is enabled");
    if (undo) undo->trigger();
    verify(
        page.document().frames.constFirst().fields.isEmpty(),
        "field insertion can be undone");

    QAction *redo = actionForShortcut(page, QKeySequence::Redo);
    verify(redo != nullptr && redo->isEnabled(), "redo is enabled");
    if (redo) redo->trigger();
    verify(
        page.document().frames.constFirst().fields.size() == 1,
        "field insertion can be redone");
    QApplication::processEvents();

    QWidget *frameWidget = page.findChild<QWidget *>(
        QStringLiteral("protocolFrameWidget"));
    verify(
        frameWidget != nullptr
            && frameWidget->height() == 120
            && frameWidget->minimumHeight() == 120
            && frameWidget->maximumHeight() == 120
            && frameWidget->sizePolicy().verticalPolicy()
                == QSizePolicy::Fixed,
        "protocol rows use the fixed compact height");

    QWidget *fieldCard = page.findChild<QWidget *>(
        QStringLiteral("protocolFieldCard"));
    verify(
        fieldCard != nullptr && fieldCard->minimumHeight() <= 48,
        "field card exists and uses the compact height");
    if (fieldCard) {
        QMouseEvent press(
            QEvent::MouseButtonPress,
            QPointF(10, 10), QPointF(10, 10),
            QPointF(fieldCard->mapToGlobal(QPoint(10, 10))),
            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(fieldCard, &press);
    }
    auto *fieldNameEditor = page.findChild<QLineEdit *>(
        QStringLiteral("protocolFieldNameEditor"));
    verify(
        fieldNameEditor != nullptr
            && fieldNameEditor->text()
                == page.document().frames.constFirst().fields.constFirst().name,
        "clicking a field keeps field selection and opens its editor");
    if (fieldNameEditor) {
        fieldNameEditor->setText(QStringLiteral("editedField"));
        QMetaObject::invokeMethod(
            fieldNameEditor, "editingFinished", Qt::DirectConnection);
    }
    verify(
        page.document().frames.constFirst().fields.constFirst().name
            == QStringLiteral("editedField"),
        "selected field properties are editable");

    auto *fieldDeleteButton = page.findChild<QToolButton *>(
        QStringLiteral("protocolFieldDeleteButton"));
    verify(fieldDeleteButton != nullptr, "each field has a delete button");
    if (fieldDeleteButton) fieldDeleteButton->click();
    verify(
        page.document().frames.constFirst().fields.isEmpty(),
        "field delete button removes its field");
    if (undo) undo->trigger();
    verify(
        page.document().frames.constFirst().fields.size() == 1,
        "field deletion can be undone");

    verify(
        page.findChild<QToolButton *>(
            QStringLiteral("protocolFrameDeleteButton")) != nullptr,
        "each protocol has a delete button");

    QAction *copy = actionForShortcut(page, QKeySequence::Copy);
    QAction *paste = actionForShortcut(page, QKeySequence::Paste);
    verify(copy != nullptr && copy->isEnabled(), "field copy is enabled");
    if (copy) copy->trigger();
    verify(paste != nullptr && paste->isEnabled(), "field paste is enabled");
    if (paste) paste->trigger();
    verify(
        page.document().frames.constFirst().fields.size() == 2,
        "field can be copied and pasted");
    verify(
        page.document().frames.constFirst().fields.at(0).id
            != page.document().frames.constFirst().fields.at(1).id,
        "pasted field receives a new UUID");
    if (undo) undo->trigger();

    QAction *addFrame = actionForShortcut(
        page, QKeySequence(QStringLiteral("Ctrl+Shift+N")));
    verify(addFrame != nullptr && addFrame->isEnabled(),
           "new frame action is enabled");
    if (addFrame) addFrame->trigger();
    verify(page.document().frames.size() == 2, "protocol frame can be added");
    const QUuid sourceFieldId =
        page.document().frames.at(0).fields.constFirst().id;
    const QUuid targetFrameId = page.document().frames.at(1).id;
    verify(
        QMetaObject::invokeMethod(
            canvas, "fieldMoveRequested", Qt::DirectConnection,
            Q_ARG(QUuid, frameId),
            Q_ARG(QUuid, sourceFieldId),
            Q_ARG(QUuid, targetFrameId),
            Q_ARG(int, 0)),
        "cross-frame move signal can be delivered");
    verify(
        page.document().frames.at(0).fields.isEmpty()
            && page.document().frames.at(1).fields.size() == 1,
        "field can move across protocol frames");
    if (undo) undo->trigger();
    verify(
        page.document().frames.at(0).fields.size() == 1,
        "cross-frame move can be undone");
    QApplication::processEvents();
    ProtocolFrameWidget *firstFrameWidget = nullptr;
    for (ProtocolFrameWidget *candidate :
         page.findChildren<ProtocolFrameWidget *>()) {
        if (candidate->frameId() == frameId) {
            firstFrameWidget = candidate;
            break;
        }
    }
    verify(firstFrameWidget != nullptr, "frame drop target exists");
    if (firstFrameWidget) {
        auto *dragHandle = firstFrameWidget->findChild<QToolButton *>(
            QStringLiteral("protocolFrameDragHandle"));
        verify(
            dragHandle != nullptr
                && dragHandle->cursor().shape() == Qt::OpenHandCursor,
            "frame row exposes its drag handle");
    }
    verify(
        firstFrameWidget
            && QMetaObject::invokeMethod(
                firstFrameWidget, "frameDropRequested",
                Qt::DirectConnection,
                Q_ARG(QUuid, targetFrameId),
                Q_ARG(QUuid, frameId),
                Q_ARG(bool, false)),
        "dropping one frame on another can be delivered");
    verify(
        page.document().frames.constFirst().id == targetFrameId,
        "protocol frames can be reordered by dropping vertically");
    if (undo) undo->trigger();
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QApplication::processEvents();
    ProtocolFrameWidget *newFrameWidget = nullptr;
    for (ProtocolFrameWidget *candidate :
         page.findChildren<ProtocolFrameWidget *>()) {
        if (candidate->frameId() == targetFrameId) {
            newFrameWidget = candidate;
            break;
        }
    }
    auto *frameDeleteButton = newFrameWidget
        ? newFrameWidget->findChild<QToolButton *>(
              QStringLiteral("protocolFrameDeleteButton"))
        : nullptr;
    verify(
        frameDeleteButton != nullptr,
        "new frame exposes its delete button");
    if (frameDeleteButton) frameDeleteButton->click();
    verify(
        page.document().frames.size() == 1,
        "clicking the frame delete button removes it immediately");
    if (undo) undo->trigger();
    verify(
        page.document().frames.size() == 2,
        "frame deletion can be undone");
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QApplication::processEvents();

    const auto clickFrameDelete = [&page](const QUuid &id) {
        for (ProtocolFrameWidget *candidate :
             page.findChildren<ProtocolFrameWidget *>()) {
            if (candidate->frameId() != id) continue;
            if (auto *button = candidate->findChild<QToolButton *>(
                    QStringLiteral("protocolFrameDeleteButton"))) {
                button->click();
                return true;
            }
        }
        return false;
    };
    verify(clickFrameDelete(targetFrameId), "second frame delete is clickable");
    verify(clickFrameDelete(frameId), "last frame delete is clickable");
    verify(
        page.document().frames.isEmpty(),
        "deleting the last frame shows the empty protocol state");
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QApplication::processEvents();
    if (undo) {
        undo->trigger();
        undo->trigger();
    }

    context.themeManager()->setMode(ThemeMode::Light);
    QApplication::processEvents();
    context.themeManager()->setMode(ThemeMode::Dark);
    QApplication::processEvents();
    verify(page.isVisible(), "page remains visible after theme changes");
}

} // namespace

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("ProtocolEditorTest"));
    QApplication::setOrganizationName(QStringLiteral("UpperComputerTests"));

    testModelAndJson();
    testRepository();
    testPage();
    return failures == 0 ? 0 : 1;
}
