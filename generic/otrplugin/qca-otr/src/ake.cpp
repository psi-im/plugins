#include "qca-otr/ake.h"

#include "qca-otr/codec.h"

namespace QcaOtr {
namespace {

constexpr quint16 OtrV3Version = 0x0003;
constexpr quint8 DhCommitType = 0x02;
constexpr quint8 DhKeyType = 0x0a;
constexpr quint8 RevealSignatureType = 0x11;
constexpr quint8 SignatureType = 0x12;
constexpr quint32 MinimumInstanceTag = 0x00000100;

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

QByteArray h2(quint8 selector, const QByteArray &secbytes)
{
    QByteArray input;
    input.reserve(secbytes.size() + 1);
    input.append(static_cast<char>(selector));
    input.append(secbytes);
    return sha256(input);
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

        const QCA::BigInteger privateExponent = unsignedInteger(random.toByteArray());
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
    const QByteArray secbytes = Wire::encodeMpi(sharedSecret, &ok);
    if (!ok)
        return false;

    const QByteArray h0 = h2(0x00, secbytes);
    const QByteArray h1 = h2(0x01, secbytes);
    const QByteArray hM1 = h2(0x02, secbytes);
    const QByteArray hM2 = h2(0x03, secbytes);
    const QByteArray hM1Prime = h2(0x04, secbytes);
    const QByteArray hM2Prime = h2(0x05, secbytes);
    if (h0.size() != 32 || h1.size() != 32 || hM1.size() != 32 || hM2.size() != 32 ||
        hM1Prime.size() != 32 || hM2Prime.size() != 32) {
        return false;
    }

    AkeKeys derived;
    derived.sessionId = h0.left(8);
    derived.c = QCA::SecureArray(h1.left(16));
    derived.cPrime = QCA::SecureArray(h1.mid(16, 16));
    derived.m1 = QCA::SecureArray(hM1);
    derived.m2 = QCA::SecureArray(hM2);
    derived.m1Prime = QCA::SecureArray(hM1Prime);
    derived.m2Prime = QCA::SecureArray(hM2Prime);
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
