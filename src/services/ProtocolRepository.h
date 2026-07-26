#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

#include <optional>

#include "models/AppTypes.h"
#include "models/ProtocolDefinition.h"
#include "models/ProtocolTypes.h"

struct ProtocolSummary {
    QString id;
    QString displayName;
    QString filePath;
};

class ProtocolRepository final : public QObject
{
    Q_OBJECT

public:
    explicit ProtocolRepository(
        QObject *parent = nullptr, const QString &directoryPath = {});

    [[nodiscard]] QString directoryPath() const;
    [[nodiscard]] QList<ProtocolSummary> availableProtocols() const;
    [[nodiscard]] std::optional<ProtocolModel::Document> protocolById(
        const QString &id) const;
    [[nodiscard]] QString filePathForId(const QString &id) const;
    [[nodiscard]] QList<ProtocolDefinition> communicationDefinitions() const;

    bool rescan(QStringList *errors = nullptr);
    bool save(
        const ProtocolModel::Document &document,
        const QString &targetPath,
        QString *savedPath = nullptr,
        QString *errorMessage = nullptr);
    bool remove(const QString &id, QString *errorMessage = nullptr);
    bool importFile(
        const QString &sourcePath,
        QString *importedId = nullptr,
        QString *errorMessage = nullptr);

signals:
    void protocolLibraryChanged();
    void notificationRequested(
        const QString &message, NotificationType type);

private:
    struct Record {
        ProtocolModel::Document document;
        QString filePath;
    };

    [[nodiscard]] static QString normalizedId(const QUuid &id);
    [[nodiscard]] static QString safeFileName(const QString &name);
    [[nodiscard]] bool loadFile(
        const QString &filePath,
        ProtocolModel::Document *document,
        QString *errorMessage) const;
    void upsertRecord(
        const ProtocolModel::Document &document, const QString &filePath);

    QString m_directoryPath;
    QHash<QString, Record> m_records;
};
