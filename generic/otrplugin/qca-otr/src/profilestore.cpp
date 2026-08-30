#include "qca-otr/profilestore.h"

#include "qca-otr/ake.h"
#include "qca-otr/crypto.h"

#include <QHash>
#include <QtCrypto>

#include <algorithm>
#include <tuple>
#include <utility>

namespace QcaOtr {
namespace {

void setError(QString *error, const QString &message)
{
    if (error)
        *error = message;
}

struct IdentityKey
{
    QByteArray account;
    QByteArray protocol;

    bool operator==(const IdentityKey &other) const noexcept
    {
        return account == other.account && protocol == other.protocol;
    }
};

struct FingerprintKey
{
    QByteArray username;
    QByteArray account;
    QByteArray protocol;
    QByteArray fingerprint;

    bool operator==(const FingerprintKey &other) const noexcept
    {
        return username == other.username && account == other.account && protocol == other.protocol &&
            fingerprint == other.fingerprint;
    }
};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
uint qHash(const IdentityKey &key, uint seed = 0) noexcept
{
    return ::qHash(key.protocol, ::qHash(key.account, seed));
}

uint qHash(const FingerprintKey &key, uint seed = 0) noexcept
{
    seed = ::qHash(key.username, seed);
    seed = ::qHash(key.account, seed);
    seed = ::qHash(key.protocol, seed);
    return ::qHash(key.fingerprint, seed);
}
#else
size_t qHash(const IdentityKey &key, size_t seed = 0) noexcept
{
    return ::qHash(key.protocol, ::qHash(key.account, seed));
}

size_t qHash(const FingerprintKey &key, size_t seed = 0) noexcept
{
    seed = ::qHash(key.username, seed);
    seed = ::qHash(key.account, seed);
    seed = ::qHash(key.protocol, seed);
    return ::qHash(key.fingerprint, seed);
}
#endif

IdentityKey identityKey(const PrivateKeyRecord &record)
{
    return {record.account, record.protocol};
}

IdentityKey identityKey(const InstanceTagRecord &record)
{
    return {record.account, record.protocol};
}

FingerprintKey fingerprintKey(const FingerprintRecord &record)
{
    return {record.username, record.account, record.protocol, record.fingerprint};
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

quint32 randomInstanceTag(const QHash<IdentityKey, InstanceTagRecord> &instanceTags)
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
        for (auto it = instanceTags.cbegin(); it != instanceTags.cend(); ++it) {
            if (it.value().instanceTag == candidate) {
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

class ProfileStore::Private
{
public:
    Private(QString directoryValue, QByteArray protocolValue) :
        directory(std::move(directoryValue)), protocolId(std::move(protocolValue))
    {
    }

    ProfileData snapshot() const
    {
        ProfileData result;
        result.privateKeys = identities.values();
        result.fingerprints = fingerprintRecords.values();
        result.instanceTags = instanceTags.values();

        std::sort(result.privateKeys.begin(), result.privateKeys.end(), [](const PrivateKeyRecord &a, const PrivateKeyRecord &b) {
            return std::tie(a.account, a.protocol) < std::tie(b.account, b.protocol);
        });
        std::sort(result.fingerprints.begin(), result.fingerprints.end(), [](const FingerprintRecord &a, const FingerprintRecord &b) {
            return std::tie(a.username, a.account, a.protocol, a.fingerprint) <
                std::tie(b.username, b.account, b.protocol, b.fingerprint);
        });
        std::sort(result.instanceTags.begin(), result.instanceTags.end(), [](const InstanceTagRecord &a, const InstanceTagRecord &b) {
            return std::tie(a.account, a.protocol) < std::tie(b.account, b.protocol);
        });
        return result;
    }

    const ProfileData &cachedSnapshot() const
    {
        if (cacheDirty) {
            cache = snapshot();
            cacheDirty = false;
        }
        return cache;
    }

    void changed()
    {
        cacheDirty = true;
    }

    QString directory;
    QByteArray protocolId;
    QHash<IdentityKey, PrivateKeyRecord> identities;
    QHash<FingerprintKey, FingerprintRecord> fingerprintRecords;
    QHash<IdentityKey, InstanceTagRecord> instanceTags;
    mutable ProfileData cache;
    mutable bool cacheDirty = true;
};

ProfileStore::ProfileStore(QString directory, QByteArray protocolId) :
    d(std::make_unique<Private>(std::move(directory), std::move(protocolId)))
{
}

ProfileStore::~ProfileStore() = default;

const QString &ProfileStore::directory() const
{
    return d->directory;
}

const QByteArray &ProfileStore::protocolId() const
{
    return d->protocolId;
}

bool ProfileStore::load(QString *error)
{
    if (d->protocolId.isEmpty()) {
        setError(error, QStringLiteral("OTR protocol id must not be empty"));
        return false;
    }

    ProfileData loaded;
    if (!Persistence::loadProfile(d->directory, &loaded, error))
        return false;

    QHash<IdentityKey, PrivateKeyRecord> identities;
    QHash<FingerprintKey, FingerprintRecord> fingerprints;
    QHash<IdentityKey, InstanceTagRecord> instanceTags;

    identities.reserve(loaded.privateKeys.size());
    fingerprints.reserve(loaded.fingerprints.size());
    instanceTags.reserve(loaded.instanceTags.size());

    // QHash::insert replaces an existing value, so sequential insertion gives
    // the same last-record-wins behavior as libotr while normalizing duplicates.
    for (const PrivateKeyRecord &record : loaded.privateKeys)
        identities.insert(identityKey(record), record);
    for (const FingerprintRecord &record : loaded.fingerprints)
        fingerprints.insert(fingerprintKey(record), record);
    for (const InstanceTagRecord &record : loaded.instanceTags)
        instanceTags.insert(identityKey(record), record);

    d->identities = std::move(identities);
    d->fingerprintRecords = std::move(fingerprints);
    d->instanceTags = std::move(instanceTags);
    d->changed();
    return true;
}

bool ProfileStore::save(QString *error) const
{
    if (d->protocolId.isEmpty()) {
        setError(error, QStringLiteral("OTR protocol id must not be empty"));
        return false;
    }
    return Persistence::saveProfile(d->directory, d->snapshot(), error);
}

QList<PrivateKeyRecord> ProfileStore::identities() const
{
    QList<PrivateKeyRecord> result;
    result.reserve(d->identities.size());
    for (auto it = d->identities.cbegin(); it != d->identities.cend(); ++it) {
        if (it.value().protocol == d->protocolId)
            result.append(it.value());
    }
    std::sort(result.begin(), result.end(), [](const PrivateKeyRecord &a, const PrivateKeyRecord &b) {
        return a.account < b.account;
    });
    return result;
}

const PrivateKeyRecord *ProfileStore::identity(const QByteArray &account) const
{
    const auto it = d->identities.constFind({account, d->protocolId});
    return it == d->identities.cend() ? nullptr : &it.value();
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

    const IdentityKey key{account, d->protocolId};
    if (d->identities.contains(key))
        return true;

    PrivateKeyRecord record;
    record.account = account;
    record.protocol = d->protocolId;
    record.key = generateIdentity(error);
    if (record.key.x <= QCA::BigInteger(0))
        return false;

    d->identities.insert(key, record);
    if (save(error)) {
        d->changed();
        return true;
    }
    d->identities.remove(key);
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

    const IdentityKey key{account, d->protocolId};
    const auto existing = d->identities.constFind(key);
    const bool hadPrevious = existing != d->identities.cend();
    const PrivateKeyRecord previous = hadPrevious ? existing.value() : PrivateKeyRecord();

    PrivateKeyRecord replacement = hadPrevious ? previous : PrivateKeyRecord();
    replacement.account = account;
    replacement.protocol = d->protocolId;
    replacement.key = generated;
    d->identities.insert(key, replacement);

    if (save(error)) {
        d->changed();
        return true;
    }
    if (hadPrevious)
        d->identities.insert(key, previous);
    else
        d->identities.remove(key);
    return false;
}

bool ProfileStore::removeIdentity(const QByteArray &account, QString *error)
{
    const IdentityKey key{account, d->protocolId};
    const auto it = d->identities.constFind(key);
    if (it == d->identities.cend())
        return true;

    const PrivateKeyRecord previous = it.value();
    d->identities.remove(key);
    if (save(error)) {
        d->changed();
        return true;
    }
    d->identities.insert(key, previous);
    return false;
}

quint32 ProfileStore::instanceTag(const QByteArray &account, bool *found) const
{
    if (found)
        *found = false;
    const auto it = d->instanceTags.constFind({account, d->protocolId});
    if (it == d->instanceTags.cend())
        return 0;
    if (found)
        *found = true;
    return it.value().instanceTag;
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

    const quint32 generated = randomInstanceTag(d->instanceTags);
    if (generated == 0) {
        setError(error, QStringLiteral("Cannot generate a secure OTR instance tag"));
        return 0;
    }

    const IdentityKey key{account, d->protocolId};
    InstanceTagRecord record;
    record.account = account;
    record.protocol = d->protocolId;
    record.instanceTag = generated;
    d->instanceTags.insert(key, record);
    if (!save(error)) {
        d->instanceTags.remove(key);
        return 0;
    }
    d->changed();
    if (created)
        *created = true;
    return generated;
}

QList<FingerprintRecord> ProfileStore::fingerprints() const
{
    QList<FingerprintRecord> result;
    result.reserve(d->fingerprintRecords.size());
    for (auto it = d->fingerprintRecords.cbegin(); it != d->fingerprintRecords.cend(); ++it) {
        if (it.value().protocol == d->protocolId)
            result.append(it.value());
    }
    std::sort(result.begin(), result.end(), [](const FingerprintRecord &a, const FingerprintRecord &b) {
        return std::tie(a.username, a.account, a.fingerprint) < std::tie(b.username, b.account, b.fingerprint);
    });
    return result;
}

const FingerprintRecord *ProfileStore::fingerprint(const QByteArray &username,
                                                   const QByteArray &account,
                                                   const QByteArray &fingerprintValue) const
{
    const auto it = d->fingerprintRecords.constFind({username, account, d->protocolId, fingerprintValue});
    return it == d->fingerprintRecords.cend() ? nullptr : &it.value();
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

    const FingerprintKey key{username, account, d->protocolId, fingerprintValue};
    if (d->fingerprintRecords.contains(key))
        return true;

    FingerprintRecord record;
    record.username = username;
    record.account = account;
    record.protocol = d->protocolId;
    record.fingerprint = fingerprintValue;
    record.trust = trust;
    d->fingerprintRecords.insert(key, record);
    if (!save(error)) {
        d->fingerprintRecords.remove(key);
        return false;
    }
    d->changed();
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
    const FingerprintKey key{username, account, d->protocolId, fingerprintValue};
    auto it = d->fingerprintRecords.find(key);
    if (it == d->fingerprintRecords.end()) {
        setError(error, QStringLiteral("OTR fingerprint not found"));
        return false;
    }

    const QByteArray previousTrust = it.value().trust;
    it.value().trust = trust;
    if (save(error)) {
        d->changed();
        return true;
    }
    it.value().trust = previousTrust;
    return false;
}

bool ProfileStore::removeFingerprint(const QByteArray &username,
                                     const QByteArray &account,
                                     const QByteArray &fingerprintValue,
                                     QString *error)
{
    const FingerprintKey key{username, account, d->protocolId, fingerprintValue};
    const auto it = d->fingerprintRecords.constFind(key);
    if (it == d->fingerprintRecords.cend())
        return true;

    const FingerprintRecord previous = it.value();
    d->fingerprintRecords.remove(key);
    if (save(error)) {
        d->changed();
        return true;
    }
    d->fingerprintRecords.insert(key, previous);
    return false;
}

const ProfileData &ProfileStore::data() const
{
    return d->cachedSnapshot();
}

} // namespace QcaOtr
