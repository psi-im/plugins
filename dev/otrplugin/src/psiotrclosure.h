/*
 * psiotrclosure.hpp
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef PSIOTRCLOSURE_H_
#define PSIOTRCLOSURE_H_

#include "OtrMessaging.hpp"

#include <QObject>
#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QMessageBox>

class QAction;
class QMenu;

namespace psiotr
{
class PsiOtrPlugin;
class OtrMessaging;

//-----------------------------------------------------------------------------

class AuthenticationDialog : public QDialog
{
    Q_OBJECT
public:
    AuthenticationDialog(OtrMessaging* otrc,
                         const QString& account, const QString& contact,
                         const QString& question, bool sender,
                         QWidget *parent = 0);
    ~AuthenticationDialog();

    void reset();
    bool finished();
    void updateSMP(int progress);
    void notify(const QMessageBox::Icon icon, const QString& message);

public slots:
    void reject();

private:
    enum AuthState {AUTH_READY, AUTH_IN_PROGRESS, AUTH_FINISHED};

    OtrMessaging* m_otr;
    int           m_method;
    QString       m_account;
    QString       m_contact;
    bool          m_isSender;
    AuthState     m_state;
    Fingerprint   m_fpr;

    QWidget*      m_methodWidget[2];
    QComboBox*    m_methodBox;
    QLineEdit*    m_questionEdit;
    QLineEdit*    m_answerEdit;
    QProgressBar* m_progressBar;
    QPushButton*  m_cancelButton;
    QPushButton*  m_startButton;
    
private slots:
    void changeMethod(int index);
    void startAuthentication();
};

//-----------------------------------------------------------------------------

class PsiOtrClosure : public QObject
{
    Q_OBJECT

public:
    PsiOtrClosure(const QString& account, const QString& contact,
                  OtrMessaging* otrc);
    ~PsiOtrClosure();
    void updateMessageState();
    void setIsLoggedIn(bool isLoggedIn);
    bool isLoggedIn() const;
    void disable();
    QAction* getChatDlgMenu(QObject* parent);
    bool encrypted() const;
    void receivedSMP(const QString& question);
    void updateSMP(int progress);

private:
    OtrMessaging* m_otr;
    QString       m_account;
    QString       m_contact;
    QMenu*        m_chatDlgMenu;
    QAction*      m_chatDlgAction;
    QAction*      m_authenticateAction;
    QAction*      m_sessionIdAction;
    QAction*      m_fingerprintAction;
    QAction*      m_startSessionAction;
    QAction*      m_endSessionAction;
    bool          m_isLoggedIn;
    QObject*      m_parentWidget;
    AuthenticationDialog* m_authDialog;

public slots:
    void initiateSession(bool b);
    void endSession(bool b);
    void authenticateContact(bool b);
    void sessionID(bool b);
    void fingerprint(bool b);
    void showMenu();
    void finishSMP();
};

//-----------------------------------------------------------------------------

} // namespace psiotr

#endif
