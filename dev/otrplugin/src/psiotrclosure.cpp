/*
 * psiotrclosure.cpp
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

#include "psiotrclosure.h"
#include "OtrMessaging.hpp"

#include <QAction>
#include <QMenu>
#include <QMessageBox>

namespace psiotr
{

//-----------------------------------------------------------------------------

PsiOtrClosure::PsiOtrClosure(const QString& account, const QString& toJid, 
                             OtrMessaging* otrc)
    : m_otr(otrc), 
      m_myAccount(account),
      m_otherJid(toJid),
      m_chatDlgMenu(0),
      m_chatDlgAction(0),
      m_verifyAction(0),
      m_sessionIdAction(0),
      m_fingerprintAction(0),
      m_startSessionAction(0),
      m_endSessionAction(0),
      m_isLoggedIn(false),
      m_parentWidget(0)
{
}

//-----------------------------------------------------------------------------

PsiOtrClosure::~PsiOtrClosure()
{
    if (m_chatDlgMenu)
    {
        delete m_chatDlgMenu;
    }
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::initiateSession(bool b)
{
    Q_UNUSED(b);
    m_otr->startSession(m_myAccount, m_otherJid);
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::verifyFingerprint(bool)
{
    Fingerprint fingerprint;
    bool found = false;

    foreach(fingerprint, m_otr->getFingerprints())
    {
        if (fingerprint.account == m_myAccount)
        {
            found = true;
            break;
        }
    }

    if (found)
    {
        QString msg("Account: " + m_myAccount + "\n" +
                    "User: " + m_otherJid + "\n" +
                    "Fingerprint: " + fingerprint.fingerprintHuman + "\n\n" +
                    "Have you verified that this is in fact the correct fingerprint?");

        QMessageBox mb(QMessageBox::Information, "psi-otr",
                       msg, QMessageBox::Yes | QMessageBox::No,
                       static_cast<QWidget*>(m_parentWidget),
                       Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

        if (mb.exec() == QMessageBox::Yes)
        {
            m_otr->verifyFingerprint(fingerprint, true);
        }
        else
        {
            m_otr->verifyFingerprint(fingerprint, false);
        }
    }
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::sessionID(bool)
{
    QString sId = m_otr->getSessionId(m_myAccount, m_otherJid);
    QString msg;

    if (sId.isEmpty() ||
        (sId.compare(QString("<b></b>")) == 0) ||
        (sId.compare(QString("<b> </b>")) == 0) ||
        (sId.compare(QString(" <b> </b>")) == 0))
    {
        msg = QString("no active encrypted session");
    }
    else
    {
        msg = "Session ID of connection from account " + m_myAccount +
              " to " + m_otherJid + " is:<br/>" + sId + ".";
    }

    QMessageBox mb(QMessageBox::Information, "psi-otr", msg, NULL,
                   static_cast<QWidget*>(m_parentWidget),
                   Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    mb.setTextFormat(Qt::RichText);
    mb.exec();
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::endSession(bool b)
{
    Q_UNUSED(b);
    m_otr->endSession(m_myAccount, m_otherJid);
    updateMessageState();
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::fingerprint(bool)
{
    QString fingerprint =  m_otr->getPrivateKeys().value(m_myAccount,
                                                         "no private key for " +
                                                         m_myAccount);

    QString msg("Fingerprint for account " + m_myAccount + " is:\n" +
                fingerprint + ".");

    QMessageBox mb(QMessageBox::Information, "psi-otr",
                   msg, NULL, static_cast<QWidget*>(m_parentWidget),
                   Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    mb.exec();
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::updateMessageState()
{
    if (m_chatDlgAction != 0)
    {
        QString stateString("OTR Plugin [" +
                            m_otr->getMessageStateString(m_myAccount,
                                                         m_otherJid) +
                            "]");
        m_chatDlgAction->setText(stateString);

        if (m_otr->getMessageState(m_myAccount, m_otherJid) ==
            OTR_MESSAGESTATE_ENCRYPTED)
        {
            m_chatDlgAction->setIcon(QIcon(":/psi-otr/otr_yes.png"));
        }
        else
        {
            m_chatDlgAction->setIcon(QIcon(":/psi-otr/otr_no.png"));
        }

        OtrMessageState state = m_otr->getMessageState(m_myAccount, m_otherJid);

        if (state == OTR_MESSAGESTATE_ENCRYPTED)
        {
            m_verifyAction->setEnabled(true);
            m_sessionIdAction->setEnabled(true);
            m_startSessionAction->setEnabled(false);
            m_endSessionAction->setEnabled(true);
        }
        else if (state == OTR_MESSAGESTATE_PLAINTEXT)
        {
            m_verifyAction->setEnabled(false);
            m_sessionIdAction->setEnabled(false);
            m_startSessionAction->setEnabled(true);
            m_endSessionAction->setEnabled(false);
        }
        else // finished, unknown
        {
            m_startSessionAction->setEnabled(true);
            m_endSessionAction->setEnabled(true);
            m_verifyAction->setEnabled(false);
            m_sessionIdAction->setEnabled(false);
        }
            
        if (m_otr->getPolicy() < OTR_POLICY_ENABLED)
        {
            m_startSessionAction->setEnabled(false);
            m_endSessionAction->setEnabled(false);
        }
    }
}

//-----------------------------------------------------------------------------

QAction* PsiOtrClosure::getChatDlgMenu(QObject* parent)
{
    m_parentWidget = parent;
    m_chatDlgAction = new QAction("", this);

    m_chatDlgMenu = new QMenu();

    m_startSessionAction = m_chatDlgMenu->addAction("Start private Conversation");
    connect(m_startSessionAction, SIGNAL(triggered(bool)),
            this, SLOT(initiateSession(bool)));

    m_endSessionAction = m_chatDlgMenu->addAction("End private Conversation");
    connect(m_endSessionAction, SIGNAL(triggered(bool)),
            this, SLOT(endSession(bool)));

    m_chatDlgMenu->insertSeparator(NULL);

    m_verifyAction = m_chatDlgMenu->addAction("Verify Fingerprint");
    connect(m_verifyAction, SIGNAL(triggered(bool)),
            this, SLOT(verifyFingerprint(bool)));

    m_sessionIdAction = m_chatDlgMenu->addAction("Show secure Session ID");
    connect(m_sessionIdAction, SIGNAL(triggered(bool)),
            this, SLOT(sessionID(bool)));

    m_fingerprintAction = m_chatDlgMenu->addAction("Show own Fingerprint");
    connect(m_fingerprintAction, SIGNAL(triggered(bool)),
            this, SLOT(fingerprint(bool)));

    connect(m_chatDlgAction, SIGNAL(triggered()),
            this, SLOT(showMenu()));

    updateMessageState();

    return m_chatDlgAction;
}

//-----------------------------------------------------------------------------

 void PsiOtrClosure::showMenu()
 {
     m_chatDlgMenu->popup(QCursor::pos(), m_chatDlgAction);
 }
    
//-----------------------------------------------------------------------------

void PsiOtrClosure::setIsLoggedIn(bool isLoggedIn)
{
    m_isLoggedIn = isLoggedIn;
}

//-----------------------------------------------------------------------------

bool PsiOtrClosure::isLoggedIn() const
{
    return m_isLoggedIn;
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::disable()
{
    if (m_chatDlgAction != 0)
    {
        m_chatDlgAction->setEnabled(false);
    }
}
    
//-----------------------------------------------------------------------------

bool PsiOtrClosure::encrypted() const
{
    return m_otr->getMessageState(m_myAccount, m_otherJid) ==
           OTR_MESSAGESTATE_ENCRYPTED;
}
   
//-----------------------------------------------------------------------------

} // namespace
