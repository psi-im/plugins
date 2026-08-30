#pragma once

#include "qca-otr/akesession.h"
#include "qca-otr/tlv.h"

#include <QByteArray>
#include <QVector>
#include <QtGlobal>

#include <memory>

namespace QcaOtr {

enum class SessionPolicy {
    Disabled,
    Manual,
    Opportunistic,
    Always
};

enum class SessionStatus {
    Ignored,
    Plaintext,
    ProtocolMessage,
    FragmentIncomplete,
    Handled,
    Authenticated,
    Message,
    ForOtherInstance,
    Error
};

struct SessionResult
{
    SessionStatus status = SessionStatus::Ignored;
    quint32 peerInstance = 0;
    QByteArray plaintext;
    QVector<Tlv> tlvs;
    QCA::SecureArray extraKey;
    quint8 flags = 0;
    QVector<QByteArray> outgoingMessages;
};

enum class OutgoingStatus {
    Plaintext,
    Encrypted,
    Negotiation,
    Error
};

struct OutgoingResult
{
    OutgoingStatus status = OutgoingStatus::Error;
    quint32 peerInstance = 0;
    QVector<QByteArray> messages;
};

// Transport-facing OTRv3 session router for one logical correspondent.
// A single object owns one local instance and any number of remote instance
// children. The protocol engines underneath still operate on raw binary OTR
// messages; this layer owns armoring, fragmentation and instance dispatch.
class OtrSession
{
public:
    OtrSession(const DsaPrivateKey &identityKey, quint32 localInstance);
    ~OtrSession();

    OtrSession(const OtrSession &) = delete;
    OtrSession &operator=(const OtrSession &) = delete;
    OtrSession(OtrSession &&) noexcept;
    OtrSession &operator=(OtrSession &&) noexcept;

    quint32 localInstance() const;

    void setPolicy(SessionPolicy policy);
    SessionPolicy policy() const;

    // Build the user-visible libotr-style Query Message used for an explicit
    // manual start. Receiving a compatible query starts the broadcast AKE.
    OutgoingResult startNegotiation(const QByteArray &ourName = {});

    // Apply plaintext/encryption policy to one outgoing application message.
    // peerInstance == 0 uses the most recently active encrypted child when
    // available. Always policy retains the last plaintext and emits a Query
    // Message until an encrypted child is established, matching libotr's
    // REQUIRE_ENCRYPTION last-message behavior.
    OutgoingResult prepareOutgoing(const QByteArray &plaintext,
                                   quint32 peerInstance = 0,
                                   const QByteArray &ourName = {},
                                   int maxMessageSize = 0,
                                   quint8 flags = 0);

    // peerInstance == 0 starts the OTRv3 broadcast/master AKE. Each remote
    // instance that answers receives an independent copy of that pending AKE
    // state, matching libotr's master/child context behavior.
    QVector<QByteArray> start(quint32 peerInstance = 0,
                              int maxMessageSize = 0,
                              bool *ok = nullptr);

    SessionResult processIncoming(const QByteArray &transportMessage,
                                  int maxMessageSize = 0);

    bool sendMessage(quint32 peerInstance,
                     const QByteArray &plaintext,
                     QVector<QByteArray> *transportMessages,
                     int maxMessageSize = 0,
                     quint8 flags = 0);
    bool sendMessage(quint32 peerInstance,
                     const QByteArray &plaintext,
                     const QVector<Tlv> &tlvs,
                     QVector<QByteArray> *transportMessages,
                     int maxMessageSize = 0,
                     quint8 flags = 0);

    bool isEncrypted(quint32 peerInstance) const;
    QVector<quint32> peerInstances() const;
    bool establishedSession(quint32 peerInstance, AkeEstablishedSession *established) const;
    void resetPeer(quint32 peerInstance);
    void reset();

private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace QcaOtr
