/*
 * psiotrclosure.cpp
 *
 * Off-the-Record Messaging plugin for Psi+
 * Copyright (C) 2007-2011  Timo Engel (timo-e@freenet.de)
 *               2011-2012  Florian Fieber
 *                    2014  Boris Pek (tehnick-8@mail.ru)
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

#include "psiotrclosure.h"

#include <QObject>
#include <QDialog>
#include <QWidget>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QFont>

namespace psiotr
{

AuthenticationDialog::AuthenticationDialog(OtrMessaging* otrc,
                                           const QString& account,
                                           const QString& contact,
                                           const QString& question,
                                           bool sender, QWidget* parent)
    : QDialog(parent),
      m_otr(otrc),
      m_method(METHOD_QUESTION),
      m_account(account),
      m_contact(contact),
      m_isSender(sender)
{
    setAttribute(Qt::WA_DeleteOnClose);

    m_contactName = m_otr->humanContact(m_account, m_contact);

    QString qaExplanation;
    QString ssExplanation;
    QString fprExplanation;
    if (m_isSender)
    {
        setWindowTitle(tr("Authenticate %1").arg(m_contactName));
        qaExplanation = tr("To authenticate via question and answer, "
                           "ask a question whose answer is only known "
                           "to you and %1.").arg(m_contactName);
        ssExplanation = tr("To authenticate via shared secret, "
                           "enter a secret only known "
                           "to you and %1.").arg(m_contactName);
        fprExplanation = tr("To authenticate manually, exchange your "
                            "fingerprints over an authenticated channel and "
                            "compare each other's fingerprint with the one "
                            "listed beneath.");
    }
    else
    {
        setWindowTitle(tr("Authenticate to %1").arg(m_contactName));
        qaExplanation = tr("%1 wants to authenticate you. To authenticate, "
                           "answer the question asked below.")
                           .arg(m_contactName);
        ssExplanation = tr("%1 wants to authenticate you. To authenticate, "
                           "enter your shared secret below.")
                           .arg(m_contactName);
    }

    m_methodBox = new QComboBox(this);
    m_methodBox->setMinimumWidth(300);

    m_methodBox->addItem(tr("Question and answer"));
    m_methodBox->addItem(tr("Shared secret"));
    m_methodBox->addItem(tr("Fingerprint verification"));

    QLabel* qaExplanationLabel = new QLabel(qaExplanation, this);
    qaExplanationLabel->setWordWrap(true);
    QLabel* ssExplanationLabel = new QLabel(ssExplanation, this);
    ssExplanationLabel->setWordWrap(true);

    m_questionEdit     = new QLineEdit(this);
    m_answerEdit       = new QLineEdit(this);
    m_sharedSecretEdit = new QLineEdit(this);

    QLabel* questionLabel = new QLabel(tr("&Question:"), this);
    questionLabel->setBuddy(m_questionEdit);

    QLabel* answerLabel = new QLabel(tr("A&nswer:"), this);
    answerLabel->setBuddy(m_answerEdit);

    QLabel* sharedSecretLabel = new QLabel(tr("&Shared Secret:"), this);
    sharedSecretLabel->setBuddy(m_sharedSecretEdit);

    m_progressBar  = new QProgressBar(this);

    m_cancelButton = new QPushButton(tr("&Cancel"), this);
    m_startButton  = new QPushButton(tr("&Authenticate"), this);

    m_startButton->setDefault(true);

    m_methodWidget[METHOD_QUESTION] = new QWidget(this);
    QVBoxLayout* qaLayout = new QVBoxLayout;
    qaLayout->setContentsMargins(0, 0, 0, 0);
    qaLayout->addWidget(qaExplanationLabel);
    qaLayout->addSpacing(20);
    qaLayout->addWidget(questionLabel);
    qaLayout->addWidget(m_questionEdit);
    qaLayout->addSpacing(10);
    qaLayout->addWidget(answerLabel);
    qaLayout->addWidget(m_answerEdit);
    qaLayout->addSpacing(20);
    m_methodWidget[METHOD_QUESTION]->setLayout(qaLayout);

    m_methodWidget[METHOD_SHARED_SECRET] = new QWidget(this);
    QVBoxLayout* ssLayout = new QVBoxLayout;
    ssLayout->setContentsMargins(0, 0, 0, 0);
    ssLayout->addWidget(ssExplanationLabel);
    ssLayout->addSpacing(20);
    ssLayout->addWidget(sharedSecretLabel);
    ssLayout->addWidget(m_sharedSecretEdit);
    ssLayout->addSpacing(20);
    m_methodWidget[METHOD_SHARED_SECRET]->setLayout(ssLayout);

    m_methodWidget[METHOD_FINGERPRINT] = NULL;
    QLabel* authenticatedLabel = NULL;
    if (m_isSender)
    {
        if (m_otr->isVerified(m_account, m_contact))
        {
            authenticatedLabel = new QLabel(QString("<b>%1</b>")
                                             .arg(tr("This contact is already "
                                                     "authenticated.")), this);
        }

        QString ownFpr = m_otr->getPrivateKeys().value(
                            m_account,
                            tr("No private key for account \"%1\"")
                              .arg(m_otr->humanAccount(m_account)));

        m_fpr = m_otr->getActiveFingerprint(m_account, m_contact);

        QLabel* fprExplanationLabel = new QLabel(fprExplanation, this);
        fprExplanationLabel->setWordWrap(true);

        QLabel* ownFprDescLabel = new QLabel(tr("Your fingerprint:"), this);
        QLabel* ownFprLabel     = new QLabel(ownFpr, this);
        QLabel* fprDescLabel    = new QLabel(tr("%1's fingerprint:")
                                                .arg(m_contactName), this);
        QLabel* fprLabel        = new QLabel(m_fpr.fingerprintHuman, this);
        ownFprLabel->setFont(QFont("monospace"));
        fprLabel->setFont(QFont("monospace"));

        m_methodWidget[METHOD_FINGERPRINT] = new QWidget(this);
        QVBoxLayout* fprLayout = new QVBoxLayout;
        fprLayout->setContentsMargins(0, 0, 0, 0);
        fprLayout->addWidget(fprExplanationLabel);
        fprLayout->addSpacing(20);
        fprLayout->addWidget(ownFprDescLabel);
        fprLayout->addWidget(ownFprLabel);
        fprLayout->addSpacing(10);
        fprLayout->addWidget(fprDescLabel);
        fprLayout->addWidget(fprLabel);
        m_methodWidget[METHOD_FINGERPRINT]->setLayout(fprLayout);
    }

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_startButton);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(20, 20, 20, 20);
    if (authenticatedLabel)
    {
        mainLayout->addWidget(authenticatedLabel);
        mainLayout->addSpacing(20);
    }
    mainLayout->addWidget(m_methodBox);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(m_methodWidget[METHOD_QUESTION]);
    mainLayout->addWidget(m_methodWidget[METHOD_SHARED_SECRET]);
    if (m_methodWidget[METHOD_FINGERPRINT])
    {
        mainLayout->addWidget(m_methodWidget[METHOD_FINGERPRINT]);
    }
    mainLayout->addWidget(m_progressBar);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);


    connect(m_methodBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(changeMethod(int)));
    connect(m_methodBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(checkRequirements()));
    connect(m_questionEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(checkRequirements()));
    connect(m_answerEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(checkRequirements()));
    connect(m_sharedSecretEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(checkRequirements()));
    connect(m_cancelButton, SIGNAL(clicked()),
            this, SLOT(reject()));
    connect(m_startButton, SIGNAL(clicked()),
            this, SLOT(startAuthentication()));

    if (!m_isSender)
    {
        if (question.isEmpty())
        {
            m_method = METHOD_SHARED_SECRET;
        }
        else
        {
            m_questionEdit->setText(question);
        }
    }

    changeMethod(m_method);
    m_methodBox->setCurrentIndex(m_method);

    reset();
}

AuthenticationDialog::~AuthenticationDialog()
{

}

void AuthenticationDialog::reject()
{
    if (m_state == AUTH_IN_PROGRESS)
    {
        m_otr->abortSMP(m_account, m_contact);
    }

    QDialog::reject();
}

void AuthenticationDialog::reset()
{
    m_state = m_isSender? AUTH_READY : AUTH_IN_PROGRESS;

    m_methodBox->setEnabled(m_isSender);
    m_questionEdit->setEnabled(m_isSender);
    m_answerEdit->setEnabled(true);
    m_sharedSecretEdit->setEnabled(true);
    m_progressBar->setEnabled(false);

    m_progressBar->setValue(0);

    checkRequirements();
}

bool AuthenticationDialog::finished()
{
    return m_state == AUTH_FINISHED;
}

void AuthenticationDialog::checkRequirements()
{
    m_startButton->setEnabled((m_method == METHOD_QUESTION &&
                               !m_questionEdit->text().isEmpty() &&
                               !m_answerEdit->text().isEmpty()) ||
                              (m_method == METHOD_SHARED_SECRET &&
                               !m_sharedSecretEdit->text().isEmpty()) ||
                              (m_method == METHOD_FINGERPRINT));
}

void AuthenticationDialog::changeMethod(int index)
{
    m_method = static_cast<Method>(index);
    for (int i=0; i<3; i++)
    {
        if (m_methodWidget[i])
        {
            m_methodWidget[i]->setVisible(i == index);
        }
    }
    m_progressBar->setVisible(m_method != METHOD_FINGERPRINT);
    adjustSize();
}

void AuthenticationDialog::startAuthentication()
{
    switch (m_method)
    {
        case METHOD_QUESTION:
            if (m_questionEdit->text().isEmpty() ||
                m_answerEdit->text().isEmpty())
            {
                return;
            }

            m_state = AUTH_IN_PROGRESS;

            m_methodBox->setEnabled(false);
            m_questionEdit->setEnabled(false);
            m_answerEdit->setEnabled(false);
            m_progressBar->setEnabled(true);
            m_startButton->setEnabled(false);

            if (m_isSender)
            {
                m_otr->startSMP(m_account, m_contact,
                                m_questionEdit->text(), m_answerEdit->text());
            }
            else
            {
                m_otr->continueSMP(m_account, m_contact, m_answerEdit->text());
            }

            updateSMP(33);

            break;

        case METHOD_SHARED_SECRET:
            if (m_sharedSecretEdit->text().isEmpty())
            {
                return;
            }

            m_state = AUTH_IN_PROGRESS;

            m_methodBox->setEnabled(false);
            m_sharedSecretEdit->setEnabled(false);
            m_progressBar->setEnabled(true);
            m_startButton->setEnabled(false);

            if (m_isSender)
            {
                m_otr->startSMP(m_account, m_contact,
                                QString(), m_sharedSecretEdit->text());
            }
            else
            {
                m_otr->continueSMP(m_account, m_contact, m_sharedSecretEdit->text());
            }

            updateSMP(33);

            break;

        case METHOD_FINGERPRINT:
            if (m_fpr.fingerprint)
            {
                QString msg(tr("Account: ") + m_otr->humanAccount(m_account) + "\n" +
                            tr("User: ") + m_contact + "\n" +
                            tr("Fingerprint: ") + m_fpr.fingerprintHuman + "\n\n" +
                            tr("Have you verified that this is in fact the correct fingerprint?"));

                QMessageBox mb(QMessageBox::Information, tr("Psi OTR"),
                               msg, QMessageBox::Yes | QMessageBox::No, this,
                               Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

                m_otr->verifyFingerprint(m_fpr,
                                         mb.exec() == QMessageBox::Yes);

                close();
            }

            break;
    }
}

void AuthenticationDialog::updateSMP(int progress)
{
    if (progress<0)
    {
        if (progress == -1)
        {
            notify(QMessageBox::Warning,
                   tr("%1 has canceled the authentication process.")
                     .arg(m_contactName));
        }
        else
        {
            notify(QMessageBox::Warning,
                   tr("An error occurred during the authentication process."));
        }

        if (m_isSender)
        {
            reset();
        }
        else
        {
            close();
        }

        return;
    }

    m_progressBar->setValue(progress);

    if (progress == 100) {
        if (m_isSender || m_method == METHOD_SHARED_SECRET)
        {
            m_otr->stateChange(m_account, m_contact,
                               psiotr::OTR_STATECHANGE_TRUST);
        }
        if (m_otr->smpSucceeded(m_account, m_contact))
        {
            m_state = AUTH_FINISHED;
            if (m_otr->isVerified(m_account, m_contact))
            {
                notify(QMessageBox::Information,
                       tr("Authentication successful."));
            }
            else
            {
                notify(QMessageBox::Information,
                       tr("You have been successfully authenticated.\n\n"
                          "You should authenticate %1 as "
                          "well by asking your own question.")
                          .arg(m_contactName));
            }
            close();
        } else {
            m_state = m_isSender? AUTH_READY : AUTH_FINISHED;
            notify(QMessageBox::Critical, tr("Authentication failed."));
            if (m_isSender)
            {
                reset();
            }
            else
            {
                close();
            }
        }
    }
}

//-----------------------------------------------------------------------------

void AuthenticationDialog::notify(const QMessageBox::Icon icon,
                                  const QString& message)
{
    QMessageBox mb(icon, tr("Psi OTR"), message, QMessageBox::Ok, this,
                   Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    mb.exec();
}

//-----------------------------------------------------------------------------

PsiOtrClosure::PsiOtrClosure(const QString& account, const QString& contact,
                             OtrMessaging* otrc)
    : m_otr(otrc),
      m_account(account),
      m_contact(contact),
      m_chatDlgMenu(0),
      m_chatDlgAction(0),
      m_authenticateAction(0),
      m_sessionIdAction(0),
      m_fingerprintAction(0),
      m_startSessionAction(0),
      m_endSessionAction(0),
      m_isLoggedIn(false),
      m_parentWidget(0),
      m_authDialog(0)
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
    m_otr->startSession(m_account, m_contact);
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::authenticateContact(bool)
{
    if (m_authDialog || !encrypted())
    {
        return;
    }

    m_authDialog = new AuthenticationDialog(m_otr,
                                            m_account, m_contact,
                                            QString(), true);

    connect(m_authDialog, SIGNAL(destroyed()),
            this, SLOT(finishAuth()));

    m_authDialog->show();
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::receivedSMP(const QString& question)
{
    if ((m_authDialog && !m_authDialog->finished()) || !encrypted())
    {
        m_otr->abortSMP(m_account, m_contact);
        return;
    }
    if (m_authDialog)
    {
        disconnect(m_authDialog, SIGNAL(destroyed()),
                   this, SLOT(finishAuth()));
        finishAuth();
    }

    m_authDialog = new AuthenticationDialog(m_otr, m_account, m_contact, question, false);

    connect(m_authDialog, SIGNAL(destroyed()),
            this, SLOT(finishAuth()));

    m_authDialog->show();
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::updateSMP(int progress)
{
    if (m_authDialog)
    {
        m_authDialog->updateSMP(progress);
        m_authDialog->show();
    }
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::finishAuth()
{
    m_authDialog = 0;

    updateMessageState();
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::sessionID(bool)
{
    QString sId = m_otr->getSessionId(m_account, m_contact);
    QString msg;

    if (sId.isEmpty())
    {
        msg = tr("No active encrypted session");
    }
    else
    {
        msg = tr("Session ID between account \"%1\" and %2: %3")
                .arg(m_otr->humanAccount(m_account))
                .arg(m_contact)
                .arg(sId);
    }

    m_otr->displayOtrMessage(m_account, m_contact, msg);
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::endSession(bool b)
{
    Q_UNUSED(b);
    m_otr->endSession(m_account, m_contact);
    updateMessageState();
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::fingerprint(bool)
{
    QString fingerprint = m_otr->getPrivateKeys()
                                    .value(m_account,
                                           tr("No private key for account \"%1\"")
                                             .arg(m_otr->humanAccount(m_account)));

    QString msg(tr("Fingerprint for account \"%1\": %2")
                   .arg(m_otr->humanAccount(m_account))
                   .arg(fingerprint));

    m_otr->displayOtrMessage(m_account, m_contact, msg);
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::updateMessageState()
{
    if (m_chatDlgAction)
    {
        OtrMessageState state = m_otr->getMessageState(m_account, m_contact);

        QString stateString(m_otr->getMessageStateString(m_account,
                                                         m_contact));

        if (state == OTR_MESSAGESTATE_ENCRYPTED)
        {
            if (m_otr->isVerified(m_account, m_contact))
            {
                m_chatDlgAction->setIcon(QIcon(":/otrplugin/otr_yes.png"));
            }
            else
            {
                m_chatDlgAction->setIcon(QIcon(":/otrplugin/otr_unverified.png"));
                stateString += ", " + tr("unverified");
            }
        }
        else
        {
            m_chatDlgAction->setIcon(QIcon(":/otrplugin/otr_no.png"));
        }

        m_chatDlgAction->setText(tr("OTR Messaging [%1]").arg(stateString));

        if (state == OTR_MESSAGESTATE_ENCRYPTED)
        {
            m_startSessionAction->setText(tr("Refre&sh private conversation"));
            m_authenticateAction->setEnabled(true);
            m_sessionIdAction->setEnabled(true);
            m_endSessionAction->setEnabled(true);
        }
        else
        {
            m_startSessionAction->setText(tr("&Start private conversation"));
            if (state == OTR_MESSAGESTATE_PLAINTEXT)
            {
                m_authenticateAction->setEnabled(false);
                m_sessionIdAction->setEnabled(false);
                m_endSessionAction->setEnabled(false);
            }
            else // finished, unknown
            {
                m_endSessionAction->setEnabled(true);
                m_authenticateAction->setEnabled(false);
                m_sessionIdAction->setEnabled(false);
            }
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
    m_chatDlgAction = new QAction(QString(), this);

    m_chatDlgMenu = new QMenu();

    m_startSessionAction = m_chatDlgMenu->addAction(QString());
    connect(m_startSessionAction, SIGNAL(triggered(bool)),
            this, SLOT(initiateSession(bool)));

    m_endSessionAction = m_chatDlgMenu->addAction(tr("&End private conversation"));
    connect(m_endSessionAction, SIGNAL(triggered(bool)),
            this, SLOT(endSession(bool)));

    m_chatDlgMenu->insertSeparator(NULL);

    m_authenticateAction = m_chatDlgMenu->addAction(tr("&Authenticate contact"));
    connect(m_authenticateAction, SIGNAL(triggered(bool)),
            this, SLOT(authenticateContact(bool)));

    m_sessionIdAction = m_chatDlgMenu->addAction(tr("Show secure session &ID"));
    connect(m_sessionIdAction, SIGNAL(triggered(bool)),
            this, SLOT(sessionID(bool)));

    m_fingerprintAction = m_chatDlgMenu->addAction(tr("Show own &fingerprint"));
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
    if (m_chatDlgAction)
    {
        m_chatDlgAction->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------

bool PsiOtrClosure::encrypted() const
{
    return m_otr->getMessageState(m_account, m_contact) ==
           OTR_MESSAGESTATE_ENCRYPTED;
}

//-----------------------------------------------------------------------------

} // namespace
