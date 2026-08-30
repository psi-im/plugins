/*
 * otrmessaging.h - Psi-facing OTR messaging interface
 */

#ifndef OTRMESSAGING_H_
#define OTRMESSAGING_H_

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>

class OtrInternal;

namespace psiotr {

enum OtrPolicy { OTR_POLICY_OFF, OTR_POLICY_ENABLED, OTR_POLICY_AUTO, OTR_POLICY_REQUIRE };

enum OtrMessageType { OTR_MESSAGETYPE_NONE, OTR_MESSAGETYPE_IGNORE, OTR_MESSAGETYPE_OTR };

enum OtrMessageState {
    OTR_MESSAGESTATE_UNKNOWN,
    OTR_MESSAGESTATE_PLAINTEXT,
    OTR_MESSAGESTATE_ENCRYPTED,
    OTR_MESSAGESTATE_FINISHED
};

enum OtrStateChange {
    OTR_STATECHANGE_GOINGSECURE,
    OTR_STATECHANGE_GONESECURE,
    OTR_STATECHANGE_GONEINSECURE,
    OTR_STATECHANGE_STILLSECURE,
    OTR_STATECHANGE_CLOSE,
    OTR_STATECHANGE_REMOTECLOSE,
    OTR_STATECHANGE_TRUST
};

enum OtrNotifyType { OTR_NOTIFY_INFO, OTR_NOTIFY_WARNING, OTR_NOTIFY_ERROR };

class OtrCallback {
public:
    virtual ~OtrCallback() = default;

    virtual QString dataDir() = 0;
    virtual void sendMessage(const QString &account, const QString &contact, const QString &message) = 0;
    virtual void notifyUser(const QString &account,
                            const QString &contact,
                            const QString &message,
                            const OtrNotifyType &type) = 0;
    virtual bool displayOtrMessage(const QString &account, const QString &contact, const QString &message) = 0;
    virtual void stateChange(const QString &account, const QString &contact, OtrStateChange change) = 0;
    virtual void receivedSMP(const QString &account, const QString &contact, const QString &question) = 0;
    virtual void updateSMP(const QString &account, const QString &contact, int progress) = 0;
    virtual QString humanAccount(const QString &accountId) = 0;
    virtual QString humanAccountPublic(const QString &accountId) = 0;
    virtual QString humanContact(const QString &accountId, const QString &contact) = 0;
};

struct Fingerprint {
    QByteArray value;
    QString account;
    QString username;
    QString fingerprintHuman;
    QString trust;

    Fingerprint() = default;
    Fingerprint(QByteArray value, QString account, QString username, QString trust);

    bool isValid() const { return value.size() == 20; }
};

class OtrMessaging {
public:
    OtrMessaging(OtrCallback *callback, OtrPolicy policy);
    OtrMessaging(const OtrMessaging &) = delete;
    OtrMessaging &operator=(const OtrMessaging &) = delete;
    ~OtrMessaging();

    QString encryptMessage(const QString &account, const QString &contact, const QString &message);
    OtrMessageType decryptMessage(const QString &account,
                                  const QString &contact,
                                  const QString &message,
                                  QString &decrypted);

    QList<Fingerprint> getFingerprints();
    void verifyFingerprint(const Fingerprint &fingerprint, bool verified);
    void deleteFingerprint(const Fingerprint &fingerprint);

    QHash<QString, QString> getPrivateKeys();
    void deleteKey(const QString &account);
    void generateKey(const QString &account);

    void startSession(const QString &account, const QString &contact);
    void endSession(const QString &account, const QString &contact);
    void expireSession(const QString &account, const QString &contact);

    void startSMP(const QString &account, const QString &contact, const QString &question, const QString &secret);
    void continueSMP(const QString &account, const QString &contact, const QString &secret);
    void abortSMP(const QString &account, const QString &contact);

    OtrMessageState getMessageState(const QString &account, const QString &contact);
    QString getMessageStateString(const QString &account, const QString &contact);
    QString getSessionId(const QString &account, const QString &contact);
    Fingerprint getActiveFingerprint(const QString &account, const QString &contact);
    bool isVerified(const QString &account, const QString &contact);
    bool smpSucceeded(const QString &account, const QString &contact);

    void setPolicy(OtrPolicy policy);
    OtrPolicy getPolicy();

    bool displayOtrMessage(const QString &account, const QString &contact, const QString &message);
    void stateChange(const QString &account, const QString &contact, OtrStateChange change);
    QString humanAccount(const QString &accountId);
    QString humanContact(const QString &accountId, const QString &contact);

private:
    OtrPolicy m_otrPolicy;
    OtrInternal *m_impl;
    OtrCallback *m_callback;
};

} // namespace psiotr

#endif
