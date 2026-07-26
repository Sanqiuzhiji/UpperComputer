#include "ProtocolRepository.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

#include <algorithm>

using namespace ProtocolModel;

namespace {

ProtocolFieldType communicationType(const DataType type)
{
    switch (type) {
    case DataType::UInt8:
    case DataType::UInt16:
    case DataType::UInt32:
    case DataType::UInt64: return ProtocolFieldType::UInt;
    case DataType::Int8:
    case DataType::Int16:
    case DataType::Int32:
    case DataType::Int64: return ProtocolFieldType::Int;
    case DataType::Float32: return ProtocolFieldType::Float;
    case DataType::Float64: return ProtocolFieldType::Double;
    case DataType::Bytes: return ProtocolFieldType::ByteArray;
    }
    return ProtocolFieldType::ByteArray;
}

::ProtocolFieldRole communicationRole(const FieldRole role)
{
    switch (role) {
    case FieldRole::Header: return ::ProtocolFieldRole::FrameHeader;
    case FieldRole::FrameId: return ::ProtocolFieldRole::MessageId;
    case FieldRole::Length: return ::ProtocolFieldRole::Length;
    case FieldRole::Checksum: return ::ProtocolFieldRole::Checksum;
    case FieldRole::Tail: return ::ProtocolFieldRole::Constant;
    case FieldRole::Skip: return ::ProtocolFieldRole::Constant;
    case FieldRole::Data: return ::ProtocolFieldRole::Value;
    }
    return ::ProtocolFieldRole::Value;
}

::ProtocolChecksumAlgorithm communicationChecksum(
    const ChecksumAlgorithm algorithm)
{
    switch (algorithm) {
    case ChecksumAlgorithm::Sum8:
        return ::ProtocolChecksumAlgorithm::Sum8;
    case ChecksumAlgorithm::Xor8:
        return ::ProtocolChecksumAlgorithm::Xor8;
    case ChecksumAlgorithm::Crc8:
        return ::ProtocolChecksumAlgorithm::Crc8;
    }
    return ::ProtocolChecksumAlgorithm::Sum8;
}

} // namespace

ProtocolRepository::ProtocolRepository(
    QObject *parent, const QString &directoryPath)
    : QObject(parent),
      m_directoryPath(directoryPath.trimmed().isEmpty()
          ? QDir(QStandardPaths::writableLocation(
                     QStandardPaths::AppDataLocation))
                .filePath(QStringLiteral("protocols"))
          : QDir::cleanPath(directoryPath))
{
}

QString ProtocolRepository::directoryPath() const
{
    return m_directoryPath;
}

QList<ProtocolSummary> ProtocolRepository::availableProtocols() const
{
    QList<ProtocolSummary> summaries;
    summaries.reserve(m_records.size());
    for (auto iterator = m_records.cbegin(); iterator != m_records.cend();
         ++iterator) {
        summaries.append({
            iterator.key(),
            iterator->document.name,
            iterator->filePath
        });
    }
    std::ranges::sort(
        summaries, [](const ProtocolSummary &left,
                      const ProtocolSummary &right) {
            return left.displayName.localeAwareCompare(right.displayName) < 0;
        });
    return summaries;
}

std::optional<Document> ProtocolRepository::protocolById(
    const QString &id) const
{
    const auto iterator = m_records.constFind(id);
    if (iterator == m_records.cend()) return std::nullopt;
    return iterator->document;
}

QString ProtocolRepository::filePathForId(const QString &id) const
{
    const auto iterator = m_records.constFind(id);
    return iterator == m_records.cend() ? QString{} : iterator->filePath;
}

