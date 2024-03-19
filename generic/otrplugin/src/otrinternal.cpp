/*
 * otrinternal.cpp - Manages the OTR connection
 *
 * Off-the-Record Messaging plugin for Psi
 * Copyright (C) 2007-2011  Timo Engel (timo-e@freenet.de)
 *               2011-2012  Florian Fieber
 *                    2013  Georg Rudoy
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "otrinternal.h"

#include <QAbstractButton>
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFutureWatcher>
#include <QHash>
#include <QList>
#include <QMessageBox>
#include <QRegularExpression>
#include <QString>
#include <Qt>
#include <QtConcurrentRun>
#include <assert.h>

//-----------------------------------------------------------------------------

static const char   *OTR_PROTOCOL_STRING   = "prpl-jabber";
static const QString OTR_FINGERPRINTS_FILE = "otr.fingerprints";
static const QString OTR_KEYS_FILE         = "otr.keys";
static const QString OTR_INSTAGS_FILE      = "otr.instags";

// ============================================================================

OtrInternal::OtrInternal(psiotr::OtrCallback *callback, psiotr::OtrPolicy &policy) :
    m_userstate(), m_uiOps(), m_callback(callback), m_otrPolicy(policy), is_generating(false)
{
    QDir profileDir(callback->dataDir());

    m_keysFile        = profileDir.filePath(OTR_KEYS_FILE);
    m_instagsFile     = profileDir.filePath(OTR_INSTAGS_FILE);
    m_fingerprintFile = profileDir.filePath(OTR_FINGERPRINTS_FILE);

    OTRL_INIT;
    m_userstate                 = otrl_userstate_create();
    m_uiOps.policy              = (*OtrInternal::cb_policy);
    m_uiOps.create_privkey      = (*OtrInternal::cb_create_privkey);
    m_uiOps.is_logged_in        = (*OtrInternal::cb_is_logged_in);
    m_uiOps.inject_message      = (*OtrInternal::cb_inject_message);
    m_uiOps.update_context_list = (*OtrInternal::cb_update_context_list);
    m_uiOps.new_fingerprint     = (*OtrInternal::cb_new_fingerprint);
    m_uiOps.write_fingerprints  = (*OtrInternal::cb_write_fingerprints);
    m_uiOps.gone_secure         = (*OtrInternal::cb_gone_secure);
    m_uiOps.gone_insecure       = (*OtrInternal::cb_gone_insecure);
    m_uiOps.still_secure        = (*OtrInternal::cb_still_secure);

    m_uiOps.max_message_size  = nullptr;
    m_uiOps.account_name      = (*OtrInternal::cb_account_name);
    m_uiOps.account_name_free = (*OtrInternal::cb_account_name_free);

    m_uiOps.handle_msg_event = (*OtrInternal::cb_handle_msg_event);
    m_uiOps.handle_smp_event = (*OtrInternal::cb_handle_smp_event);
    m_uiOps.create_instag    = (*OtrInternal::cb_create_instag);

    otrl_privkey_read(m_userstate, QFile::encodeName(m_keysFile).constData());
    otrl_privkey_read_fingerprints(m_userstate, QFile::encodeName(m_fingerprintFile).constData(), nullptr, nullptr);
    otrl_instag_read(m_userstate, QFile::encodeName(m_instagsFile).constData());
}

//-----------------------------------------------------------------------------

OtrInternal::~OtrInternal() { otrl_userstate_free(m_userstate); }

//-----------------------------------------------------------------------------

QString OtrInternal::encryptMessage(const QString &account, const QString &contact, const QString &message)
{
    char        *encMessage = nullptr;
    gcry_error_t err;

    err = otrl_message_sending(m_userstate, &m_uiOps, this, account.toUtf8().constData(), OTR_PROTOCOL_STRING,
                               contact.toUtf8().constData(), OTRL_INSTAG_BEST, message.toUtf8().constData(), nullptr,
                               &encMessage, OTRL_FRAGMENT_SEND_SKIP, nullptr, nullptr, nullptr);
    if (err) {
        QString err_message = QObject::tr("Encrypting message to %1 "
                                          "failed.\nThe message was not sent.")
                                  .arg(contact);
        if (!m_callback->displayOtrMessage(account, contact, err_message)) {
            m_callback->notifyUser(account, contact, err_message, psiotr::OTR_NOTIFY_ERROR);
        }
        return QString();
    }

    if (encMessage) {
        QString retMessage(QString::fromUtf8(encMessage));
        otrl_message_free(encMessage);

        return retMessage;
    }
    return message;
}

//-----------------------------------------------------------------------------

psiotr::OtrMessageType OtrInternal::decryptMessage(const QString &account, const QString &contact,
                                                   const QString &cryptedMessage, QString &decrypted)
{
    QByteArray  accArray    = account.toUtf8();
    QByteArray  userArray   = contact.toUtf8();
    const char *accountName = accArray.constData();
    const char *userName    = userArray.constData();

    int      ignoreMessage = 0;
    char    *newMessage    = nullptr;
    OtrlTLV *tlvs          = nullptr;
    OtrlTLV *tlv           = nullptr;

    ignoreMessage
        = otrl_message_receiving(m_userstate, &m_uiOps, this, accountName, OTR_PROTOCOL_STRING, userName,
                                 cryptedMessage.toUtf8().constData(), &newMessage, &tlvs, nullptr, nullptr, nullptr);
    tlv = otrl_tlv_find(tlvs, OTRL_TLV_DISCONNECTED);
    if (tlv) {
        m_callback->stateChange(accountName, userName, psiotr::OTR_STATECHANGE_REMOTECLOSE);
    }

    // Magic hack to force it work similar to libotr < 4.0.0.
    // If user received unencrypted message he (she) should be notified.
    // See OTRL_MSGEVENT_RCVDMSG_UNENCRYPTED as well.
    if (ignoreMessage && !newMessage && !cryptedMessage.startsWith("?OTR")) {
        ignoreMessage = 0;
    }

    otrl_tlv_free(tlvs);

    if (ignoreMessage == 1) {
        // Internal protocol message

        return psiotr::OTR_MESSAGETYPE_IGNORE;
    } else if ((ignoreMessage == 0) && newMessage) {
        // Message has been decrypted, replace it
        decrypted = QString::fromUtf8(newMessage);
        otrl_message_free(newMessage);
        return psiotr::OTR_MESSAGETYPE_OTR;
    }

    return psiotr::OTR_MESSAGETYPE_NONE;
}

//-----------------------------------------------------------------------------

QList<psiotr::Fingerprint> OtrInternal::getFingerprints()
{
    QList<psiotr::Fingerprint> fpList;
    ConnContext               *context;
    ::Fingerprint             *fingerprint;

    for (context = m_userstate->context_root; context != nullptr; context = context->next) {
        fingerprint = context->fingerprint_root.next;
        while (fingerprint) {
            psiotr::Fingerprint fp(fingerprint->fingerprint, QString::fromUtf8(context->accountname),
                                   QString::fromUtf8(context->username), QString::fromUtf8(fingerprint->trust));

            fpList.append(fp);
            fingerprint = fingerprint->next;
        }
    }
    return fpList;
}

//-----------------------------------------------------------------------------

void OtrInternal::verifyFingerprint(const psiotr::Fingerprint &fingerprint, bool verified)
{
    ConnContext *context = otrl_context_find(m_userstate, fingerprint.username.toUtf8().constData(),
                                             fingerprint.account.toUtf8().constData(), OTR_PROTOCOL_STRING,
                                             OTRL_INSTAG_BEST, false, nullptr, nullptr, nullptr);
    if (context) {
        ::Fingerprint *fp = otrl_context_find_fingerprint(context, fingerprint.fingerprint, 0, nullptr);
        if (fp) {
            otrl_context_set_trust(fp, verified ? QObject::tr("verified").toUtf8().constData() : "");
            write_fingerprints();

            if (context->active_fingerprint == fp) {
                m_callback->stateChange(QString::fromUtf8(context->accountname), QString::fromUtf8(context->username),
                                        psiotr::OTR_STATECHANGE_TRUST);
            }
        }
    }
}

//-----------------------------------------------------------------------------

void OtrInternal::deleteFingerprint(const psiotr::Fingerprint &fingerprint)
{
    ConnContext *context = otrl_context_find(m_userstate, fingerprint.username.toUtf8().constData(),
                                             fingerprint.account.toUtf8().constData(), OTR_PROTOCOL_STRING,
                                             OTRL_INSTAG_BEST, false, nullptr, nullptr, nullptr);
    if (context) {
        ::Fingerprint *fp = otrl_context_find_fingerprint(context, fingerprint.fingerprint, 0, nullptr);
        if (fp) {
            if (context->active_fingerprint == fp) {
                otrl_context_force_finished(context);
            }
            otrl_context_forget_fingerprint(fp, true);
            write_fingerprints();
        }
    }
}

//-----------------------------------------------------------------------------

QHash<QString, QString> OtrInternal::getPrivateKeys()
{
    QHash<QString, QString> privKeyList;
    OtrlPrivKey            *privKey;

    for (privKey = m_userstate->privkey_root; privKey != nullptr; privKey = privKey->next) {
        char  fingerprintBuf[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];
        char *success
            = otrl_privkey_fingerprint(m_userstate, fingerprintBuf, privKey->accountname, OTR_PROTOCOL_STRING);
        if (success) {
            privKeyList.insert(QString::fromUtf8(privKey->accountname), QString(fingerprintBuf));
        }
    }

    return privKeyList;
}

//-----------------------------------------------------------------------------

void OtrInternal::deleteKey(const QString &account)
{
    OtrlPrivKey *privKey = otrl_privkey_find(m_userstate, account.toUtf8().constData(), OTR_PROTOCOL_STRING);

    otrl_privkey_forget(privKey);

    otrl_privkey_write(m_userstate, QFile::encodeName(m_keysFile).constData());
}

//-----------------------------------------------------------------------------

void OtrInternal::startSession(const QString &account, const QString &contact)
{
    m_callback->stateChange(account, contact, psiotr::OTR_STATECHANGE_GOINGSECURE);

    if (!otrl_privkey_find(m_userstate, account.toUtf8().constData(), OTR_PROTOCOL_STRING)) {
        create_privkey(account.toUtf8().constData(), OTR_PROTOCOL_STRING);
    }

    // TODO: make allowed otr versions configurable
    char *msg = otrl_proto_default_query_msg(m_callback->humanAccountPublic(account).toUtf8().constData(),
                                             OTRL_POLICY_DEFAULT);

    m_callback->sendMessage(account, contact, QString::fromUtf8(msg));

    free(msg);
}

//-----------------------------------------------------------------------------

void OtrInternal::endSession(const QString &account, const QString &contact)
{
    ConnContext *context = otrl_context_find(m_userstate, contact.toUtf8().constData(), account.toUtf8().constData(),
                                             OTR_PROTOCOL_STRING, OTRL_INSTAG_BEST, false, nullptr, nullptr, nullptr);
    if (context && (context->msgstate != OTRL_MSGSTATE_PLAINTEXT)) {
        m_callback->stateChange(account, contact, psiotr::OTR_STATECHANGE_CLOSE);
    }
    otrl_message_disconnect(m_userstate, &m_uiOps, this, account.toUtf8().constData(), OTR_PROTOCOL_STRING,
                            contact.toUtf8().constData(), OTRL_INSTAG_BEST);
}

//-----------------------------------------------------------------------------

void OtrInternal::expireSession(const QString &account, const QString &contact)
{
    ConnContext *context = otrl_context_find(m_userstate, contact.toUtf8().constData(), account.toUtf8().constData(),
                                             OTR_PROTOCOL_STRING, OTRL_INSTAG_BEST, false, nullptr, nullptr, nullptr);
    if (context && (context->msgstate == OTRL_MSGSTATE_ENCRYPTED)) {
        otrl_context_force_finished(context);
        m_callback->stateChange(account, contact, psiotr::OTR_STATECHANGE_GONEINSECURE);
    }
}

//-----------------------------------------------------------------------------

void OtrInternal::startSMP(const QString &account, const QString &contact, const QString &question,
                           const QString &secret)
{
    ConnContext *context = otrl_context_find(m_userstate, contact.toUtf8().constData(), account.toUtf8().constData(),
                                             OTR_PROTOCOL_STRING, OTRL_INSTAG_BEST, false, nullptr, nullptr, nullptr);
    if (context) {
        QByteArray  secretArray   = secret.toUtf8();
        const char *secretPointer = secretArray.constData();
        size_t      secretLength  = qstrlen(secretPointer);

        if (question.isEmpty()) {
            otrl_message_initiate_smp(m_userstate, &m_uiOps, this, context,
                                      reinterpret_cast<const unsigned char *>(const_cast<char *>(secretPointer)),
                                      secretLength);
        } else {
            otrl_message_initiate_smp_q(m_userstate, &m_uiOps, this, context, question.toUtf8().constData(),
                                        reinterpret_cast<const unsigned char *>(const_cast<char *>(secretPointer)),
                                        secretLength);
        }
    }
}

void OtrInternal::continueSMP(const QString &account, const QString &contact, const QString &secret)
{
    ConnContext *context = otrl_context_find(m_userstate, contact.toUtf8().constData(), account.toUtf8().constData(),
                                             OTR_PROTOCOL_STRING, OTRL_INSTAG_BEST, false, nullptr, nullptr, nullptr);
    if (context) {
        QByteArray  secretArray   = secret.toUtf8();
        const char *secretPointer = secretArray.constData();
        size_t      secretLength  = qstrlen(secretPointer);

        otrl_message_respond_smp(m_userstate, &m_uiOps, this, context,
                                 reinterpret_cast<const unsigned char *>(secretPointer), secretLength);
    }
}

void OtrInternal::abortSMP(const QString &account, const QString &contact)
{
    ConnContext *context = otrl_context_find(m_userstate, contact.toUtf8().constData(), account.toUtf8().constData(),
                                             OTR_PROTOCOL_STRING, OTRL_INSTAG_BEST, false, nullptr, nullptr, nullptr);
    if (context) {
        abortSMP(context);
    }
}

void OtrInternal::abortSMP(ConnContext *context) { otrl_message_abort_smp(m_userstate, &m_uiOps, this, context); }

//-----------------------------------------------------------------------------

psiotr::OtrMessageState OtrInternal::getMessageState(const QString &account, const QString &contact)
{
    ConnContext *context = otrl_context_find(m_userstate, contact.toUtf8().constData(), account.toUtf8().constData(),
                                             OTR_PROTOCOL_STRING, OTRL_INSTAG_BEST, false, nullptr, nullptr, nullptr);
    if (context) {
        if (context->msgstate == OTRL_MSGSTATE_PLAINTEXT) {
            return psiotr::OTR_MESSAGESTATE_PLAINTEXT;
        } else if (context->msgstate == OTRL_MSGSTATE_ENCRYPTED) {
            return psiotr::OTR_MESSAGESTATE_ENCRYPTED;
        } else if (context->msgstate == OTRL_MSGSTATE_FINISHED) {
            return psiotr::OTR_MESSAGESTATE_FINISHED;
        }
    }

    return psiotr::OTR_MESSAGESTATE_UNKNOWN;
}

//-----------------------------------------------------------------------------

QString OtrInternal::getMessageStateString(const QString &account, const QString &contact)
{
    psiotr::OtrMessageState state = getMessageState(account, contact);

    if (state == psiotr::OTR_MESSAGESTATE_PLAINTEXT) {
        return QObject::tr("plaintext");
    } else if (state == psiotr::OTR_MESSAGESTATE_ENCRYPTED) {
        return QObject::tr("encrypted");
    } else if (state == psiotr::OTR_MESSAGESTATE_FINISHED) {
        return QObject::tr("finished");
    }

    return QObject::tr("unknown");
}

//-----------------------------------------------------------------------------

QString OtrInternal::getSessionId(const QString &account, const QString &contact)
{
    ConnContext *context;
    context = otrl_context_find(m_userstate, contact.toUtf8().constData(), account.toUtf8().constData(),
                                OTR_PROTOCOL_STRING, OTRL_INSTAG_BEST, false, nullptr, nullptr, nullptr);
    if (context && (context->sessionid_len > 0)) {
        QString firstHalf;
        QString secondHalf;

        for (unsigned int i = 0; i < context->sessionid_len / 2; i++) {
            if (context->sessionid[i] <= 0xf) {
                firstHalf.append("0");
            }
            firstHalf.append(QString::number(context->sessionid[i], 16));
        }
        for (size_t i = context->sessionid_len / 2; i < context->sessionid_len; i++) {
            if (context->sessionid[i] <= 0xf) {
                secondHalf.append("0");
            }
            secondHalf.append(QString::number(context->sessionid[i], 16));
        }

        if (context->sessionid_half == OTRL_SESSIONID_FIRST_HALF_BOLD) {
            return QString("<b>" + firstHalf + "</b> " + secondHalf);
        } else {
            return QString(firstHalf + " <b>" + secondHalf + "</b>");
        }
    }

    return QString();
}

//-----------------------------------------------------------------------------

psiotr::Fingerprint OtrInternal::getActiveFingerprint(const QString &account, const QString &contact)
{
    ConnContext *context;
    context = otrl_context_find(m_userstate, contact.toUtf8().constData(), account.toUtf8().constData(),
                                OTR_PROTOCOL_STRING, OTRL_INSTAG_BEST, false, nullptr, nullptr, nullptr);

    if (context && context->active_fingerprint) {
        return psiotr::Fingerprint(context->active_fingerprint->fingerprint, QString::fromUtf8(context->accountname),
                                   QString::fromUtf8(context->username),
                                   QString::fromUtf8(context->active_fingerprint->trust));
    }

    return psiotr::Fingerprint();
}

//-----------------------------------------------------------------------------

bool OtrInternal::isVerified(const QString &account, const QString &contact)
{
    ConnContext *context;
    context = otrl_context_find(m_userstate, contact.toUtf8().constData(), account.toUtf8().constData(),
                                OTR_PROTOCOL_STRING, OTRL_INSTAG_BEST, false, nullptr, nullptr, nullptr);

    return isVerified(context);
}

//-----------------------------------------------------------------------------

bool OtrInternal::isVerified(ConnContext *context)
{

    if (context && context->active_fingerprint) {
        return (context->active_fingerprint->trust && context->active_fingerprint->trust[0]);
    }

    return false;
}

//-----------------------------------------------------------------------------

bool OtrInternal::smpSucceeded(const QString &account, const QString &contact)
{
    ConnContext *context;
    context = otrl_context_find(m_userstate, contact.toUtf8().constData(), account.toUtf8().constData(),
                                OTR_PROTOCOL_STRING, OTRL_INSTAG_BEST, false, nullptr, nullptr, nullptr);

    if (context) {
        return context->smstate->sm_prog_state == OTRL_SMP_PROG_SUCCEEDED;
    }

    return false;
}

//-----------------------------------------------------------------------------

void OtrInternal::generateKey(const QString &account)
{
    create_privkey(account.toUtf8().constData(), OTR_PROTOCOL_STRING);
}

//-----------------------------------------------------------------------------

QString OtrInternal::humanFingerprint(const unsigned char *fingerprint)
{
    char fpHash[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];
    otrl_privkey_hash_to_human(fpHash, fingerprint);
    return QString(fpHash);
}

//-----------------------------------------------------------------------------
/***  implemented callback functions for libotr ***/

