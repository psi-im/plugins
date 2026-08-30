#pragma once

#include "qca-otr/persistence.h"

#include <QString>

namespace QcaOtr {

inline constexpr char OtrKeysFileName[] = "otr.keys";
inline constexpr char OtrFingerprintsFileName[] = "otr.fingerprints";
inline constexpr char OtrInstanceTagsFileName[] = "otr.instags";

struct ProfileData
{
    QList<PrivateKeyRecord> privateKeys;
    QList<FingerprintRecord> fingerprints;
    QList<InstanceTagRecord> instanceTags;
};

namespace Persistence {

bool loadProfile(const QString &directory, ProfileData *profile, QString *error = nullptr);
bool saveProfile(const QString &directory, const ProfileData &profile, QString *error = nullptr);

// The protocol identifier is application-defined opaque persistence metadata.
// qca-otr deliberately assigns no XMPP/Psi/libpurple-specific default.
const PrivateKeyRecord *findPrivateKey(const ProfileData &profile,
                                       const QByteArray &account,
                                       const QByteArray &protocol);
const FingerprintRecord *findFingerprint(const ProfileData &profile,
                                         const QByteArray &username,
                                         const QByteArray &account,
                                         const QByteArray &fingerprint,
                                         const QByteArray &protocol);
quint32 findInstanceTag(const ProfileData &profile,
                        const QByteArray &account,
                        const QByteArray &protocol,
                        bool *found = nullptr);

} // namespace Persistence
} // namespace QcaOtr
