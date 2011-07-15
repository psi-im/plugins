/*
 * OtrInternal.cpp - manages the otr connection.
 *
 * Copyright (C) Timo Engel (timo-e@freenet.de), Berlin 2007.
 * This program was written as part of a diplom thesis advised by 
 * Prof. Dr. Ruediger Weis (PST Labor)
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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "OtrInternal.hpp"
#include "HtmlTidy.hpp"

#include <assert.h>
#include <QString>
#include <QtGui>

//-----------------------------------------------------------------------------

static const char*   OTR_PROTOCOL_STRING = "prpl-jabber";
static const QString OTR_FINGERPRINTS_FILE = "otr.fingerprints";
static const QString OTR_KEYS_FILE = "otr.keys";

//-----------------------------------------------------------------------------

QString otrlMessageTypeToString(const OtrlMessageType& type)
{
    if (type == OTRL_MSGTYPE_NOTOTR)
    {
        return "no OTR Message";
    }
    else if (type == OTRL_MSGTYPE_TAGGEDPLAINTEXT)
    {
        return "OTR TaggedPlaintexMessage";
    }
    else if (type == OTRL_MSGTYPE_QUERY)
    {
        return "OTR QueryMessage";
    }
    else if (type == OTRL_MSGTYPE_DH_COMMIT)
    {
        return "OTR DH-Commit Message";
    }
    else if (type == OTRL_MSGTYPE_DH_KEY)
    {
        return "OTR DH-Key Message";
    }
    else if (type == OTRL_MSGTYPE_REVEALSIG)
    {
        return "OTR Reveal Signature Message";
    }
    else if (type == OTRL_MSGTYPE_SIGNATURE)
    {
        return "OTR Signature Message";
    }
    else if (type == OTRL_MSGTYPE_V1_KEYEXCH)
    {
        return "OTR Version 1 Key Exchange Message";
    }
    else if (type == OTRL_MSGTYPE_DATA)
    {
        return "OTR Data Message";
    }
    else if (type == OTRL_MSGTYPE_ERROR)
    {
        return "OTR Error Message";
    }
    else if (type == OTRL_MSGTYPE_UNKNOWN)
    {
        return "OTR Unknown Message";
    }
    else
    {
        return "Unknown OTR Message Type";
    }
}

//-----------------------------------------------------------------------------

class KeyGeneratorThread : public QThread
{
public:
    KeyGeneratorThread(const OtrlUserState& userstate, const QString& keysFile,
                       const char* accountname, const char* protocol)
    : m_userstate(userstate),
      m_keysFile(keysFile),
      m_accountname(accountname),
      m_protocol(protocol)
    {
    }
    
    void run()
    {
        otrl_privkey_generate(m_userstate, m_keysFile.toStdString().c_str(),
                              m_accountname, m_protocol);
    }

private:
    const OtrlUserState& m_userstate;
    const QString& m_keysFile;
    const char* m_accountname;
    const char* m_protocol;
};

// ============================================================================

OtrInternal::OtrInternal(psiotr::OtrCallback* callback,
                         psiotr::OtrPolicy& policy)
    : m_userstate(),
      m_uiOps(),
      m_callback(callback),
      m_keysFile(callback->dataDir() + "/" + OTR_KEYS_FILE),
      m_fingerprintFile(callback->dataDir() + "/" + OTR_FINGERPRINTS_FILE),
      m_otrPolicy(policy)
{
    OTRL_INIT;
    m_userstate                 = otrl_userstate_create();
    m_uiOps.policy              = (*OtrInternal::cb_policy);
    m_uiOps.create_privkey      = (*OtrInternal::cb_create_privkey);
    m_uiOps.is_logged_in        = (*OtrInternal::cb_is_logged_in);
    m_uiOps.inject_message      = (*OtrInternal::cb_inject_message);
    m_uiOps.notify              = (*OtrInternal::cb_notify);
    m_uiOps.display_otr_message = (*OtrInternal::cb_display_otr_message);
    m_uiOps.update_context_list = (*OtrInternal::cb_update_context_list);
    m_uiOps.protocol_name       = (*OtrInternal::cb_protocol_name);
    m_uiOps.protocol_name_free  = (*OtrInternal::cb_protocol_name_free);
    m_uiOps.new_fingerprint     = (*OtrInternal::cb_new_fingerprint);
    m_uiOps.write_fingerprints  = (*OtrInternal::cb_write_fingerprints);
    m_uiOps.gone_secure         = (*OtrInternal::cb_gone_secure);
    m_uiOps.gone_insecure       = (*OtrInternal::cb_gone_insecure);
    m_uiOps.still_secure        = (*OtrInternal::cb_still_secure);
    m_uiOps.log_message         = (*OtrInternal::cb_log_message);

#if not (OTRL_VERSION_MAJOR==3 && OTRL_VERSION_MINOR==0)
    m_uiOps.max_message_size = NULL;
    m_uiOps.account_name = NULL;
    m_uiOps.account_name_free = NULL;
#endif

    otrl_privkey_read(m_userstate, m_keysFile.toStdString().c_str());
    otrl_privkey_read_fingerprints(m_userstate,
                                   m_fingerprintFile.toStdString().c_str(),
                                   NULL, NULL);
}

//-----------------------------------------------------------------------------

OtrInternal::~OtrInternal()
{
    otrl_userstate_free(m_userstate);
}

//-----------------------------------------------------------------------------

QString OtrInternal::encryptMessage(const QString& from, const QString& to,
                                      const QString& message)
{
    char* encMessage = NULL;
    gcry_error_t err;

    err = otrl_message_sending(m_userstate, &m_uiOps, this,
                               from.toStdString().c_str(), OTR_PROTOCOL_STRING,
                               to.toStdString().c_str(),
                               message.toUtf8().data(),
                               NULL, &encMessage, NULL, NULL);
    if (err != 0)
    {
        m_callback->notifyUser(psiotr::OTR_NOTIFY_ERROR, 
                               "Encrypting message from " +
                               QString(from) + " to " + QString(to) +
                               " failed.\nThe message was not sent.");
        return QString();
    }

    if (encMessage != NULL)
    {
        QString retMessage(QString::fromUtf8(encMessage));
        otrl_message_free(encMessage);

        return retMessage;
    }
    return message;
}


//-----------------------------------------------------------------------------

bool OtrInternal::decryptMessage(const QString& from, const QString& to,
                                 const QString& cryptedMessage,
                                 QString& decrypted)
{
    int ignoreMessage = 0;
    char *newMessage = NULL;

    ignoreMessage = otrl_message_receiving(m_userstate, &m_uiOps, this,
                                           to.toStdString().c_str(),
                                           OTR_PROTOCOL_STRING,
                                           from.toStdString().c_str(),
                                           cryptedMessage.toUtf8().data(),
                                           &newMessage,
                                           NULL, NULL, NULL);

    if (ignoreMessage == 1) // internal protocol message
    {
        OtrlMessageType type = otrl_proto_message_type(
                cryptedMessage.toStdString().c_str());


        QString retMessage("received " + otrlMessageTypeToString(type) +
                           " [" + getMessageStateString(to, from) + "]") ;

        if (getMessageState(to, from) == psiotr::OTR_MESSAGESTATE_ENCRYPTED)
        {
            retMessage.append("\nsessionId: " + getSessionId(to, from));
        }

        decrypted = "<body xmlns=\"http://www.w3.org/1999/xhtml\">" +
                    retMessage + "</body>";
        return true;
    }
    else if (ignoreMessage == 0)
    {
        if (newMessage != NULL) // message has been decrypted. replace it
        {
            QString retMessage = QString::fromUtf8(newMessage);
            otrl_message_free(newMessage);
            HtmlTidy htmlTidy("<body xmlns=\"http://www.w3.org/1999/xhtml\">" +
                              retMessage + "</body>");
            decrypted = htmlTidy.output();
            return true;
        }
        else // received message was not an otr message
        {
            return false;
        }
    }

    assert(false);
    return false;
}

//-----------------------------------------------------------------------------

QList<psiotr::Fingerprint> OtrInternal::getFingerprints()
{
    QList<psiotr::Fingerprint> fpList;
    ConnContext* context;
    ::Fingerprint* fingerprint;

    for (context = m_userstate->context_root; context != NULL;
         context = context->next)
    {
        fingerprint = context->fingerprint_root.next;
        while(fingerprint)
        {
            psiotr::Fingerprint fp (fingerprint->fingerprint,
                                    QString(context->accountname),
                                    QString(context->username),
                                    QString(fingerprint->trust),
                                    (fingerprint == context->active_fingerprint)?
                                        QString(getMessageStateString(
                                            context->accountname,
                                            context->username)) :
                                        QString());

            fpList.append(fp);
            fingerprint = fingerprint->next;
        }
    }
    return fpList;
}

//-----------------------------------------------------------------------------

void OtrInternal::verifyFingerprint(const psiotr::Fingerprint& fingerprint,
                                    bool verified)
{
    ConnContext* context;
    ::Fingerprint* fp;

    for (context = m_userstate->context_root; context != NULL;
         context = context->next)
    {
        fp = otrl_context_find_fingerprint(context, fingerprint.fingerprint,
                                           0, NULL);
        if (verified)
        {
            otrl_context_set_trust(fp, "verified");
        }
        else
        {
            otrl_context_set_trust(fp, "");
        }
    }

    write_fingerprints();
}

//-----------------------------------------------------------------------------

void OtrInternal::deleteFingerprint(const psiotr::Fingerprint& fingerprint)
{
    ConnContext* context;
    ::Fingerprint* fp;

    for (context = m_userstate->context_root; context != NULL;
         context = context->next)
    {
        fp = otrl_context_find_fingerprint(context, fingerprint.fingerprint, 0,
                                           NULL);
        if (fp != NULL)
        {
            otrl_context_forget_fingerprint(fp, true);
            break;
        }
    }
    write_fingerprints();
}

//-----------------------------------------------------------------------------

QHash<QString, QString> OtrInternal::getPrivateKeys()
{
    QHash<QString, QString> privKeyList;
    ConnContext* context;

    for (context = m_userstate->context_root; context != NULL;
         context = context->next)
    {
        char fingerprintBuf[45];
        char* success = otrl_privkey_fingerprint(m_userstate,
                                                 fingerprintBuf,
                                                 context->accountname,
                                                 OTR_PROTOCOL_STRING);
        if (success)
        {
            privKeyList.insert(QString(context->accountname),
                                       QString(fingerprintBuf));
        }
    }

    return privKeyList;
}

//-----------------------------------------------------------------------------

void OtrInternal::startSession(const QString& account, const QString& jid)
{
    char fingerprint[45];
    if (!otrl_privkey_fingerprint(m_userstate, fingerprint,
                                  account.toStdString().c_str(), 
                                  OTR_PROTOCOL_STRING))
    {
        create_privkey(account.toStdString().c_str(), OTR_PROTOCOL_STRING);
    }

    //TODO: make allowed otr versions configureable
    char* msg = otrl_proto_default_query_msg(account.toStdString().c_str(),
                                             OTRL_POLICY_DEFAULT);

    m_callback->sendMessage(account, jid, msg);
}

//-----------------------------------------------------------------------------

void OtrInternal::endSession(const QString& account, const QString& jid)
{
    otrl_message_disconnect(m_userstate, &m_uiOps, this, 
                            account.toStdString().c_str(), OTR_PROTOCOL_STRING,
                            jid.toStdString().c_str());
}

//-----------------------------------------------------------------------------

psiotr::OtrMessageState OtrInternal::getMessageState(const QString& thisJid,
                                                     const QString& remoteJid)
{
    ConnContext* context = otrl_context_find(m_userstate,
                                             remoteJid.toStdString().c_str(),
                                             thisJid.toStdString().c_str(),
                                             OTR_PROTOCOL_STRING, false, NULL, NULL,
                                             NULL);
    if (context != NULL)
    {
        if (context->msgstate == OTRL_MSGSTATE_PLAINTEXT)
        {
            return psiotr::OTR_MESSAGESTATE_PLAINTEXT;
        }
        else if (context->msgstate == OTRL_MSGSTATE_ENCRYPTED)
        {
            return psiotr::OTR_MESSAGESTATE_ENCRYPTED;
        }
        else if (context->msgstate == OTRL_MSGSTATE_FINISHED)
        {
            return psiotr::OTR_MESSAGESTATE_FINISHED;
        }
    }

    return psiotr::OTR_MESSAGESTATE_UNKNOWN;
}

//-----------------------------------------------------------------------------

QString OtrInternal::getMessageStateString(const QString& thisJid,
                                           const QString& remoteJid)
{
    psiotr::OtrMessageState state = getMessageState(thisJid, remoteJid);

    if (state == psiotr::OTR_MESSAGESTATE_PLAINTEXT)
    {
        return "plaintext";
    }
    else if (state == psiotr::OTR_MESSAGESTATE_ENCRYPTED)
    {
        return "encrypted";
    }
    else if (state == psiotr::OTR_MESSAGESTATE_FINISHED)
    {
        return "finished";
    }
        
    return "unknown";
}

//-----------------------------------------------------------------------------

QString OtrInternal::getSessionId(const QString& thisJid,
                                  const QString& remoteJid)
{
    ConnContext* context;
    context = otrl_context_find(m_userstate, remoteJid.toStdString().c_str(),
                                thisJid.toStdString().c_str(), OTR_PROTOCOL_STRING,
                                false, NULL, NULL, NULL);
    if (context != NULL)
    {
        QString firstHalf;
        QString secondHalf;

        for (unsigned int i = 0; i < context->sessionid_len / 2; i++)
        {
            if (context->sessionid[i] <= 0xf)
            {
                firstHalf.append("0");
            }
            firstHalf.append(QString::number(context->sessionid[i], 16));
        }
        for (unsigned int i = context->sessionid_len / 2;
             i < context->sessionid_len; i++)
        {
            if (context->sessionid[i] <= 0xf)
            {
                secondHalf.append("0");
            }
            secondHalf.append(QString::number(context->sessionid[i], 16));
        }

        if (context->sessionid_half == OTRL_SESSIONID_FIRST_HALF_BOLD)
        {
            return QString("<b>" + firstHalf + "</b> " + secondHalf);
        }
        else
        {
            return QString(firstHalf + " <b>" + secondHalf + "</b>");
        }
    }

    return QString();
}

//-----------------------------------------------------------------------------

psiotr::Fingerprint OtrInternal::getActiveFingerprint(const QString& thisJid,
                                                      const QString& remoteJid)
{
    ConnContext* context;
    context = otrl_context_find(m_userstate, remoteJid.toStdString().c_str(),
                                thisJid.toStdString().c_str(), OTR_PROTOCOL_STRING,
                                false, NULL, NULL, NULL);

    if ((context != NULL) &&
        (context->active_fingerprint != NULL))
    {
        return psiotr::Fingerprint(context->active_fingerprint->fingerprint,
                                   QString(context->accountname),
                                   QString(context->username),
                                   QString(context->active_fingerprint->trust),
                                   QString(getMessageStateString(
                                               context->accountname,
                                               context->username)));
    }

    return psiotr::Fingerprint();
}

//-----------------------------------------------------------------------------

bool OtrInternal::isVerified(const QString& thisJid,
                             const QString& remoteJid)
{
    ConnContext* context;
    context = otrl_context_find(m_userstate, remoteJid.toStdString().c_str(),
                                thisJid.toStdString().c_str(), OTR_PROTOCOL_STRING,
                                false, NULL, NULL, NULL);

    if ((context != NULL) &&
        (context->active_fingerprint != NULL))
    {
        return (QString("verified") == context->active_fingerprint->trust);
    }

    return false;
}

//-----------------------------------------------------------------------------

void OtrInternal::generateKey(const QString& account)
{
    otrl_privkey_generate(m_userstate, m_keysFile.toStdString().c_str(),
                          account.toStdString().c_str(), OTR_PROTOCOL_STRING);
}

//-----------------------------------------------------------------------------

QString OtrInternal::humanFingerprint(const unsigned char *fingerprint)
{
    char fpHash[45];
    otrl_privkey_hash_to_human(fpHash, fingerprint);
    return QString(fpHash);
}

//-----------------------------------------------------------------------------
/***  implemented callback functions for libotr ***/

