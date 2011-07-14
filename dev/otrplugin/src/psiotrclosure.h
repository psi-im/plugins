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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef PSIOTRCLOSURE_H_
#define PSIOTRCLOSURE_H_

#include <QObject>

class QAction;
class QMenu;

namespace psiotr
{
class PsiOtrPlugin;
class OtrMessaging;

//-----------------------------------------------------------------------------

 class PsiOtrClosure : public QObject
{
    Q_OBJECT

public:
    PsiOtrClosure(const QString& account, const QString& toJid,
                  OtrMessaging* otrc);
    ~PsiOtrClosure();
    void updateMessageState();
    void setIsLoggedIn(bool isLoggedIn);
    bool isLoggedIn() const;
    void disable();
    QAction* getChatDlgMenu(QObject* parent);
    bool encrypted() const;

private:
    OtrMessaging* m_otr;
    QString       m_myAccount;
    QString       m_otherJid;
    QMenu*        m_chatDlgMenu;
    QAction*      m_chatDlgAction;
    QAction*      m_verifyAction;
    QAction*      m_sessionIdAction;
    QAction*      m_fingerprintAction;
    QAction*      m_startSessionAction;
    QAction*      m_endSessionAction;
    bool          m_isLoggedIn;
    QObject*      m_parentWidget;

public slots:
    void initiateSession(bool b);
    void endSession(bool b);
    void verifyFingerprint(bool b);
    void sessionID(bool b);
    void fingerprint(bool b);
    void showMenu();
};

//-----------------------------------------------------------------------------

} // namespace psiotr

#endif
