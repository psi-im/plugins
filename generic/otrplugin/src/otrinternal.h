/*
 * otrinternal.h - Native qca-otr backend for the Psi OTR plugin
 *
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef OTRINTERNAL_H_
#define OTRINTERNAL_H_

#include "otrmessaging.h"

#include "qca-otr/profilestore.h"
#include "qca-otr/session.h"

#include <QHash>
#include <QString>

#include <memory>

class OtrInternal
{
public:
    OtrInternal(psiotr::OtrCallback *callback, psiotr::OtrPolicy &policy);
    ~OtrInternal();

    QString encryptMessage(const QString &account, const QString &contact, const QString &message);
    psiotr::OtrMessageType decryptMessage(const QString &account,
                                          const QString &contact,
                                          const QString &message,
                                          QString &decrypted);

    QList<psiotr::Fingerprint> getFingerprints();
    void verifyFingerprint(const psiotr::Fingerprint &fingerprint, bool verified);
    void deleteFingerprint(const psiotr::Fingerprint &fingerprint);

    QHash<QString, QString> getPrivateKeys();
    void deleteKey(const QString &account);
    void generateKey(const QString &account);

    void startSession(const QString &account, const QString &contact);
    void endSession(const QString &account, const QString &contact);
    void expireSession(const QString &account, const QString &contact);

    void startSMP(const QString &account, const QString &contact, const QString &question, const QString &secret);
    void continueSMP(const QString &account, const QString &contact, const QString &secret);
    void abortSMP(const QString &account, const QString &contact);

    psiotr::OtrMessageState getMessageState(const QString &account, const QString &contact);
    QString getMessageStateString(const QString &account, const QString &contact);
    QString getSessionId(const QString &account, const QString &contact);
    psiotr::Fingerprint getActiveFingerprint(const QString &account, const QString &contact);
    bool isVerified(const QString &account, const QString &contact);
    bool smpSucceeded(const QString &account, const QString &contact);

private:
    struct Conversation;

    static QcaOtr::SessionPolicy nativePolicy(psiotr::OtrPolicy policy);
    static QString conversationKey(const QString &account, const QString &contact);

    Conversation *conversation(const QString &account, const QString &contact, bool create);
    const Conversation *conversation(const QString &account, const QString &contact) const;
    bool ensureAccount(const QString &account, QString *error = nullptr);

    void sendTransport(const QString &account, const QString &contact, const QVector<QByteArray> &messages);
    void handleSmpEvent(Conversation *conversation, const QcaOtr::SessionResult &result);
    void handleAuthenticated(Conversation *conversation, quint32 peerInstance, bool wasSecure);
    bool rememberFingerprint(Conversation *conversation, quint32 peerInstance, bool *created = nullptr);
    quint32 activePeer(const Conversation *conversation) const;
    bool activeEstablished(const Conversation *conversation,
                           quint32 *peerInstance,
                           QcaOtr::AkeEstablishedSession *established) const;
    bool activeFingerprintMatches(const psiotr::Fingerprint &fingerprint, Conversation **conversation = nullptr) const;
    void displayProtocolError(const QString &account, const QString &contact, const QString &message);

    psiotr::OtrCallback *m_callback;
    psiotr::OtrPolicy &m_otrPolicy;
    std::unique_ptr<QCA::Initializer> m_qca;
    std::unique_ptr<QcaOtr::ProfileStore> m_profile;
    bool m_profileLoaded = false;
    QHash<QString, std::shared_ptr<Conversation>> m_conversations;
};

#endif
