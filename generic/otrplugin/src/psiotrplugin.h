/*
 * psiotrplugin.h
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
 */

#ifndef PSIOTRPLUGIN_H_
#define PSIOTRPLUGIN_H_

#include <QMessageBox>
#include <QObject>
#include <QQueue>
#include <QSet>

#include "accountinfoaccessor.h"
#include "applicationinfoaccessor.h"
#include "contactinfoaccessor.h"
#include "encryptionmethodaccessor.h"
#include "encryptionmethodprovider.h"
#include "eventcreatinghost.h"
#include "eventcreator.h"
#include "eventfilter.h"
#include "iconfactoryaccessor.h"
#include "optionaccessinghost.h"
#include "optionaccessor.h"
#include "otrmessaging.h"
#include "plugininfoprovider.h"
#include "psiaccountcontroller.h"
#include "psiplugin.h"
#include "stanzafilter.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"
#include "toolbariconaccessor.h"

class ApplicationInfoAccessingHost;
class PsiAccountControllingHost;
class AccountInfoAccessingHost;
class ContactInfoAccessingHost;
class IconFactoryAccessingHost;

class QDomElement;
class QString;
class QAction;

namespace psiotr {

class PsiOtrClosure;

class PsiOtrPlugin : public QObject,
                     public PsiPlugin,
                     public PluginInfoProvider,
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
                     public EncryptionMethodAccessor {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.psi-plus.PsiOtrPlugin" FILE "psiplugin.json")
    Q_INTERFACES(PsiPlugin PluginInfoProvider EventCreator OptionAccessor StanzaSender ApplicationInfoAccessor
                     PsiAccountController StanzaFilter ToolbarIconAccessor AccountInfoAccessor ContactInfoAccessor
                         IconFactoryAccessor EncryptionMethodAccessor)

public:
    PsiOtrPlugin();
    ~PsiOtrPlugin();

    QString name() const override;
    QWidget *options() override;
    bool enable() override;
    bool disable() override;
    void applyOptions() override;
    void restoreOptions() override;

    QString pluginInfo() override;
    void setEventCreatingHost(EventCreatingHost *host) override;
    void setOptionAccessingHost(OptionAccessingHost *host) override;
    void optionChanged(const QString &option) override;
    void setStanzaSendingHost(StanzaSendingHost *host) override;
    void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host) override;
    void setPsiAccountControllingHost(PsiAccountControllingHost *host) override;
    bool incomingStanza(int accountIndex, const QDomElement &xml) override;
    bool outgoingStanza(int accountIndex, QDomElement &xml) override;
    QList<QVariantHash> getButtonParam() override;
    QAction *getAction(QObject *parent, int accountIndex, const QString &contact) override;
    void setAccountInfoAccessingHost(AccountInfoAccessingHost *host) override;
    void setContactInfoAccessingHost(ContactInfoAccessingHost *host) override;
    void setIconFactoryAccessingHost(IconFactoryAccessingHost *host) override;
    void setEncryptionMethodAccessingHost(EncryptionMethodAccessingHost *host) override;

    /**
     * Applies native OTR processing to an incoming message DOM in place.
     * Protocol-only messages clear @p message; decrypted application messages
     * replace body/XHTML content and receive an EME marker when needed.
     */
    bool decryptMessageElement(int account, QDomElement &message, const QString &contact = QString());

    /**
     * Encrypts the message body for a resource-bound OTR conversation in place.
     * On encryption failure @p message is cleared so the caller cannot send the
     * original plaintext accidentally.
     */
    bool encryptMessageElement(int account, QDomElement &message, const QString &contact = QString());

    QString dataDir() override;
    void sendMessage(const QString &account, const QString &contact, const QString &message) override;
    void notifyUser(const QString &account,
                    const QString &contact,
                    const QString &message,
                    const OtrNotifyType &type) override;
    bool displayOtrMessage(const QString &account, const QString &contact, const QString &message) override;
    void stateChange(const QString &account, const QString &contact, OtrStateChange change) override;
    void receivedSMP(const QString &account, const QString &contact, const QString &question) override;
    void updateSMP(const QString &account, const QString &contact, int progress) override;
    QString humanAccount(const QString &accountId) override;
    QString humanAccountPublic(const QString &accountId) override;
    QString humanContact(const QString &accountId, const QString &contact) override;

    /** Displays a rich-text system message for the given account/contact pair. */
    bool appendSysMsg(const QString &account, const QString &contact, const QString &message, const QString &icon = "");

    /** Returns the Psi account index for @p accountId, or -1 when it is unknown. */
    int getAccountIndexById(const QString &accountId);

    /** Returns the configured human-readable account name. */
    QString getAccountNameById(const QString &accountId);

    /** Returns the public JID of the account identified by @p accountId. */
    QString getAccountJidById(const QString &accountId);

private slots:
    void eventActivated();

private:
    friend class OtrEncryptionProvider;

    /**
     * Returns a full JID for private/MUC contacts and a bare JID for ordinary
     * roster contacts. OTR itself remains resource-bound once a session starts.
     */
    QString getCorrectJid(int accountIndex, const QString &fullJid);

    bool m_enabled;
    OtrMessaging *m_otrConnection;
    QHash<QString, QHash<QString, PsiOtrClosure *>> m_onlineUsers;
    OptionAccessingHost *m_optionHost;
    StanzaSendingHost *m_senderHost;
    ApplicationInfoAccessingHost *m_applicationInfo;
    PsiAccountControllingHost *m_accountHost;
    AccountInfoAccessingHost *m_accountInfo;
    ContactInfoAccessingHost *m_contactInfo;
    IconFactoryAccessingHost *m_iconHost;
    EventCreatingHost *m_psiEvent;
    EncryptionMethodAccessingHost *m_encryptionHost;
    EncryptionMethodProvider *m_encryptionProvider;
    QSet<QString> m_otrDiscoveredResources;
    QQueue<QMessageBox *> m_messageBoxList;
};

} // namespace psiotr

#endif
