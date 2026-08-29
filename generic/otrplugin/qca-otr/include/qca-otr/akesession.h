#pragma once

#include "qca-otr/ake.h"

#include <QByteArray>
#include <QtGlobal>

namespace QcaOtr {

enum class AkeState {
    None,
    AwaitingDhKey,
    AwaitingRevealSignature,
    AwaitingSignature,
    Authenticated
};

enum class AkeHandleStatus {
    Ignored,
    Handled,
    Authenticated,
    Error
};

struct AkeEstablishedSession
{
    DhKeyPair       localDh;
    QCA::BigInteger peerDhPublic;
    quint32         localKeyId = 0;
    quint32         peerKeyId = 0;
    DsaPublicKey    peerIdentity;
    QByteArray      peerFingerprint;
    QByteArray      sessionId;
    bool            initiated = false;
};

struct AkeHandleResult
{
    AkeHandleStatus status = AkeHandleStatus::Ignored;
    QByteArray      outgoingMessage;
};

class AkeSession
{
public:
    AkeSession(const DsaPrivateKey &identityKey, quint32 localInstance, quint32 peerInstance = 0);

    AkeState state() const { return state_; }
    quint32 localInstance() const { return localInstance_; }
    quint32 peerInstance() const { return peerInstance_; }
    bool isAuthenticated() const { return state_ == AkeState::Authenticated; }
    const AkeEstablishedSession &established() const { return established_; }

    // A broadcast v3 Commit uses receiver instance 0. A router may copy that
    // pending AKE state for each remote instance that answers and bind the copy
    // before processing the instance-specific D-H Key. Existing bindings are
    // immutable so one child session can never migrate to another peer.
    bool bindPeerInstance(quint32 peerInstance);

    QByteArray start(bool *ok = nullptr);
    AkeHandleResult processIncoming(const QByteArray &encoded);
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
