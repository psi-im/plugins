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

#include "storage.h"
#include <QtSql>

extern "C" {
#include "key_helper.h"
}

namespace psiomemo {
void Storage::init(signal_context *ctx, const QString &dataPath, const QString &accountId)
{
    m_storeContext           = nullptr;
    m_databaseConnectionName = "OMEMO db " + accountId;
    QSqlDatabase _db         = QSqlDatabase::addDatabase("QSQLITE", m_databaseConnectionName);

    if (QDir(dataPath).exists("omemo.sqlite")) {
        QDir(dataPath).rename("omemo.sqlite", "omemo-" + accountId + ".sqlite");
    }

    _db.setDatabaseName(QDir(dataPath).filePath("omemo-" + accountId + ".sqlite"));
    if (!_db.open()) {
        qWarning() << _db.lastError();
    }

    initializeDB(ctx);
    db().exec("VACUUM");

    signal_protocol_session_store        session_store        = { /*.load_session_func =*/&loadSession,
                                                    /*.get_sub_device_sessions_func =*/nullptr,
                                                    /*.store_session_func =*/&storeSession,
                                                    /*.contains_session_func =*/&containsSession,
                                                    /*.delete_session_func =*/nullptr,
                                                    /*.delete_all_sessions_func =*/nullptr,
                                                    /*.destroy_func =*/nullptr,
                                                    /*.user_data =*/this };
    signal_protocol_pre_key_store        pre_key_store        = { /*.load_pre_key =*/&loadPreKey,
                                                    /*.store_pre_key =*/nullptr,
                                                    /*.contains_pre_key =*/nullptr,
                                                    /*.remove_pre_key =*/&removePreKey,
                                                    /*.destroy_func =*/nullptr,
                                                    /*.user_data =*/this };
    signal_protocol_signed_pre_key_store signed_pre_key_store = { /*.load_signed_pre_key =*/&loadSignedPreKey,
                                                                  /*.store_signed_pre_key =*/nullptr,
                                                                  /*.contains_signed_pre_key =*/nullptr,
                                                                  /*.remove_signed_pre_key =*/nullptr,
                                                                  /*.destroy_func =*/nullptr,
                                                                  /*.user_data =*/this };
    signal_protocol_identity_key_store   identity_key_store   = { /*.get_identity_key_pair =*/&getIdentityKeyPair,
                                                              /*.get_local_registration_id =*/&getLocalRegistrationId,
                                                              /*.save_identity =*/&saveIdentity,
                                                              /*.is_trusted_identity =*/&isTrustedIdentity,
                                                              /*.destroy_func =*/nullptr,
                                                              /*.user_data =*/this };

    signal_protocol_store_context_create(&m_storeContext, ctx);
    signal_protocol_store_context_set_session_store(m_storeContext, &session_store);
    signal_protocol_store_context_set_pre_key_store(m_storeContext, &pre_key_store);
    signal_protocol_store_context_set_signed_pre_key_store(m_storeContext, &signed_pre_key_store);
    signal_protocol_store_context_set_identity_key_store(m_storeContext, &identity_key_store);
}

void Storage::deinit()
{
    db().exec("VACUUM");
    QSqlDatabase::database(m_databaseConnectionName).close();
    QSqlDatabase::removeDatabase(m_databaseConnectionName);

    if (m_storeContext != nullptr) {
        signal_protocol_store_context_destroy(m_storeContext);
        m_storeContext = nullptr;
    }
}

void Storage::initializeDB(signal_context *signalContext)
{
    QSqlDatabase _db = db();
    _db.transaction();

    QString error;
    if (!_db.exec("PRAGMA table_info(simple_store)").next()) {
        _db.exec("CREATE TABLE IF NOT EXISTS enabled_buddies (jid TEXT NOT NULL PRIMARY KEY)");
        _db.exec("CREATE TABLE IF NOT EXISTS devices (jid TEXT NOT NULL, device_id INTEGER NOT NULL, trust INTEGER NOT "
                 "NULL, label TEXT, PRIMARY KEY(jid, device_id))");
        _db.exec("CREATE TABLE IF NOT EXISTS identity_key_store (jid TEXT NOT NULL, device_id INTEGER NOT NULL, key "
                 "BLOB NOT NULL, PRIMARY KEY(jid, device_id))");
        _db.exec("CREATE TABLE IF NOT EXISTS pre_key_store (id INTEGER NOT NULL PRIMARY KEY, pre_key BLOB NOT NULL)");
        _db.exec("CREATE TABLE IF NOT EXISTS session_store (jid TEXT NOT NULL, device_id INTEGER NOT NULL, session "
                 "BLOB NOT NULL, PRIMARY KEY(jid, device_id))");
        _db.exec("CREATE TABLE IF NOT EXISTS simple_store (key TEXT NOT NULL PRIMARY KEY, value BLOB NOT NULL)");

        storeValue("db_ver", 2);

        uint32_t deviceId;
        if (signal_protocol_key_helper_generate_registration_id(&deviceId, 1, signalContext) == SG_SUCCESS) {
            storeValue("registration_id", deviceId);

            ratchet_identity_key_pair *identity_key_pair = nullptr;
            if (signal_protocol_key_helper_generate_identity_key_pair(&identity_key_pair, signalContext)
                == SG_SUCCESS) {
                signal_buffer *key_buf = nullptr;

                if (ec_public_key_serialize(&key_buf, ratchet_identity_key_pair_get_public(identity_key_pair))
                    == SG_SUCCESS) {
                    storeValue("own_public_key", toQByteArray(key_buf));
                    signal_buffer_bzero_free(key_buf);

                    if (ec_private_key_serialize(&key_buf, ratchet_identity_key_pair_get_private(identity_key_pair))
                        == SG_SUCCESS) {
                        storeValue("own_private_key", toQByteArray(key_buf));
                        signal_buffer_bzero_free(key_buf);

                        uint32_t signed_pre_key_id;
                        if (signal_protocol_key_helper_generate_registration_id(&signed_pre_key_id, 1, signalContext)
                            == SG_SUCCESS) {
                            session_signed_pre_key *signed_pre_key = nullptr;
                            auto timestamp = static_cast<uint64_t>(QDateTime::currentMSecsSinceEpoch());
                            if (signal_protocol_key_helper_generate_signed_pre_key(
                                    &signed_pre_key, identity_key_pair, signed_pre_key_id, timestamp, signalContext)
                                == SG_SUCCESS) {
                                if (session_signed_pre_key_serialize(&key_buf, signed_pre_key) == SG_SUCCESS) {
                                    storeValue("signed_pre_key_id", signed_pre_key_id);
                                    storeValue("signed_pre_key", toQByteArray(key_buf));
                                    signal_buffer_bzero_free(key_buf);
                                } else {
                                    error = "Could not serialize signed prekey";
                                }

                                SIGNAL_UNREF(signed_pre_key);
                            } else {
                                error = "Could not generate signed prekey";
                            }
                        } else {
                            error = "Could not generate signed prekey ID";
                        }
                    } else {
                        error = "Could not serialize identity private key";
                    }
                } else {
                    error = "Could not serialize identity public key";
                }
                SIGNAL_UNREF(identity_key_pair);
            } else {
                error = "Could not generate identity key pair";
            }
        } else {
            error = "Could not generate registration ID";
        }
    } else if (lookupValue(this, "db_ver").toInt() != 3) {
        migrateDatabase();
    }

    if (!error.isNull()) {
        qWarning() << error;
        _db.rollback();
    } else {
        _db.commit();
    }
}

void Storage::migrateDatabase()
{
    QSqlDatabase _db = db();
    _db.exec("CREATE TABLE IF NOT EXISTS enabled_buddies (jid TEXT NOT NULL PRIMARY KEY)");
    _db.exec("DROP TABLE disabled_buddies");

    {   // Update old tables without "label" column
        QSqlQuery q(db());
        q.exec("PRAGMA table_info(devices)");
        bool labelColumnExist = false;
        while (q.next()) {
            if (q.value(1).toString() == QStringLiteral("label")) {
                labelColumnExist = true;
                break;
            }
        }
        if (!labelColumnExist) {
            q.exec("ALTER TABLE devices ADD COLUMN label TEXT");
        }
    }
    storeValue("db_ver", 3);
}

QSqlDatabase Storage::db() const { return QSqlDatabase::database(m_databaseConnectionName); }

QMap<uint32_t, QByteArray> Storage::getKeysMap(const QString &user)
{
    QSqlQuery q(db());
    q.prepare("SELECT device_id, key FROM identity_key_store WHERE jid IS ?");
    q.bindValue(0, user);
    q.exec();

    QMap<uint32_t, QByteArray> out;
    while (q.next()) {
        out.insert(q.value(0).toUInt(), q.value(1).toByteArray());
    }
    return out;
}

QSet<uint32_t> Storage::getDeviceList(const QString &user, bool onlyTrusted)
{
    QSqlQuery q(db());
    if (onlyTrusted) {
        q.prepare("SELECT device_id FROM devices WHERE jid IS ? AND trust IS ?");
        q.bindValue(1, TRUSTED);
    } else {
        q.prepare("SELECT device_id FROM devices WHERE jid IS ?");
    }
    q.bindValue(0, user);
    q.exec();

    QSet<uint32_t> knownIds;
    while (q.next()) {
        knownIds.insert(q.value(0).toUInt());
    }
    return knownIds;
}

QSet<uint32_t> Storage::getUndecidedDeviceList(const QString &user)
{
    QSqlQuery q(db());
    q.prepare("SELECT device_id FROM devices WHERE jid IS ? AND trust IS ?");
    q.addBindValue(user);
    q.addBindValue(UNDECIDED);
    q.exec();

    QSet<uint32_t> ids;
    while (q.next()) {
        ids.insert(q.value(0).toUInt());
    }
    return ids;
}

void Storage::updateDeviceList(const QString &user, const QSet<uint32_t> &actualIds, QMap<uint32_t, QString> &deviceLabels)
{
    QSet<uint32_t> knownIds = getDeviceList(user, false);

    auto         added   = QSet<uint32_t>(actualIds).subtract(knownIds);
    auto         removed = QSet<uint32_t>(knownIds).subtract(actualIds);
    auto       intersect = QSet<uint32_t>(knownIds).intersect(actualIds);

    QSqlDatabase _db(db());
    QSqlQuery    q(_db);

    if (!added.isEmpty()) {
        q.prepare("INSERT INTO devices (jid, device_id, trust) VALUES (?, ?, ?)");
        q.bindValue(0, user);
        q.bindValue(2, UNDECIDED);
        for (auto id : added) {
            q.bindValue(1, id);
            q.exec();
        }
    }

    if (!removed.isEmpty()) {
        q.prepare("DELETE FROM devices WHERE jid IS ? AND device_id IS ?");
        q.bindValue(0, user);

        QSqlQuery q2(_db);
        q2.prepare("DELETE FROM identity_key_store WHERE jid IS ? AND device_id IS ?");
        q2.bindValue(0, user);

        QSqlQuery q3(_db);
        q3.prepare("DELETE FROM session_store WHERE jid IS ? AND device_id IS ?");
        q3.bindValue(0, user);

        _db.transaction();
        for (auto id : removed) {
            q.bindValue(1, id);
            q.exec();

            // q2.bindValue(1, id);
            // q2.exec();
            //
            // q3.bindValue(1, id);
            // q3.exec();
        }
        _db.commit();
    }

    if (!deviceLabels.isEmpty() &&!intersect.isEmpty()) {
        q.prepare("UPDATE devices SET label = ? WHERE jid IS ? AND device_id IS ?");
        q.bindValue(1, user);

        _db.transaction();
        for (auto id : intersect) {
            if (deviceLabels.contains(id)) {
                q.bindValue(0, deviceLabels[id]);
                q.bindValue(2, id);
                q.exec();
            }
        }
        _db.commit();
    }
}

QVector<QPair<uint32_t, QByteArray>> Storage::loadAllPreKeys(int limit)
{
    QVector<QPair<uint32_t, QByteArray>> results;
    QSqlQuery                            q(db());
    q.prepare("SELECT id, pre_key FROM pre_key_store ORDER BY id ASC limit ?");
    q.addBindValue(limit);
    q.exec();
    while (q.next()) {
        results.append(qMakePair(q.value(0).toUInt(), q.value(1).toByteArray()));
    }
    return results;
}

uint32_t Storage::preKeyCount()
{
    QSqlQuery q(db());
    q.prepare("SELECT COUNT(*) FROM pre_key_store");
    q.exec();
    q.next();
    return q.value(0).toUInt();
}

uint32_t Storage::maxPreKeyId()
{
    QSqlQuery q(db());
    q.prepare("SELECT MAX(id) FROM pre_key_store");
    q.exec();
    q.next();
    return q.value(0).toUInt();
}

void Storage::storePreKeys(QVector<QPair<uint32_t, QByteArray>> keys)
{
    QSqlDatabase database = db();
    QSqlQuery    q(database);
    q.prepare("INSERT INTO pre_key_store (id, pre_key) VALUES (?, ?)");

    database.transaction();

    for (auto key : keys) {
        q.bindValue(0, key.first);
        q.bindValue(1, key.second);
        q.exec();
    }

    database.commit();
}

QSqlQuery Storage::getQuery(const void *user_data)
{
    return QSqlQuery((static_cast<const Storage *>(user_data))->db());
}

QString Storage::toQString(const char *name, size_t name_len)
{
    return QString(QByteArray(name, static_cast<int>(name_len)));
}

QString Storage::addrName(const signal_protocol_address *address)
{
    return toQString(address->name, address->name_len);
}

int Storage::toSignalBuffer(const QVariant &q, signal_buffer **record)
{
    QByteArray data = q.toByteArray();
    *record = signal_buffer_create(reinterpret_cast<const uint8_t *>(data.data()), static_cast<size_t>(data.size()));
    return 1;
}

QSqlQuery Storage::lookupSession(const signal_protocol_address *address, const void *user_data)
{
    QSqlQuery q = getQuery(user_data);
    q.prepare("SELECT session FROM session_store WHERE jid IS ? AND device_id IS ?");
    q.addBindValue(addrName(address));
    q.addBindValue(address->device_id);
    q.exec();
    return q;
}

#ifdef OLD_SIGNAL
int Storage::loadSession(signal_buffer **record, const signal_protocol_address *address, void *user_data)
{
#else
int Storage::loadSession(signal_buffer **record, signal_buffer **user_record, const signal_protocol_address *address,
                         void *user_data)
{
    (void)user_record;
#endif
    QSqlQuery q = lookupSession(address, user_data);
    return q.next() ? toSignalBuffer(q.value(0), record) : 0;
}

#ifdef OLD_SIGNAL
int Storage::storeSession(const signal_protocol_address *address, uint8_t *record, size_t record_len, void *user_data)
{
#else
int Storage::storeSession(const signal_protocol_address *address, uint8_t *record, size_t record_len,
                          uint8_t *user_record, size_t user_record_len, void *user_data)
{
    (void)user_record;
    (void)user_record_len;
#endif
    QSqlQuery q = getQuery(user_data);
    q.prepare("INSERT OR REPLACE INTO session_store (jid, device_id, session) VALUES (?, ?, ?)");
    q.addBindValue(addrName(address));
    q.addBindValue(address->device_id);
    q.addBindValue(QByteArray(reinterpret_cast<char *>(record), static_cast<int>(record_len)));
    return q.exec() ? SG_SUCCESS : -1;
}

int Storage::containsSession(const signal_protocol_address *address, void *user_data)
{
    QSqlQuery q = lookupSession(address, user_data);
    return q.next() ? 1 : 0;
}

int Storage::loadPreKey(signal_buffer **record, uint32_t pre_key_id, void *user_data)
{
    QSqlQuery q = getQuery(user_data);
    q.prepare("SELECT pre_key FROM pre_key_store WHERE id IS ?");
    q.addBindValue(pre_key_id);
    q.exec();
    return q.next() ? toSignalBuffer(q.value(0), record) : SG_ERR_INVALID_KEY_ID;
}

int Storage::removePreKey(uint32_t pre_key_id, void *user_data)
{
    QSqlQuery q = getQuery(user_data);
    q.prepare("DELETE FROM pre_key_store WHERE id IS ?");
    q.addBindValue(pre_key_id);
    return q.exec() ? SG_SUCCESS : -1;
}

int Storage::loadSignedPreKey(signal_buffer **record, uint32_t signed_pre_key_id, void *user_data)
{
    QVariant value = lookupValue(user_data, "signed_pre_key_id");
    if (value.isNull() || value.toUInt() != signed_pre_key_id) {
        return SG_ERR_INVALID_KEY_ID;
    }
    value = lookupValue(user_data, "signed_pre_key");
    return value.isNull() ? SG_ERR_INVALID_KEY_ID : toSignalBuffer(value, record);
}

uint32_t Storage::signedPreKeyid() { return lookupValue(this, "signed_pre_key_id").toUInt(); }

int Storage::getIdentityKeyPair(signal_buffer **public_data, signal_buffer **private_data, void *user_data)
{
    QVariant result = lookupValue(user_data, "own_public_key");
    if (result.isNull()) {
        return SG_ERR_INVALID_KEY_ID;
    }
    toSignalBuffer(result, public_data);

    result = lookupValue(user_data, "own_private_key");
    if (result.isNull()) {
        return SG_ERR_INVALID_KEY_ID;
    }
    toSignalBuffer(result, private_data);

    return SG_SUCCESS;
}

QVariant Storage::lookupValue(void *user_data, const QString &key)
{
    QSqlQuery q = getQuery(user_data);
    q.prepare("SELECT value FROM simple_store WHERE key IS ?");
    q.addBindValue(key);
    q.exec();
    return q.next() ? q.value(0) : QVariant();
}

void Storage::storeValue(const QString &key, const QVariant &value)
{
    QSqlQuery q(db());
    q.prepare("INSERT OR REPLACE INTO simple_store (key, value) VALUES (?, ?)");
    q.addBindValue(key);
    q.addBindValue(value);
    q.exec();
}

int Storage::getLocalRegistrationId(void *user_data, uint32_t *registration_id)
{
    QVariant result = lookupValue(user_data, "registration_id");
    if (result.isNull()) {
        return -1;
    }
    *registration_id = result.toUInt();
    return SG_SUCCESS;
}

bool Storage::identityExists(const signal_protocol_address *addr_p) const
{
    QSqlQuery q(db());
    q.prepare("SELECT COUNT(*) FROM identity_key_store WHERE jid IS ? AND device_id IS ?");
    q.addBindValue(addrName(addr_p));
    q.addBindValue(addr_p->device_id);
    q.exec();
    return q.next() && q.value(0).toInt() == 1;
}

int Storage::saveIdentity(const signal_protocol_address *addr_p, uint8_t *key_data, size_t key_len, void *user_data)
{
    QSqlQuery q = getQuery(user_data);

    if (key_data != nullptr) {
        q.prepare("INSERT OR REPLACE INTO identity_key_store (key, jid, device_id) VALUES (?, ?, ?)");
        q.addBindValue(QByteArray(reinterpret_cast<char *>(key_data), static_cast<int>(key_len)));
    } else {
        q.prepare("DELETE FROM identity_key_store WHERE jid IS ? AND device_id IS ?");
    }
    q.addBindValue(addrName(addr_p));
    q.addBindValue(addr_p->device_id);
    return q.exec() ? SG_SUCCESS : -1;
}

int Storage::isTrustedIdentity(const signal_protocol_address *addr_p, uint8_t *key_data, size_t key_len,
                               void *user_data)
{
    Q_UNUSED(addr_p);
    Q_UNUSED(key_data);
    Q_UNUSED(key_len);
    Q_UNUSED(user_data);
    return 1;
}

signal_protocol_store_context *Storage::storeContext() const { return m_storeContext; }

bool Storage::isTrusted(QString const &user, uint32_t deviceId)
{
    QSqlQuery q(db());
    q.prepare("SELECT trust FROM devices where jid IS ? AND device_id IS ?");
    q.addBindValue(user);
    q.addBindValue(deviceId);
    q.exec();
    return q.next() && q.value(0).toInt() == TRUSTED;
}

QByteArray Storage::loadDeviceIdentity(const QString &user, uint32_t deviceId)
{
    QSqlQuery q(db());
    q.prepare("SELECT key FROM identity_key_store WHERE jid IS ? AND device_id IS ?");
    q.addBindValue(user);
    q.addBindValue(deviceId);
    q.exec();

    QByteArray res;
    if (q.next()) {
        res = q.value(0).toByteArray();
    }
    return res;
}

void Storage::removeDevice(const QString &user, uint32_t deviceId)
{
    QSqlDatabase _db(db());
    QSqlQuery    q(_db);

    _db.transaction();
    {
        q.prepare("DELETE FROM devices WHERE jid IS ? AND device_id IS ?");
        q.addBindValue(user);
        q.addBindValue(deviceId);
        q.exec();

        // q.prepare("DELETE FROM identity_key_store WHERE jid IS ? AND device_id IS ?");
        // q.addBindValue(user);
        // q.addBindValue(deviceId);
        // q.exec();
        //
        // q.prepare("DELETE FROM session_store WHERE jid IS ? AND device_id IS ?");
        // q.addBindValue(user);
        // q.addBindValue(deviceId);
        // q.exec();
    }
    _db.commit();
}

void Storage::setDeviceTrust(const QString &user, uint32_t deviceId, bool trusted)
{
    QSqlQuery q(db());
    q.prepare("UPDATE devices SET trust = ? WHERE jid IS ? AND device_id IS ?");
    q.addBindValue(trusted ? TRUSTED : UNTRUSTED);
    q.addBindValue(user);
    q.addBindValue(deviceId);
    q.exec();
}

void Storage::removeCurrentDevice()
{
    QSqlDatabase _db(db());
    QSqlQuery    q(_db);

    _db.transaction();
    {
        q.exec("DROP TABLE devices");
        q.exec("DROP TABLE enabled_buddies");
        q.exec("DROP TABLE identity_key_store");
        q.exec("DROP TABLE pre_key_store");
        q.exec("DROP TABLE session_store");
        q.exec("DROP TABLE simple_store");
    }
    _db.commit();
}

bool Storage::isEnabledForUser(const QString &user)
{
    QSqlQuery q(db());
    q.prepare("SELECT jid FROM enabled_buddies WHERE jid IS ?");
    q.addBindValue(user);
    q.exec();
    return q.next();
}

void Storage::setEnabledForUser(const QString &user, bool enabled)
{
    QSqlQuery q(db());
    q.prepare(enabled ? "INSERT OR REPLACE INTO enabled_buddies (jid) VALUES (?)"
                      : "DELETE FROM enabled_buddies WHERE jid IS ?");
    q.addBindValue(user);
    q.exec();
}

QVector<std::tuple<QString, QByteArray, uint32_t, TRUST_STATE>> Storage::getKnownFingerprints()
{
    QVector<std::tuple<QString, QByteArray, uint32_t, TRUST_STATE>> res;
    QSqlQuery                                                       q(db());
    q.prepare("SELECT devices.jid, key, devices.device_id, trust FROM devices, identity_key_store WHERE "
              "devices.jid=identity_key_store.jid and devices.device_id=identity_key_store.device_id");
    q.exec();
    while (q.next()) {
        res.append(std::make_tuple(q.value(0).toString(), q.value(1).toByteArray(), q.value(2).toUInt(),
                                   static_cast<TRUST_STATE>(q.value(3).toInt())));
    }
    return res;
}

QByteArray toQByteArray(signal_buffer *buffer)
{
    return QByteArray(reinterpret_cast<const char *>(signal_buffer_data(buffer)),
                      static_cast<int>(signal_buffer_len(buffer)));
}
}
