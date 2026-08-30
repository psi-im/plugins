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

/** User policy controlling when OTR negotiation/encryption is attempted. */
enum OtrPolicy {
    /** Disable OTR handling for ordinary conversations. */
    OTR_POLICY_OFF,
    /** Manual mode: OTR is available, but negotiation starts only explicitly. */
    OTR_POLICY_ENABLED,
    /** Opportunistic mode: advertise/discover OTR and negotiate when possible. */
    OTR_POLICY_AUTO,
    /** Require encryption: do not allow application plaintext to leave unencrypted. */
    OTR_POLICY_REQUIRE
};

/** Result of processing an incoming message through the OTR backend. */
enum OtrMessageType {
    /** Message is ordinary plaintext and should continue through Psi unchanged. */
    OTR_MESSAGETYPE_NONE,
    /** Message is OTR protocol/control traffic and must not be shown to the user. */
    OTR_MESSAGETYPE_IGNORE,
    /** Message contained authenticated OTR application plaintext in @c decrypted. */
    OTR_MESSAGETYPE_OTR
};

/** State of the currently selected peer OTR session. */
enum OtrMessageState {
    /** No conversation/session state is available for the account/contact pair. */
    OTR_MESSAGESTATE_UNKNOWN,
    /** Conversation is currently carrying ordinary plaintext. */
    OTR_MESSAGESTATE_PLAINTEXT,
    /** An authenticated encrypted OTR session is established. */
    OTR_MESSAGESTATE_ENCRYPTED,
    /** OTR was explicitly finished; a new negotiation is required before secure messaging resumes. */
    OTR_MESSAGESTATE_FINISHED
};

/** Conversation-state changes reported by the backend to the Psi UI. */
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

    /** Returns a user-facing label for an account, falling back to its internal id. */
    virtual QString humanAccount(const QString &accountId) = 0;

    /** Returns the public XMPP address/JID associated with an account id. */
    virtual QString humanAccountPublic(const QString &accountId) = 0;

    /** Returns a user-facing contact name for the given account/contact pair. */
    virtual QString humanContact(const QString &accountId, const QString &contact) = 0;
};

/** Value object describing one stored peer fingerprint. */
struct Fingerprint {
    /** Raw 20-byte OTR fingerprint. */
    QByteArray value;
    /** Psi account that owns the local side of this trust record. */
    QString account;
    /** Remote OTR identity/JID to which the fingerprint belongs. */
    QString username;
    /** Human-readable hexadecimal representation used by the UI. */
    QString fingerprintHuman;
    /** Persisted libotr-compatible trust string; empty means unverified. */
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
    /**
     * Creates the facade with the default @p policy.
     * @p callback is non-owning and must outlive this OtrMessaging instance.
     */
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

    /** Regenerates the local OTR identity synchronously before returning. */
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

    /** Forwards an OTR system message to the application callback/UI. */
    bool displayOtrMessage(const QString &account, const QString &contact, const QString &message);

    /** Forwards a native backend state change to the application callback/UI. */
    void stateChange(const QString &account, const QString &contact, OtrStateChange change);

    /** Returns the application's user-facing label for @p accountId. */
    QString humanAccount(const QString &accountId);

    /** Returns the application's user-facing name for @p contact. */
    QString humanContact(const QString &accountId, const QString &contact);

private:
    OtrPolicy m_otrPolicy;
    OtrInternal *m_impl;
    OtrCallback *m_callback;
};

} // namespace psiotr

#endif
