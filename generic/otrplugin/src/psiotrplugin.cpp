/*
 * psiotrplugin.cpp
 *
 * Off-the-Record Messaging plugin for Psi
 * Copyright (C) 2007-2011  Timo Engel (timo-e@freenet.de)
 *                    2011  Florian Fieber
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

#include "psiotrplugin.h"
#include "accountinfoaccessinghost.h"
#include "applicationinfoaccessinghost.h"
#include "contactinfoaccessinghost.h"
#include "htmltidy.h"
#include "iconfactoryaccessinghost.h"
#include "psiaccountcontrollinghost.h"
#include "psiotrclosure.h"
#include "psiotrconfig.h"

#include <QAction>
#include <QDomElement>
#include <QtGui>

namespace psiotr {

// ---------------------------------------------------------------------------

namespace {

    // ---------------------------------------------------------------------------

    /**
     * Removes the resource from a given JID.
     * Example:
     * removeResource("user@jabber.org/Home")
     * returns "user@jabber.org"
     */
    QString removeResource(const QString &aJid)
    {
        QString addr(aJid);
        int     pos = aJid.indexOf("/");
        if (pos > -1) {
            addr.truncate(pos);
        }
        return addr;
    }

    // ---------------------------------------------------------------------------

    /**
     * Reverts Qt::escape()
     */
    QString unescape(const QString &escaped)
    {
        QString plain(escaped);
        plain.replace("&lt;", "<").replace("&gt;", ">").replace("&quot;", "\"").replace("&amp;", "&");
        return plain;
    }

    // ---------------------------------------------------------------------------

    /**
     * Converts HTML to plaintext
     */
    QString htmlToPlain(const QString &html)
    {
        QString plain(html);
        plain.replace(QRegularExpression(" ?\\n"), " ")
            .replace(QRegularExpression("<br(?:\\s[^>]*)?/>"), "\n")
            .replace(QRegularExpression("<b(?:\\s[^>]*)?>([^<]+)</b>"), "*\\1*")
            .replace(QRegularExpression("<i(?:\\s[^>]*)?>([^<]+)</i>"), "/\\1/")
            .replace(QRegularExpression("<u(?:\\s[^>]*)?>([^<]+)</u>"), "_\\1_")
            .remove(QRegularExpression("<[^>]*>"));
        return plain;
    }

    // ---------------------------------------------------------------------------

} // namespace

// ===========================================================================

PsiOtrPlugin::PsiOtrPlugin() :
    m_enabled(false), m_otrConnection(nullptr), m_onlineUsers(), m_optionHost(nullptr), m_senderHost(nullptr),
    m_applicationInfo(nullptr), m_accountHost(nullptr), m_accountInfo(nullptr), m_contactInfo(nullptr),
    m_iconHost(nullptr), m_psiEvent(nullptr), m_messageBoxList()
{
}

// ---------------------------------------------------------------------------

PsiOtrPlugin::~PsiOtrPlugin() { }

// ---------------------------------------------------------------------------

QString PsiOtrPlugin::name() const { return "OTR Plugin"; }

// ---------------------------------------------------------------------------

QWidget *PsiOtrPlugin::options()
{
    if (!m_enabled) {
        return nullptr;
    } else {
        return new ConfigDialog(m_otrConnection, m_optionHost, m_accountInfo);
    }
}

// ---------------------------------------------------------------------------

bool PsiOtrPlugin::enable()
{
    QVariant policyOption = m_optionHost->getPluginOption(OPTION_POLICY, DEFAULT_POLICY);
    m_otrConnection       = new OtrMessaging(this, static_cast<OtrPolicy>(policyOption.toInt()));
    m_enabled             = true;

    QFile f(":/otrplugin/otr_yes.png");
    f.open(QIODevice::ReadOnly);
    m_iconHost->addIcon("otrplugin/otr_yes", f.readAll());
    f.close();

    f.setFileName(":/otrplugin/otr_no.png");
    f.open(QIODevice::ReadOnly);
    m_iconHost->addIcon("otrplugin/otr_no", f.readAll());
    f.close();

    f.setFileName(":/otrplugin/otr_unverified.png");
    f.open(QIODevice::ReadOnly);
    m_iconHost->addIcon("otrplugin/otr_unverified", f.readAll());
    f.close();

    return true;
}

