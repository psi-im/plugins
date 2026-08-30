#include "qca-otr/profilestore.h"

#include "qca-otr/ake.h"
#include "qca-otr/crypto.h"

#include <QtCrypto>

#include <utility>

namespace QcaOtr {
namespace {

void setError(QString *error, const QString &message)
{
    if (error)
        *error = message;
}

DsaPrivateKey generateIdentity(QString *error)
{
    if (!QCA::isSupported("pkey") || !QCA::PKey::supportedTypes().contains(QCA::PKey::DSA)) {
        setError(error, QStringLiteral("QCA provider does not support DSA key generation"));
        return {};
    }

    QCA::KeyGenerator generator;
    const QCA::DLGroup domain = generator.createDLGroup(QCA::DSA_1024);
    if (domain.isNull()) {
        setError(error, QStringLiteral("Cannot generate DSA domain parameters"));
        return {};
    }

    const QCA::PrivateKey generated = generator.createDSA(domain);
    if (generated.isNull() || !generated.isDSA()) {
        setError(error, QStringLiteral("Cannot generate DSA private key"));
        return {};
    }

    const QCA::DSAPrivateKey dsa = generated.toDSA();
    if (dsa.isNull() || dsa.x() <= QCA::BigInteger(0)) {
        setError(error, QStringLiteral("Generated DSA private key is invalid"));
        return {};
    }

    const QCA::DLGroup keyDomain = dsa.domain();
    DsaPrivateKey key;
    key.domain.p = keyDomain.p();
    key.domain.q = keyDomain.q();
    key.domain.g = keyDomain.g();
    key.x = dsa.x();
    if (dsaPublicKey(key).y <= QCA::BigInteger(0)) {
        setError(error, QStringLiteral("Generated DSA public key is invalid"));
        return {};
    }
    return key;
}

quint32 randomInstanceTag(const ProfileData &profile)
{
    if (!QCA::haveSecureRandom())
        return 0;

    for (int attempt = 0; attempt < 64; ++attempt) {
        const QCA::SecureArray bytes = QCA::Random::randomArray(4);
        if (bytes.size() != 4)
            return 0;

        const auto *p = reinterpret_cast<const unsigned char *>(bytes.constData());
        const quint32 candidate = (static_cast<quint32>(p[0]) << 24) |
            (static_cast<quint32>(p[1]) << 16) |
            (static_cast<quint32>(p[2]) << 8) |
            static_cast<quint32>(p[3]);
        if (candidate < 0x00000100)
            continue;

        bool duplicate = false;
        for (const InstanceTagRecord &record : profile.instanceTags) {
            if (record.instanceTag == candidate) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate)
            return candidate;
    }
    return 0;
}

} // namespace

ProfileStore::ProfileStore(QString directory, QByteArray protocolId) :
    directory_(std::move(directory)), protocolId_(std::move(protocolId))
{
}

const QString &ProfileStore::directory() const
{
    return directory_;
}

const QByteArray &ProfileStore::protocolId() const
{
    return protocolId_;
}

bool ProfileStore::load(QString *error)
{
    if (protocolId_.isEmpty()) {
        setError(error, QStringLiteral("OTR protocol id must not be empty"));
        return false;
    }
    return Persistence::loadProfile(directory_, &data_, error);
}

bool ProfileStore::save(QString *error) const
{
    if (protocolId_.isEmpty()) {
        setError(error, QStringLiteral("OTR protocol id must not be empty"));
        return false;
    }
    return Persistence::saveProfile(directory_, data_, error);
}

const PrivateKeyRecord *ProfileStore::identity(const QByteArray &account) const
{
    return Persistence::findPrivateKey(data_, account, protocolId_);
}

QByteArray ProfileStore::identityFingerprint(const QByteArray &account) const
{
    const PrivateKeyRecord *record = identity(account);
    return record ? dsaPublicKeyFingerprint(dsaPublicKey(record->key)) : QByteArray();
}

bool ProfileStore::ensureIdentity(const QByteArray &account, QString *error)
{
    if (account.isEmpty()) {
        setError(error, QStringLiteral("OTR account id must not be empty"));
        return false;
    }
    if (identity(account))
        return true;

    PrivateKeyRecord record;
    record.account = account;
    record.protocol = protocolId_;
    record.key = generateIdentity(error);
    if (record.key.x <= QCA::BigInteger(0))
        return false;

    data_.privateKeys.append(record);
    if (save(error))
        return true;
    data_.privateKeys.removeLast();
    return false;
}

bool ProfileStore::regenerateIdentity(const QByteArray &account, QString *error)
{
    if (account.isEmpty()) {
        setError(error, QStringLiteral("OTR account id must not be empty"));
        return false;
    }

    DsaPrivateKey generated = generateIdentity(error);
    if (generated.x <= QCA::BigInteger(0))
        return false;

    int replaceIndex = -1;
    for (int i = data_.privateKeys.size() - 1; i >= 0; --i) {
        const PrivateKeyRecord &record = data_.privateKeys.at(i);
        if (record.account == account && record.protocol == protocolId_) {
            replaceIndex = i;
            break;
        }
    }

    if (replaceIndex < 0) {
        PrivateKeyRecord record;
        record.account = account;
        record.protocol = protocolId_;
        record.key = generated;
        data_.privateKeys.append(record);
        if (save(error))
            return true;
        data_.privateKeys.removeLast();
        return false;
    }

    const DsaPrivateKey previous = data_.privateKeys.at(replaceIndex).key;
    data_.privateKeys[replaceIndex].key = generated;
    if (save(error))
        return true;
    data_.privateKeys[replaceIndex].key = previous;
    return false;
}

bool ProfileStore::removeIdentity(const QByteArray &account, QString *error)
{
    bool removed = false;
    for (int i = data_.privateKeys.size() - 1; i >= 0; --i) {
        const PrivateKeyRecord &record = data_.privateKeys.at(i);
        if (record.account == account && record.protocol == protocolId_) {
            data_.privateKeys.removeAt(i);
            removed = true;
        }
    }
    return !removed || save(error);
}

quint32 ProfileStore::instanceTag(const QByteArray &account, bool *found) const
{
    return Persistence::findInstanceTag(data_, account, protocolId_, found);
}

quint32 ProfileStore::ensureInstanceTag(const QByteArray &account, bool *created, QString *error)
{
    if (created)
        *created = false;
    bool found = false;
    const quint32 existing = instanceTag(account, &found);
    if (found)
        return existing;

    if (account.isEmpty()) {
        setError(error, QStringLiteral("OTR account id must not be empty"));
        return 0;
    }

    const quint32 generated = randomInstanceTag(data_);
    if (generated == 0) {
        setError(error, QStringLiteral("Cannot generate a secure OTR instance tag"));
        return 0;
    }

    InstanceTagRecord record;
    record.account = account;
    record.protocol = protocolId_;
    record.instanceTag = generated;
    data_.instanceTags.append(record);
    if (!save(error)) {
        data_.instanceTags.removeLast();
        return 0;
    }
    if (created)
        *created = true;
    return generated;
}

QList<FingerprintRecord> ProfileStore::fingerprints() const
{
    QList<FingerprintRecord> result;
    for (const FingerprintRecord &record : data_.fingerprints) {
        if (record.protocol == protocolId_)
            result.append(record);
    }
    return result;
}

const FingerprintRecord *ProfileStore::fingerprint(const QByteArray &username,
                                                   const QByteArray &account,
                                                   const QByteArray &fingerprintValue) const
{
    return Persistence::findFingerprint(data_, username, account, fingerprintValue, protocolId_);
}

bool ProfileStore::rememberFingerprint(const QByteArray &username,
                                       const QByteArray &account,
                                       const QByteArray &fingerprintValue,
                                       const QByteArray &trust,
                                       bool *created,
                                       QString *error)
{
    if (created)
        *created = false;
    if (username.isEmpty() || account.isEmpty() || fingerprintValue.size() != 20) {
        setError(error, QStringLiteral("Invalid OTR fingerprint identity"));
        return false;
    }
    if (fingerprint(username, account, fingerprintValue))
        return true;

    FingerprintRecord record;
    record.username = username;
    record.account = account;
    record.protocol = protocolId_;
    record.fingerprint = fingerprintValue;
    record.trust = trust;
    data_.fingerprints.append(record);
    if (!save(error)) {
        data_.fingerprints.removeLast();
        return false;
    }
    if (created)
        *created = true;
    return true;
}

bool ProfileStore::setFingerprintTrust(const QByteArray &username,
                                       const QByteArray &account,
                                       const QByteArray &fingerprintValue,
                                       const QByteArray &trust,
                                       QString *error)
{
    for (int i = data_.fingerprints.size() - 1; i >= 0; --i) {
        FingerprintRecord &record = data_.fingerprints[i];
        if (record.username == username && record.account == account && record.protocol == protocolId_ &&
            record.fingerprint == fingerprintValue) {
            record.trust = trust;
            return save(error);
        }
    }
    setError(error, QStringLiteral("OTR fingerprint not found"));
    return false;
}

bool ProfileStore::removeFingerprint(const QByteArray &username,
                                     const QByteArray &account,
                                     const QByteArray &fingerprintValue,
                                     QString *error)
{
    bool removed = false;
    for (int i = data_.fingerprints.size() - 1; i >= 0; --i) {
        const FingerprintRecord &record = data_.fingerprints.at(i);
        if (record.username == username && record.account == account && record.protocol == protocolId_ &&
            record.fingerprint == fingerprintValue) {
            data_.fingerprints.removeAt(i);
            removed = true;
        }
    }
    return !removed || save(error);
}

const ProfileData &ProfileStore::data() const
{
    return data_;
}

} // namespace QcaOtr
