/*
 * OMEMO Plugin for Psi
 * Copyright (C) 2018 Vyacheslav Karpukhin
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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "signal.h"
#include "crypto.h"
#include <QMessageBox>

extern "C" {
#include "session_cipher.h"
#include "session_builder.h"
#include "protocol.h"
#include "key_helper.h"
}

namespace psiomemo {
  void Signal::signal_log(int level, const char *message, size_t len, void *user_data) {
    Q_UNUSED(level);
    Q_UNUSED(user_data);
    qDebug() << "Signal: " << QByteArray(message, static_cast<int>(len));
  }

  void Signal::init(const QString &dataPath) {
    std::random_device rd;
    randomGen.seed(rd());
    signal_context_create(&m_signalContext, nullptr);
    signal_context_set_log_function(m_signalContext, &signal_log);

    Crypto::initCryptoProvider(m_signalContext);
    m_storage.init(m_signalContext, dataPath);

    signal_protocol_identity_get_local_registration_id(m_storage.storeContext(), &m_deviceId);
  }

  void Signal::deinit() {
    m_storage.deinit();
    signal_context_destroy(m_signalContext);
  }

  uint32_t Signal::getDeviceId() {
    return m_deviceId;
  }

  void Signal::generatePreKeys() {
    const uint32_t preKeyCount = 100;
    uint32_t actualCount = m_storage.preKeyCount();

    if (actualCount < preKeyCount) {
      uint32_t count = preKeyCount - actualCount;
      uint32_t startId = m_storage.maxPreKeyId() + 1;
      if (startId + count >= PRE_KEY_MEDIUM_MAX_VALUE) {
        startId = 1;
      }

      signal_protocol_key_helper_pre_key_list_node *pre_keys_head = nullptr;
      if (signal_protocol_key_helper_generate_pre_keys(&pre_keys_head, startId, count, m_signalContext) == SG_SUCCESS) {
        QVector<QPair<uint32_t, QByteArray>> keys;

        signal_protocol_key_helper_pre_key_list_node *cur_node = pre_keys_head;
        signal_buffer *key_buf = nullptr;
        while (cur_node) {
          session_pre_key *pre_key = signal_protocol_key_helper_key_list_element(cur_node);
          if (session_pre_key_serialize(&key_buf, pre_key) == SG_SUCCESS) {
            keys.append(qMakePair(session_pre_key_get_id(pre_key), toQByteArray(key_buf)));
            signal_buffer_bzero_free(key_buf);
          }
          cur_node = signal_protocol_key_helper_key_list_next(cur_node);
        }
        signal_protocol_key_helper_key_list_free(pre_keys_head);

        m_storage.storePreKeys(keys);
      }
    }
  }

  Bundle Signal::collectBundle() {
    generatePreKeys();

    Bundle bundle;

    bundle.signedPreKeyId = m_storage.signedPreKeyid();

    session_signed_pre_key *signed_pre_key = nullptr;
    if (signal_protocol_signed_pre_key_load_key(m_storage.storeContext(), &signed_pre_key, bundle.signedPreKeyId) != SG_SUCCESS) {
      return bundle;
    }

    bundle.signedPreKeySignature = toQByteArray(session_signed_pre_key_get_signature(signed_pre_key),
                                                session_signed_pre_key_get_signature_len(signed_pre_key));

    QByteArray signedPreKeyPublicKey = getPublicKey(session_signed_pre_key_get_key_pair(signed_pre_key));
    if (!signedPreKeyPublicKey.isNull()) {
      bundle.signedPreKeyPublic = signedPreKeyPublicKey;
      bundle.identityKeyPublic = getIdentityPublicKey();

      foreach (auto preKey, m_storage.loadAllPreKeys()) {
        session_pre_key *pre_key = nullptr;
        if (session_pre_key_deserialize(&pre_key,
                                        reinterpret_cast<const uint8_t *>(preKey.second.data()),
                                        static_cast<size_t>(preKey.second.size()),
                                        m_signalContext) == SG_SUCCESS) {
          QByteArray preKeyPublicKey = getPublicKey(session_pre_key_get_key_pair(pre_key));
          if (!preKeyPublicKey.isNull()) {
            bundle.preKeys.append(qMakePair(preKey.first, preKeyPublicKey));
          }
          SIGNAL_UNREF(pre_key);
        }
      }
    }
    SIGNAL_UNREF(signed_pre_key);
    return bundle;
  }

  void Signal::processBundle(const QString &from, uint32_t deviceId, const Bundle &bundle) {
    std::uniform_int_distribution<> randDis(0, bundle.preKeys.size() - 1);
    QPair<uint32_t, QByteArray> preKey = bundle.preKeys.at(randDis(randomGen));// starting from Qt5.10 QRandomGenerator::global()->bounded(bundle.preKeys.size())

    ec_public_key *pre_key_public = curveDecodePoint(preKey.second);
    if (pre_key_public != nullptr) {
      ec_public_key *signed_pre_key_public = curveDecodePoint(bundle.signedPreKeyPublic);

      if (signed_pre_key_public != nullptr) {
        ec_public_key *identity_key = curveDecodePoint(bundle.identityKeyPublic);

        if (identity_key != nullptr) {
          session_pre_key_bundle *pre_key_bundle = nullptr;

          if (session_pre_key_bundle_create(&pre_key_bundle,
                                            deviceId,
                                            0,
                                            preKey.first,
                                            pre_key_public,
                                            bundle.signedPreKeyId,
                                            signed_pre_key_public,
                                            reinterpret_cast<const uint8_t *>(bundle.signedPreKeySignature.data()),
                                            static_cast<size_t>(bundle.signedPreKeySignature.size()),
                                            identity_key) == SG_SUCCESS) {
            session_builder *session_builder = nullptr;
            const QByteArray& fromUtf8 = from.toUtf8();
            signal_protocol_address addr = getAddress(deviceId, fromUtf8);

            if (session_builder_create(&session_builder, m_storage.storeContext(), &addr, m_signalContext) == SG_SUCCESS) {
              session_builder_process_pre_key_bundle(session_builder, pre_key_bundle);
              session_builder_free(session_builder);
            }
            SIGNAL_UNREF(pre_key_bundle);
          }
          SIGNAL_UNREF(identity_key);
        }
        SIGNAL_UNREF(signed_pre_key_public);
      }
      SIGNAL_UNREF(pre_key_public);
    }
  }

  ec_public_key* Signal::curveDecodePoint(const QByteArray &bytes) const {
    ec_public_key *publicKey = nullptr;
    curve_decode_point(&publicKey, reinterpret_cast<const uint8_t *>(bytes.data()), static_cast<size_t>(bytes.size()), m_signalContext);
    return publicKey;
  }

  QByteArray Signal::getPublicKey(const ec_key_pair *key_pair) const {
    QByteArray result;
    ec_public_key *public_key = ec_key_pair_get_public(key_pair);
    signal_buffer *public_serialized = nullptr;
    if (ec_public_key_serialize(&public_serialized, public_key) == SG_SUCCESS) {
      result = toQByteArray(public_serialized);
      signal_buffer_bzero_free(public_serialized);
    }
    return result;
  }

  void Signal::updateDeviceList(const QString &user, const QSet<uint32_t> &actualIds) {
    m_storage.updateDeviceList(user, actualIds);
  }

  QVector<uint32_t> Signal::invalidSessions(const QString &recipient) {
    QVector<uint32_t> result;
    const QByteArray &recipientUtf8 = recipient.toUtf8();
    QSet<uint32_t> recipientDevices = m_storage.retrieveDeviceList(recipient, false);
    foreach (uint32_t deviceId, recipientDevices) {
      if (!sessionIsValid(getAddress(deviceId, recipientUtf8))) {
        result.append(deviceId);
      }
    }
    return result;
  }

  bool Signal::sessionIsValid(const signal_protocol_address &addr) const {
    return signal_protocol_session_contains_session(m_storage.storeContext(), &addr) && m_storage.identityExists(&addr);
  }

  QList<EncryptedKey> Signal::encryptKey(const QString &ownJid, const QString &recipient, const QCA::SymmetricKey &key) {
    QList<EncryptedKey> results;
    const QByteArray &recipientUtf8 = recipient.toUtf8();
    const QByteArray &ownJidUtf8 = ownJid.toUtf8();

    QSet<uint32_t> ownDevices = m_storage.retrieveDeviceList(ownJid);
    QSet<uint32_t> recipientDevices = m_storage.retrieveDeviceList(recipient);

    if (recipientDevices.isEmpty()) {
      return results;
    }

    QSet<uint32_t> devices;
    devices.unite(ownDevices).unite(recipientDevices).remove(m_deviceId);

    foreach (uint32_t deviceId, devices) {
      const QByteArray &name = recipientDevices.contains(deviceId) ? recipientUtf8 : ownJidUtf8;
      signal_protocol_address addr = getAddress(deviceId, name);
      if (!sessionIsValid(addr)) continue;

      bool isPreKey;
      QByteArray encKey;
      doWithCipher(&addr, key,
                   [&](session_cipher *cipher, const uint8_t *keyData, size_t keyLen) {
                     ciphertext_message *cipher_msg = nullptr;
                     if (session_cipher_encrypt(cipher, keyData, keyLen, &cipher_msg) == SG_SUCCESS) {
                       encKey = toQByteArray(ciphertext_message_get_serialized(cipher_msg));
                       isPreKey = ciphertext_message_get_type(cipher_msg) == CIPHERTEXT_PREKEY_TYPE;
                       SIGNAL_UNREF(cipher_msg);
                     }
                   });
      if (!encKey.isNull()) {
        results.append(EncryptedKey(static_cast<uint32_t>(addr.device_id), isPreKey, encKey));
      }
    }

    return results;
  }

  QPair<QByteArray, bool> Signal::decryptKey(const QString &sender, const EncryptedKey &encryptedKey) {
    QByteArray key;
    bool buildSessionWithPreKey = false;

    const QByteArray &senderUtf8 = sender.toUtf8();
    signal_protocol_address sender_addr = getAddress(encryptedKey.deviceId, senderUtf8);

    if (encryptedKey.isPreKey) {
      session_builder *session_builder = nullptr;
      if (session_builder_create(&session_builder, m_storage.storeContext(), &sender_addr, m_signalContext) == SG_SUCCESS) {
        doWithCipher(&sender_addr, encryptedKey.key,
                     [&](session_cipher *cipher, const uint8_t *keyData, size_t keyLen) {
                       pre_key_signal_message *message = nullptr;
                       if (pre_key_signal_message_deserialize(&message, keyData, keyLen, m_signalContext) == SG_SUCCESS) {
                         signal_buffer *plaintext = nullptr;
                         int result = session_cipher_decrypt_pre_key_signal_message(cipher, message, nullptr, &plaintext);
                         if (result == SG_SUCCESS) {
                           key = toQByteArray(plaintext);
                           signal_buffer_bzero_free(plaintext);
                         }
                         else if (result == SG_ERR_INVALID_KEY_ID) {
                           buildSessionWithPreKey = true;
                         }
                         SIGNAL_UNREF(message);
                       }
                     });
        session_builder_free(session_builder);
      }
    } else {
      doWithCipher(&sender_addr, encryptedKey.key,
                   [&](session_cipher *cipher, const uint8_t *keyData, size_t keyLen) {
                     signal_message *message = nullptr;
                     if (signal_message_deserialize(&message, keyData, keyLen, m_signalContext) == SG_SUCCESS) {
                       signal_buffer *plaintext = nullptr;
                       int result = session_cipher_decrypt_signal_message(cipher, message, nullptr, &plaintext);
                       if (result == SG_SUCCESS) {
                         key = toQByteArray(plaintext);
                         signal_buffer_bzero_free(plaintext);
                       }
                       SIGNAL_UNREF(message);
                     }
                   });
    }
    return qMakePair(key, buildSessionWithPreKey);
  }

  bool Signal::isTrusted(const QString &user, uint32_t deviceId) {
    return m_storage.isTrusted(user, deviceId);
  }

  template<typename T>
  void Signal::doWithCipher(signal_protocol_address *addr, const QCA::SymmetricKey &key, T&& lambda) {
    session_cipher *cipher = nullptr;
    if (session_cipher_create(&cipher, m_storage.storeContext(), addr, m_signalContext) == SG_SUCCESS) {
      lambda(cipher, reinterpret_cast<const uint8_t *>(key.data()), static_cast<size_t>(key.size()));
      session_cipher_free(cipher);
    }
  }

  signal_protocol_address Signal::getAddress(uint32_t deviceId, const QByteArray &name) const {
    signal_protocol_address addr = {};
    addr.device_id = deviceId;
    addr.name = name.data();
    addr.name_len = static_cast<size_t>(name.length());
    return addr;
  }

  uint32_t Signal::preKeyCount() {
    return m_storage.preKeyCount();
  }

  void Signal::processUndecidedDevices(const QString &user, bool ownJid) {
    QSet<uint32_t> devices = m_storage.retrieveUndecidedDeviceList(user);
    foreach (uint32_t deviceId, devices) {
      confirmDeviceTrust(user, deviceId, false, ownJid);
    }
  }

  void Signal::confirmDeviceTrust(const QString &user, uint32_t deviceId, bool skipNewDevicePart, bool ownJid) {
    QString publicKey = getFingerprint(m_storage.loadDeviceIdentity(user, deviceId));
    QString message;
    if (!skipNewDevicePart) {
      message += QString("New OMEMO device has been discovered for %1.<br/><br/>").arg(user);
    }
    message += ownJid ?
               "Do you want to trust this device and allow it to decrypt copies of your messages?<br/><br/>" :
               "Do you want to trust this device and allow it to receive the encrypted messages from you?<br/><br/>";
    message += QString("Device public key:<br/><code>%1</code>").arg(publicKey);
    QMessageBox messageBox(QMessageBox::Warning, "New OMEMO Device", message);
    messageBox.addButton("Trust", QMessageBox::AcceptRole);
    messageBox.addButton("Do Not Trust", QMessageBox::RejectRole);
    int res = messageBox.exec();
    m_storage.setDeviceTrust(user, deviceId, res == 0);
  }

  QString Signal::getFingerprint(const QByteArray &publicKeyBytes) const {
    QByteArray bytes = publicKeyBytes.right(publicKeyBytes.size() - 1); // the first byte is DJB_TYPE
    QString publicKey = bytes.toHex().toUpper();
    for (int pos = 8, groups = 0; pos < publicKey.length(); pos += 9, groups++) {
      publicKey.insert(pos, ' ');
    }
    return publicKey;
  }

  bool Signal::isAvailableForUser(const QString &user) {
    return !m_storage.retrieveDeviceList(user, false).isEmpty();
  };

  bool Signal::isEnabledForUser(const QString &user) {
    return m_storage.isEnabledForUser(user);
  }

  void Signal::setEnabledForUser(const QString &user, bool enabled) {
    m_storage.setEnabledForUser(user, enabled);
  }

  QString Signal::getOwnFingerprint() {
    return getFingerprint(getIdentityPublicKey());
  }

  QByteArray Signal::getIdentityPublicKey() const {
    QByteArray res;
    ratchet_identity_key_pair *key_pair = nullptr;
    if (signal_protocol_identity_get_key_pair(m_storage.storeContext(), &key_pair) == SG_SUCCESS) {
      ec_public_key *identity_key_public = ratchet_identity_key_pair_get_public(key_pair);
      signal_buffer *identity_key_public_data = nullptr;
      if (ec_public_key_serialize(&identity_key_public_data, identity_key_public) == SG_SUCCESS) {
        res = toQByteArray(identity_key_public_data);
        signal_buffer_bzero_free(identity_key_public_data);
      }
      SIGNAL_UNREF(key_pair);
    }
    return res;
  }

  QList<Fingerprint> Signal::getKnownFingerprints() {
    QList<Fingerprint> res;
    foreach (auto item, m_storage.getKnownFingerprints()) {
      Fingerprint fp(std::get<0>(item), getFingerprint(std::get<1>(item)), std::get<2>(item), std::get<3>(item));
      res.append(fp);
    }
    return res;
  };

  bool Bundle::isValid() {
    return !signedPreKeyPublic.isNull() && !signedPreKeySignature.isNull() && !identityKeyPublic.isNull() && !preKeys.isEmpty();
  }
}
