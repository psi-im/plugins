/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "qca-otr/persistence.h"

#include <QString>

namespace QcaOtr {

/** Canonical libotr-compatible private-key file name. */
inline constexpr char OtrKeysFileName[] = "otr.keys";

/** Canonical libotr-compatible fingerprint file name. */
inline constexpr char OtrFingerprintsFileName[] = "otr.fingerprints";

/** Canonical libotr-compatible instance-tag file name. */
inline constexpr char OtrInstanceTagsFileName[] = "otr.instags";

/** In-memory representation of the three OTR profile stores. */
struct ProfileData
{
    QList<PrivateKeyRecord> privateKeys;
    QList<FingerprintRecord> fingerprints;
    QList<InstanceTagRecord> instanceTags;
};

namespace Persistence {

/**
 * Loads all OTR profile stores from @p directory.
 * Missing files are treated as empty stores; malformed private-key data fails
 * closed, while tolerant record filtering is implemented at the profile layer.
 */
bool loadProfile(const QString &directory, ProfileData *profile, QString *error = nullptr);

/** Saves all three profile stores atomically where supported by the file layer. */
bool saveProfile(const QString &directory, const ProfileData &profile, QString *error = nullptr);

/**
 * Finds the effective private key for an account/protocol pair.
 * The protocol identifier is opaque application-defined persistence metadata;
 * qca-otr deliberately assigns no XMPP/Psi/libpurple-specific default.
 */
const PrivateKeyRecord *findPrivateKey(const ProfileData &profile,
                                       const QByteArray &account,
                                       const QByteArray &protocol);

/** Finds the effective matching fingerprint record using last-record-wins semantics. */
const FingerprintRecord *findFingerprint(const ProfileData &profile,
                                         const QByteArray &username,
                                         const QByteArray &account,
                                         const QByteArray &fingerprint,
                                         const QByteArray &protocol);

/**
 * Returns the effective instance tag for an account/protocol pair.
 * @param found optionally receives whether a matching record existed.
 */
quint32 findInstanceTag(const ProfileData &profile,
                        const QByteArray &account,
                        const QByteArray &protocol,
                        bool *found = nullptr);

} // namespace Persistence
} // namespace QcaOtr
