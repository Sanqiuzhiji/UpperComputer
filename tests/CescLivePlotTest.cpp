#include "models/ConnectionTypes.h"
#include "services/ChannelDataHub.h"
#include "services/ConnectionManager.h"
#include "services/cesc/CescSession.h"
#include "services/cesc/CescTelemetryClient.h"

#include <QCoreApplication>
#include <QTextStream>
#include <QTimer>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const QString portName = argc > 1
        ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("COM15");
    ConnectionManager connection;
    CescSession session(&connection);
    ChannelDataHub hub;
    CescTelemetryClient telemetry(&session, &hub);
    QTextStream output(stdout);
    int batches = 0;

    QObject::connect(&connection, &ConnectionManager::errorOccurred,
                     &app, [&output, &app](const QString &message) {
        output << "TRANSPORT ERROR: " << message << Qt::endl;
        app.exit(2);
    });
    QObject::connect(&session, &CescSession::protocolError,
                     &app, [&output, &app](const QString &message) {
        output << "PROTOCOL ERROR: " << message << Qt::endl;
        app.exit(3);
    });
    QObject::connect(&session, &CescSession::stateChanged,
                     &app, [&session, &telemetry, &output](CescSession::State state) {
        output << "SESSION STATE: " << int(state) << Qt::endl;
        if (state == CescSession::State::Ready) {
            telemetry.enumerate();
            telemetry.subscribe({0U, 1U, 2U}, 10000U, 1U);
        }
    });
    QObject::connect(&hub, &ChannelDataHub::samplesAppended,
                     &app, [&hub, &output, &app, &batches](const QStringList &ids) {
        ++batches;
        if (batches < 20) return;
        output << "CHANNELS:" << Qt::endl;
        for (const ChannelDescriptor &channel : hub.channels()) {
            output << "  " << channel.id << " name=" << channel.displayName
                   << " unit=" << channel.unit
                   << " latest=" << hub.latestTimestamp(channel.id)
                   << Qt::endl;
        }
        output << "SAMPLE BATCHES: " << batches << Qt::endl;
        app.exit(ids.isEmpty() || hub.channels().size() < 3 ? 4 : 0);
    });
    QTimer::singleShot(5000, &app, [&output, &app, &hub] {
        output << "TIMEOUT channels=" << hub.channels().size() << Qt::endl;
        app.exit(5);
    });

    SerialConfig config;
    config.portName = portName;
    config.baudRate = 115200;
    session.setEnabled(true);
    connection.connectTransport(TransportType::SerialPort, config);
    return app.exec();
}
