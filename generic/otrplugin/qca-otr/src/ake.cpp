/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#include "qca-otr/ake.h"

#include "qca-otr/codec.h"

#include <cstring>

namespace QcaOtr {
namespace {

constexpr quint16 OtrV3Version = 0x0003;
constexpr quint8 DhCommitType = 0x02;
constexpr quint8 DhKeyType = 0x0a;
constexpr quint8 RevealSignatureType = 0x11;
constexpr quint8 SignatureType = 0x12;
constexpr quint32 MinimumInstanceTag = 0x00000100;
constexpr int DsaSignatureComponentSize = 20;
constexpr int DsaSignatureSize = 40;

QCA::BigInteger zero()
{
    return QCA::BigInteger(0);
}

QCA::BigInteger two()
{
    return QCA::BigInteger(2);
}

QCA::BigInteger unsignedInteger(const QByteArray &bytes)
{
    QByteArray positive;
    positive.reserve(bytes.size() + 1);
    positive.append('\0');
    positive.append(bytes);
    return QCA::BigInteger(QCA::SecureArray(positive));
}

QCA::BigInteger unsignedInteger(const QCA::SecureArray &bytes)
{
    QCA::SecureArray positive(1, '\0');
    positive.append(bytes);
    return QCA::BigInteger(positive);
}

QCA::SecureArray secureSlice(const QCA::SecureArray &value, int offset, int length)
{
    if (offset < 0 || length < 0 || offset > value.size() || length > value.size() - offset)
        return {};

    QCA::SecureArray result(length);
    if (length > 0)
        std::memcpy(result.data(), value.constData() + offset, static_cast<size_t>(length));
    return result;
}

bool fixedWidthDsaInteger(const QCA::BigInteger &value, QByteArray *encoded)
{
    if (!encoded || value < zero())
        return false;

    QByteArray bytes = value.toArray().toByteArray();
    while (!bytes.isEmpty() && bytes.front() == '\0')
        bytes.remove(0, 1);
    if (bytes.size() > DsaSignatureComponentSize)
        return false;

    *encoded = QByteArray(DsaSignatureComponentSize - bytes.size(), '\0') + bytes;
    return true;
}

bool encodeDsaSignature(const DsaSignature &signature, QByteArray *encoded)
{
    if (!encoded)
        return false;

    QByteArray r;
    QByteArray s;
    if (!fixedWidthDsaInteger(signature.r, &r) || !fixedWidthDsaInteger(signature.s, &s))
        return false;

    *encoded = r + s;
    return true;
}

bool decodeDsaSignature(const QByteArray &encoded, DsaSignature *signature)
{
    if (!signature || encoded.size() != DsaSignatureSize)
        return false;

    DsaSignature decoded;
    decoded.r = unsignedInteger(encoded.left(DsaSignatureComponentSize));
    decoded.s = unsignedInteger(encoded.mid(DsaSignatureComponentSize, DsaSignatureComponentSize));
    *signature = decoded;
    return true;
}

bool validInstanceTags(quint32 sender, quint32 receiver)
{
    return sender >= MinimumInstanceTag && (receiver == 0 || receiver >= MinimumInstanceTag);
}

void writeHeader(Wire::Writer *writer, quint8 type, quint32 sender, quint32 receiver)
{
    writer->writeShort(OtrV3Version);
    writer->writeByte(type);
    writer->writeInt(sender);
    writer->writeInt(receiver);
}

bool readHeader(Wire::Reader *reader, quint8 expectedType, quint32 *sender, quint32 *receiver)
{
    quint16 version = 0;
    quint8 type = 0;
    if (!reader->readShort(&version) || !reader->readByte(&type) || version != OtrV3Version ||
        type != expectedType || !reader->readInt(sender) || !reader->readInt(receiver)) {
        return false;
    }
    return validInstanceTags(*sender, *receiver);
}

QCA::SecureArray h2(quint8 selector, const QCA::SecureArray &secbytes)
{
    QCA::SecureArray input(secbytes.size() + 1);
    input[0] = static_cast<char>(selector);
    if (!secbytes.isEmpty())
        std::memcpy(input.data() + 1, secbytes.constData(), static_cast<size_t>(secbytes.size()));
    return sha256Secure(input);
}

} // namespace

QCA::BigInteger dhModulus()
{
    return unsignedInteger(QByteArray::fromHex(
        "ffffffffffffffffc90fdaa22168c234c4c6628b80dc1cd1"
        "29024e088a67cc74020bbea63b139b22514a08798e3404dd"
        "ef9519b3cd3a431b302b0a6df25f14374fe1356d6d51c245"
        "e485b576625e7ec6f44c42e9a637ed6b0bff5cb6f406b7ed"
        "ee386bfb5a899fa5ae9f24117c4b1fe649286651ece45b3d"
        "c2007cb8a163bf0598da48361c55d39a69163fa8fd24cf5f"
        "83655d23dca3ad961c62f356208552bb9ed529077096966d"
        "670c354e4abc9804f1746c08ca237327ffffffffffffffff"));
}

QCA::BigInteger dhGenerator()
{
    return two();
}

bool isValidDhPublicValue(const QCA::BigInteger &value)
{
    QCA::BigInteger maximum = dhModulus();
    maximum -= two();
    return value >= two() && value <= maximum;
}

bool generateDhKeyPair(DhKeyPair *keyPair)
{
    if (!keyPair)
        return false;

    // libotr generates exactly 40 random bytes and interprets them as an
    // unsigned 320-bit exponent. Do the same for byte-for-byte protocol
    // compatibility instead of biasing the value through a modulus.
    for (int attempt = 0; attempt < 16; ++attempt) {
        const QCA::SecureArray random = QCA::Random::randomArray(40);
        if (random.size() != 40)
            return false;

        const QCA::BigInteger privateExponent = unsignedInteger(random);
        if (privateExponent <= zero())
            continue;

        const QCA::BigInteger publicValue =
            QCA::BigIntegerMath::modPow(dhGenerator(), privateExponent, dhModulus());
        if (!isValidDhPublicValue(publicValue))
            return false;

        keyPair->privateExponent = privateExponent;
        keyPair->publicValue = publicValue;
        return true;
    }

    return false;
}

bool computeDhSharedSecret(const QCA::BigInteger &privateExponent,
                           const QCA::BigInteger &peerPublicValue,
                           QCA::BigInteger *sharedSecret)
{
    if (!sharedSecret || privateExponent <= zero() || !isValidDhPublicValue(peerPublicValue))
        return false;

    const QCA::BigInteger secret =
        QCA::BigIntegerMath::modPow(peerPublicValue, privateExponent, dhModulus());
    if (secret <= zero())
        return false;

    *sharedSecret = secret;
    return true;
}

bool deriveAkeKeys(const QCA::BigInteger &sharedSecret, AkeKeys *keys)
{
    if (!keys || sharedSecret <= zero())
        return false;

    bool ok = false;
    const QCA::SecureArray secbytes = Wire::encodeMpiSecure(sharedSecret, &ok);
    if (!ok)
        return false;

    const QCA::SecureArray h0 = h2(0x00, secbytes);
    const QCA::SecureArray h1 = h2(0x01, secbytes);
    const QCA::SecureArray hM1 = h2(0x02, secbytes);
    const QCA::SecureArray hM2 = h2(0x03, secbytes);
    const QCA::SecureArray hM1Prime = h2(0x04, secbytes);
    const QCA::SecureArray hM2Prime = h2(0x05, secbytes);
    if (h0.size() != 32 || h1.size() != 32 || hM1.size() != 32 || hM2.size() != 32 ||
        hM1Prime.size() != 32 || hM2Prime.size() != 32) {
        return false;
    }

    AkeKeys derived;
    derived.sessionId = QByteArray(h0.constData(), 8);
    derived.c = secureSlice(h1, 0, 16);
    derived.cPrime = secureSlice(h1, 16, 16);
    derived.m1 = hM1;
    derived.m2 = hM2;
    derived.m1Prime = hM1Prime;
    derived.m2Prime = hM2Prime;
    *keys = derived;
    return true;
}

QByteArray akeSignatureDigest(const QCA::BigInteger &firstDhPublic,
                              const QCA::BigInteger &secondDhPublic,
                              const DsaPublicKey &publicKey,
                              quint32 keyId,
                              const QCA::SecureArray &macKey)
{
    if (keyId == 0 || macKey.size() != 32)
        return {};

    Wire::Writer writer;
    if (!writer.writeMpi(firstDhPublic) || !writer.writeMpi(secondDhPublic) ||
        !writer.writeDsaPublicKey(publicKey)) {
        return {};
    }
    writer.writeInt(keyId);
    return hmacSha256(macKey, writer.data());
}

QByteArray akeSignatureMac(const QByteArray &encryptedSignature,
                           const QCA::SecureArray &macKey)
{
    if (macKey.size() != 32)
        return {};

    Wire::Writer writer;
    writer.writeData(encryptedSignature);
    const QByteArray fullMac = hmacSha256(macKey, writer.data());
    return fullMac.size() == 32 ? fullMac.left(20) : QByteArray();
}

QByteArray dsaPublicKeyFingerprint(const DsaPublicKey &publicKey)
{
    // libotr fingerprints only the four encoded DSA MPIs, excluding the
    // two-byte OTR public-key type field.
    Wire::Writer writer;
    if (!writer.writeMpi(publicKey.domain.p) || !writer.writeMpi(publicKey.domain.q) ||
        !writer.writeMpi(publicKey.domain.g) || !writer.writeMpi(publicKey.y)) {
        return {};
    }
    const QByteArray fingerprint = sha1(writer.data());
    return fingerprint.size() == 20 ? fingerprint : QByteArray();
}

bool createAkeAuthenticator(const DsaPrivateKey &identityKey,
                            quint32 keyId,
                            const QCA::BigInteger &senderDhPublic,
                            const QCA::BigInteger &receiverDhPublic,
                            const QCA::SecureArray &macKey,
                            const QCA::SecureArray &encryptionKey,
                            QByteArray *encryptedAuthenticator)
{
    if (!encryptedAuthenticator || keyId == 0 || !isValidDhPublicValue(senderDhPublic) ||
        !isValidDhPublicValue(receiverDhPublic) || macKey.size() != 32 || encryptionKey.size() != 16) {
        return false;
    }

    const DsaPublicKey publicKey = dsaPublicKey(identityKey);
    const QByteArray digest = akeSignatureDigest(senderDhPublic, receiverDhPublic, publicKey, keyId, macKey);
    if (digest.size() != 32)
        return false;

    DsaSignature signature;
    if (!dsaSignDigest(identityKey, digest, &signature))
        return false;

    QByteArray encodedSignature;
    if (!encodeDsaSignature(signature, &encodedSignature))
        return false;

    Wire::Writer writer;
    if (!writer.writeDsaPublicKey(publicKey))
        return false;
    writer.writeInt(keyId);
    writer.writeBytes(encodedSignature);

    QByteArray encrypted;
    if (!aes128Ctr(encryptionKey, QByteArray(16, '\0'), writer.data(), &encrypted))
        return false;

    *encryptedAuthenticator = encrypted;
    return true;
}

bool verifyAkeAuthenticator(const QByteArray &encryptedAuthenticator,
                            const QCA::BigInteger &senderDhPublic,
                            const QCA::BigInteger &receiverDhPublic,
                            const QCA::SecureArray &macKey,
                            const QCA::SecureArray &encryptionKey,
                            AkeAuthenticator *authenticator,
                            QByteArray *fingerprint)
{
    if (!authenticator || encryptedAuthenticator.isEmpty() || !isValidDhPublicValue(senderDhPublic) ||
        !isValidDhPublicValue(receiverDhPublic) || macKey.size() != 32 || encryptionKey.size() != 16) {
        return false;
    }

    QByteArray cleartext;
    if (!aes128Ctr(encryptionKey, QByteArray(16, '\0'), encryptedAuthenticator, &cleartext))
        return false;

    Wire::Reader reader(cleartext);
    AkeAuthenticator decoded;
    QByteArray encodedSignature;
    if (!reader.readDsaPublicKey(&decoded.publicKey) || !reader.readInt(&decoded.keyId) || decoded.keyId == 0 ||
        !reader.readBytes(DsaSignatureSize, &encodedSignature) || !reader.atEnd() ||
        !decodeDsaSignature(encodedSignature, &decoded.signature)) {
        return false;
    }

    const QByteArray digest =
        akeSignatureDigest(senderDhPublic, receiverDhPublic, decoded.publicKey, decoded.keyId, macKey);
    if (digest.size() != 32 || !dsaVerifyDigest(decoded.publicKey, digest, decoded.signature))
        return false;

    const QByteArray decodedFingerprint = dsaPublicKeyFingerprint(decoded.publicKey);
    if (decodedFingerprint.size() != 20)
        return false;

    *authenticator = decoded;
    if (fingerprint)
        *fingerprint = decodedFingerprint;
    return true;
}

namespace Wire {

QByteArray encodeDhCommitMessage(const DhCommitMessage &message, bool *ok)
{
    if (ok)
        *ok = false;
    if (!validInstanceTags(message.senderInstance, message.receiverInstance) || message.encryptedGx.size() < 4 ||
        message.hashedGx.size() != 32) {
        return {};
    }

    Writer writer;
    writeHeader(&writer, DhCommitType, message.senderInstance, message.receiverInstance);
    writer.writeData(message.encryptedGx);
    writer.writeData(message.hashedGx);
    if (ok)
        *ok = true;
    return writer.take();
}

bool decodeDhCommitMessage(const QByteArray &encoded, DhCommitMessage *message)
{
    if (!message)
        return false;

    Reader reader(encoded);
    DhCommitMessage decoded;
    if (!readHeader(&reader, DhCommitType, &decoded.senderInstance, &decoded.receiverInstance) ||
        !reader.readData(&decoded.encryptedGx) || !reader.readData(&decoded.hashedGx) || !reader.atEnd() ||
        decoded.encryptedGx.size() < 4 || decoded.hashedGx.size() != 32) {
        return false;
    }

    *message = decoded;
    return true;
}

QByteArray encodeDhKeyMessage(const DhKeyMessage &message, bool *ok)
{
    if (ok)
        *ok = false;
    if (!validInstanceTags(message.senderInstance, message.receiverInstance) ||
        !isValidDhPublicValue(message.dhPublicValue)) {
        return {};
    }

    Writer writer;
    writeHeader(&writer, DhKeyType, message.senderInstance, message.receiverInstance);
    if (!writer.writeMpi(message.dhPublicValue))
        return {};
    if (ok)
        *ok = true;
    return writer.take();
}

bool decodeDhKeyMessage(const QByteArray &encoded, DhKeyMessage *message)
{
    if (!message)
        return false;

    Reader reader(encoded);
    DhKeyMessage decoded;
    if (!readHeader(&reader, DhKeyType, &decoded.senderInstance, &decoded.receiverInstance) ||
        !reader.readMpi(&decoded.dhPublicValue) || !reader.atEnd() || !isValidDhPublicValue(decoded.dhPublicValue)) {
        return false;
    }

    *message = decoded;
    return true;
}

QByteArray encodeRevealSignatureMessage(const RevealSignatureMessage &message, bool *ok)
{
    if (ok)
        *ok = false;
    if (!validInstanceTags(message.senderInstance, message.receiverInstance) || message.revealedKey.size() != 16 ||
        message.mac.size() != 20) {
        return {};
    }

    Writer writer;
    writeHeader(&writer, RevealSignatureType, message.senderInstance, message.receiverInstance);
    writer.writeData(message.revealedKey);
    writer.writeData(message.encryptedSignature);
    writer.writeBytes(message.mac);
    if (ok)
        *ok = true;
    return writer.take();
}

bool decodeRevealSignatureMessage(const QByteArray &encoded, RevealSignatureMessage *message)
{
    if (!message)
        return false;

    Reader reader(encoded);
    RevealSignatureMessage decoded;
    if (!readHeader(&reader, RevealSignatureType, &decoded.senderInstance, &decoded.receiverInstance) ||
        !reader.readData(&decoded.revealedKey) || !reader.readData(&decoded.encryptedSignature) ||
        !reader.readBytes(20, &decoded.mac) || !reader.atEnd() || decoded.revealedKey.size() != 16) {
        return false;
    }

    *message = decoded;
    return true;
}

QByteArray encodeSignatureMessage(const SignatureMessage &message, bool *ok)
{
    if (ok)
        *ok = false;
    if (!validInstanceTags(message.senderInstance, message.receiverInstance) || message.mac.size() != 20)
        return {};

    Writer writer;
    writeHeader(&writer, SignatureType, message.senderInstance, message.receiverInstance);
    writer.writeData(message.encryptedSignature);
    writer.writeBytes(message.mac);
    if (ok)
        *ok = true;
    return writer.take();
}

bool decodeSignatureMessage(const QByteArray &encoded, SignatureMessage *message)
{
    if (!message)
        return false;

    Reader reader(encoded);
    SignatureMessage decoded;
    if (!readHeader(&reader, SignatureType, &decoded.senderInstance, &decoded.receiverInstance) ||
        !reader.readData(&decoded.encryptedSignature) || !reader.readBytes(20, &decoded.mac) || !reader.atEnd()) {
        return false;
    }

    *message = decoded;
    return true;
}

} // namespace Wire
} // namespace QcaOtr
