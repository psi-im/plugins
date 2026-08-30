/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "qca-otr/akesession.h"
#include "qca-otr/tlv.h"

#include <QByteArray>
#include <QtGlobal>

namespace QcaOtr {

/** Decoded OTRv3 Data Message fields. */
struct DataMessage
{
    quint32 senderInstance = 0;
    quint32 receiverInstance = 0;
    quint8 flags = 0;
    quint32 senderKeyId = 0;
    quint32 recipientKeyId = 0;
    QCA::BigInteger nextDhPublic;
    QByteArray counter;
    QByteArray encryptedData;
    QByteArray mac;
    QByteArray revealedMacKeys;
};

/** Directional encryption/MAC material for one OTR ratchet key slot. */
struct DataSessionKeys
{
    QCA::SecureArray sendEncryptionKey;
    QCA::SecureArray receiveEncryptionKey;
    QCA::SecureArray sendMacKey;
    QCA::SecureArray receiveMacKey;
    QCA::SecureArray extraKey;
    QByteArray sendCounter = QByteArray(16, '\0');
    QByteArray receiveCounter = QByteArray(16, '\0');
    bool sendMacUsed = false;
    bool receiveMacUsed = false;
};

/** Derives directional OTRv3 data keys for one local/peer D-H pair. */
bool deriveDataSessionKeys(const DhKeyPair &localDh,
                           const QCA::BigInteger &peerDhPublic,
                           DataSessionKeys *keys);

/** Returns the exact serialized prefix covered by an OTR Data Message MAC. */
QByteArray dataMessageMacInput(const DataMessage &message, bool *ok = nullptr);

namespace Wire {

/** Encodes a complete raw OTRv3 Data Message. */
QByteArray encodeDataMessage(const DataMessage &message, bool *ok = nullptr);

/** Decodes and validates a complete raw OTRv3 Data Message. */
bool decodeDataMessage(const QByteArray &encoded, DataMessage *message);

} // namespace Wire

/** Result category for encrypted data processing. */
enum class DataReceiveStatus {
    Ignored,
    Message,
    Error
};

/** Plaintext/control material recovered from one valid Data Message. */
struct DataReceiveResult
{
    DataReceiveStatus status = DataReceiveStatus::Ignored;
    QByteArray plaintext;
    QVector<Tlv> tlvs;
    QCA::SecureArray extraKey;
    quint8 flags = 0;
};

/**
 * OTRv3 encrypted-data state for one authenticated remote instance.
 *
 * The object owns D-H ratchet state and the four current/old key combinations
 * required by the OTRv3 key rotation rules. Symmetric keys remain in
 * `QCA::SecureArray`.
 */
class DataSession
{
public:
    /** Initializes data-message state from a completed authenticated key exchange. */
    DataSession(const AkeEstablishedSession &ake, quint32 localInstance, quint32 peerInstance);

    bool isReady() const { return ready_; }
    quint32 localKeyId() const { return localKeyId_; }
    quint32 peerKeyId() const { return peerKeyId_; }
    const QCA::BigInteger &localDhPublic() const { return localDh_.publicValue; }
    const QCA::BigInteger &peerDhPublic() const { return peerDhPublic_; }

    /**
     * Returns the extra symmetric key for the exact slot used by the next
     * outgoing Data Message. The key is returned in secure memory for control
     * APIs such as the OTR symmetric-key TLV.
     */
    QCA::SecureArray currentSendExtraKey() const
    {
        if (!ready_ || !sessionValid_[1][0])
            return {};
        return sessions_[1][0].extraKey;
    }

    /** Encrypts an application plaintext into one raw OTRv3 Data Message. */
    bool sendMessage(const QByteArray &plaintext, QByteArray *encoded, quint8 flags = 0);

    /** Encrypts plaintext and the supplied TLVs into one raw OTRv3 Data Message. */
    bool sendMessage(const QByteArray &plaintext,
                     const QVector<Tlv> &tlvs,
                     QByteArray *encoded,
                     quint8 flags = 0);

    /** Authenticates/decrypts one raw OTRv3 Data Message and advances ratchet state. */
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
