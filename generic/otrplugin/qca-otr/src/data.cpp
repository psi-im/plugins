#include "qca-otr/data.h"

#include "qca-otr/codec.h"
#include "qca-otr/crypto.h"

#include <cstring>
#include <limits>

namespace QcaOtr {
namespace {

constexpr quint16 OtrV3Version = 0x0003;
constexpr quint8 DataMessageType = 0x03;
constexpr quint32 MinimumInstanceTag = 0x00000100;
constexpr int CounterSize = 8;
constexpr int CounterBlockSize = 16;
constexpr int MacSize = 20;

bool validInstanceTags(quint32 sender, quint32 receiver)
{
    return sender >= MinimumInstanceTag && receiver >= MinimumInstanceTag;
}

bool constantTimeEqual(const QByteArray &left, const QByteArray &right)
{
    if (left.size() != right.size())
        return false;

    unsigned char difference = 0;
    for (int i = 0; i < left.size(); ++i)
        difference |= static_cast<unsigned char>(left.at(i)) ^ static_cast<unsigned char>(right.at(i));
    return difference == 0;
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

QCA::SecureArray hashDataKey(quint8 selector, const QCA::SecureArray &encodedSharedSecret)
{
    QCA::SecureArray input(encodedSharedSecret.size() + 1);
    input[0] = static_cast<char>(selector);
    if (!encodedSharedSecret.isEmpty())
        std::memcpy(input.data() + 1,
                    encodedSharedSecret.constData(),
                    static_cast<size_t>(encodedSharedSecret.size()));
    return sha1Secure(input);
}

bool incrementCounter(QByteArray *counter)
{
    if (!counter || counter->size() != CounterBlockSize)
        return false;

    for (int i = CounterSize - 1; i >= 0; --i) {
        unsigned char value = static_cast<unsigned char>(counter->at(i));
        ++value;
        (*counter)[i] = static_cast<char>(value);
        if (value != 0)
            return true;
    }
    return false;
}

int compareCounter(const QByteArray &left, const QByteArray &rightBlock)
{
    if (left.size() != CounterSize || rightBlock.size() != CounterBlockSize)
        return 0;

    for (int i = 0; i < CounterSize; ++i) {
        const auto a = static_cast<unsigned char>(left.at(i));
        const auto b = static_cast<unsigned char>(rightBlock.at(i));
        if (a < b)
            return -1;
        if (a > b)
            return 1;
    }
    return 0;
}

QByteArray counterBlock(const QByteArray &counter)
{
    if (counter.size() != CounterSize)
        return {};
    QByteArray block(CounterBlockSize, '\0');
    std::memcpy(block.data(), counter.constData(), CounterSize);
    return block;
}

} // namespace

bool deriveDataSessionKeys(const DhKeyPair &localDh,
                           const QCA::BigInteger &peerDhPublic,
                           DataSessionKeys *keys)
{
    if (!keys || localDh.privateExponent <= QCA::BigInteger(0) ||
        !isValidDhPublicValue(localDh.publicValue) || !isValidDhPublicValue(peerDhPublic)) {
        return false;
    }

    QCA::BigInteger sharedSecret;
    if (!computeDhSharedSecret(localDh.privateExponent, peerDhPublic, &sharedSecret))
        return false;

    bool ok = false;
    const QCA::SecureArray encodedSharedSecret = Wire::encodeMpiSecure(sharedSecret, &ok);
    if (!ok)
        return false;

    // libotr assigns selector 0x01 to the high DH endpoint and 0x02 to the
    // low endpoint. This makes one side's send keys the other side's receive
    // keys without any role-specific state.
    const quint8 sendSelector = localDh.publicValue > peerDhPublic ? 0x01 : 0x02;
    const quint8 receiveSelector = sendSelector == 0x01 ? 0x02 : 0x01;
    const QCA::SecureArray sendHash = hashDataKey(sendSelector, encodedSharedSecret);
    const QCA::SecureArray receiveHash = hashDataKey(receiveSelector, encodedSharedSecret);
    if (sendHash.size() != 20 || receiveHash.size() != 20)
        return false;

    const QCA::SecureArray sendEncryption = secureSlice(sendHash, 0, 16);
    const QCA::SecureArray receiveEncryption = secureSlice(receiveHash, 0, 16);
    const QCA::SecureArray sendMac = sha1Secure(sendEncryption);
    const QCA::SecureArray receiveMac = sha1Secure(receiveEncryption);

    QCA::SecureArray extraInput(encodedSharedSecret.size() + 1);
    extraInput[0] = static_cast<char>(0xff);
    if (!encodedSharedSecret.isEmpty())
        std::memcpy(extraInput.data() + 1,
                    encodedSharedSecret.constData(),
                    static_cast<size_t>(encodedSharedSecret.size()));
    const QCA::SecureArray extra = sha256Secure(extraInput);
    if (sendMac.size() != MacSize || receiveMac.size() != MacSize || extra.size() != 32)
        return false;

    DataSessionKeys derived;
    derived.sendEncryptionKey = sendEncryption;
    derived.receiveEncryptionKey = receiveEncryption;
    derived.sendMacKey = sendMac;
    derived.receiveMacKey = receiveMac;
    derived.extraKey = extra;
    *keys = derived;
    return true;
}

QByteArray dataMessageMacInput(const DataMessage &message, bool *ok)
{
    if (ok)
        *ok = false;

    if (!validInstanceTags(message.senderInstance, message.receiverInstance) || message.senderKeyId == 0 ||
        message.recipientKeyId == 0 || !isValidDhPublicValue(message.nextDhPublic) ||
        message.counter.size() != CounterSize) {
        return {};
    }

    Wire::Writer writer;
    writer.writeShort(OtrV3Version);
    writer.writeByte(DataMessageType);
    writer.writeInt(message.senderInstance);
    writer.writeInt(message.receiverInstance);
    writer.writeByte(message.flags);
    writer.writeInt(message.senderKeyId);
    writer.writeInt(message.recipientKeyId);
    if (!writer.writeMpi(message.nextDhPublic))
        return {};
    writer.writeBytes(message.counter);
    writer.writeData(message.encryptedData);

    if (ok)
        *ok = true;
    return writer.take();
}

namespace Wire {

QByteArray encodeDataMessage(const DataMessage &message, bool *ok)
{
    if (ok)
        *ok = false;
    if (message.mac.size() != MacSize)
        return {};

    bool prefixOk = false;
    const QByteArray prefix = dataMessageMacInput(message, &prefixOk);
    if (!prefixOk)
        return {};

    Writer writer;
    writer.writeBytes(prefix);
    writer.writeBytes(message.mac);
    writer.writeData(message.revealedMacKeys);
    if (ok)
        *ok = true;
    return writer.take();
}

bool decodeDataMessage(const QByteArray &encoded, DataMessage *message)
{
    if (!message)
        return false;

    Reader reader(encoded);
    quint16 version = 0;
    quint8 type = 0;
    DataMessage decoded;
    if (!reader.readShort(&version) || !reader.readByte(&type) || version != OtrV3Version ||
        type != DataMessageType || !reader.readInt(&decoded.senderInstance) ||
        !reader.readInt(&decoded.receiverInstance) ||
        !validInstanceTags(decoded.senderInstance, decoded.receiverInstance) || !reader.readByte(&decoded.flags) ||
        !reader.readInt(&decoded.senderKeyId) || !reader.readInt(&decoded.recipientKeyId) ||
        decoded.senderKeyId == 0 || decoded.recipientKeyId == 0 || !reader.readMpi(&decoded.nextDhPublic) ||
        !isValidDhPublicValue(decoded.nextDhPublic) || !reader.readBytes(CounterSize, &decoded.counter) ||
        !reader.readData(&decoded.encryptedData) || !reader.readBytes(MacSize, &decoded.mac) ||
        !reader.readData(&decoded.revealedMacKeys) || !reader.atEnd()) {
        return false;
    }

    *message = decoded;
    return true;
}

} // namespace Wire

DataSession::DataSession(const AkeEstablishedSession &ake, quint32 localInstance, quint32 peerInstance)
    : localInstance_(localInstance), peerInstance_(peerInstance)
{
    ready_ = validInstanceTags(localInstance_, peerInstance_) && initialize(ake);
}

bool DataSession::initialize(const AkeEstablishedSession &ake)
{
    if (!isValidDhPublicValue(ake.localDh.publicValue) || ake.localDh.privateExponent <= QCA::BigInteger(0) ||
        !isValidDhPublicValue(ake.peerDhPublic) || ake.localKeyId == 0 || ake.peerKeyId == 0 ||
        ake.localKeyId == std::numeric_limits<quint32>::max()) {
        return false;
    }

    DhKeyPair nextLocal;
    if (!generateDhKeyPair(&nextLocal))
        return false;

    DataSessionKeys currentCurrent;
    DataSessionKeys oldCurrent;
    if (!deriveDataSessionKeys(nextLocal, ake.peerDhPublic, &currentCurrent) ||
        !deriveDataSessionKeys(ake.localDh, ake.peerDhPublic, &oldCurrent)) {
        return false;
    }

    localDh_ = nextLocal;
    localOldDh_ = ake.localDh;
    localKeyId_ = ake.localKeyId + 1;
    peerDhPublic_ = ake.peerDhPublic;
    peerOldDhPublic_ = QCA::BigInteger(0);
    hasPeerOldDh_ = false;
    peerKeyId_ = ake.peerKeyId;

    sessions_[0][0] = currentCurrent;
    sessions_[1][0] = oldCurrent;
    sessionValid_[0][0] = true;
    sessionValid_[1][0] = true;
    sessionValid_[0][1] = false;
    sessionValid_[1][1] = false;
    savedMacKeys_.clear();
    return true;
}

void DataSession::saveMacKeys(DataSessionKeys *first, DataSessionKeys *second)
{
    auto save = [this](DataSessionKeys *keys) {
        if (!keys)
            return;
        if (keys->receiveMacUsed)
            savedMacKeys_.append(keys->receiveMacKey.toByteArray());
        if (keys->sendMacUsed)
            savedMacKeys_.append(keys->sendMacKey.toByteArray());
    };
    save(first);
    save(second);
}

bool DataSession::rotateLocalDh()
{
    if (localKeyId_ == std::numeric_limits<quint32>::max())
        return false;

    DhKeyPair nextLocal;
    if (!generateDhKeyPair(&nextLocal))
        return false;

    DataSessionKeys nextCurrent;
    if (!deriveDataSessionKeys(nextLocal, peerDhPublic_, &nextCurrent))
        return false;

    DataSessionKeys nextOldPeer;
    if (hasPeerOldDh_ && !deriveDataSessionKeys(nextLocal, peerOldDhPublic_, &nextOldPeer))
        return false;

    saveMacKeys(sessionValid_[1][0] ? &sessions_[1][0] : nullptr,
                sessionValid_[1][1] ? &sessions_[1][1] : nullptr);

    localOldDh_ = localDh_;
    localDh_ = nextLocal;
    ++localKeyId_;

    sessions_[1][0] = sessions_[0][0];
    sessionValid_[1][0] = sessionValid_[0][0];
    sessions_[1][1] = sessions_[0][1];
    sessionValid_[1][1] = sessionValid_[0][1];

    sessions_[0][0] = nextCurrent;
    sessionValid_[0][0] = true;
    if (hasPeerOldDh_) {
        sessions_[0][1] = nextOldPeer;
        sessionValid_[0][1] = true;
    } else {
        sessions_[0][1] = DataSessionKeys();
        sessionValid_[0][1] = false;
    }
    return true;
}

bool DataSession::rotatePeerDh(const QCA::BigInteger &newPeerPublic)
{
    if (!isValidDhPublicValue(newPeerPublic) || peerKeyId_ == std::numeric_limits<quint32>::max())
        return false;

    DataSessionKeys currentNewPeer;
    DataSessionKeys oldNewPeer;
    if (!deriveDataSessionKeys(localDh_, newPeerPublic, &currentNewPeer) ||
        !deriveDataSessionKeys(localOldDh_, newPeerPublic, &oldNewPeer)) {
        return false;
    }

    saveMacKeys(sessionValid_[0][1] ? &sessions_[0][1] : nullptr,
                sessionValid_[1][1] ? &sessions_[1][1] : nullptr);

    peerOldDhPublic_ = peerDhPublic_;
    peerDhPublic_ = newPeerPublic;
    hasPeerOldDh_ = true;
    ++peerKeyId_;

    sessions_[0][1] = sessions_[0][0];
    sessionValid_[0][1] = sessionValid_[0][0];
    sessions_[1][1] = sessions_[1][0];
    sessionValid_[1][1] = sessionValid_[1][0];

    sessions_[0][0] = currentNewPeer;
    sessions_[1][0] = oldNewPeer;
    sessionValid_[0][0] = true;
    sessionValid_[1][0] = true;
    return true;
}

bool DataSession::sendMessage(const QByteArray &plaintext, QByteArray *encoded, quint8 flags)
{
    if (!ready_ || !encoded || peerKeyId_ == 0 || localKeyId_ <= 1 || !sessionValid_[1][0])
        return false;

    DataSessionKeys &session = sessions_[1][0];
    QByteArray nextCounter = session.sendCounter;
    if (!incrementCounter(&nextCounter))
        return false;

    QByteArray cleartext = plaintext;
    cleartext.append('\0');

    QByteArray encryptedData;
    if (!aes128Ctr(session.sendEncryptionKey, nextCounter, cleartext, &encryptedData))
        return false;

    DataMessage message;
    message.senderInstance = localInstance_;
    message.receiverInstance = peerInstance_;
    message.flags = flags;
    message.senderKeyId = localKeyId_ - 1;
    message.recipientKeyId = peerKeyId_;
    message.nextDhPublic = localDh_.publicValue;
    message.counter = nextCounter.left(CounterSize);
    message.encryptedData = encryptedData;
    message.revealedMacKeys = savedMacKeys_;

    bool prefixOk = false;
    const QByteArray prefix = dataMessageMacInput(message, &prefixOk);
    if (!prefixOk)
        return false;
    message.mac = hmacSha1(session.sendMacKey, prefix);
    if (message.mac.size() != MacSize)
        return false;

    bool encodeOk = false;
    const QByteArray result = Wire::encodeDataMessage(message, &encodeOk);
    if (!encodeOk)
        return false;

    session.sendCounter = nextCounter;
    session.sendMacUsed = true;
    savedMacKeys_.clear();
    *encoded = result;
    return true;
}

DataReceiveResult DataSession::processIncoming(const QByteArray &encoded)
{
    DataReceiveResult result;
    if (!ready_) {
        result.status = DataReceiveStatus::Error;
        return result;
    }

    DataMessage message;
    if (!Wire::decodeDataMessage(encoded, &message)) {
        result.status = DataReceiveStatus::Error;
        return result;
    }

    if (message.receiverInstance != localInstance_ || message.senderInstance != peerInstance_)
        return result;

    if (message.senderKeyId > peerKeyId_ || peerKeyId_ - message.senderKeyId > 1 ||
        message.recipientKeyId > localKeyId_ || localKeyId_ - message.recipientKeyId > 1) {
        result.status = DataReceiveStatus::Error;
        return result;
    }

    const quint32 peerOffset = peerKeyId_ - message.senderKeyId;
    const quint32 localOffset = localKeyId_ - message.recipientKeyId;
    if ((peerOffset == 1 && !hasPeerOldDh_) || !sessionValid_[localOffset][peerOffset]) {
        result.status = DataReceiveStatus::Error;
        return result;
    }

    DataSessionKeys &session = sessions_[localOffset][peerOffset];
    bool prefixOk = false;
    const QByteArray prefix = dataMessageMacInput(message, &prefixOk);
    const QByteArray expectedMac = prefixOk ? hmacSha1(session.receiveMacKey, prefix) : QByteArray();
    if (expectedMac.size() != MacSize || !constantTimeEqual(expectedMac, message.mac)) {
        result.status = DataReceiveStatus::Error;
        return result;
    }

    if (compareCounter(message.counter, session.receiveCounter) <= 0) {
        result.status = DataReceiveStatus::Error;
        return result;
    }

    const QByteArray block = counterBlock(message.counter);
    QByteArray cleartext;
    if (block.size() != CounterBlockSize ||
        !aes128Ctr(session.receiveEncryptionKey, block, message.encryptedData, &cleartext)) {
        result.status = DataReceiveStatus::Error;
        return result;
    }

    session.receiveCounter = block;
    session.receiveMacUsed = true;
    result.extraKey = session.extraKey;
    result.flags = message.flags;

    const int nul = cleartext.indexOf('\0');
    result.plaintext = nul >= 0 ? cleartext.left(nul) : cleartext;

    // Match libotr's receive-side ratchet order exactly. A message addressed
    // to our current key first advances our own DH keypair. A message from the
    // peer's current key then installs the advertised next peer public key.
    if (message.recipientKeyId == localKeyId_ && !rotateLocalDh()) {
        result.status = DataReceiveStatus::Error;
        ready_ = false;
        return result;
    }
    if (message.senderKeyId == peerKeyId_ && !rotatePeerDh(message.nextDhPublic)) {
        result.status = DataReceiveStatus::Error;
        ready_ = false;
        return result;
    }

    result.status = DataReceiveStatus::Message;
    return result;
}

} // namespace QcaOtr
