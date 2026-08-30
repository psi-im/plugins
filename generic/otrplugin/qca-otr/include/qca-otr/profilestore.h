#pragma once

#include "qca-otr/profile.h"

#include <QByteArray>
#include <QList>
#include <QString>

namespace QcaOtr {

// Mutable application-facing view of the three libotr-compatible profile
// stores. The protocol id is opaque application metadata; qca-otr does not
// assign an XMPP/libpurple-specific value.
class ProfileStore
{
public:
    ProfileStore(QString directory, QByteArray protocolId);

    const QString &directory() const;
    const QByteArray &protocolId() const;

    bool load(QString *error = nullptr);
    bool save(QString *error = nullptr) const;

    const PrivateKeyRecord *identity(const QByteArray &account) const;
    QByteArray identityFingerprint(const QByteArray &account) const;
    bool ensureIdentity(const QByteArray &account, QString *error = nullptr);
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

    const ProfileData &data() const;

private:
    QString directory_;
    QByteArray protocolId_;
    ProfileData data_;
};

} // namespace QcaOtr