OtrlPolicy OtrInternal::policy(ConnContext *)
{
    if (m_otrPolicy == psiotr::OTR_POLICY_OFF) {
        return OTRL_POLICY_NEVER; // otr disabled
    } else if (m_otrPolicy == psiotr::OTR_POLICY_ENABLED) {
        return OTRL_POLICY_MANUAL; // otr enabled, session started manual
    } else if (m_otrPolicy == psiotr::OTR_POLICY_AUTO) {
        return OTRL_POLICY_OPPORTUNISTIC; // automatically initiate private messaging
    } else if (m_otrPolicy == psiotr::OTR_POLICY_REQUIRE) {
        return OTRL_POLICY_ALWAYS; // require private messaging
    }

    return OTRL_POLICY_NEVER;
}

// ---------------------------------------------------------------------------

void OtrInternal::create_privkey(const char *accountname, const char *protocol)
{
    if (is_generating) {
        return;
    }

    QMessageBox qMB(QMessageBox::Question, QObject::tr("Confirm action"),
                    QObject::tr("Private keys for account \"%1\" need to be generated. "
                                "This takes quite some time (from a few seconds to a "
                                "couple of minutes), and while you can use Psi in the "
                                "meantime, all the messages will be sent unencrypted "
                                "until keys are generated. You will be notified when "
                                "this process finishes.\n"
                                "\n"
                                "Do you want to generate keys now?")
                        .arg(m_callback->humanAccount(QString::fromUtf8(accountname))),
                    QMessageBox::Yes | QMessageBox::No);

    if (qMB.exec() != QMessageBox::Yes) {
        return;
    }

    void *newkeyp;
    if (otrl_privkey_generate_start(m_userstate, accountname, protocol, &newkeyp) == gcry_error(GPG_ERR_EEXIST)) {
        qWarning("libotr reports it's still generating a previous key while it shouldn't be");
        return;
    }

    is_generating = true;

    QEventLoop                   loop;
    QFutureWatcher<gcry_error_t> watcher;

    // TODO: update after stopping support of Ubuntu Xenial:
    QObject::connect(&watcher, SIGNAL(finished()), &loop, SLOT(quit()));

    QFuture<gcry_error_t> future = QtConcurrent::run(otrl_privkey_generate_calculate, newkeyp);
    watcher.setFuture(future);

    loop.exec();

    is_generating = false;

    if (future.result() == gcry_error(GPG_ERR_NO_ERROR)) {
        otrl_privkey_generate_finish(m_userstate, newkeyp, QFile::encodeName(m_keysFile));
    }

    char fingerprint[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];
    if (otrl_privkey_fingerprint(m_userstate, fingerprint, accountname, protocol)) {
        QMessageBox infoMb(QMessageBox::Information, QObject::tr("Confirm action"),
                           QObject::tr("Keys have been generated. "
                                       "Fingerprint for account \"%1\":\n"
                                       "%2\n"
                                       "\n"
                                       "Thanks for your patience.")
                               .arg(m_callback->humanAccount(QString::fromUtf8(accountname)), QString(fingerprint)));
        infoMb.exec();
    } else {
        QMessageBox failMb(QMessageBox::Critical, QObject::tr("Confirm action"),
                           QObject::tr("Failed to generate keys for account \"%1\"."
                                       "\nThe OTR Plugin will not work.")
                               .arg(m_callback->humanAccount(QString::fromUtf8(accountname))),
                           QMessageBox::Ok);
        failMb.exec();
    }
}

