#pragma once

#include "services/cesc/CescPacketCodec.h"
#include "services/cesc/CescTransactionManager.h"
#include <QObject>

class ConnectionManager;

class CescSession final : public QObject
{
    Q_OBJECT
public:
    enum class State { Disconnected, TransportOpen, Negotiating, Ready, Error };
    Q_ENUM(State)
    explicit CescSession(ConnectionManager *connection, QObject *parent=nullptr);
    [[nodiscard]] State state() const noexcept;
    [[nodiscard]] bool isReady() const noexcept;
    [[nodiscard]] quint16 maximumPayload() const noexcept;
    [[nodiscard]] quint64 capabilities() const noexcept;
    [[nodiscard]] quint32 sessionId() const noexcept;
    [[nodiscard]] CescTransactionManager *transactions() noexcept;
    [[nodiscard]] Cesc::CodecDiagnostics diagnostics() const noexcept;
    void setEnabled(bool enabled);
    [[nodiscard]] bool isEnabled() const noexcept;
signals:
    void stateChanged(CescSession::State state);
    void identityChanged();
    void packetReceived(const Cesc::Packet &packet);
    void sessionChanged(quint32 oldId, quint32 newId);
    void protocolError(const QString &message);
private:
    void transportStateChanged(int state);
    void setState(State state);
    void hello();
    ConnectionManager *m_connection{};
    CescPacketCodec m_codec;
    CescTransactionManager m_transactions;
    State m_state{State::Disconnected};
    quint16 m_maximumPayload{Cesc::AbsoluteMaximumPayload};
    quint64 m_capabilities{};
    quint32 m_sessionId{};
    bool m_enabled{};
};