OtrlPolicy OtrInternal::policy(ConnContext*)
{
    if (m_otrPolicy == psiotr::OTR_POLICY_OFF)
    {
        return OTRL_POLICY_NEVER; // otr disabled
    }
    else if (m_otrPolicy == psiotr::OTR_POLICY_ENABLED)
    {
        return OTRL_POLICY_MANUAL; // otr enabled, session started manual
    }
    else if (m_otrPolicy == psiotr::OTR_POLICY_AUTO)
    {
        return OTRL_POLICY_OPPORTUNISTIC; // automatically initiate private messaging
    }
    else if (m_otrPolicy == psiotr::OTR_POLICY_REQUIRE)
    {
        return OTRL_POLICY_ALWAYS; // require private messaging 
    }
    
    return OTRL_POLICY_NEVER;
}

// ---------------------------------------------------------------------------

void OtrInternal::create_privkey(const char *accountname,
                                 const char *protocol)
{
    m_callback->stopMessages();
    
    KeyGeneratorThread keyGenerator(m_userstate, m_keysFile,
                                    accountname, protocol);
    keyGenerator.start();
    
    QMessageBox infoMb(QMessageBox::Information, "psi-otr",
                       "Generating keys for account " +
                       QString(accountname) + "."
                       "\nThis may take a while.",
                       QMessageBox::Ok, NULL,
                       Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    infoMb.button(QMessageBox::Ok)->setEnabled(false);
    infoMb.button(QMessageBox::Ok)->setText("please wait...");
    infoMb.setWindowModality(Qt::ApplicationModal);
    infoMb.show();

    while (!keyGenerator.isFinished())
    {
        QCoreApplication::processEvents();
    }

    infoMb.button(QMessageBox::Ok)->setEnabled(true);
    infoMb.button(QMessageBox::Ok)->setText("Ok");

    char fingerprint[45];
    if (otrl_privkey_fingerprint(m_userstate, fingerprint, accountname,
                                 protocol) == NULL)
    {
        QMessageBox failMb(QMessageBox::Critical, "psi-otr",
                           "Failed to generate key for account " +
                           QString(accountname) + "."
                           "\nThe OTR Plugin will not work.",
                           QMessageBox::Ok, NULL,
                           Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
        failMb.exec();
    }
    else
    {
        infoMb.setText("The fingerprint for account " + QString(accountname) +
                       " is\n" + QString(fingerprint) + ".");
    }
    infoMb.exec();

    m_callback->startMessages();
}

// ---------------------------------------------------------------------------

int OtrInternal::is_logged_in(const char *accountname, const char *protocol,
                              const char *recipient)
{
    Q_UNUSED(protocol);

    if (m_callback->isLoggedIn(accountname, recipient))
    {
        return 1; // contact online
    }
    else
    {
        return 0; // contact offline
    }
}

// ---------------------------------------------------------------------------
                    
void OtrInternal::inject_message(const char *accountname,
                                 const char *protocol, const char *recipient,
                                 const char *message)
{
    Q_UNUSED(protocol);

    m_callback->sendMessage(accountname, recipient, message);
}
                
// ---------------------------------------------------------------------------

void OtrInternal::notify(OtrlNotifyLevel level, const char *accountname,
                         const char *protocol, const char *username,
                         const char *title, const char *primary, const char *secondary)
{
    Q_UNUSED(accountname);
    Q_UNUSED(protocol);
    Q_UNUSED(username);
    Q_UNUSED(title);

    psiotr::OtrNotifyType type;

    if (level == OTRL_NOTIFY_ERROR )
    {
        type = psiotr::OTR_NOTIFY_ERROR;
    }
    else if (level == OTRL_NOTIFY_WARNING)
    {
        type = psiotr::OTR_NOTIFY_WARNING;
    }
    else
    {
        type = psiotr::OTR_NOTIFY_ERROR;
    }

    m_callback->notifyUser(type, QString(primary) + "\n" + QString(secondary));
}
        
// ---------------------------------------------------------------------------
    
int OtrInternal::display_otr_message(const char *accountname,
                                     const char *protocol,
                                     const char *username,
                                     const char *msg)
{
    Q_UNUSED(accountname);
    Q_UNUSED(protocol);
    Q_UNUSED(username);
    Q_UNUSED(msg);
    
    return -1; // use notify() or inline message
}
                
// ---------------------------------------------------------------------------

void OtrInternal::update_context_list()
{
}

// ---------------------------------------------------------------------------

const char* OtrInternal::protocol_name(const char* protocol)
{
    Q_UNUSED(protocol);
    return OTR_PROTOCOL_STRING;
}

// ---------------------------------------------------------------------------

void OtrInternal::protocol_name_free(const char *protocol_name)
{
    Q_UNUSED(protocol_name);
}

// ---------------------------------------------------------------------------

void OtrInternal::new_fingerprint(OtrlUserState us, const char *accountname,
                                  const char *protocol, const char *username,
                                  unsigned char fingerprint[20])
{
    Q_UNUSED(us);
    Q_UNUSED(protocol);

    char fpHuman[45];
    otrl_privkey_hash_to_human(fpHuman, fingerprint);

    m_callback->notifyUser(psiotr::OTR_NOTIFY_INFO,
                           "Account " + QString(accountname) +
                           " has received new fingerprint from "
                           + QString(username) + ":\n" + QString(fpHuman));
}
            
// ---------------------------------------------------------------------------

void OtrInternal::write_fingerprints()
{
    otrl_privkey_write_fingerprints(m_userstate,
                                    m_fingerprintFile.toStdString().c_str());
}

// ---------------------------------------------------------------------------

void OtrInternal::gone_secure(ConnContext *context)
{
    Q_UNUSED(context);
}

// ---------------------------------------------------------------------------

void OtrInternal::gone_insecure(ConnContext *context)
{
    Q_UNUSED(context);
}
    
// ---------------------------------------------------------------------------

void OtrInternal::still_secure(ConnContext *context, int is_reply)
{
    Q_UNUSED(context);
    Q_UNUSED(is_reply);
}

// ---------------------------------------------------------------------------
    
void OtrInternal::log_message(const char *message)
{
    Q_UNUSED(message);
}

// ---------------------------------------------------------------------------
/*** static wrapper functions ***/
    
OtrlPolicy OtrInternal::cb_policy(void *opdata, ConnContext *context) {
    return static_cast<OtrInternal*>(opdata)->policy(context);
}
    
void OtrInternal::cb_create_privkey(void *opdata, const char *accountname, const char *protocol) {
    static_cast<OtrInternal*>(opdata)->create_privkey(accountname, protocol);
}             
                        
int OtrInternal::cb_is_logged_in(void *opdata, const char *accountname, const char *protocol, const char *recipient) {
    return static_cast<OtrInternal*>(opdata)->is_logged_in(accountname, protocol, recipient);
}

void OtrInternal::cb_inject_message(void *opdata, const char *accountname, const char *protocol, const char *recipient, const char *message) {
    static_cast<OtrInternal*>(opdata)->inject_message(accountname, protocol, recipient, message);
}

void OtrInternal::cb_notify(void *opdata, OtrlNotifyLevel level, const char *accountname, const char *protocol, const char *username, const char *title, const char *primary, const char *secondary) {
    static_cast<OtrInternal*>(opdata)->notify(level, accountname, protocol, username, title, primary, secondary);
}
    
int OtrInternal::cb_display_otr_message(void *opdata, const char *accountname, const char *protocol, const char *username, const char *msg) {
    return static_cast<OtrInternal*>(opdata)->display_otr_message(accountname, protocol, username, msg);
}
                   
void OtrInternal::cb_update_context_list(void *opdata) {
    static_cast<OtrInternal*>(opdata)->update_context_list();
}
        
const char* OtrInternal::cb_protocol_name(void *opdata, const char *protocol) {
    return static_cast<OtrInternal*>(opdata)->protocol_name(protocol);
}
    
void OtrInternal::cb_protocol_name_free(void *opdata, const char *protocol_name) {
    static_cast<OtrInternal*>(opdata)->protocol_name(protocol_name);
}

void OtrInternal::cb_new_fingerprint(void *opdata, OtrlUserState us, const char *accountname, const char *protocol, const char *username, unsigned char fingerprint[20]) {
    static_cast<OtrInternal*>(opdata)->new_fingerprint(us, accountname, protocol, username, fingerprint);
}
            
void OtrInternal::cb_write_fingerprints(void *opdata) {
    static_cast<OtrInternal*>(opdata)->write_fingerprints();
}
    
void OtrInternal::cb_gone_secure(void *opdata, ConnContext *context) {
    static_cast<OtrInternal*>(opdata)->gone_secure(context);
}

void OtrInternal::cb_gone_insecure(void *opdata, ConnContext *context) {
    static_cast<OtrInternal*>(opdata)->gone_insecure(context);
}
    
void OtrInternal::cb_still_secure(void *opdata, ConnContext *context, int is_reply) {
    static_cast<OtrInternal*>(opdata)->still_secure(context, is_reply);
}
    
void OtrInternal::cb_log_message(void *opdata, const char *message) {
    static_cast<OtrInternal*>(opdata)->log_message(message);
}

// ---------------------------------------------------------------------------