// ---------------------------------------------------------------------------

int OtrInternal::is_logged_in(const char *accountname, const char *protocol, const char *recipient)
{
    Q_UNUSED(protocol);

    if (m_callback->isLoggedIn(QString::fromUtf8(accountname), QString::fromUtf8(recipient))) {
        return 1; // contact online
    } else {
        return 0; // contact offline
    }
}

// ---------------------------------------------------------------------------

void OtrInternal::inject_message(const char *accountname, const char *protocol, const char *recipient,
                                 const char *message)
{
    Q_UNUSED(protocol);

    m_callback->sendMessage(QString::fromUtf8(accountname), QString::fromUtf8(recipient), QString::fromUtf8(message));
}

// ---------------------------------------------------------------------------

void OtrInternal::handle_msg_event(OtrlMessageEvent msg_event, ConnContext *context, const char *message,
                                   gcry_error_t err)
{
    Q_UNUSED(err);
    Q_UNUSED(message);

    QString account = QString::fromUtf8(context->accountname);
    QString contact = QString::fromUtf8(context->username);

    QString errorString;
    switch (msg_event) {
    case OTRL_MSGEVENT_RCVDMSG_UNENCRYPTED:
        errorString = QObject::tr("<b>The following message received "
                                  "from %1 was <i>not</i> encrypted:</b>")
                          .arg(m_callback->humanContact(account, contact));
        break;
    case OTRL_MSGEVENT_CONNECTION_ENDED:
        errorString = QObject::tr("Your message was not sent. Either end your "
                                  "private conversation, or restart it.");
        break;
    case OTRL_MSGEVENT_RCVDMSG_UNRECOGNIZED:
        errorString = QObject::tr("Unreadable encrypted message was received.");
        break;
    case OTRL_MSGEVENT_RCVDMSG_NOT_IN_PRIVATE:
        errorString = QObject::tr("Received an encrypted message but it cannot "
                                  "be read because no private connection is "
                                  "established yet.");
        break;
    case OTRL_MSGEVENT_RCVDMSG_UNREADABLE:
        errorString = QObject::tr("Received message is unreadable.");
        break;
    case OTRL_MSGEVENT_RCVDMSG_MALFORMED:
        errorString = QObject::tr("Received message contains malformed data.");
        break;
    default:;
    }

    if (!errorString.isEmpty()) {
        m_callback->displayOtrMessage(QString::fromUtf8(context->accountname), QString::fromUtf8(context->username),
                                      errorString);
    }
}

