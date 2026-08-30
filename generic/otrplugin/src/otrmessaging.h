/*
 * otrmessaging.h - Psi-facing OTR messaging interface
 *
 * Off-the-Record Messaging plugin for Psi
 * Copyright (C) 2007-2011  Timo Engel (timo-e@freenet.de)
 *                    2011  Florian Fieber
 *                    2014  Boris Pek (tehnick-8@mail.ru)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

/**
 * Application callbacks needed by the native qca-otr backend.
 *
 * Account and contact identifiers are opaque Psi identifiers/JIDs. The backend
 * owns protocol state; the callback only provides transport, UI and profile
 * integration.
 */
class OtrCallback {
public:
    virtual ~OtrCallback() = default;

    /** Returns the current profile data directory containing the OTR stores. */
    virtual QString dataDir() = 0;

    /** Sends one OTR transport message from @p account to @p contact. */
    virtual void sendMessage(const QString &account, const QString &contact, const QString &message) = 0;

    /** Reports a protocol notification which was not rendered inline. */
    virtual void notifyUser(const QString &account,
                            const QString &contact,
                            const QString &message,
                            const OtrNotifyType &type) = 0;

    /** Displays an OTR system message; returns true when it was handled inline. */
    virtual bool displayOtrMessage(const QString &account, const QString &contact, const QString &message) = 0;

    /** Reports a conversation state transition to the plugin UI. */
    virtual void stateChange(const QString &account, const QString &contact, OtrStateChange change) = 0;

    /** Requests user input for an incoming SMP challenge. */
    virtual void receivedSMP(const QString &account, const QString &contact, const QString &question) = 0;

    /** Reports SMP progress; negative values denote abort/error states. */
    virtual void updateSMP(const QString &account, const QString &contact, int progress) = 0;

    virtual QString humanAccount(const QString &accountId) = 0;
    virtual QString humanAccountPublic(const QString &accountId) = 0;
    virtual QString humanContact(const QString &accountId, const QString &contact) = 0;
};

/** Value object describing one stored peer fingerprint. */
struct Fingerprint {
    /** Raw 20-byte OTR fingerprint. */
    QByteArray value;
    QString account;
    QString username;
    QString fingerprintHuman;
    QString trust;

    Fingerprint() = default;
    Fingerprint(QByteArray value, QString account, QString username, QString trust);

    /** Returns true when @ref value has the required OTR fingerprint length. */
    bool isValid() const { return value.size() == 20; }
};

/**
 * Psi-facing facade around the native qca-otr backend.
 *
 * This class keeps the historical plugin API while all OTR protocol and
 * persistence work is delegated to OtrInternal/qca-otr.
 */
class OtrMessaging {
public:
    OtrMessaging(OtrCallback *callback, OtrPolicy policy);
    OtrMessaging(const OtrMessaging &) = delete;
    OtrMessaging &operator=(const OtrMessaging &) = delete;
    ~OtrMessaging();

    /** Processes an outgoing application message and returns transport text. */
    QString encryptMessage(const QString &account, const QString &contact, const QString &message);

    /**
     * Processes an incoming transport message.
     * @param decrypted receives plaintext when OTR_MESSAGETYPE_OTR is returned.
     */
    OtrMessageType decryptMessage(const QString &account,
                                  const QString &contact,
                                  const QString &message,
                                  QString &decrypted);

    /** Returns all peer fingerprints stored in the current OTR profile. */
    QList<Fingerprint> getFingerprints();

    /** Marks the specified stored fingerprint as verified or unverified. */
    void verifyFingerprint(const Fingerprint &fingerprint, bool verified);

    /** Removes the specified stored peer fingerprint from the profile. */
    void deleteFingerprint(const Fingerprint &fingerprint);

    /** Returns own identity fingerprints keyed by Psi account id. */
    QHash<QString, QString> getPrivateKeys();

    /** Removes the local OTR identity for @p account. */
    void deleteKey(const QString &account);

    /** Regenerates the local OTR identity for @p account. */
    void generateKey(const QString &account);

    /** Starts an explicit OTR negotiation with @p contact. */
    void startSession(const QString &account, const QString &contact);

    /** Sends OTR disconnect messages and returns the conversation to plaintext. */
    void endSession(const QString &account, const QString &contact);

    /** Drops local secure state without sending a disconnect message. */
    void expireSession(const QString &account, const QString &contact);

    /** Starts SMP for the active encrypted session, optionally with a question. */
    void startSMP(const QString &account, const QString &contact, const QString &question, const QString &secret);

    /** Supplies the answer/secret requested by an incoming SMP exchange. */
    void continueSMP(const QString &account, const QString &contact, const QString &secret);

    /** Aborts the SMP exchange for the active OTR session. */
    void abortSMP(const QString &account, const QString &contact);

    /** Returns plaintext/encrypted/finished state for the active peer session. */
    OtrMessageState getMessageState(const QString &account, const QString &contact);

    /** Returns a user-visible description of the active peer session state. */
    QString getMessageStateString(const QString &account, const QString &contact);

    /** Returns the formatted secure-session identifier for the active session. */
    QString getSessionId(const QString &account, const QString &contact);

    /** Returns the peer fingerprint currently bound to the active OTR session. */
    Fingerprint getActiveFingerprint(const QString &account, const QString &contact);

    /** Returns true when the active session's peer fingerprint is trusted/verified. */
    bool isVerified(const QString &account, const QString &contact);

    /** Returns true when SMP has succeeded for the active OTR session. */
    bool smpSucceeded(const QString &account, const QString &contact);

    /** Sets the default policy used for newly processed OTR conversations. */
    void setPolicy(OtrPolicy policy);

    /** Returns the currently configured default OTR policy. */
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
