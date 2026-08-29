#pragma once

#include "qca-otr/crypto.h"

#include <QByteArray>
#include <QtGlobal>

namespace QcaOtr {

struct DhKeyPair
{
    QCA::BigInteger privateExponent;
    QCA::BigInteger publicValue;
};

struct AkeKeys
{
    QByteArray       sessionId;
    QCA::SecureArray c;
    QCA::SecureArray cPrime;
    QCA::SecureArray m1;
    QCA::SecureArray m2;
    QCA::SecureArray m1Prime;
    QCA::SecureArray m2Prime;
};

QCA::BigInteger dhModulus();
QCA::BigInteger dhGenerator();
bool isValidDhPublicValue(const QCA::BigInteger &value);
bool generateDhKeyPair(DhKeyPair *keyPair);
bool computeDhSharedSecret(const QCA::BigInteger &privateExponent,
                           const QCA::BigInteger &peerPublicValue,
                           QCA::BigInteger *sharedSecret);
bool deriveAkeKeys(const QCA::BigInteger &sharedSecret, AkeKeys *keys);

QByteArray akeSignatureDigest(const QCA::BigInteger &firstDhPublic,
                              const QCA::BigInteger &secondDhPublic,
                              const DsaPublicKey &publicKey,
                              quint32 keyId,
                              const QCA::SecureArray &macKey);
QByteArray akeSignatureMac(const QByteArray &encryptedSignature,
                           const QCA::SecureArray &macKey);

struct DhCommitMessage
{
    quint32    senderInstance = 0;
    quint32    receiverInstance = 0;
    QByteArray encryptedGx;
    QByteArray hashedGx;
};

struct DhKeyMessage
{
    quint32         senderInstance = 0;
    quint32         receiverInstance = 0;
    QCA::BigInteger dhPublicValue;
};

struct RevealSignatureMessage
{
    quint32    senderInstance = 0;
    quint32    receiverInstance = 0;
    QByteArray revealedKey;
    QByteArray encryptedSignature;
    QByteArray mac;
};

struct SignatureMessage
{
    quint32    senderInstance = 0;
    quint32    receiverInstance = 0;
    QByteArray encryptedSignature;
    QByteArray mac;
};

namespace Wire {

QByteArray encodeDhCommitMessage(const DhCommitMessage &message, bool *ok = nullptr);
bool decodeDhCommitMessage(const QByteArray &encoded, DhCommitMessage *message);

QByteArray encodeDhKeyMessage(const DhKeyMessage &message, bool *ok = nullptr);
bool decodeDhKeyMessage(const QByteArray &encoded, DhKeyMessage *message);

QByteArray encodeRevealSignatureMessage(const RevealSignatureMessage &message, bool *ok = nullptr);
bool decodeRevealSignatureMessage(const QByteArray &encoded, RevealSignatureMessage *message);

QByteArray encodeSignatureMessage(const SignatureMessage &message, bool *ok = nullptr);
bool decodeSignatureMessage(const QByteArray &encoded, SignatureMessage *message);

} // namespace Wire
} // namespace QcaOtr
