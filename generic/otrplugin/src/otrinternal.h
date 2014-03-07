/*
 * otrinternal.h - Manages the OTR connection
 *
 * Off-the-Record Messaging plugin for Psi+
 * Copyright (C) 2007-2011  Timo Engel (timo-e@freenet.de)
 *                    2011  Florian Fieber
 *               2013-2014  Boris Pek (tehnick-8@mail.ru)
 *
 * This program was originally written as part of a diplom thesis
 * advised by Prof. Dr. Ruediger Weis (PST Labor)
 * at the Technical University of Applied Sciences Berlin.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef OTRINTERNAL_H_
#define OTRINTERNAL_H_

#include "otrmessaging.h"

#include <QList>
#include <QHash>

extern "C"
{
#include <libotr/proto.h>
#include <libotr/message.h>
#include <libotr/privkey.h>
#ifndef OTRL_PRIVKEY_FPRINT_HUMAN_LEN
#define OTRL_PRIVKEY_FPRINT_HUMAN_LEN 45
#endif
#if (OTRL_VERSION_MAJOR >= 4)
#include <libotr/instag.h>
#endif
#include "otrlextensions.h"
}

class QString;

// ---------------------------------------------------------------------------

/**
 * Handles all libotr calls and callbacks.
 */
class OtrInternal
{
public:

    OtrInternal(psiotr::OtrCallback* callback, psiotr::OtrPolicy& policy);

    ~OtrInternal();

    QString encryptMessage(const QString& account, const QString& contact,
                           const QString& message);

    psiotr::OtrMessageType decryptMessage(const QString& account,
                                          const QString& contact,
                                          const QString& message,
                                          QString& decrypted);

    QList<psiotr::Fingerprint> getFingerprints();

    void verifyFingerprint(const psiotr::Fingerprint& fingerprint, bool verified);

    void deleteFingerprint(const psiotr::Fingerprint& fingerprint);

    QHash<QString, QString> getPrivateKeys();

    void deleteKey(const QString& account);


    void startSession(const QString& account, const QString& contact);

    void endSession(const QString& account, const QString& contact);

    void expireSession(const QString& account, const QString& contact);


    void startSMP(const QString& account, const QString& contact,
                  const QString& question, const QString& secret);

    void continueSMP(const QString& account, const QString& contact,
                     const QString& secret);

    void abortSMP(const QString& account, const QString& contact);
    void abortSMP(ConnContext* context);


    psiotr::OtrMessageState getMessageState(const QString& account,
                                            const QString& contact);

    QString getMessageStateString(const QString& account,
                                  const QString& contact);

    QString getSessionId(const QString& account, const QString& contact);

    psiotr::Fingerprint getActiveFingerprint(const QString& account,
                                             const QString& contact);

    bool isVerified(const QString& account, const QString& contact);
    bool isVerified(ConnContext* context);

    bool smpSucceeded(const QString& account, const QString& contact);

    void generateKey(const QString& account);

    static QString humanFingerprint(const unsigned char* fingerprint);

    /*** otr callback functions ***/
    OtrlPolicy policy(ConnContext* context);
    void create_privkey(const char* accountname, const char* protocol);
    int is_logged_in(const char* accountname, const char* protocol,
                     const char* recipient);
    void inject_message(const char* accountname, const char* protocol,
                        const char* recipient, const char* message);
    void update_context_list();
    void new_fingerprint(OtrlUserState us, const char* accountname,
                         const char* protocol, const char* username,
                         unsigned char fingerprint[20]);
    void write_fingerprints();
    void gone_secure(ConnContext* context);
    void gone_insecure(ConnContext* context);
    void still_secure(ConnContext* context, int is_reply);
#if (OTRL_VERSION_MAJOR >= 4)
    void handle_msg_event(OtrlMessageEvent msg_event, ConnContext* context,
                          const char* message, gcry_error_t err);
    void handle_smp_event(OtrlSMPEvent smp_event, ConnContext* context,
                          unsigned short progress_percent, char* question);
    void create_instag(const char* accountname, const char* protocol);
#else
    void log_message(const char* message);
    void notify(OtrlNotifyLevel level, const char* accountname,
                const char* protocol, const char* username, const char* title,
                const char* primary, const char* secondary);
    int display_otr_message(const char* accountname, const char* protocol,
                            const char* username, const char* msg);
    const char* protocol_name(const char* protocol);
    void protocol_name_free(const char* protocol_name);
#endif

    const char* account_name(const char* account,
                             const char* protocol);
    void account_name_free(const char* account_name);


    /*** static otr callback wrapper-functions ***/
    static OtrlPolicy cb_policy(void* opdata, ConnContext* context);
    static void cb_create_privkey(void* opdata, const char* accountname,
                                  const char* protocol);
    static int cb_is_logged_in(void* opdata, const char* accountname,
                               const char* protocol, const char* recipient);
    static void cb_inject_message(void* opdata, const char* accountname,
                                  const char* protocol, const char* recipient,
                                  const char* message);
    static void cb_update_context_list(void* opdata);
    static void cb_new_fingerprint(void* opdata, OtrlUserState us,
                                   const char* accountname, const char* protocol,
                                   const char* username, unsigned char fingerprint[20]);
    static void cb_write_fingerprints(void* opdata);
    static void cb_gone_secure(void* opdata, ConnContext* context);
    static void cb_gone_insecure(void* opdata, ConnContext* context);
    static void cb_still_secure(void* opdata, ConnContext* context, int is_reply);
#if (OTRL_VERSION_MAJOR >= 4)
    static void cb_handle_msg_event(void* opdata, OtrlMessageEvent msg_event,
                                    ConnContext* context, const char* message,
                                    gcry_error_t err);
    static void cb_handle_smp_event(void* opdata, OtrlSMPEvent smp_event,
                                    ConnContext* context, unsigned short progress_percent,
                                    char* question);
    static void cb_create_instag(void* opdata, const char* accountname, const char* protocol);
#else
    static void cb_log_message(void* opdata, const char* message);
    static void cb_notify(void* opdata, OtrlNotifyLevel level,
                          const char* accountname, const char* protocol,
                          const char* username, const char* title,
                          const char* primary, const char* secondary);
    static int cb_display_otr_message(void* opdata, const char* accountname,
                                      const char* protocol, const char* username,
                                      const char* msg);
    static const char* cb_protocol_name(void* opdata, const char* protocol);
    static void cb_protocol_name_free(void* opdata, const char* protocol_name);
#endif

    static const char* cb_account_name(void* opdata, const char* account, const char* protocol);
    static void cb_account_name_free(void* opdata, const char* account_name);
private:

    /**
     * The userstate contains keys and known fingerprints.
     */
    OtrlUserState m_userstate;

    /**
     * Pointers to callback functions.
     */
    OtrlMessageAppOps m_uiOps;

    /**
     * Pointer to a class for callbacks from OTR to application.
     */
    psiotr::OtrCallback* m_callback;

    /**
     * Name of the file storing dsa-keys.
     */
    QString m_keysFile;

    /**
     * Name of the file storing instance tags.
     */
    QString m_instagsFile;

    /**
     * Name of the file storing known fingerprints.
     */
    QString m_fingerprintFile;

    /**
     * Reference to the default OTR policy
     */
    psiotr::OtrPolicy& m_otrPolicy;

    /**
     * Variable used during generating of private key.
     */
    bool is_generating;
};

// ---------------------------------------------------------------------------

#endif
