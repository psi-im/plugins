#pragma once

#include "qca-otr/akesession.h"

#include <QByteArray>
#include <QtGlobal>

namespace QcaOtr {

struct DataMessage
{
    quint32         senderInstance = 0;
    quint32         receiverInstance = 0;
    quint8          flags = 0;
    quint32         senderKeyId = 0;
    quint32         recipientKeyId = 0;
    QCA::BigInteger nextDhPublic;
    QByteArray      counter;
    QByteArray      encryptedData;
    QByteArray      mac;
    QByteArray      revealedMacKeys;
};

struct DataSessionKeys
{
    QCA::SecureArray sendEncryptionKey;
    QCA::SecureArray receiveEncryptionKey;
    QCA::SecureArray sendMacKey;
    QCA::SecureArray receiveMacKey;
    QCA::SecureArray extraKey;
    QByteArray       sendCounter = QByteArray(16, '\0');
    QByteArray       receiveCounter = QByteArray(16, '\0');
    bool             sendMacUsed = false;
    bool             receiveMacUsed = false;
};

bool deriveDataSessionKeys(const DhKeyPair &localDh,
                           const QCA::BigInteger &peerDhPublic,
                           DataSessionKeys *keys);
QByteArray dataMessageMacInput(const DataMessage &message, bool *ok = nullptr);

namespace Wire {

QByteArray encodeDataMessage(const DataMessage &message, bool *ok = nullptr);
bool decodeDataMessage(const QByteArray &encoded, DataMessage *message);

} // namespace Wire

enum class DataReceiveStatus {
    Ignored,
    Message,
    Error
};

struct DataReceiveResult
{
    DataReceiveStatus status = DataReceiveStatus::Ignored;
    QByteArray plaintext;
    QByteArray extraKey;
    quint8 flags = 0;
};

class DataSession
{
public:
    DataSession(const AkeEstablishedSession &ake, quint32 localInstance, quint32 peerInstance);

    bool isReady() const { return ready_; }
    quint32 localKeyId() const { return localKeyId_; }
    quint32 peerKeyId() const { return peerKeyId_; }
    const QCA::BigInteger &localDhPublic() const { return localDh_.publicValue; }
    const QCA::BigInteger &peerDhPublic() const { return peerDhPublic_; }

    bool sendMessage(const QByteArray &plaintext, QByteArray *encoded, quint8 flags = 0);
    DataReceiveResult processIncoming(const QByteArray &encoded);

private:
    bool initialize(const AkeEstablishedSession &ake);
    bool rotateLocalDh();
    bool rotatePeerDh(const QCA::BigInteger &newPeerPublic);
    void saveMacKeys(DataSessionKeys *first, DataSessionKeys *second);

    quint32 localInstance_ = 0;
    quint32 peerInstance_ = 0;
    bool ready_ = false;

    DhKeyPair localDh_;
    DhKeyPair localOldDh_;
    quint32 localKeyId_ = 0;

    QCA::BigInteger peerDhPublic_;
    QCA::BigInteger peerOldDhPublic_;
    bool hasPeerOldDh_ = false;
    quint32 peerKeyId_ = 0;

    DataSessionKeys sessions_[2][2];
    bool sessionValid_[2][2] = {{false, false}, {false, false}};
    QByteArray savedMacKeys_;
};

} // namespace QcaOtr
