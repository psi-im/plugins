/*
 * psi-otr.cpp - off-the-record messaging plugin for psi
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

#include "PsiOtrPlugin.hpp"
#include "psiotrclosure.h"
#include "PsiOtrConfig.hpp"
#include "applicationinfoaccessinghost.h"

namespace psiotr
{

// ---------------------------------------------------------------------------

namespace
{

// ---------------------------------------------------------------------------

/**
 * Removes the resource from a given JID. 
 * Example:
 * removeResource("user@jabber.org/Home")
 * returns "user@jabber.org"
 */
QString removeResource(const QString& aJid)
{
    QString addr(aJid);
    int pos = aJid.indexOf("/");
    if (pos > -1)
    {
        addr.truncate(pos);
        return addr;
    }
    return addr;
}

// ---------------------------------------------------------------------------

} // namespace

// ===========================================================================

PsiOtrPlugin::PsiOtrPlugin()
    : m_enabled(false),
      m_otrConnection(NULL),
      m_onlineUsers(),
      m_psiDataDir(),
      m_optionHost(NULL),
      m_senderHost(NULL),
      m_applicationInfo(NULL)
{
}

// ---------------------------------------------------------------------------

PsiOtrPlugin::~PsiOtrPlugin()
{
}

// ---------------------------------------------------------------------------

QString PsiOtrPlugin::name() const
{
    return "Off-the-Record Messaging";
}

// ---------------------------------------------------------------------------

QString PsiOtrPlugin::shortName() const
{
    return "psi-otr";
}

// ---------------------------------------------------------------------------

QString PsiOtrPlugin::version() const
{
    return "0.9";
}

// ---------------------------------------------------------------------------

QWidget* PsiOtrPlugin::options()
{
    if (!m_enabled)
    {
        return 0;
    }
    else
    {
        return new ConfigDialog(m_otrConnection, m_optionHost);
    }
} 

// ---------------------------------------------------------------------------

bool PsiOtrPlugin::enable()
{
    QVariant policyOption = m_optionHost->getGlobalOption(PSI_CONFIG_POLICY);
    m_otrConnection = new OtrMessaging(this,
                                       static_cast<OtrPolicy>(policyOption.toInt()));
    m_enabled = true;
    return true;
}

// ---------------------------------------------------------------------------

bool PsiOtrPlugin::disable()
{
    foreach (QString account, m_onlineUsers.keys())
    {
        foreach(QString jid, m_onlineUsers.value(account).keys())
        {
            m_otrConnection->endSession(account, jid);
            m_onlineUsers[account][jid]->updateMessageState();
            m_onlineUsers[account][jid]->disable();
        }
    }

    
    delete m_otrConnection;
    m_enabled = false;
    return true;
}

// ---------------------------------------------------------------------------

void PsiOtrPlugin::applyOptions()
{
}

// ---------------------------------------------------------------------------

void PsiOtrPlugin::restoreOptions()
{
}

//-----------------------------------------------------------------------------