// ---------------------------------------------------------------------------

bool PsiOtrPlugin::disable()
{
    const QStringList accounts = m_onlineUsers.keys();
    for (const QString &account : accounts) {
        const QStringList contacts = m_onlineUsers.value(account).keys();
        for (const QString &contact : contacts) {
            if (m_onlineUsers[account][contact]->encrypted()) {
                m_otrConnection->endSession(account, contact);
            }
            m_onlineUsers[account][contact]->disable();
            delete m_onlineUsers[account][contact];
        }
        m_onlineUsers[account].clear();
    }
    m_onlineUsers.clear();

    while (!m_messageBoxList.empty()) {
        qDeleteAll(m_messageBoxList.begin(), m_messageBoxList.end());
        m_messageBoxList.clear();
    }

    delete m_otrConnection;
    m_enabled = false;
    return true;
}

// ---------------------------------------------------------------------------

void PsiOtrPlugin::applyOptions() { }

// ---------------------------------------------------------------------------

void PsiOtrPlugin::restoreOptions() { }

//-----------------------------------------------------------------------------

QString PsiOtrPlugin::pluginInfo()
{
    QString out;
    out += tr("Off-the-Record Messaging (OTR) is a cryptographic protocol "
              "that provides encryption for instant messaging conversations. "
              "In addition to authentication and encryption, OTR provides "
              "forward secrecy and malleable encryption.")
        + "<br/>";
    out += "<br/>";
    out += tr("In comparison with OpenPGP and OMEMO, the OTR protocol does "
              "not depend on XMPP specific structures which allows one to use it "
              "for protecting conversations via XMPP transports (to Telegram, "
              "Skype, VK, QQ and other networks).")
        + "<br/>";
    out += "<br/>";
    out += tr("OTR features:") + "<br/>";
    out += tr("* Fast and easy update of encryption keys.") + "<br/>";
    out += tr("* Simple and convenient authentication of interlocutor "
              "without necessity of comparing public key fingerprints "
              "through an outside communication channel.")
        + "<br/>";
    out += "<br/>";
    out += tr("OTR limitations:") + "<br/>";
    out += tr("* No support of offline messages.") + "<br/>";
    out += tr("* No support of carbon copies to other XMPP resources.") + "<br/>";
    out += tr("* No support of multi-user chats.") + "<br/>";
    out += tr("* No support of file transfer.") + "<br/>";
    out += "<br/>";
    out += tr("OTR provides the following guarantees:");
    out += "<dl>";
    out += "<dt>" + tr("Encryption") + "</dt>";
    out += "<dd>" + tr("No one else can read your instant messages.") + "</dd>";
    out += "<dt>" + tr("Authentication") + "</dt>";
    out += "<dd>" + tr("You are assured the correspondent is who you think it is.") + "</dd>";
    out += "<dt>" + tr("Deniability") + "</dt>";
    out += "<dd>"
        + tr("The messages you send do not have digital signatures that "
             "are checkable by a third party. Anyone can forge messages "
             "after a conversation to make them look like they came from "
             "you. However, during a conversation, your correspondent is "
             "assured the messages (s)he sees are authentic and unmodified.")
        + "</dd>";
    out += "<dt>" + tr("Perfect forward secrecy") + "</dt>";
    out += "<dd>"
        + tr("If you lose control of your private keys, no previous "
             "conversation is compromised.")
        + "</dd>";
    out += "</dl>";
    out += tr("For further information, see "
              "&lt;<a href=\"https://otr.cypherpunks.ca/\">"
              "https://otr.cypherpunks.ca/</a>&gt;.");
    return out;
}

//-----------------------------------------------------------------------------