QList<ProtocolDefinition> ProtocolRepository::communicationDefinitions() const
{
    QList<ProtocolDefinition> definitions;
    const QList<ProtocolSummary> summaries = availableProtocols();
    definitions.reserve(summaries.size());
    for (const ProtocolSummary &summary : summaries) {
        const auto source = protocolById(summary.id);
        if (!source) continue;
        ProtocolDefinition definition;
        definition.id = summary.id;
        definition.displayName = source->name;
        for (int frameIndex = 0;
             frameIndex < source->frames.size();
             ++frameIndex) {
            const Frame &frame = source->frames.at(frameIndex);
            MessageDefinition message;
            message.id = normalizedId(frame.id);
            message.displayName = frame.name;
            int fieldIndex = 0;
            for (const Field &field : frame.fields) {
                const int sequence = ++fieldIndex;
                FieldDefinition target;
                target.id = normalizedId(field.id);
                target.displayName = field.name.trimmed().isEmpty()
                    ? QStringLiteral("data%1").arg(sequence)
                    : field.name;
                target.type = communicationType(field.dataType);
                target.bitWidth = field.byteCount * 8;
                target.editable = field.role == FieldRole::Data;
                target.role = communicationRole(field.role);
                target.littleEndian =
                    field.byteOrder == ByteOrder::LittleEndian;
                target.scale = field.scale;
                target.offset = field.offset;
                target.checksumAlgorithm =
                    communicationChecksum(field.checksumAlgorithm);
                if (field.role == FieldRole::Header
                    || field.role == FieldRole::Tail) {
                    target.defaultValue = field.fixedBytes;
                    target.fixedValue = field.fixedBytes;
                } else if (field.role == FieldRole::Skip) {
                    target.defaultValue =
                        QByteArray(field.byteCount, '\0');
                    target.fixedValue = target.defaultValue;
                } else if (field.role == FieldRole::FrameId) {
                    target.defaultValue = frameIndex;
                    target.fixedValue = frameIndex;
                }
                message.fields.append(target);
            }
            definition.receiveMessages.append(message);
            definition.sendMessages.append(message);
        }
        definitions.append(definition);
    }
    return definitions;
}

bool ProtocolRepository::rescan(QStringList *errors)
{
    QDir directory(m_directoryPath);
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        const QString message =
            tr("无法访问协议目录：%1").arg(m_directoryPath);
        if (errors) errors->append(message);
        emit notificationRequested(message, NotificationType::Error);
        return false;
    }

    QHash<QString, Record> scanned;
    const QFileInfoList files = directory.entryInfoList(
        {QStringLiteral("*.ucproto.json")},
        QDir::Files | QDir::Readable,
        QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo &fileInfo : files) {
        Document document;
        QString error;
        if (!loadFile(fileInfo.absoluteFilePath(), &document, &error)) {
            const QString message = tr("协议文件“%1”加载失败：%2")
                                        .arg(fileInfo.fileName(), error);
            if (errors) errors->append(message);
            emit notificationRequested(message, NotificationType::Error);
            continue;
        }
        const QString id = normalizedId(document.id);
        if (scanned.contains(id)) {
            const QString message =
                tr("协议 ID 重复，已忽略“%1”").arg(fileInfo.fileName());
            if (errors) errors->append(message);
            emit notificationRequested(message, NotificationType::Warning);
            continue;
        }
        scanned.insert(id, {document, fileInfo.absoluteFilePath()});
    }
    m_records = std::move(scanned);
    emit protocolLibraryChanged();
    return true;
}