bool PsiOtrPlugin::processEvent(int accountNo, QDomElement& e)
{
    if (m_enabled && e.attribute("type") == "MessageEvent" &&
        !e.firstChildElement("message").isNull())
    {
        QDomElement messageElement = e.firstChildElement("message");
        QString contact = removeResource(messageElement.attribute("from"));
        QString account = QString::number(accountNo);

        QDomElement htmlElement = messageElement.firstChildElement("html");
        QDomElement plainBody   = messageElement.firstChildElement("body");
        QString cyphertext;
        if (!htmlElement.isNull())
        {
            QTextStream textStream(&cyphertext);
            htmlElement.firstChildElement("body").save(textStream, 0);
        }
        else if (!plainBody.isNull())
        {
            cyphertext = plainBody.firstChild().toText().nodeValue();
        }
        else
        {
            return false;
        }

        QString decrypted;
        if (m_otrConnection->decryptMessage(contact, account, cyphertext,
                                            decrypted))
        {
            if (m_onlineUsers.contains(account) && 
                m_onlineUsers.value(account).contains(contact))
            {
                m_onlineUsers[account][contact]->updateMessageState();
            }
            
            // replace plaintext body
            plainBody.removeChild(plainBody.firstChild());
            QString bodyText = decrypted;
            bodyText.remove(QRegExp("<[^>]*>")); bodyText.remove("\n");
            plainBody.appendChild(e.ownerDocument().createTextNode(bodyText));

            // replace html body
            if (htmlElement.isNull())
            {
                htmlElement = e.ownerDocument().createElement("html");
                htmlElement.setAttribute("xmlns",
                                         "http://jabber.org/protocol/xhtml-im");
                messageElement.appendChild(htmlElement);
            }
            else
            {
                htmlElement.removeChild(htmlElement.firstChildElement("body"));
            }

            QDomDocument document;
            int errorLine = 0, errorColumn = 0;
            QString errorText;
            if (!document.setContent(decrypted, true, &errorText, &errorLine,
                                     &errorColumn))
            {
                qWarning() << "---- parsing error:\n" << decrypted <<
                              "\n----\n" << errorText << " line:" <<
                              errorLine << " column:" << errorColumn;
                QDomElement domBody = e.ownerDocument().createElement("body");
                domBody.appendChild(e.ownerDocument().createTextNode(bodyText));
                htmlElement.appendChild(domBody);
            }
            htmlElement.appendChild(document.documentElement());
        }
    }
    return false;
}

//-----------------------------------------------------------------------------

bool PsiOtrPlugin::processMessage(int, const QString&, const QString&,
                                  const QString&)
{
    return false;
}

//-----------------------------------------------------------------------------

bool PsiOtrPlugin::processOutgoingMessage(int account, const QString& toJid,
                                          QString& body, const QString& type,
                                          QString&)
{
    if (!m_enabled || type == "groupchat")
    {
        return false;
    }
    
    QString encrypted = m_otrConnection->encryptMessage(
        QString::number(account),
        removeResource(toJid),
        body);

    body = encrypted;

    return false;
}

// ---------------------------------------------------------------------------

void PsiOtrPlugin::logout(int accountNo)
{
    if (!m_enabled)
    {
        return;
    }
    
    QString account = QString::number(accountNo);

    if (m_onlineUsers.contains(account))
    {
        foreach(QString jid, m_onlineUsers.value(account).keys())
        {
            m_otrConnection->endSession(account, jid);
            m_onlineUsers[account][jid]->setIsLoggedIn(false);
            m_onlineUsers[account][jid]->updateMessageState();
        }
    }
}

//-----------------------------------------------------------------------------

void PsiOtrPlugin::setOptionAccessingHost(OptionAccessingHost* host)
{
    m_optionHost = host;
}

//-----------------------------------------------------------------------------

void PsiOtrPlugin::optionChanged(const QString&)
{
    QVariant policyOption = m_optionHost->getGlobalOption(PSI_CONFIG_POLICY);
    m_otrConnection->setPolicy(static_cast<OtrPolicy>(policyOption.toInt()));
}

//-----------------------------------------------------------------------------

void PsiOtrPlugin::setStanzaSendingHost(StanzaSendingHost *host)
{
    m_senderHost = host;
}

//-----------------------------------------------------------------------------

void PsiOtrPlugin::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host)
{
    m_applicationInfo = host;
}

//-----------------------------------------------------------------------------

bool PsiOtrPlugin::incomingStanza(int accountNo, const QDomElement& xml)
{
    if (!m_enabled || xml.nodeName() != "presence")
    {
        return false;
    }
    
    QString account = QString::number(accountNo);
    QString contact = removeResource(xml.attribute("from"));
    QString type = xml.attribute("type", "available");
    
    if (type == "available")
    {
        if (!m_onlineUsers.value(account).contains(contact))
        {
            m_onlineUsers[account][contact] = new PsiOtrClosure(account,
                                                                contact,
                                                                m_otrConnection);
        }
        
        m_onlineUsers[account][contact]->setIsLoggedIn(true);
    }
    else if (type == "unavailable")
    {
        if (m_onlineUsers.contains(account) && 
            m_onlineUsers.value(account).contains(contact))
        {
            m_onlineUsers[account][contact]->setIsLoggedIn(false);
            m_onlineUsers[account][contact]->updateMessageState();
        }
    }

    return false;
}

