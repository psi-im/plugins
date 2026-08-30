#pragma once

#include "qca-otr/profile.h"

#include <QByteArray>
#include <QList>
#include <QString>

#include <memory>

namespace QcaOtr {

// Mutable application-facing view of the three libotr-compatible profile
// stores. Persistence remains line-oriented, while the in-memory view is
// normalized by logical key with libotr-compatible last-record-wins semantics.
// The protocol id is opaque application metadata; qca-otr does not assign an
// XMPP/libpurple-specific value.
class ProfileStore
{
public:
    ProfileStore(QString directory, QByteArray protocolId);
    ~ProfileStore();

    ProfileStore(const ProfileStore &) = delete;
    ProfileStore &operator=(const ProfileStore &) = delete;

    const QString &directory() const;
    const QByteArray &protocolId() const;

    bool load(QString *error = nullptr);
    bool save(QString *error = nullptr) const;

    QList<PrivateKeyRecord> identities() const;
    const PrivateKeyRecord *identity(const QByteArray &account) const;
    QByteArray identityFingerprint(const QByteArray &account) const;
    bool ensureIdentity(const QByteArray &account, QString *error = nullptr);
    bool regenerateIdentity(const QByteArray &account, QString *error = nullptr);
    bool removeIdentity(const QByteArray &account, QString *error = nullptr);

    quint32 instanceTag(const QByteArray &account, bool *found = nullptr) const;
    quint32 ensureInstanceTag(const QByteArray &account, bool *created = nullptr, QString *error = nullptr);

    QList<FingerprintRecord> fingerprints() const;
    const FingerprintRecord *fingerprint(const QByteArray &username,
                                         const QByteArray &account,
                                         const QByteArray &fingerprint) const;
    bool rememberFingerprint(const QByteArray &username,
                             const QByteArray &account,
                             const QByteArray &fingerprint,
                             const QByteArray &trust = {},
                             bool *created = nullptr,
                             QString *error = nullptr);
    bool setFingerprintTrust(const QByteArray &username,
                             const QByteArray &account,
                             const QByteArray &fingerprint,
                             const QByteArray &trust,
                             QString *error = nullptr);
    bool removeFingerprint(const QByteArray &username,
                           const QByteArray &account,
                           const QByteArray &fingerprint,
                           QString *error = nullptr);

    // Deterministic persistence DTO. The returned lists are normalized and
    // sorted; ProfileStore's canonical in-memory representation is hash-based.
    const ProfileData &data() const;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QcaOtr