bool ProtocolRepository::save(
    const Document &document,
    const QString &targetPath,
    QString *savedPath,
    QString *errorMessage)
{
    const QVector<ValidationIssue> issues = validate(document);
    for (const ValidationIssue &issue : issues) {
        if (issue.severity == ValidationSeverity::Error) {
            if (errorMessage) *errorMessage = issue.message;
            return false;
        }
    }
    QString path = targetPath;
    if (path.trimmed().isEmpty()) {
        path = filePathForId(normalizedId(document.id));
    }
    if (path.trimmed().isEmpty()) {
        path = QDir(m_directoryPath).filePath(
            safeFileName(document.name) + QStringLiteral(".ucproto.json"));
    }
    if (!path.endsWith(QStringLiteral(".ucproto.json"),
                       Qt::CaseInsensitive)) {
        path += QStringLiteral(".ucproto.json");
    }
    const QFileInfo fileInfo(path);
    if (!QDir().mkpath(fileInfo.absolutePath())) {
        if (errorMessage) *errorMessage = tr("无法创建保存目录");
        return false;
    }

    QSaveFile file(fileInfo.absoluteFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) *errorMessage = file.errorString();
        return false;
    }
    const QJsonDocument json(toJson(document));
    if (file.write(json.toJson(QJsonDocument::Indented)) < 0
        || !file.commit()) {
        if (errorMessage) *errorMessage = file.errorString();
        return false;
    }
    upsertRecord(document, fileInfo.absoluteFilePath());
    if (savedPath) *savedPath = fileInfo.absoluteFilePath();
    emit protocolLibraryChanged();
    emit notificationRequested(
        tr("协议“%1”已保存").arg(document.name),
        NotificationType::Success);
    return true;
}

bool ProtocolRepository::remove(
    const QString &id, QString *errorMessage)
{
    const auto iterator = m_records.find(id);
    if (iterator == m_records.end()) {
        if (errorMessage) *errorMessage = tr("找不到要删除的协议");
        return false;
    }
    const QString path = iterator->filePath;
    const QString name = iterator->document.name;
    if (!QFile::remove(path)) {
        if (errorMessage) {
            *errorMessage = tr("无法删除协议文件：%1").arg(path);
        }
        return false;
    }
    m_records.erase(iterator);
    emit protocolLibraryChanged();
    emit notificationRequested(
        tr("协议“%1”已删除").arg(name), NotificationType::Success);
    return true;
}

bool ProtocolRepository::importFile(
    const QString &sourcePath,
    QString *importedId,
    QString *errorMessage)
{
    Document document;
    if (!loadFile(sourcePath, &document, errorMessage)) return false;

    QString id = normalizedId(document.id);
    if (m_records.contains(id)) {
        document.id = QUuid::createUuid();
        id = normalizedId(document.id);
    }
    const QString baseName = safeFileName(document.name);
    QString target = QDir(m_directoryPath).filePath(
        baseName + QStringLiteral(".ucproto.json"));
    int suffix = 2;
    while (QFileInfo::exists(target)) {
        target = QDir(m_directoryPath).filePath(
            baseName + QStringLiteral("_import%1.ucproto.json")
                           .arg(suffix++));
    }
    QString saved;
    if (!save(document, target, &saved, errorMessage)) return false;
    if (importedId) *importedId = id;
    emit notificationRequested(
        tr("协议“%1”已导入").arg(document.name),
        NotificationType::Success);
    return true;
}

QString ProtocolRepository::normalizedId(const QUuid &id)
{
    return id.toString(QUuid::WithoutBraces);
}

QString ProtocolRepository::safeFileName(const QString &name)
{
    QString result = name.trimmed();
    result.replace(
        QRegularExpression(QStringLiteral(R"([<>:"/\\|?*\x00-\x1F])")),
        QStringLiteral("_"));
    result = result.left(96);
    return result.isEmpty() ? QStringLiteral("Protocol") : result;
}

bool ProtocolRepository::loadFile(
    const QString &filePath,
    Document *document,
    QString *errorMessage) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) *errorMessage = file.errorString();
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument json =
        QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError
        || !json.isObject()) {
        if (errorMessage) {
            *errorMessage = parseError.error == QJsonParseError::NoError
                ? tr("根节点必须是 JSON 对象")
                : parseError.errorString();
        }
        return false;
    }
    return fromJson(json.object(), document, errorMessage);
}

void ProtocolRepository::upsertRecord(
    const Document &document, const QString &filePath)
{
    m_records.insert(
        normalizedId(document.id),
        {document, QFileInfo(filePath).absoluteFilePath()});
}
