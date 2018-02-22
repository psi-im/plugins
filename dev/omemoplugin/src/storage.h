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

#ifndef PSIOMEMO_STORAGE_H
#define PSIOMEMO_STORAGE_H

#include <QtSql>

extern "C" {
#include "signal_protocol.h"
}

namespace psiomemo {
  enum TRUST_STATE {
    UNDECIDED,
    TRUSTED,
    UNTRUSTED
  };

  class Storage {
  public:
    void init(signal_context *ctx, const QString &dataPath);
    void deinit();

    QSet<uint32_t> retrieveDeviceList(const QString &user, bool onlyTrusted = true);
    QSet<uint32_t> retrieveUndecidedDeviceList(const QString &user);
    void updateDeviceList(const QString &user, const QSet<uint32_t> &actualIds);

    QVector<QPair<uint32_t, QByteArray>> loadAllPreKeys(int limit = 100);
    uint32_t preKeyCount();
    void storePreKeys(QVector<QPair<uint32_t, QByteArray>> keys);
    bool isTrusted(const QString &user, uint32_t deviceId);
    QByteArray loadDeviceIdentity(const QString &user, uint32_t deviceId);
    void setDeviceTrust(const QString &user, uint32_t deviceId, bool trusted);
    QVector<std::tuple<QString, QByteArray, uint32_t, TRUST_STATE>> getKnownFingerprints();

    uint32_t maxPreKeyId();
    signal_protocol_store_context *storeContext() const;
    bool identityExists(const signal_protocol_address *addr_p) const;
    uint32_t signedPreKeyid();
    bool isEnabledForUser(const QString &user);
    void setEnabledForUser(const QString &user, bool enabled);

  private:
    QLatin1String m_databaseConnectionName;

    signal_protocol_store_context *m_storeContext = nullptr;

    void initializeDB(signal_context *signalContext);
    void migrateDatabase();

    QSqlDatabase db() const;
    static QSqlQuery getQuery(const void *user_data);
    static QString toQString(const char *name, size_t name_len);
    static QString addrName(const signal_protocol_address *address);

    static int toSignalBuffer(const QVariant &q, signal_buffer **record);
    static QSqlQuery lookupSession(const signal_protocol_address *address, const void *user_data);
#ifdef OLD_SIGNAL
    static int loadSession(signal_buffer **record, const signal_protocol_address *address, void *user_data);
    static int storeSession(const signal_protocol_address *address, uint8_t *record, size_t record_len, void *user_data);
#else
    static int loadSession(signal_buffer **record, signal_buffer **user_record, const signal_protocol_address *address, void *user_data);
    static int storeSession(const signal_protocol_address *address, uint8_t *record, size_t record_len, uint8_t *user_record, size_t user_record_len, void *user_data);
#endif
    static int containsSession(const signal_protocol_address *address, void *user_data);
    static int loadPreKey(signal_buffer **record, uint32_t pre_key_id, void *user_data);
    static int removePreKey(uint32_t pre_key_id, void *user_data);

    static int loadSignedPreKey(signal_buffer **record, uint32_t signed_pre_key_id, void *user_data);

    static int getIdentityKeyPair(signal_buffer **public_data, signal_buffer **private_data, void *user_data);
    static int getLocalRegistrationId(void *user_data, uint32_t *registration_id);
    static int saveIdentity(const signal_protocol_address *addr_p, uint8_t *key_data, size_t key_len, void *user_data);
    static int isTrustedIdentity(const signal_protocol_address *addr_p, uint8_t *key_data, size_t key_len, void *user_data);
    static QVariant lookupValue(void *user_data, const QString &key);
    void storeValue(const QString &key, const QVariant &value);
  };

  QByteArray toQByteArray(signal_buffer *buffer);
}

#endif //PSIOMEMO_STORAGE_H