bool PsiOtrPlugin::decryptMessageElement(int accountIndex, QDomElement &messageElement)
{
    if (!m_enabled || messageElement.isNull() || messageElement.attribute("type") == "error"
        || messageElement.attribute("type") == "groupchat"
        || messageElement.firstChild().toElement().namespaceURI() == "urn:xmpp:carbons:2") {
        return false;
    }

    bool decryptedOtrMassage = false;
    bool ignore              = false;

    const QString &&contact = getCorrectJid(accountIndex, messageElement.attribute("from"));
    const QString &&account = m_accountInfo->getId(accountIndex);

    QDomElement htmlElement = messageElement.firstChildElement("html");
    QDomElement plainBody   = messageElement.firstChildElement("body");
    QString     cyphertext;
    if (!htmlElement.isNull()) {
        QTextStream textStream(&cyphertext);
        htmlElement.firstChildElement("body").save(textStream, 0);
    } else if (!plainBody.isNull()) {
        cyphertext = plainBody.firstChild().toText().nodeValue().toHtmlEscaped();
    } else {
        return false;
    }

    QString        decrypted;
    OtrMessageType messageType = m_otrConnection->decryptMessage(account, contact, cyphertext, decrypted);
    switch (messageType) {
    case OTR_MESSAGETYPE_NONE:
        break;
    case OTR_MESSAGETYPE_IGNORE:
        ignore = true;
        break;
    case OTR_MESSAGETYPE_OTR:
        decryptedOtrMassage = true;
        QString bodyText;

        bool isHTML = !htmlElement.isNull() || Qt::mightBeRichText(decrypted);

        if (!isHTML) {
            bodyText = decrypted;
        } else {
            HtmlTidy htmlTidy("<body xmlns=\"http://www.w3.org/1999/xhtml\">" + decrypted + "</body>");
            decrypted = htmlTidy.output();
            bodyText  = htmlToPlain(decrypted);

            // replace html body
            if (htmlElement.isNull()) {
                htmlElement
                    = messageElement.ownerDocument().createElementNS("http://jabber.org/protocol/xhtml-im", "html");
                messageElement.appendChild(htmlElement);
            } else {
                htmlElement.removeChild(htmlElement.firstChildElement("body"));
            }

            QDomDocument document;
            int          errorLine = 0, errorColumn = 0;
            QString      errorText;
            if (document.setContent(decrypted, true, &errorText, &errorLine, &errorColumn)) {
                htmlElement.appendChild(document.documentElement());
            } else {
                qWarning() << "---- parsing error:\n"
                           << decrypted << "\n----\n"
                           << errorText << " line:" << errorLine << " column:" << errorColumn;
                messageElement.removeChild(htmlElement);
            }
        }

        // replace plaintext body
        plainBody.removeChild(plainBody.firstChild());
        plainBody.appendChild(messageElement.ownerDocument().createTextNode(unescape(bodyText)));

        // for compatibility with XMPP clients which do not support of XEP-0380
        if (messageElement.elementsByTagNameNS("urn:xmpp:eme:0", "encryption").isEmpty()) {
            QDomElement encElement = messageElement.ownerDocument().createElementNS("urn:xmpp:eme:0", "encryption");
            encElement.setAttribute("namespace", "urn:xmpp:otr:0");
            messageElement.appendChild(encElement);
        }
        break;
    }
    if (ignore) {
        messageElement = QDomElement();
    }
    return decryptedOtrMassage;
}

//-----------------------------------------------------------------------------

bool PsiOtrPlugin::encryptMessageElement(int accountIndex, QDomElement &message)
{
    if (!m_enabled || message.attribute("type") == "groupchat") {
        return false;
    }

    const QString &&account     = m_accountInfo->getId(accountIndex);
    const QString &&contact     = getCorrectJid(accountIndex, message.attribute("to"));
    QDomElement     bodyElement = message.firstChildElement("body");

    if (bodyElement.isNull()) {
        return false;
    }

    QDomNode body = bodyElement.firstChild();

    QString encrypted = m_otrConnection->encryptMessage(account, contact, body.nodeValue().toHtmlEscaped());

    // if there has been an error, drop the message
    if (encrypted.isEmpty()) {
        message = QDomElement();
        return false;
    }

    body.setNodeValue(unescape(encrypted));

    if (!m_onlineUsers.value(account).contains(contact)) {
        m_onlineUsers[account][contact] = new PsiOtrClosure(account, contact, m_otrConnection);
    }

    QDomElement htmlElement = message.firstChildElement("html");
    if (m_onlineUsers[account][contact]->encrypted() && !htmlElement.isNull()) {
        message.removeChild(htmlElement);
    }

    if (m_onlineUsers[account][contact]->encrypted()) {
        htmlElement = message.ownerDocument().createElementNS("urn:xmpp:eme:0", "encryption");
        htmlElement.setAttribute("namespace", "urn:xmpp:otr:0");
        message.appendChild(htmlElement);

        if (message.attribute("to").contains("/")) {
            // if not a bare jid
            htmlElement = message.ownerDocument().createElementNS("urn:xmpp:hints", "no-copy");
            message.appendChild(htmlElement);
        }

        htmlElement = message.ownerDocument().createElementNS("urn:xmpp:hints", "no-permanent-store");
        message.appendChild(htmlElement);

        htmlElement = message.ownerDocument().createElementNS("urn:xmpp:carbons:2", "private");
        message.appendChild(htmlElement);
    }

    return true;
}

