#include "qca-otr/akesession.h"

#include "qca-otr/codec.h"

#include <cstring>

namespace QcaOtr {
namespace {

constexpr quint32 MinimumInstanceTag = 0x00000100;

bool constantTimeEqual(const QByteArray &a, const QByteArray &b)
{
    if (a.size() != b.size())
        return false;

    unsigned char difference = 0;
    for (int i = 0; i < a.size(); ++i)
        difference |= static_cast<unsigned char>(a.at(i)) ^ static_cast<unsigned char>(b.at(i));
    return difference == 0;
}

AkeHandleResult errorResult()
{
    AkeHandleResult result;
    result.status = AkeHandleStatus::Error;
    return result;
}

AkeHandleResult ignoredResult()
{
    return {};
}

AkeHandleResult handledResult(const QByteArray &outgoing = {})
{
    AkeHandleResult result;
    result.status = AkeHandleStatus::Handled;
    result.outgoingMessage = outgoing;
    return result;
}

AkeHandleResult authenticatedResult(const QByteArray &outgoing = {})
{
    AkeHandleResult result;
    result.status = AkeHandleStatus::Authenticated;
    result.outgoingMessage = outgoing;
    return result;
}

} // namespace

AkeSession::AkeSession(const DsaPrivateKey &identityKey, quint32 localInstance, quint32 peerInstance) :
    identityKey_(identityKey), localInstance_(localInstance), peerInstance_(peerInstance)
{
}

bool AkeSession::bindPeerInstance(quint32 peerInstance)
{
    if (peerInstance < MinimumInstanceTag)
        return false;
    if (peerInstance_ != 0 && peerInstance_ != peerInstance)
        return false;

    peerInstance_ = peerInstance;
    return true;
}

void AkeSession::clearPending()
{
    initiated_ = false;
    localDh_ = {};
    localKeyId_ = 0;
    peerDhPublic_ = QCA::BigInteger(0);
    revealKey_.clear();
    encryptedGx_.clear();
    hashedGx_.clear();
    keys_ = {};
    lastOutgoing_.clear();
}

void AkeSession::reset()
{
    clearPending();
    established_ = {};
    state_ = AkeState::None;
}

bool AkeSession::acceptRoute(quint32 senderInstance, quint32 receiverInstance)
{
    if (senderInstance < MinimumInstanceTag ||
        (receiverInstance != 0 && receiverInstance != localInstance_)) {
        return false;
    }
    if (peerInstance_ != 0 && senderInstance != peerInstance_)
        return false;
    if (peerInstance_ == 0)
        peerInstance_ = senderInstance;
    return true;
}

QByteArray AkeSession::start(bool *ok)
{
    if (ok)
        *ok = false;
    if (localInstance_ < MinimumInstanceTag || (peerInstance_ != 0 && peerInstance_ < MinimumInstanceTag))
        return {};

    clearPending();
    established_ = {};
    state_ = AkeState::None;

    DhKeyPair keyPair;
    if (!generateDhKeyPair(&keyPair))
        return {};

    const QCA::SecureArray randomKey = QCA::Random::randomArray(16);
    if (randomKey.size() != 16)
        return {};

    bool mpiOk = false;
    const QByteArray clearGx = Wire::encodeMpi(keyPair.publicValue, &mpiOk);
    if (!mpiOk)
        return {};

    const QByteArray hashGx = sha256(clearGx);
    if (hashGx.size() != 32)
        return {};

    QByteArray encryptedGx;
    if (!aes128Ctr(randomKey, QByteArray(16, '\0'), clearGx, &encryptedGx))
        return {};

    DhCommitMessage commit;
    commit.senderInstance = localInstance_;
    commit.receiverInstance = peerInstance_;
    commit.encryptedGx = encryptedGx;
    commit.hashedGx = hashGx;

    bool encodedOk = false;
    const QByteArray encoded = Wire::encodeDhCommitMessage(commit, &encodedOk);
    if (!encodedOk)
        return {};

    initiated_ = true;
    localDh_ = keyPair;
    localKeyId_ = 1;
    revealKey_ = randomKey.toByteArray();
    encryptedGx_ = encryptedGx;
    hashedGx_ = hashGx;
    lastOutgoing_ = encoded;
    state_ = AkeState::AwaitingDhKey;

    if (ok)
        *ok = true;
    return encoded;
}

bool AkeSession::beginAsResponder(const DhCommitMessage &message, QByteArray *outgoing)
{
    if (!outgoing)
        return false;

    DhKeyPair keyPair;
    if (!generateDhKeyPair(&keyPair))
        return false;

    DhKeyMessage keyMessage;
    keyMessage.senderInstance = localInstance_;
    keyMessage.receiverInstance = peerInstance_;
    keyMessage.dhPublicValue = keyPair.publicValue;

    bool ok = false;
    const QByteArray encoded = Wire::encodeDhKeyMessage(keyMessage, &ok);
    if (!ok)
        return false;

    clearPending();
    established_ = {};
    initiated_ = false;
    localDh_ = keyPair;
    localKeyId_ = 1;
    encryptedGx_ = message.encryptedGx;
    hashedGx_ = message.hashedGx;
    lastOutgoing_ = encoded;
    state_ = AkeState::AwaitingRevealSignature;
    *outgoing = encoded;
    return true;
}

bool AkeSession::computeKeys(const QCA::BigInteger &peerPublic, AkeKeys *keys) const
{
    QCA::BigInteger sharedSecret;
    return computeDhSharedSecret(localDh_.privateExponent, peerPublic, &sharedSecret) &&
        deriveAkeKeys(sharedSecret, keys);
}

bool AkeSession::makeRevealSignature(const QCA::BigInteger &peerPublic,
                                     const AkeKeys &keys,
                                     QByteArray *outgoing) const
{
    if (!outgoing || revealKey_.size() != 16)
        return false;

    QByteArray encryptedAuthenticator;
    if (!createAkeAuthenticator(identityKey_,
                                localKeyId_,
                                localDh_.publicValue,
                                peerPublic,
                                keys.m1,
                                keys.c,
                                &encryptedAuthenticator)) {
        return false;
    }

    RevealSignatureMessage reveal;
    reveal.senderInstance = localInstance_;
    reveal.receiverInstance = peerInstance_;
    reveal.revealedKey = revealKey_;
    reveal.encryptedSignature = encryptedAuthenticator;
    reveal.mac = akeSignatureMac(encryptedAuthenticator, keys.m2);
    if (reveal.mac.size() != 20)
        return false;

    bool ok = false;
    *outgoing = Wire::encodeRevealSignatureMessage(reveal, &ok);
    return ok;
}

bool AkeSession::makeSignature(const QCA::BigInteger &peerPublic,
                               const AkeKeys &keys,
                               QByteArray *outgoing) const
{
    if (!outgoing)
        return false;

    QByteArray encryptedAuthenticator;
    if (!createAkeAuthenticator(identityKey_,
                                localKeyId_,
                                localDh_.publicValue,
                                peerPublic,
                                keys.m1Prime,
                                keys.cPrime,
                                &encryptedAuthenticator)) {
        return false;
    }

    SignatureMessage signature;
    signature.senderInstance = localInstance_;
    signature.receiverInstance = peerInstance_;
    signature.encryptedSignature = encryptedAuthenticator;
    signature.mac = akeSignatureMac(encryptedAuthenticator, keys.m2Prime);
    if (signature.mac.size() != 20)
        return false;

    bool ok = false;
    *outgoing = Wire::encodeSignatureMessage(signature, &ok);
    return ok;
}

AkeHandleResult AkeSession::handleCommit(const DhCommitMessage &message)
{
    if (!acceptRoute(message.senderInstance, message.receiverInstance))
        return ignoredResult();

    switch (state_) {
    case AkeState::None:
    case AkeState::Authenticated:
    case AkeState::AwaitingSignature: {
        QByteArray outgoing;
        return beginAsResponder(message, &outgoing) ? handledResult(outgoing) : errorResult();
    }

    case AkeState::AwaitingDhKey:
        if (hashedGx_.size() != 32)
            return errorResult();
        // This is the simultaneous-initiation tie-break used by libotr. The
        // larger public commitment hash wins and retransmits its original
        // commit; the loser discards its initiation and becomes responder.
        if (std::memcmp(hashedGx_.constData(), message.hashedGx.constData(), 32) > 0)
            return handledResult(lastOutgoing_);
        else {
            QByteArray outgoing;
            return beginAsResponder(message, &outgoing) ? handledResult(outgoing) : errorResult();
        }

    case AkeState::AwaitingRevealSignature:
        // libotr accepts a replacement commit here while keeping the same
        // responder DH key and retransmitting the exact previous D-H Key.
        encryptedGx_ = message.encryptedGx;
        hashedGx_ = message.hashedGx;
        return handledResult(lastOutgoing_);
    }

    return ignoredResult();
}

AkeHandleResult AkeSession::handleDhKey(const DhKeyMessage &message)
{
    if (!acceptRoute(message.senderInstance, message.receiverInstance))
        return ignoredResult();

    if (state_ == AkeState::AwaitingSignature) {
        if (message.dhPublicValue == peerDhPublic_)
            return handledResult(lastOutgoing_);
        return ignoredResult();
    }

    if (state_ != AkeState::AwaitingDhKey)
        return ignoredResult();

    AkeKeys derived;
    if (!computeKeys(message.dhPublicValue, &derived))
        return errorResult();

    QByteArray outgoing;
    if (!makeRevealSignature(message.dhPublicValue, derived, &outgoing))
        return errorResult();

    peerDhPublic_ = message.dhPublicValue;
    keys_ = derived;
    lastOutgoing_ = outgoing;
    state_ = AkeState::AwaitingSignature;
    return handledResult(outgoing);
}

AkeHandleResult AkeSession::handleRevealSignature(const RevealSignatureMessage &message)
{
    if (!acceptRoute(message.senderInstance, message.receiverInstance))
        return ignoredResult();
    if (state_ != AkeState::AwaitingRevealSignature)
        return ignoredResult();

    QByteArray clearGx;
    if (!aes128Ctr(QCA::SecureArray(message.revealedKey),
                   QByteArray(16, '\0'),
                   encryptedGx_,
                   &clearGx)) {
        return errorResult();
    }

    const QByteArray clearHash = sha256(clearGx);
    if (clearHash.size() != 32 || !constantTimeEqual(clearHash, hashedGx_))
        return errorResult();

    QCA::BigInteger peerPublic;
    if (!Wire::decodeMpi(clearGx, &peerPublic) || !isValidDhPublicValue(peerPublic))
        return errorResult();

    AkeKeys derived;
    if (!computeKeys(peerPublic, &derived))
        return errorResult();

    const QByteArray expectedMac = akeSignatureMac(message.encryptedSignature, derived.m2);
    if (expectedMac.size() != 20 || !constantTimeEqual(expectedMac, message.mac))
        return errorResult();

    AkeAuthenticator peerAuthenticator;
    QByteArray peerFingerprint;
    if (!verifyAkeAuthenticator(message.encryptedSignature,
                                peerPublic,
                                localDh_.publicValue,
                                derived.m1,
                                derived.c,
                                &peerAuthenticator,
                                &peerFingerprint)) {
        return errorResult();
    }

    QByteArray outgoing;
    if (!makeSignature(peerPublic, derived, &outgoing))
        return errorResult();

    peerDhPublic_ = peerPublic;
    keys_ = derived;
    lastOutgoing_ = outgoing;

    established_.localDh = localDh_;
    established_.peerDhPublic = peerPublic;
    established_.localKeyId = localKeyId_;
    established_.peerKeyId = peerAuthenticator.keyId;
    established_.peerIdentity = peerAuthenticator.publicKey;
    established_.peerFingerprint = peerFingerprint;
    established_.sessionId = derived.sessionId;
    established_.initiated = false;

    state_ = AkeState::Authenticated;
    return authenticatedResult(outgoing);
}

AkeHandleResult AkeSession::handleSignature(const SignatureMessage &message)
{
    if (!acceptRoute(message.senderInstance, message.receiverInstance))
        return ignoredResult();
    if (state_ != AkeState::AwaitingSignature)
        return ignoredResult();

    const QByteArray expectedMac = akeSignatureMac(message.encryptedSignature, keys_.m2Prime);
    if (expectedMac.size() != 20 || !constantTimeEqual(expectedMac, message.mac))
        return errorResult();

    AkeAuthenticator peerAuthenticator;
    QByteArray peerFingerprint;
    if (!verifyAkeAuthenticator(message.encryptedSignature,
                                peerDhPublic_,
                                localDh_.publicValue,
                                keys_.m1Prime,
                                keys_.cPrime,
                                &peerAuthenticator,
                                &peerFingerprint)) {
        return errorResult();
    }

    established_.localDh = localDh_;
    established_.peerDhPublic = peerDhPublic_;
    established_.localKeyId = localKeyId_;
    established_.peerKeyId = peerAuthenticator.keyId;
    established_.peerIdentity = peerAuthenticator.publicKey;
    established_.peerFingerprint = peerFingerprint;
    established_.sessionId = keys_.sessionId;
    established_.initiated = true;

    state_ = AkeState::Authenticated;
    return authenticatedResult();
}

AkeHandleResult AkeSession::processIncoming(const QByteArray &encoded)
{
    if (encoded.size() < 3)
        return errorResult();

    const quint8 type = static_cast<quint8>(encoded.at(2));
    switch (type) {
    case 0x02: {
        DhCommitMessage message;
        return Wire::decodeDhCommitMessage(encoded, &message) ? handleCommit(message) : errorResult();
    }
    case 0x0a: {
        DhKeyMessage message;
        return Wire::decodeDhKeyMessage(encoded, &message) ? handleDhKey(message) : errorResult();
    }
    case 0x11: {
        RevealSignatureMessage message;
        return Wire::decodeRevealSignatureMessage(encoded, &message) ? handleRevealSignature(message) : errorResult();
    }
    case 0x12: {
        SignatureMessage message;
        return Wire::decodeSignatureMessage(encoded, &message) ? handleSignature(message) : errorResult();
    }
    default:
        return ignoredResult();
    }
}

} // namespace QcaOtr
