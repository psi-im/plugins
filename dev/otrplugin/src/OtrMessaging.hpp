/*
 * OtrMessaging.hpp - interface to libotr
 * Copyright (C) 2007  Timo Engel (timo-e@freenet.de)
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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef OTRMESSAGING_HPP_
#define OTRMESSAGING_HPP_

#include <QString>
#include <QList>
#include <QHash>

class OtrInternal;

// ---------------------------------------------------------------------------

namespace psiotr
{

class PsiOtrPlugin;

// ---------------------------------------------------------------------------

enum OtrPolicy
{
    OTR_POLICY_OFF,
    OTR_POLICY_ENABLED,
    OTR_POLICY_AUTO,
    OTR_POLICY_REQUIRE
};

// ---------------------------------------------------------------------------

enum OtrMessageState
{
    OTR_MESSAGESTATE_UNKNOWN,
    OTR_MESSAGESTATE_PLAINTEXT,
    OTR_MESSAGESTATE_ENCRYPTED,
    OTR_MESSAGESTATE_FINISHED
};

// ---------------------------------------------------------------------------

enum OtrNotifyType
{
    OTR_NOTIFY_INFO,
    OTR_NOTIFY_WARNING,
    OTR_NOTIFY_ERROR
};

// ---------------------------------------------------------------------------

/**
* Interface for callbacks from libotr to application
*
*/
class OtrCallback
{
public:
    virtual QString dataDir() = 0;

    /**
     * Sends a message from the Account account to the user toJid.
     * The method is called from the OtrConnection to send messages
     * during key-exchange.
     */
    virtual void sendMessage(const QString& account, const QString& recipient,
                             const QString& message) = 0;

    virtual bool isLoggedIn(const QString& account, const QString& recipient) = 0;

    virtual void notifyUser(const OtrNotifyType& type,
                            const QString& message) = 0;

    virtual void receivedSMP(const QString& account, const QString& contact,
                             const QString& question) = 0;

    virtual void updateSMP(const QString& account, const QString& contact,
                           int progress) = 0;

    virtual void stopMessages() = 0;
    virtual void startMessages() = 0;

    virtual QString humanAccount(const QString& accountId) = 0;
    virtual QString humanAccountPublic(const QString& accountId) = 0;
};

// ---------------------------------------------------------------------------

/** 
* This struct contains all data shown in the table of 'Known Fingerprints'.
*/
struct Fingerprint
{
    /** 
    * Pointer to fingerprint in libotr struct. Binary format.
    */
    unsigned char* fingerprint; 

    /** 
    * own account
    */
    QString account;

    /** 
    * owner of the fingerprint
    */
    QString username;

    /** 
    * The fingerprint in a human-readable format
    */
    QString fingerprintHuman;

    /** 
    * the level of trust 
    */
    QString trust;

    /** 
    * The messageState of the context (i.e. plaintext, encrypted, finished)
    */
    QString messageState;

    Fingerprint();
    Fingerprint(const Fingerprint &fp);
    Fingerprint(unsigned char* fingerprint,
                QString account, QString username,
                QString trust, QString messageState);
};

// ---------------------------------------------------------------------------

/** 
* This class it the interface to the off the record messaging library.
* See the libotr documentation for more information.
*
*/
class OtrMessaging
{
public:

    /** 
    * Constructor
    * 
    * @param plugin Pointer to the plugin, used for sending messages.
    * @param policy The default OTR policy
    */
    OtrMessaging(OtrCallback* callback, OtrPolicy policy);

    /** 
    * Deconstructor
    */
    ~OtrMessaging();

    /** 
    * Process an outgoing message.
    * 
    * @param from Account the message is send from
    * @param to Recipient of message
    * @param message The message itself.
    * 
    * @return The encrypted message.
    */
    QString encryptMessage(const QString& from, const QString& to,
                           const QString& message);

    /** 
    * Decrypt an incoming message
    * 
    * @param from Sender of the message
    * @param to Account the message is send to.
    * @param message the mesasge itself.
    * @param decrypted The decrypted message if the original message was
    *                  encrypted.
    * @return true, if decrypted was set.
    */
    bool decryptMessage(const QString& from, const QString& to,
                        const QString& message, QString& decrypted);

    /** 
    * Returns a list of known fingerprints.
    */
    QList<Fingerprint> getFingerprints();

    /** 
    * Set fingerprint verified/not verified.
    */
    void verifyFingerprint(const Fingerprint& fingerprint, bool verified);

    /** 
    * Delete a known fingerprint.
    */
    void deleteFingerprint(const Fingerprint& fingerprint);

    /** 
    * Get hash of fingerprints of own private keys.
    * Account -> KeyFingerprint
    */
    QHash<QString, QString> getPrivateKeys();

    /** 
    * Delete a private key.
    */
    void deleteKey(const QString& account);

    /** 
    * Send an OTR query message from account to contact.
    */
    void startSession(const QString& account, const QString& contact);

    /** 
    * Send otr-finished message to user.
    */
    void endSession(const QString& account, const QString& contact);

    /** 
    * Force a session to expire.
    */
    void expireSession(const QString& account, const QString& contact);

    /**
    * Start the SMP with an optional question
    */
    void startSMP(const QString& account, const QString& contact,
                  const QString& question, const QString& secret);

    /**
    * Continue the SMP
    */
    void continueSMP(const QString& account, const QString& contact,
                     const QString& secret);

    /**
    * Abort the SMP
    */
    void abortSMP(const QString& account, const QString& contact);

    /**
    * Return the messageState of a context.
    * i.e. plaintext, encrypted, finished
    */
    OtrMessageState getMessageState(const QString& account,
                                    const QString& contact);

    /** 
    * returns the messageState in human-readable string.
    */
    QString getMessageStateString(const QString& account,
                                  const QString& contact);

    /** 
    * Return the secure session id (ssid) for a context
    */
    QString getSessionId(const QString& account, const QString& contact);

    /** 
    * Return the active fingerprint for a context
    */
    psiotr::Fingerprint getActiveFingerprint(const QString& account,
                                             const QString& contact);

    /** 
    * Return true if the active fingerprint has been verified
    */
    bool isVerified(const QString& account, const QString& contact);

    /** 
    * Return true if Socialist Millionaires' Protocol succeeded
    */
    bool smpSucceeded(const QString& account, const QString& contact);

    /** 
    * Set the default OTR policy.
    */
    void setPolicy(OtrPolicy policy);

    /** 
    * @return The default OTR policy
    */
    OtrPolicy getPolicy();

    /**
    * Generate own keys.
    * This function blocks until keys are available.
    */
    void generateKey(const QString& account);

    /**
     * Return a human-readable representation
     * of an account identified by accountId
     */
    QString humanAccount(const QString accountId);

private:
    OtrPolicy    m_otrPolicy;
    OtrInternal* m_impl;
    OtrCallback* m_callback;
};

// ---------------------------------------------------------------------------

} // namespace psiotr

#endif
