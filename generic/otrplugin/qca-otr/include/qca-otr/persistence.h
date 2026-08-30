#pragma once

#include "qca-otr/crypto.h"

#include <QByteArray>
#include <QList>
#include <QString>
#include <QtCrypto>
#include <QtGlobal>

namespace QcaOtr {

struct PrivateKeyRecord
{
    QByteArray account;
    QByteArray protocol;
    DsaPrivateKey key;
};

struct FingerprintRecord
{
    QByteArray username;
    QByteArray account;
    QByteArray protocol;
    QByteArray fingerprint;
    QByteArray trust;
};

struct InstanceTagRecord
{
    QByteArray account;
    QByteArray protocol;
    quint32 instanceTag = 0;
};

namespace Persistence {

// Private-key stores contain long-lived identity secrets. Keep their complete
// serialized representation in QCA secure memory; account/protocol metadata in
// the parsed records remains ordinary QByteArray data.
bool parsePrivateKeys(const QCA::SecureArray &data, QList<PrivateKeyRecord> *records, QString *error = nullptr);
QCA::SecureArray serializePrivateKeys(const QList<PrivateKeyRecord> &records, bool *ok = nullptr);

bool parseFingerprints(const QByteArray &data, QList<FingerprintRecord> *records, QString *error = nullptr);
QByteArray serializeFingerprints(const QList<FingerprintRecord> &records, bool *ok = nullptr);

bool parseInstanceTags(const QByteArray &data, QList<InstanceTagRecord> *records, QString *error = nullptr);
QByteArray serializeInstanceTags(const QList<InstanceTagRecord> &records, bool *ok = nullptr);

bool readPrivateKeysFile(const QString &path, QList<PrivateKeyRecord> *records, QString *error = nullptr);
bool writePrivateKeysFile(const QString &path, const QList<PrivateKeyRecord> &records, QString *error = nullptr);

bool readFingerprintsFile(const QString &path, QList<FingerprintRecord> *records, QString *error = nullptr);
bool writeFingerprintsFile(const QString &path, const QList<FingerprintRecord> &records, QString *error = nullptr);

bool readInstanceTagsFile(const QString &path, QList<InstanceTagRecord> *records, QString *error = nullptr);
bool writeInstanceTagsFile(const QString &path, const QList<InstanceTagRecord> &records, QString *error = nullptr);

} // namespace Persistence
} // namespace QcaOtr
