/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "qca-otr/profile.h"

#include <QByteArray>
#include <QList>
#include <QString>

#include <memory>

namespace QcaOtr {

/**
 * Mutable application-facing view of the three libotr-compatible profile stores.
 *
 * Persistence remains line-oriented, while the in-memory representation is
 * normalized by logical key with libotr-compatible last-record-wins semantics.
 * The protocol id is opaque application metadata; qca-otr does not assign an
 * XMPP/libpurple-specific value.
 */
class ProfileStore
{
public:
    /** Creates a store rooted at @p directory for one opaque @p protocolId. */
    ProfileStore(QString directory, QByteArray protocolId);
    ~ProfileStore();

    ProfileStore(const ProfileStore &) = delete;
    ProfileStore &operator=(const ProfileStore &) = delete;

    const QString &directory() const;
    const QByteArray &protocolId() const;

    /** Loads all profile files and normalizes duplicate logical records. */
    bool load(QString *error = nullptr);

    /** Writes the current normalized state back to the profile files. */
    bool save(QString *error = nullptr) const;

    /** Returns normalized identities for this store's protocol id. */
    QList<PrivateKeyRecord> identities() const;

    /** Returns the effective identity for @p account, or nullptr if absent. */
    const PrivateKeyRecord *identity(const QByteArray &account) const;

    /** Returns the 20-byte fingerprint of @p account's current identity. */
    QByteArray identityFingerprint(const QByteArray &account) const;

    /** Creates an identity only when @p account does not already have one. */
    bool ensureIdentity(const QByteArray &account, QString *error = nullptr);

    /** Replaces @p account's identity with a newly generated DSA key. */
    bool regenerateIdentity(const QByteArray &account, QString *error = nullptr);

    /** Removes @p account's identity from the store. */
    bool removeIdentity(const QByteArray &account, QString *error = nullptr);

    /** Returns the effective instance tag for @p account. */
    quint32 instanceTag(const QByteArray &account, bool *found = nullptr) const;

    /** Ensures @p account has a valid random instance tag and returns it. */
    quint32 ensureInstanceTag(const QByteArray &account, bool *created = nullptr, QString *error = nullptr);

    /** Returns normalized fingerprint records for this store's protocol id. */
    QList<FingerprintRecord> fingerprints() const;

    /** Finds the effective fingerprint record for the exact logical key. */
    const FingerprintRecord *fingerprint(const QByteArray &username,
                                         const QByteArray &account,
                                         const QByteArray &fingerprint) const;

    /**
     * Adds or updates a fingerprint record.
     * @param created optionally receives true when no equivalent logical record
     * existed before the call.
     */
    bool rememberFingerprint(const QByteArray &username,
                             const QByteArray &account,
                             const QByteArray &fingerprint,
                             const QByteArray &trust = {},
                             bool *created = nullptr,
                             QString *error = nullptr);

    /** Replaces the opaque trust bytes associated with one fingerprint. */
    bool setFingerprintTrust(const QByteArray &username,
                             const QByteArray &account,
                             const QByteArray &fingerprint,
                             const QByteArray &trust,
                             QString *error = nullptr);

    /** Removes one fingerprint logical record. */
    bool removeFingerprint(const QByteArray &username,
                           const QByteArray &account,
                           const QByteArray &fingerprint,
                           QString *error = nullptr);

    /**
     * Returns a deterministic persistence DTO.
     *
     * The returned lists are normalized and sorted; ProfileStore's canonical
     * in-memory representation is hash-based.
     */
    const ProfileData &data() const;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QcaOtr