//-----------------------------------------------------------------------------

void PsiOtrPlugin::setOptionAccessingHost(OptionAccessingHost *host) { m_optionHost = host; }

//-----------------------------------------------------------------------------

void PsiOtrPlugin::optionChanged(const QString &)
{
    QVariant policyOption = m_optionHost->getPluginOption(OPTION_POLICY, DEFAULT_POLICY);
    m_otrConnection->setPolicy(static_cast<OtrPolicy>(policyOption.toInt()));
}

//-----------------------------------------------------------------------------

void PsiOtrPlugin::setStanzaSendingHost(StanzaSendingHost *host) { m_senderHost = host; }

//-----------------------------------------------------------------------------

void PsiOtrPlugin::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host) { m_applicationInfo = host; }

//-----------------------------------------------------------------------------

void PsiOtrPlugin::setPsiAccountControllingHost(PsiAccountControllingHost *host)
{
    m_accountHost = host;
    host->subscribeLogout(this, [this](int accountIndex) {
        if (!m_enabled) {
            return;
        }

        QString account = m_accountInfo->getId(accountIndex);

        if (m_onlineUsers.contains(account)) {
            const QStringList contacts = m_onlineUsers.value(account).keys();
            for (const QString &contact : contacts) {
                m_otrConnection->endSession(account, contact);
                m_onlineUsers[account][contact]->setIsLoggedIn(false);
                m_onlineUsers[account][contact]->updateMessageState();
            }
        }
    });
}

//-----------------------------------------------------------------------------

void PsiOtrPlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost *host) { m_accountInfo = host; }

//-----------------------------------------------------------------------------

void PsiOtrPlugin::setContactInfoAccessingHost(ContactInfoAccessingHost *host) { m_contactInfo = host; }

//-----------------------------------------------------------------------------

void PsiOtrPlugin::setIconFactoryAccessingHost(IconFactoryAccessingHost *host) { m_iconHost = host; }

//-----------------------------------------------------------------------------
void PsiOtrPlugin::setEventCreatingHost(EventCreatingHost *host) { m_psiEvent = host; }

//-----------------------------------------------------------------------------

bool PsiOtrPlugin::incomingStanza(int accountIndex, const QDomElement &xml)
{
    if (!m_enabled || xml.nodeName() != "presence") {
        return false;
    }

    QString account = m_accountInfo->getId(accountIndex);
    QString contact = getCorrectJid(accountIndex, xml.attribute("from"));
    QString type    = xml.attribute("type", "available");

    if (type == "available") {
        if (!m_onlineUsers.value(account).contains(contact)) {
            m_onlineUsers[account][contact] = new PsiOtrClosure(account, contact, m_otrConnection);
        }

        m_onlineUsers[account][contact]->setIsLoggedIn(true);
    } else if (type == "unavailable") {
        if (m_onlineUsers.contains(account) && m_onlineUsers.value(account).contains(contact)) {
            if (m_optionHost->getPluginOption(OPTION_END_WHEN_OFFLINE, DEFAULT_END_WHEN_OFFLINE).toBool()) {
                m_otrConnection->expireSession(account, contact);
            }
            m_onlineUsers[account][contact]->setIsLoggedIn(false);
            m_onlineUsers[account][contact]->updateMessageState();
        }
    }

    return false;
}

//-----------------------------------------------------------------------------

bool PsiOtrPlugin::outgoingStanza(int accountIndex, QDomElement &xml) { return false; }

//-----------------------------------------------------------------------------

