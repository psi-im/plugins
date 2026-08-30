/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "qca-otr/akesession.h"
#include "qca-otr/tlv.h"

#include <QByteArray>
#include <QVector>
#include <QtGlobal>

#include <memory>

namespace QcaOtr {

/** OTR Data Message flag requesting silent handling of unreadable messages. */
constexpr quint8 DataFlagIgnoreUnreadable = 0x01;

/** Application policy controlling when OTR negotiation/encryption is attempted. */
enum class SessionPolicy {
    Disabled,
    Manual,
    Opportunistic,
    Always
};

/** Current OTR state of one concrete remote instance. */
enum class PeerState {
    Plaintext,
    Encrypted,
    Finished
};

/** High-level result of processing one incoming transport message. */
enum class SessionStatus {
    Ignored,
    Plaintext,
    ProtocolMessage,
    FragmentIncomplete,
    Handled,
    Authenticated,
    Message,
    Disconnected,
    Unreadable,
    RemoteError,
    ForOtherInstance,
    Error
};

/** Application-visible SMP event produced while processing incoming traffic. */
enum class SmpEvent {
    None,
    Error,
    Abort,
    Cheated,
    AskForAnswer,
    AskForSecret,
    InProgress,
    Success,
    Failure
};

/**
 * Result of processing one incoming OTR transport message.
 *
 * `plaintext` and `tlvs` are populated for an authenticated Data Message.
 * Secret key material (`extraKey`, `symmetricKey`) remains in
 * `QCA::SecureArray`. `outgoingMessages` contains protocol responses that the
 * caller must transmit in order.
 */
struct SessionResult
{
    SessionStatus status = SessionStatus::Ignored;
    quint32 peerInstance = 0;
    QByteArray plaintext;
    QVector<Tlv> tlvs;
    QCA::SecureArray extraKey;
    QByteArray errorText;
    quint8 flags = 0;

    bool hasSymmetricKey = false;
    quint32 symmetricKeyUse = 0;
    QByteArray symmetricKeyData;
    QCA::SecureArray symmetricKey;

    SmpEvent smpEvent = SmpEvent::None;
    quint16 smpProgress = 0;
    QByteArray smpQuestion;

    QVector<QByteArray> outgoingMessages;
};

/** High-level result of preparing one outgoing application message. */
enum class OutgoingStatus {
    Plaintext,
    Encrypted,
    Negotiation,
    Finished,
    Error
};

/** Transport messages emitted while applying the current outgoing policy. */
struct OutgoingResult
{
    OutgoingStatus status = OutgoingStatus::Error;
    quint32 peerInstance = 0;
    QVector<QByteArray> messages;
};

/**
 * Transport-facing OTRv3 session router for one logical correspondent.
 *
 * One `OtrSession` owns the local instance and any number of remote-instance
 * children. The underlying AKE/data/SMP engines operate on raw binary OTR
 * messages; this layer owns negotiation policy, ASCII armor, fragmentation,
 * instance routing and control TLVs.
 */
class OtrSession
{
public:
    /**
     * Creates a session router for one local OTR identity and instance tag.
     * @p localInstance must be a valid non-zero OTRv3 instance tag.
     */
    OtrSession(const DsaPrivateKey &identityKey, quint32 localInstance);
    ~OtrSession();

    OtrSession(const OtrSession &) = delete;
    OtrSession &operator=(const OtrSession &) = delete;
    OtrSession(OtrSession &&) noexcept;
    OtrSession &operator=(OtrSession &&) noexcept;

    quint32 localInstance() const;

    /** Changes the policy used by @ref prepareOutgoing and incoming negotiation. */
    void setPolicy(SessionPolicy policy);
    SessionPolicy policy() const;

    /**
     * Builds the user-visible Query Message for an explicit manual start.
     * Receiving a compatible query starts the broadcast OTRv3 AKE.
     */
    OutgoingResult startNegotiation(const QByteArray &ourName = {});

    /**
     * Applies the current policy to one outgoing application message.
     *
     * @param peerInstance remote instance to target. Zero selects the most
     * recently active encrypted child when possible.
     * @param ourName human-readable local name placed in a Query Message.
     * @param maxMessageSize zero disables fragmentation; positive values bound
     * each transport message.
     * @param flags OTR Data Message flags when encryption is possible.
     *
     * `SessionPolicy::Always` retains the last plaintext and emits a Query
     * Message until an encrypted child is established, matching libotr's
     * REQUIRE_ENCRYPTION last-message behavior. A Finished child suppresses
     * application traffic until a new AKE establishes private state again.
     */
    OutgoingResult prepareOutgoing(const QByteArray &plaintext,
                                   quint32 peerInstance = 0,
                                   const QByteArray &ourName = {},
                                   int maxMessageSize = 0,
                                   quint8 flags = 0);