void OtrInternal::handle_smp_event(OtrlSMPEvent smp_event, ConnContext *context, unsigned short progress_percent,
                                   char *question)
{
    if (smp_event == OTRL_SMPEVENT_CHEATED || smp_event == OTRL_SMPEVENT_ERROR) {
        abortSMP(context);
        m_callback->updateSMP(QString::fromUtf8(context->accountname), QString::fromUtf8(context->username), -2);
    } else if (smp_event == OTRL_SMPEVENT_ASK_FOR_SECRET || smp_event == OTRL_SMPEVENT_ASK_FOR_ANSWER) {
        m_callback->receivedSMP(QString::fromUtf8(context->accountname), QString::fromUtf8(context->username),
                                QString::fromUtf8(question));
    } else {
        m_callback->updateSMP(QString::fromUtf8(context->accountname), QString::fromUtf8(context->username),
                              progress_percent);
    }
}

void OtrInternal::create_instag(const char *accountname, const char *protocol)
{
    otrl_instag_generate(m_userstate, QFile::encodeName(m_instagsFile).constData(), accountname, protocol);
}

// ---------------------------------------------------------------------------

void OtrInternal::update_context_list() { }

// ---------------------------------------------------------------------------

void OtrInternal::new_fingerprint(OtrlUserState us, const char *accountname, const char *protocol, const char *username,
                                  unsigned char fingerprint[20])
{
    Q_UNUSED(us);
    Q_UNUSED(protocol);

    QString account = QString::fromUtf8(accountname);
    QString contact = QString::fromUtf8(username);
    QString message = QObject::tr("You have received a new "
                                  "fingerprint from %1:\n%2")
                          .arg(m_callback->humanContact(account, contact), humanFingerprint(fingerprint));

    if (!m_callback->displayOtrMessage(account, contact, message)) {
        m_callback->notifyUser(account, contact, message, psiotr::OTR_NOTIFY_INFO);
    }
}