QList<QVariantHash> PsiOtrPlugin::getButtonParam() { return QList<QVariantHash>(); }

//-----------------------------------------------------------------------------

QAction *PsiOtrPlugin::getAction(QObject *parent, int accountIndex, const QString &contactJid)
{
    if (!m_enabled) {
        return nullptr;
    }

    QString contact = getCorrectJid(accountIndex, contactJid);
    QString account = m_accountInfo->getId(accountIndex);

    if (!m_onlineUsers.value(account).contains(contact)) {
        m_onlineUsers[account][contact] = new PsiOtrClosure(account, contact, m_otrConnection);
    }

    return m_onlineUsers[account][contact]->getChatDlgMenu(parent);
}

//-----------------------------------------------------------------------------

QString PsiOtrPlugin::dataDir()
{
    return m_applicationInfo->appCurrentProfileDir(ApplicationInfoAccessingHost::DataLocation);
}

//-----------------------------------------------------------------------------

void PsiOtrPlugin::sendMessage(const QString &account, const QString &contact, const QString &message)
{
    int accountIndex = getAccountIndexById(account);
    if (accountIndex != -1) {
        m_senderHost->sendMessage(accountIndex, contact, htmlToPlain(message), "", "chat");
    }
}

//-----------------------------------------------------------------------------

bool PsiOtrPlugin::isLoggedIn(const QString &account, const QString &contact)
{
    if (m_onlineUsers.contains(account) && m_onlineUsers.value(account).contains(contact)) {
        return m_onlineUsers.value(account).value(contact)->isLoggedIn();
    }

    return false;
}

//-----------------------------------------------------------------------------