//-----------------------------------------------------------------------------

bool PsiOtrPlugin::outgoingStanza(int accountNo, QDomElement& xml)
{
    if (!m_enabled || xml.nodeName() != "message")
    {
        return false;
    }

    QString account = QString::number(accountNo);
    QString toJid = removeResource(xml.attribute("to"));
        
    if (!m_onlineUsers.value(account).contains(toJid))
    {
        m_onlineUsers[account][toJid] = new PsiOtrClosure(account, toJid,
                                                          m_otrConnection);
    }

    QDomElement htmlElement = xml.firstChildElement("html");
    if (m_onlineUsers[account][toJid]->encrypted() && !htmlElement.isNull())
    {
        xml.removeChild(htmlElement);
    }

    return false;
}

//-----------------------------------------------------------------------------

QList<QVariantHash> PsiOtrPlugin::getButtonParam()
{
    return QList<QVariantHash>();
}

//-----------------------------------------------------------------------------

QAction* PsiOtrPlugin::getAction(QObject* parent, int accountNo,
                                 const QString& contactJid)
{
    if (!m_enabled)
    {
        return 0;
    }

    QString contact = removeResource(contactJid);
    QString account = QString::number(accountNo);
    
    if (!m_onlineUsers.value(account).contains(contact))
    {
        m_onlineUsers[account][contact] = new PsiOtrClosure(account,
                                                            contact,
                                                            m_otrConnection);
    }

    return m_onlineUsers[account][contact]->getChatDlgMenu(parent);
}

//-----------------------------------------------------------------------------

QString PsiOtrPlugin::dataDir()
{
    return m_applicationInfo->appCurrentProfileDir(
    ApplicationInfoAccessingHost::DataLocation);
}
    
//-----------------------------------------------------------------------------

void PsiOtrPlugin::sendMessage(const QString& account, const QString& toJid,
                               const QString& message)
{
    m_senderHost->sendMessage(account.toInt(), toJid, message, "", "chat");
}

//-----------------------------------------------------------------------------

bool PsiOtrPlugin::isLoggedIn(const QString& account, const QString& jid)
{
    if (m_onlineUsers.contains(account) &&
        m_onlineUsers.value(account).contains(jid))
    {
        return m_onlineUsers.value(account).value(jid)->isLoggedIn();
    }

    return false;
}

//-----------------------------------------------------------------------------

void PsiOtrPlugin::notifyUser(const OtrNotifyType& type, const QString& message)
{
    QMessageBox::Icon messageBoxIcon;
    if (type == OTR_NOTIFY_ERROR)
    {
        messageBoxIcon = QMessageBox::Critical; 
    }
    else if (type == OTR_NOTIFY_WARNING)
    {
        messageBoxIcon = QMessageBox::Warning;
    }
    else
    {
        messageBoxIcon = QMessageBox::Information;
    }

    QMessageBox mb(messageBoxIcon, "psi-otr", message, QMessageBox::Ok, NULL,
                   Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    mb.exec();
}

//-----------------------------------------------------------------------------

void PsiOtrPlugin::stopMessages()
{
    m_enabled = false;
}

//-----------------------------------------------------------------------------

void PsiOtrPlugin::startMessages()
{
    m_enabled = true;
}

//-----------------------------------------------------------------------------

} // namespace psiotr

//-----------------------------------------------------------------------------

Q_EXPORT_PLUGIN2(psiOtrPlugin, psiotr::PsiOtrPlugin)

//-----------------------------------------------------------------------------
