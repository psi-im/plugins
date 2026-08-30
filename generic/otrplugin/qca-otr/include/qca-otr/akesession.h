/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "qca-otr/ake.h"

#include <QByteArray>
#include <QtGlobal>

namespace QcaOtr {

/** Current state of one OTRv3 authenticated key exchange. */
enum class AkeState {
    None,
    AwaitingDhKey,
    AwaitingRevealSignature,
    AwaitingSignature,
    Authenticated
};

/** Result category returned by AKE message processing. */
enum class AkeHandleStatus {
    Ignored,
    Handled,
    Authenticated,
    Error
};

/** Immutable protocol material exposed once an AKE authenticates successfully. */
struct AkeEstablishedSession
{
    DhKeyPair localDh;
    QCA::BigInteger peerDhPublic;
    quint32 localKeyId = 0;
    quint32 peerKeyId = 0;
    DsaPublicKey peerIdentity;
    QByteArray peerFingerprint;
    QByteArray sessionId;
    bool initiated = false;
};

/** Result of processing one raw AKE protocol message. */
struct AkeHandleResult
{
    AkeHandleStatus status = AkeHandleStatus::Ignored;
    QByteArray outgoingMessage;
};

/**
 * Stateful OTRv3 authenticated key exchange for one local/remote instance pair.
 *
 * `AkeSession` consumes and produces raw binary OTR messages. Transport armor,
 * fragmentation and multi-instance routing are handled by higher layers.
 */
class AkeSession
{
public:
    /**
     * Creates an AKE using @p identityKey and @p localInstance.
     * @p peerInstance may be zero for the broadcast/master AKE until a concrete
     * remote instance responds.
     */
    AkeSession(const DsaPrivateKey &identityKey, quint32 localInstance, quint32 peerInstance = 0);

    AkeState state() const { return state_; }
    quint32 localInstance() const { return localInstance_; }
    quint32 peerInstance() const { return peerInstance_; }
    bool isAuthenticated() const { return state_ == AkeState::Authenticated; }

    /** Valid only after @ref isAuthenticated returns true. */
    const AkeEstablishedSession &established() const { return established_; }

    /**
     * Binds a broadcast pending AKE to one concrete remote instance.
     *
     * A broadcast v3 Commit uses receiver instance 0. A router may copy that
     * pending state for every remote instance that answers and then bind each
     * copy before processing its D-H Key. Existing non-zero bindings are
     * immutable so one child session cannot migrate between peers.
     */
    bool bindPeerInstance(quint32 peerInstance);

    /** Starts the initiator side and returns a raw D-H Commit message. */
    QByteArray start(bool *ok = nullptr);

    /** Processes one complete raw AKE message and advances the state machine. */
    AkeHandleResult processIncoming(const QByteArray &encoded);

    /** Clears pending/authenticated state while retaining identity/instance configuration. */
    void reset();

private:
    bool acceptRoute(quint32 senderInstance, quint32 receiverInstance);
    bool beginAsResponder(const DhCommitMessage &message, QByteArray *outgoing);
    bool computeKeys(const QCA::BigInteger &peerPublic, AkeKeys *keys) const;
    bool makeRevealSignature(const QCA::BigInteger &peerPublic,
                             const AkeKeys &keys,
                             QByteArray *outgoing) const;
    bool makeSignature(const QCA::BigInteger &peerPublic,
                       const AkeKeys &keys,
                       QByteArray *outgoing) const;
    void clearPending();

    AkeHandleResult handleCommit(const DhCommitMessage &message);
    AkeHandleResult handleDhKey(const DhKeyMessage &message);
    AkeHandleResult handleRevealSignature(const RevealSignatureMessage &message);
    AkeHandleResult handleSignature(const SignatureMessage &message);

    DsaPrivateKey identityKey_;
    quint32 localInstance_ = 0;
    quint32 peerInstance_ = 0;

    AkeState state_ = AkeState::None;
    bool initiated_ = false;
    DhKeyPair localDh_;
    quint32 localKeyId_ = 0;
    QCA::BigInteger peerDhPublic_;
    QCA::SecureArray revealKey_;
    QByteArray encryptedGx_;
    QByteArray hashedGx_;
    AkeKeys keys_;
    QByteArray lastOutgoing_;
    AkeEstablishedSession established_;
};

} // namespace QcaOtr
