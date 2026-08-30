#include "qca-otr/profile.h"

#include <QDir>
#include <QFileInfo>

namespace QcaOtr::Persistence {
namespace {

template<typename Record, typename Reader>
bool readOptionalStore(const QString &path, QList<Record> *records, Reader reader, QString *error)
{
    if (!records)
        return false;
    records->clear();
    if (!QFileInfo::exists(path))
        return true;
    return reader(path, records, error);
}

} // namespace

bool loadProfile(const QString &directory, ProfileData *profile, QString *error)
{
    if (!profile)
        return false;
    if (error)
        error->clear();

    const QDir dir(directory);
    ProfileData loaded;
    if (!readOptionalStore(dir.filePath(OtrKeysFileName), &loaded.privateKeys, readPrivateKeysFile, error) ||
        !readOptionalStore(dir.filePath(OtrFingerprintsFileName), &loaded.fingerprints, readFingerprintsFile, error) ||
        !readOptionalStore(dir.filePath(OtrInstanceTagsFileName), &loaded.instanceTags, readInstanceTagsFile, error)) {
        return false;
    }

    *profile = loaded;
    return true;
}

bool saveProfile(const QString &directory, const ProfileData &profile, QString *error)
{
    if (error)
        error->clear();

    QDir dir(directory);
    if (!dir.exists() && !QDir().mkpath(directory)) {
        if (error)
            *error = QStringLiteral("Cannot create OTR profile directory");
        return false;
    }

    return writePrivateKeysFile(dir.filePath(OtrKeysFileName), profile.privateKeys, error) &&
        writeFingerprintsFile(dir.filePath(OtrFingerprintsFileName), profile.fingerprints, error) &&
        writeInstanceTagsFile(dir.filePath(OtrInstanceTagsFileName), profile.instanceTags, error);
}

const PrivateKeyRecord *findPrivateKey(const ProfileData &profile,
                                       const QByteArray &account,
                                       const QByteArray &protocol)
{
    for (int i = profile.privateKeys.size() - 1; i >= 0; --i) {
        const PrivateKeyRecord &record = profile.privateKeys.at(i);
        if (record.account == account && record.protocol == protocol)
            return &record;
    }
    return nullptr;
}

const FingerprintRecord *findFingerprint(const ProfileData &profile,
                                         const QByteArray &username,
                                         const QByteArray &account,
                                         const QByteArray &fingerprint,
                                         const QByteArray &protocol)
{
    for (int i = profile.fingerprints.size() - 1; i >= 0; --i) {
        const FingerprintRecord &record = profile.fingerprints.at(i);
        if (record.username == username && record.account == account && record.protocol == protocol &&
            record.fingerprint == fingerprint) {
            return &record;
        }
    }
    return nullptr;
}

quint32 findInstanceTag(const ProfileData &profile,
                        const QByteArray &account,
                        const QByteArray &protocol,
                        bool *found)
{
    if (found)
        *found = false;
    for (int i = profile.instanceTags.size() - 1; i >= 0; --i) {
        const InstanceTagRecord &record = profile.instanceTags.at(i);
        if (record.account == account && record.protocol == protocol) {
            if (found)
                *found = true;
            return record.instanceTag;
        }
    }
    return 0;
}

} // namespace QcaOtr::Persistence