    /**
     * Starts a raw OTRv3 AKE and returns transport-ready protocol messages.
     *
     * @p peerInstance equal to zero starts the broadcast/master AKE. Each
     * remote instance that answers receives an independent copy of that pending
     * state, matching libotr's master/child context behavior.
     */
    QVector<QByteArray> start(quint32 peerInstance = 0,
                              int maxMessageSize = 0,
                              bool *ok = nullptr);

    /**
     * Processes one incoming plaintext/protocol/fragment transport message.
     * Any required replies are returned in `SessionResult::outgoingMessages`.
     */
    SessionResult processIncoming(const QByteArray &transportMessage,
                                  int maxMessageSize = 0);

    /** Encrypts one plaintext message for a concrete encrypted peer instance. */
    bool sendMessage(quint32 peerInstance,
                     const QByteArray &plaintext,
                     QVector<QByteArray> *transportMessages,
                     int maxMessageSize = 0,
                     quint8 flags = 0);

    /** Encrypts plaintext plus explicit OTR TLVs for a concrete peer instance. */
    bool sendMessage(quint32 peerInstance,
                     const QByteArray &plaintext,
                     const QVector<Tlv> &tlvs,
                     QVector<QByteArray> *transportMessages,
                     int maxMessageSize = 0,
                     quint8 flags = 0);

    /**
     * Disconnects one concrete remote instance.
     *
     * If encrypted, sends an empty Data Message carrying the DISCONNECTED TLV
     * and IGNORE_UNREADABLE flag, then returns that child to Plaintext. Unknown
     * or already-plain children are successful no-ops, matching
     * `otrl_message_disconnect()` behavior.
     */
    bool disconnect(quint32 peerInstance,
                    QVector<QByteArray> *transportMessages,
                    int maxMessageSize = 0);

    /**
     * Sends an OTR type-8 symmetric-key TLV.
     *
     * The TLV contains a 32-bit @p use identifier followed by opaque
     * @p useData. @p symmetricKey receives the exact extra key used by the
     * encrypted Data Message and remains in secure memory.
     */
    bool sendSymmetricKey(quint32 peerInstance,
                          quint32 use,
                          const QByteArray &useData,
                          QCA::SecureArray *symmetricKey,
                          QVector<QByteArray> *transportMessages,
                          int maxMessageSize = 0);

    /**
     * Starts SMP without a question using the user-entered secret.
     *
     * The libotr-compatible combined secret is derived internally from ordered
     * fingerprints, secure session id and @p secret. `peerInstance == 0`
     * selects the current encrypted child when possible.
     */
    bool startSmp(quint32 peerInstance,
                  const QCA::SecureArray &secret,
                  QVector<QByteArray> *transportMessages,
                  int maxMessageSize = 0);

    /** Starts SMP with a human-readable @p question. */
    bool startSmp(quint32 peerInstance,
                  const QByteArray &question,
                  const QCA::SecureArray &secret,
                  QVector<QByteArray> *transportMessages,
                  int maxMessageSize = 0);

    /** Responds to a pending SMP challenge using the user-entered secret. */
    bool respondSmp(quint32 peerInstance,
                    const QCA::SecureArray &secret,
                    QVector<QByteArray> *transportMessages,
                    int maxMessageSize = 0);

    /** Sends an SMP abort for the selected encrypted peer when applicable. */
    bool abortSmp(quint32 peerInstance,
                  QVector<QByteArray> *transportMessages,
                  int maxMessageSize = 0);

    /** Returns the state of @p peerInstance, or Plaintext for an unknown peer. */
    PeerState peerState(quint32 peerInstance) const;

    /** Returns true only when @p peerInstance currently has authenticated data keys. */
    bool isEncrypted(quint32 peerInstance) const;

    /** Returns all concrete remote instances known to this router. */
    QVector<quint32> peerInstances() const;

    /** Copies authenticated AKE material for @p peerInstance into @p established. */
    bool establishedSession(quint32 peerInstance, AkeEstablishedSession *established) const;

    /** Drops all protocol state belonging to one concrete remote instance. */
    void resetPeer(quint32 peerInstance);

    /** Drops all remote-instance, negotiation, fragment and pending-message state. */
    void reset();

private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace QcaOtr