// ---------------------------------------------------------------------------

void OtrInternal::write_fingerprints()
{
    otrl_privkey_write_fingerprints(m_userstate, QFile::encodeName(m_fingerprintFile).constData());
}

// ---------------------------------------------------------------------------

void OtrInternal::gone_secure(ConnContext *context)
{
    m_callback->stateChange(QString::fromUtf8(context->accountname), QString::fromUtf8(context->username),
                            psiotr::OTR_STATECHANGE_GONESECURE);
}

// ---------------------------------------------------------------------------

void OtrInternal::gone_insecure(ConnContext *context)
{
    m_callback->stateChange(QString::fromUtf8(context->accountname), QString::fromUtf8(context->username),
                            psiotr::OTR_STATECHANGE_GONEINSECURE);
}

// ---------------------------------------------------------------------------

void OtrInternal::still_secure(ConnContext *context, int is_reply)
{
    Q_UNUSED(is_reply);
    m_callback->stateChange(QString::fromUtf8(context->accountname), QString::fromUtf8(context->username),
                            psiotr::OTR_STATECHANGE_STILLSECURE);
}

// ---------------------------------------------------------------------------

const char *OtrInternal::account_name(const char *account, const char *protocol)
{
    Q_UNUSED(protocol);
    return qstrdup(m_callback->humanAccountPublic(QString::fromUtf8(account)).toUtf8().constData());
}

