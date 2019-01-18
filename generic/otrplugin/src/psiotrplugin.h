/*
 * psiotrplugin.h
 *
 * Off-the-Record Messaging plugin for Psi+
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef PSIOTRPLUGIN_H_
#define PSIOTRPLUGIN_H_

#include <QObject>
#include <QMessageBox>
#include <QQueue>

#include "otrmessaging.h"
#include "psiplugin.h"
#include "plugininfoprovider.h"
#include "eventfilter.h"
#include "optionaccessinghost.h"
#include "optionaccessor.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"
#include "applicationinfoaccessor.h"
#include "psiaccountcontroller.h"
#include "stanzafilter.h"
#include "toolbariconaccessor.h"
#include "accountinfoaccessor.h"
#include "contactinfoaccessor.h"
#include "iconfactoryaccessor.h"
#include "eventcreatinghost.h"
#include "eventcreator.h"
#include "encryptionsupport.h"

class ApplicationInfoAccessingHost;
class PsiAccountControllingHost;
class AccountInfoAccessingHost;
class ContactInfoAccessingHost;
class IconFactoryAccessingHost;

class QDomElement;
class QString;
class QAction;

namespace psiotr
{

class PsiOtrClosure;

//-----------------------------------------------------------------------------

class PsiOtrPlugin : public QObject,
                     public PsiPlugin,
                     public PluginInfoProvider,
                     public EventFilter,
                     public EventCreator,
                     public OptionAccessor,
                     public StanzaSender,
                     public ApplicationInfoAccessor,
                     public PsiAccountController,
                     public StanzaFilter,
                     public ToolbarIconAccessor,
                     public AccountInfoAccessor,
                     public ContactInfoAccessor,
                     public IconFactoryAccessor,
                     public OtrCallback,
                     public EncryptionSupport
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.psi-plus.PsiOtrPlugin")
    Q_INTERFACES(PsiPlugin
                 PluginInfoProvider
                 EventFilter
                 EventCreator
                 OptionAccessor
                 StanzaSender
                 ApplicationInfoAccessor
                 PsiAccountController
                 StanzaFilter
                 ToolbarIconAccessor
                 AccountInfoAccessor
                 ContactInfoAccessor
                 IconFactoryAccessor
                 EncryptionSupport)

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
    virtual QPixmap icon() const;

    // PluginInfoProvider
    virtual QString pluginInfo();

    // EventFilter
    virtual bool processEvent(int accountIndex, QDomElement& e);
    virtual bool processMessage(int accountIndex, const QString& contact,
                                const QString& body, const QString& subject);
    virtual bool processOutgoingMessage(int accountIndex, const QString& contact,
                                        QString& body, const QString& type,
                                        QString& subject);
    virtual void logout(int accountIndex);

    // EventCreator
    virtual void setEventCreatingHost(EventCreatingHost *host);

    // OptionAccessor
    virtual void setOptionAccessingHost(OptionAccessingHost* host);
    virtual void optionChanged(const QString& option);

    // StanzaSender
    virtual void setStanzaSendingHost(StanzaSendingHost* host);

    // ApplicationInfoAccessor
    virtual void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host);

    // PsiAccountController
    virtual void setPsiAccountControllingHost(PsiAccountControllingHost* host);

    // StanzaFilter
    virtual bool incomingStanza(int accountIndex, const QDomElement& xml);
    virtual bool outgoingStanza(int accountIndex, QDomElement& xml);

    // ToolbarIconAccessor
    virtual QList<QVariantHash> getButtonParam();
    virtual QAction* getAction(QObject* parent, int accountIndex,
                               const QString& contact);

    // AccountInfoAccessor
    virtual void setAccountInfoAccessingHost(AccountInfoAccessingHost* host);

    // ContactInfoAccessor
    virtual void setContactInfoAccessingHost(ContactInfoAccessingHost* host);

    // IconFactoryAccessingHost
    virtual void setIconFactoryAccessingHost(IconFactoryAccessingHost* host);

    // EncryptionSupport
    virtual bool decryptMessageElement(int account, QDomElement &message);
    virtual bool encryptMessageElement(int account, QDomElement &message);

    // OtrCallback
    virtual QString dataDir();
    virtual void sendMessage(const QString& account, const QString& contact,
                             const QString& message);
    virtual bool isLoggedIn(const QString& account, const QString& contact);
    virtual void notifyUser(const QString& account, const QString& contact,
                            const QString& message, const OtrNotifyType& type);

    virtual bool displayOtrMessage(const QString& account, const QString& contact,
                                   const QString& message);
    virtual void stateChange(const QString& account, const QString& contact,
                             OtrStateChange change);

    virtual void receivedSMP(const QString& account, const QString& contact,
                             const QString& question);
    virtual void updateSMP(const QString& account, const QString& contact,
                           int progress);

    virtual void stopMessages();
    virtual void startMessages();

    virtual QString humanAccount(const QString& accountId);
    virtual QString humanAccountPublic(const QString& accountId);
    virtual QString humanContact(const QString& accountId,
                                 const QString& contact);

    /**
     * Displays a rich text system message for (account, contact)
     */
    bool appendSysMsg(const QString& account, const QString& contact,
                      const QString& message, const QString& icon = "");

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

private slots:
    void eventActivated();

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
    PsiAccountControllingHost*                      m_accountHost;
    AccountInfoAccessingHost*                       m_accountInfo;
    ContactInfoAccessingHost*                       m_contactInfo;
    IconFactoryAccessingHost*                       m_iconHost;
    EventCreatingHost*                              m_psiEvent;
    QQueue<QMessageBox*>                            m_messageBoxList;
};

//-----------------------------------------------------------------------------

} // namespace psiotr

#endif
