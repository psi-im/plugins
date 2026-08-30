/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "qca-otr/crypto.h"

#include <QByteArray>
#include <QList>
#include <QString>
#include <QtCrypto>
#include <QtGlobal>

namespace QcaOtr {

/** One libotr-compatible private-key record. */
struct PrivateKeyRecord
{
    QByteArray account;
    QByteArray protocol;
    DsaPrivateKey key;
};

/** One libotr-compatible trusted-fingerprint record. */
struct FingerprintRecord
{
    QByteArray username;
    QByteArray account;
    QByteArray protocol;
    QByteArray fingerprint;
    QByteArray trust;
};

/** One libotr-compatible instance-tag record. */
struct InstanceTagRecord
{
    QByteArray account;
    QByteArray protocol;
    quint32 instanceTag = 0;
};

namespace Persistence {

/**
 * Parses a complete `otr.keys` payload.
 *
 * Private-key stores contain long-lived identity secrets, so their serialized
 * representation stays in QCA secure memory. Account/protocol metadata in the
 * parsed records is ordinary non-secret `QByteArray` data.
 */
bool parsePrivateKeys(const QCA::SecureArray &data, QList<PrivateKeyRecord> *records, QString *error = nullptr);

/** Serializes private keys in libotr-compatible gcrypt S-expression form. */
QCA::SecureArray serializePrivateKeys(const QList<PrivateKeyRecord> &records, bool *ok = nullptr);

/** Parses a complete `otr.fingerprints` payload. */
bool parseFingerprints(const QByteArray &data, QList<FingerprintRecord> *records, QString *error = nullptr);

/** Serializes fingerprints using the libotr-compatible line format. */
QByteArray serializeFingerprints(const QList<FingerprintRecord> &records, bool *ok = nullptr);

/** Parses a complete `otr.instags` payload. */
bool parseInstanceTags(const QByteArray &data, QList<InstanceTagRecord> *records, QString *error = nullptr);

/** Serializes instance tags using the libotr-compatible line format. */
QByteArray serializeInstanceTags(const QList<InstanceTagRecord> &records, bool *ok = nullptr);

/** Reads and strictly parses a private-key file using secure file I/O. */
bool readPrivateKeysFile(const QString &path, QList<PrivateKeyRecord> *records, QString *error = nullptr);

/** Atomically writes a private-key file using secure file I/O. */
bool writePrivateKeysFile(const QString &path, const QList<PrivateKeyRecord> &records, QString *error = nullptr);

/** Reads and parses a fingerprint store. */
bool readFingerprintsFile(const QString &path, QList<FingerprintRecord> *records, QString *error = nullptr);

/** Atomically writes a fingerprint store. */
bool writeFingerprintsFile(const QString &path, const QList<FingerprintRecord> &records, QString *error = nullptr);

/** Reads and parses an instance-tag store. */
bool readInstanceTagsFile(const QString &path, QList<InstanceTagRecord> *records, QString *error = nullptr);

/** Atomically writes an instance-tag store. */
bool writeInstanceTagsFile(const QString &path, const QList<InstanceTagRecord> &records, QString *error = nullptr);

} // namespace Persistence
} // namespace QcaOtr