// ---------------------------------------------------------------------------

void OtrInternal::account_name_free(const char *account_name) { delete[] account_name; }

// ---------------------------------------------------------------------------
/*** static wrapper functions ***/

OtrlPolicy OtrInternal::cb_policy(void *opdata, ConnContext *context)
{
    return static_cast<OtrInternal *>(opdata)->policy(context);
}

void OtrInternal::cb_create_privkey(void *opdata, const char *accountname, const char *protocol)
{
    static_cast<OtrInternal *>(opdata)->create_privkey(accountname, protocol);
}

int OtrInternal::cb_is_logged_in(void *opdata, const char *accountname, const char *protocol, const char *recipient)
{
    return static_cast<OtrInternal *>(opdata)->is_logged_in(accountname, protocol, recipient);
}

void OtrInternal::cb_inject_message(void *opdata, const char *accountname, const char *protocol, const char *recipient,
                                    const char *message)
{
    static_cast<OtrInternal *>(opdata)->inject_message(accountname, protocol, recipient, message);
}

void OtrInternal::cb_handle_msg_event(void *opdata, OtrlMessageEvent msg_event, ConnContext *context,
                                      const char *message, gcry_error_t err)
{
    static_cast<OtrInternal *>(opdata)->handle_msg_event(msg_event, context, message, err);
}

