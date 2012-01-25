/*
 * psi-otr.h - off-the-record messaging plugin for psi
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

#ifndef PSIOTRPLUGIN_HPP_
#define PSIOTRPLUGIN_HPP_

#include <QObject>
#include <QtGui>
#include <QDomElement>

#include "OtrMessaging.hpp"
#include "psiplugin.h"
#include "eventfilter.h"
#include "optionaccessinghost.h"
#include "optionaccessor.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"
#include "applicationinfoaccessor.h"
#include "stanzafilter.h"
#include "toolbariconaccessor.h"
#include "accountinfoaccessinghost.h"
#include "accountinfoaccessor.h"
#include "contactinfoaccessinghost.h"
#include "contactinfoaccessor.h"

class ApplicationInfoAccessingHost;

namespace psiotr
{

class ConfigDlg;
class PsiOtrClosure;
    
//-----------------------------------------------------------------------------

class PsiOtrPlugin : public QObject,
                     public PsiPlugin,
                     public EventFilter,
                     public OptionAccessor,
                     public StanzaSender,
                     public ApplicationInfoAccessor,
                     public StanzaFilter,
                     public ToolbarIconAccessor,
                     public AccountInfoAccessor,
                     public ContactInfoAccessor,
                     public OtrCallback
{
Q_OBJECT
Q_INTERFACES(PsiPlugin
             EventFilter
             OptionAccessor
             StanzaSender
             ApplicationInfoAccessor
             StanzaFilter
             ToolbarIconAccessor
             AccountInfoAccessor
             ContactInfoAccessor)

public:
    PsiOtrPlugin();
    ~PsiOtrPlugin();

    // PsiPlugin
    virtual QString name() const;
    virtual QString shortName() const;
    virtual QString version() const;
    virtual QWidget* options(); 
    virtual bool enable();
    virtual bool disable();
    virtual void applyOptions();
    virtual void restoreOptions();

    // EventFilter
    virtual bool processEvent(int accountIndex, QDomElement& e);
    virtual bool processMessage(int accountIndex, const QString& contact,
                                const QString& body, const QString& subject);
    virtual bool processOutgoingMessage(int accountIndex, const QString& contact,
                                        QString& body, const QString& type,
                                        QString& subject);
    virtual void logout(int accountIndex);

    // OptionAccessor
    virtual void setOptionAccessingHost(OptionAccessingHost* host);
    virtual void optionChanged(const QString& option);

    // StanzaSender
    virtual void setStanzaSendingHost(StanzaSendingHost *host);

    // ApplicationInfoAccessor
    virtual void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host);

    // StanzaFilter
    virtual bool incomingStanza(int accountIndex, const QDomElement& xml);
    virtual bool outgoingStanza(int accountIndex, QDomElement &xml);

    // ToolbarIconAccessor
    virtual QList<QVariantHash> getButtonParam();
    virtual QAction* getAction(QObject* parent, int accountIndex,
                               const QString& contact);

    // AccountInfoAccessor
    virtual void setAccountInfoAccessingHost(AccountInfoAccessingHost* host);

    // ContactInfoAccessor
    virtual void setContactInfoAccessingHost(ContactInfoAccessingHost* host);

    // OtrCallback
    virtual QString dataDir();
    virtual void sendMessage(const QString& account, const QString& contact,
                             const QString& message);
    virtual bool isLoggedIn(const QString& account, const QString& contact);
    virtual void notifyUser(const OtrNotifyType& type, const QString& message);
    virtual void receivedSMP(const QString& account, const QString& contact,
                             const QString& question);
    virtual void updateSMP(const QString& account, const QString& contact,
                           int progress);
    virtual void stopMessages();
    virtual void startMessages();
    virtual QString humanAccount(const QString& accountId);
    virtual QString humanAccountPublic(const QString& accountId);

    // Helper methods
    /**
     * Returns the index of the account identified by accountId or -1
     */
    int getAccountIndexById(const QString& accountId);

    /**
     * Returns the name of the account identified by accountId or ""
     */
    QString getAccountNameById(const QString& accountId);

    /**
     * Returns the Jid of the account identified by accountId or "-1"
     */
    QString getAccountJidById(const QString& accountId);

private:
    /**
     * Returns full Jid for private contacts,
     * bare Jid for non-private contacts.
     */
    QString getCorrectJid(int accountIndex, const QString& fullJid);

    bool                                            m_enabled;
    OtrMessaging*                                   m_otrConnection;
    QHash<QString, QHash<QString, PsiOtrClosure*> > m_onlineUsers;
    OptionAccessingHost*                            m_optionHost;
    StanzaSendingHost*                              m_senderHost;
    ApplicationInfoAccessingHost*                   m_applicationInfo;
    AccountInfoAccessingHost*                       m_accountInfo;
    ContactInfoAccessingHost*                       m_contactInfo;
};

//-----------------------------------------------------------------------------

} // namespace psiotr

#endif