void PsiOtrPlugin::notifyUser(const QString &account, const QString &contact, const QString &message,
                              const OtrNotifyType &type)
{
    QMessageBox::Icon messageBoxIcon;
    if (type == OTR_NOTIFY_ERROR) {
        messageBoxIcon = QMessageBox::Critical;
    } else if (type == OTR_NOTIFY_WARNING) {
        messageBoxIcon = QMessageBox::Warning;
    } else {
        messageBoxIcon = QMessageBox::Information;
    }

    QMessageBox *messageBox = new QMessageBox(messageBoxIcon, tr("Confirm action"), message, QMessageBox::Ok, nullptr,
                                              Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    m_messageBoxList.enqueue(messageBox);

    m_psiEvent->createNewEvent(getAccountIndexById(account), contact, tr("OTR Plugin: event from %1").arg(contact),
                               this, SLOT(eventActivated()));
}

//-----------------------------------------------------------------------------

void PsiOtrPlugin::eventActivated()
{
    if (!m_messageBoxList.empty()) {
        QMessageBox *messageBox = m_messageBoxList.dequeue();
        if (messageBox) {
            messageBox->exec();
            delete messageBox;
        }
    }
}

//-----------------------------------------------------------------------------

bool PsiOtrPlugin::displayOtrMessage(const QString &account, const QString &contact, const QString &message)
{
    return appendSysMsg(account, contact, message);
}

//-----------------------------------------------------------------------------

void PsiOtrPlugin::stateChange(const QString &account, const QString &contact, OtrStateChange change)
{
    if (!m_onlineUsers.value(account).contains(contact)) {
        m_onlineUsers[account][contact] = new PsiOtrClosure(account, contact, m_otrConnection);
    }

    m_onlineUsers[account][contact]->updateMessageState();

    bool    verified  = m_otrConnection->isVerified(account, contact);
    bool    encrypted = m_onlineUsers[account][contact]->encrypted();
    QString msg;
    QString icon;

    switch (change) {
    case OTR_STATECHANGE_GOINGSECURE:
        msg = encrypted ? tr("Attempting to refresh the private conversation")
                        : tr("Attempting to start a private conversation");
        break;

    case OTR_STATECHANGE_GONESECURE:
        msg  = verified ? tr("Private conversation started") : tr("Unverified conversation started");
        icon = verified ? "otrplugin/otr_yes" : "otrplugin/otr_unverified";
        break;

    case OTR_STATECHANGE_GONEINSECURE:
        msg  = tr("Private conversation lost");
        icon = "otrplugin/otr_no";
        break;

    case OTR_STATECHANGE_CLOSE:
        msg  = tr("Private conversation closed");
        icon = "otrplugin/otr_no";
        break;

    case OTR_STATECHANGE_REMOTECLOSE:
        msg = tr("%1 has ended the private conversation with you; "
                 "you should do the same.")
                  .arg(humanContact(account, contact));
        icon = "otrplugin/otr_no";
        break;

    case OTR_STATECHANGE_STILLSECURE:
        msg  = verified ? tr("Private conversation refreshed") : tr("Unverified conversation refreshed");
        icon = verified ? "otrplugin/otr_yes" : "otrplugin/otr_unverified";
        break;

    case OTR_STATECHANGE_TRUST:
        msg  = verified ? tr("Contact authenticated") : tr("Contact not authenticated");
        icon = verified ? "otrplugin/otr_yes" : "otrplugin/otr_unverified";
        break;
    }

    appendSysMsg(account, contact, msg, icon);
}

//-----------------------------------------------------------------------------

void PsiOtrPlugin::receivedSMP(const QString &account, const QString &contact, const QString &question)
{
    if (m_onlineUsers.contains(account) && m_onlineUsers.value(account).contains(contact)) {
        m_onlineUsers[account][contact]->receivedSMP(question);
    }
}

//-----------------------------------------------------------------------------

void PsiOtrPlugin::updateSMP(const QString &account, const QString &contact, int progress)
{

    if (m_onlineUsers.contains(account) && m_onlineUsers.value(account).contains(contact)) {
        m_onlineUsers[account][contact]->updateSMP(progress);
    }
}

//-----------------------------------------------------------------------------

void PsiOtrPlugin::stopMessages() { m_enabled = false; }

//-----------------------------------------------------------------------------

void PsiOtrPlugin::startMessages() { m_enabled = true; }

//-----------------------------------------------------------------------------

QString PsiOtrPlugin::humanAccount(const QString &accountId)
{
    QString human(getAccountNameById(accountId));

    return human.isEmpty() ? accountId : human;
}

//-----------------------------------------------------------------------------

QString PsiOtrPlugin::humanAccountPublic(const QString &accountId) { return getAccountJidById(accountId); }

//-----------------------------------------------------------------------------

QString PsiOtrPlugin::humanContact(const QString &accountId, const QString &contact)
{
    return m_contactInfo->name(getAccountIndexById(accountId), contact);
}

//-----------------------------------------------------------------------------

bool PsiOtrPlugin::appendSysMsg(const QString &account, const QString &contact, const QString &message,
                                const QString &icon)
{
    QString iconTag;
    if (!icon.isEmpty()) {
        iconTag = QString("<icon name=\"%1\"> ").arg(icon);
    }
    return m_accountHost->appendSysHtmlMsg(getAccountIndexById(account), contact, iconTag + message);
}

// ---------------------------------------------------------------------------

int PsiOtrPlugin::getAccountIndexById(const QString &accountId)
{
    QString id;
    int     accountIndex = 0;
    while (((id = m_accountInfo->getId(accountIndex)) != "-1") && (id != accountId)) {
        accountIndex++;
    }
    return (id == "-1") ? -1 : accountIndex;
}

// ---------------------------------------------------------------------------

QString PsiOtrPlugin::getAccountNameById(const QString &accountId)
{
    return m_accountInfo->getName(getAccountIndexById(accountId));
}

// ---------------------------------------------------------------------------

QString PsiOtrPlugin::getAccountJidById(const QString &accountId)
{
    return m_accountInfo->getJid(getAccountIndexById(accountId));
}

// ---------------------------------------------------------------------------

QString PsiOtrPlugin::getCorrectJid(int accountIndex, const QString &fullJid)
{
    QString correctJid;
    if (m_contactInfo->isPrivate(accountIndex, fullJid)) {
        correctJid = fullJid;
    } else {
        correctJid = removeResource(fullJid);

        // If the contact is private but not (yet) in the roster,
        // it will not be known as private.
        // Therefore, check if the bare Jid is a conference.
        if (m_contactInfo->isConference(accountIndex, correctJid)) {
            correctJid = fullJid;
        }
    }
    return correctJid;
}

//-----------------------------------------------------------------------------

} // namespace psiotr

//-----------------------------------------------------------------------------
