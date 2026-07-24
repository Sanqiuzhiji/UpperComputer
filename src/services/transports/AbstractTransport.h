#pragma once

#include <QObject>

#include "models/AppTypes.h"
#include "models/ConnectionTypes.h"

class AbstractTransport : public QObject
{
    Q_OBJECT

public:
    explicit AbstractTransport(QObject *parent = nullptr);
    ~AbstractTransport() override = default;

    virtual void open(const TransportConfig &config) = 0;
    virtual void close() = 0;
    [[nodiscard]] virtual bool isOpen() const = 0;
    [[nodiscard]] virtual bool write(const QByteArray &data,
                                     QString *errorMessage) = 0;

signals:
    void stateChanged(ConnectionState state);
    void dataReceived(const QByteArray &data);
    void dataWritten(qint64 byteCount);
    void errorOccurred(const QString &message);
};