void OtrInternal::cb_handle_smp_event(void *opdata, OtrlSMPEvent smp_event, ConnContext *context,
                                      unsigned short progress_percent, char *question)
{
    static_cast<OtrInternal *>(opdata)->handle_smp_event(smp_event, context, progress_percent, question);
}

void OtrInternal::cb_create_instag(void *opdata, const char *accountname, const char *protocol)
{
    static_cast<OtrInternal *>(opdata)->create_instag(accountname, protocol);
}

void OtrInternal::cb_update_context_list(void *opdata) { static_cast<OtrInternal *>(opdata)->update_context_list(); }

void OtrInternal::cb_new_fingerprint(void *opdata, OtrlUserState us, const char *accountname, const char *protocol,
                                     const char *username, unsigned char fingerprint[20])
{
    static_cast<OtrInternal *>(opdata)->new_fingerprint(us, accountname, protocol, username, fingerprint);
}

void OtrInternal::cb_write_fingerprints(void *opdata) { static_cast<OtrInternal *>(opdata)->write_fingerprints(); }

void OtrInternal::cb_gone_secure(void *opdata, ConnContext *context)
{
    static_cast<OtrInternal *>(opdata)->gone_secure(context);
}

void OtrInternal::cb_gone_insecure(void *opdata, ConnContext *context)
{
    static_cast<OtrInternal *>(opdata)->gone_insecure(context);
}

void OtrInternal::cb_still_secure(void *opdata, ConnContext *context, int is_reply)
{
    static_cast<OtrInternal *>(opdata)->still_secure(context, is_reply);
}

const char *OtrInternal::cb_account_name(void *opdata, const char *account, const char *protocol)
{
    return static_cast<OtrInternal *>(opdata)->account_name(account, protocol);
}

void OtrInternal::cb_account_name_free(void *opdata, const char *account_name)
{
    static_cast<OtrInternal *>(opdata)->account_name_free(account_name);
}
// ---------------------------------------------------------------------------
