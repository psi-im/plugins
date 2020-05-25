/*
 * OMEMO Plugin for Psi
 * Copyright (C) 2018 Vyacheslav Karpukhin
 * Copyright (C) 2020 Boris Pek
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef PSIOMEMO_SIGNAL_H
#define PSIOMEMO_SIGNAL_H

#include "storage.h"
#include <QMap>
#include <QObject>
#include <utility>

namespace psiomemo {
class Bundle {
public:
    Bundle() : signedPreKeyId(), signedPreKeyPublic(), signedPreKeySignature(), identityKeyPublic(), preKeys() { }
    uint32_t                             signedPreKeyId;
    QByteArray                           signedPreKeyPublic;
    QByteArray                           signedPreKeySignature;
    QByteArray                           identityKeyPublic;
    QVector<QPair<uint32_t, QByteArray>> preKeys;
    bool                                 isValid();
};

class EncryptedKey {
public:
    EncryptedKey(uint32_t deviceId_, bool isPreKey_, const QByteArray &key_) :
        deviceId(deviceId_), isPreKey(isPreKey_), key(qMove(key_))
    {
    }
    uint32_t   deviceId;
    bool       isPreKey;
    QByteArray key;
};

class Fingerprint {
public:
    Fingerprint(const QString &contact, const QString &fingerprint, const uint32_t deviceId, const TRUST_STATE trust) :
        contact(qMove(contact)), fingerprint(qMove(fingerprint)), deviceId(deviceId), trust(trust)
    {
    }
    QString     contact;
    QString     fingerprint;
    uint32_t    deviceId;
    TRUST_STATE trust;
};

class Signal {
public:
    void     init(const QString &dataPath, const QString &accountId);
    void     deinit();
    Bundle   collectBundle();
    void     processBundle(const QString &from, uint32_t deviceId, const Bundle &bundle);
    uint32_t getDeviceId();
    void updateDeviceList(const QString &user, const QSet<uint32_t> &actualIds, QMap<uint32_t, QString> &deviceLabels);
    QList<EncryptedKey>     encryptKey(const QString &ownJid, const QString &recipient, const QByteArray &key);
    QPair<QByteArray, bool> decryptKey(const QString &sender, const EncryptedKey &encryptedKey);
    QVector<uint32_t>       invalidSessions(const QString &recipient);
    uint32_t                preKeyCount();
    bool                    isTrusted(const QString &user, uint32_t deviceId);
    void                    processUndecidedDevices(const QString &user, const bool ownJid, const bool trusted);
    QSet<uint32_t>          getUnknownDevices(const QString &user);
    bool                    isAvailableForUser(const QString &user);
    bool                    isEnabledForUser(const QString &user);
    bool                    isDisabledForUser(const QString &user);
    void                    setEnabledForUser(const QString &user, bool value);
    void                    setDisabledForUser(const QString &user, bool value);

    QString                 getOwnFingerprint();
    QSet<uint32_t>          getDeviceList(const QString &user);
    QMap<uint32_t, QString> getFingerprintsMap(const QString &user);
    QList<Fingerprint>      getKnownFingerprints();

    void askDeviceTrust(const QString &user, uint32_t deviceId, bool skipNewDevicePart, bool ownJid = false);
    bool removeDevice(const QString &user, uint32_t deviceId);
    void confirmDeviceTrust(const QString &user, uint32_t deviceId);
    void revokeDeviceTrust(const QString &user, uint32_t deviceId);
    void removeCurrentDevice();

private:
    signal_context *m_signalContext = nullptr;
    uint32_t        m_deviceId      = 0;
    Storage         m_storage;

    QByteArray getPublicKey(const ec_key_pair *key_pair) const;

    template <typename T> void doWithCipher(signal_protocol_address *addr, const QByteArray &key, T &&);
    signal_protocol_address    getAddress(uint32_t deviceId, const QByteArray &name) const;
    bool                       sessionIsValid(const signal_protocol_address &addr) const;
    static void                signal_log(int level, const char *message, size_t len, void *user_data);
    ec_public_key *            curveDecodePoint(const QByteArray &bytes) const;
    void                       generatePreKeys();
    QString                    getFingerprint(const QByteArray &publicKeyBytes) const;
    QByteArray                 getIdentityPublicKey() const;
};
}
#endif // PSIOMEMO_SIGNAL_H
