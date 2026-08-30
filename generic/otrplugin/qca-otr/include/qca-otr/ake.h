/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "qca-otr/crypto.h"

#include <QByteArray>
#include <QtGlobal>

namespace QcaOtr {

/** Ephemeral Diffie-Hellman key pair used by the OTRv3 AKE. */
struct DhKeyPair
{
    QCA::BigInteger privateExponent;
    QCA::BigInteger publicValue;
};

/** Session and MAC/encryption keys derived from the AKE shared secret. */
struct AkeKeys
{
    QByteArray sessionId;
    QCA::SecureArray c;
    QCA::SecureArray cPrime;
    QCA::SecureArray m1;
    QCA::SecureArray m2;
    QCA::SecureArray m1Prime;
    QCA::SecureArray m2Prime;
};

/** Decrypted and verified AKE authenticator payload. */
struct AkeAuthenticator
{
    DsaPublicKey publicKey;
    quint32 keyId = 0;
    DsaSignature signature;
};

/** Returns the fixed OTRv3 Diffie-Hellman modulus. */
QCA::BigInteger dhModulus();

/** Returns the fixed OTRv3 Diffie-Hellman generator. */
QCA::BigInteger dhGenerator();

/** Validates a peer Diffie-Hellman public value against the OTR group. */
bool isValidDhPublicValue(const QCA::BigInteger &value);

/** Generates a fresh ephemeral Diffie-Hellman key pair. */
bool generateDhKeyPair(DhKeyPair *keyPair);

/** Computes and validates the Diffie-Hellman shared secret. */
bool computeDhSharedSecret(const QCA::BigInteger &privateExponent,
                           const QCA::BigInteger &peerPublicValue,
                           QCA::BigInteger *sharedSecret);

/** Derives the OTRv3 AKE key schedule from @p sharedSecret. */
bool deriveAkeKeys(const QCA::BigInteger &sharedSecret, AkeKeys *keys);

/** Computes the digest signed inside an OTRv3 AKE authenticator. */
QByteArray akeSignatureDigest(const QCA::BigInteger &firstDhPublic,
                              const QCA::BigInteger &secondDhPublic,
                              const DsaPublicKey &publicKey,
                              quint32 keyId,
                              const QCA::SecureArray &macKey);

/** Computes the authenticator MAC over the encrypted signature payload. */
QByteArray akeSignatureMac(const QByteArray &encryptedSignature,
                           const QCA::SecureArray &macKey);

/** Returns the canonical 20-byte OTR fingerprint of a DSA public key. */
QByteArray dsaPublicKeyFingerprint(const DsaPublicKey &publicKey);

/**
 * Creates and encrypts an OTRv3 AKE authenticator for @p identityKey.
 * The identity private key and derived symmetric keys remain in secure types.
 */
bool createAkeAuthenticator(const DsaPrivateKey &identityKey,
                            quint32 keyId,
                            const QCA::BigInteger &senderDhPublic,
                            const QCA::BigInteger &receiverDhPublic,
                            const QCA::SecureArray &macKey,
                            const QCA::SecureArray &encryptionKey,
                            QByteArray *encryptedAuthenticator);

/**
 * Decrypts and verifies a peer AKE authenticator.
 * @param fingerprint optionally receives the peer identity fingerprint.
 */
bool verifyAkeAuthenticator(const QByteArray &encryptedAuthenticator,
                            const QCA::BigInteger &senderDhPublic,
                            const QCA::BigInteger &receiverDhPublic,
                            const QCA::SecureArray &macKey,
                            const QCA::SecureArray &encryptionKey,
                            AkeAuthenticator *authenticator,
                            QByteArray *fingerprint = nullptr);

/** Decoded OTRv3 D-H Commit message. */
struct DhCommitMessage
{
    quint32 senderInstance = 0;
    quint32 receiverInstance = 0;
    QByteArray encryptedGx;
    QByteArray hashedGx;
};

/** Decoded OTRv3 D-H Key message. */
struct DhKeyMessage
{
    quint32 senderInstance = 0;
    quint32 receiverInstance = 0;
    QCA::BigInteger dhPublicValue;
};

/** Decoded OTRv3 Reveal Signature message. */
struct RevealSignatureMessage
{
    quint32 senderInstance = 0;
    quint32 receiverInstance = 0;
    QByteArray revealedKey;
    QByteArray encryptedSignature;
    QByteArray mac;
};

/** Decoded OTRv3 Signature message. */
struct SignatureMessage
{
    quint32 senderInstance = 0;
    quint32 receiverInstance = 0;
    QByteArray encryptedSignature;
    QByteArray mac;
};

namespace Wire {

/** Encodes a complete raw OTRv3 D-H Commit message. */
QByteArray encodeDhCommitMessage(const DhCommitMessage &message, bool *ok = nullptr);

/** Decodes and validates a complete raw OTRv3 D-H Commit message. */
bool decodeDhCommitMessage(const QByteArray &encoded, DhCommitMessage *message);

/** Encodes a complete raw OTRv3 D-H Key message. */
QByteArray encodeDhKeyMessage(const DhKeyMessage &message, bool *ok = nullptr);

/** Decodes and validates a complete raw OTRv3 D-H Key message. */
bool decodeDhKeyMessage(const QByteArray &encoded, DhKeyMessage *message);

/** Encodes a complete raw OTRv3 Reveal Signature message. */
QByteArray encodeRevealSignatureMessage(const RevealSignatureMessage &message, bool *ok = nullptr);

/** Decodes and validates a complete raw OTRv3 Reveal Signature message. */
bool decodeRevealSignatureMessage(const QByteArray &encoded, RevealSignatureMessage *message);

/** Encodes a complete raw OTRv3 Signature message. */
QByteArray encodeSignatureMessage(const SignatureMessage &message, bool *ok = nullptr);

/** Decodes and validates a complete raw OTRv3 Signature message. */
bool decodeSignatureMessage(const QByteArray &encoded, SignatureMessage *message);

} // namespace Wire
} // namespace QcaOtr
