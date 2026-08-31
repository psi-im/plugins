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

    // PsiPlugin
    QString name() const override;
    QWidget *options() override;
    bool enable() override;
    bool disable() override;
    void applyOptions() override;
    void restoreOptions() override;

    // PluginInfoProvider
    QString pluginInfo() override;

    // EventCreator
    void setEventCreatingHost(EventCreatingHost *host) override;

    // OptionAccessor
    void setOptionAccessingHost(OptionAccessingHost *host) override;
    void optionChanged(const QString &option) override;

    // StanzaSender
    void setStanzaSendingHost(StanzaSendingHost *host) override;

    // ApplicationInfoAccessor
    void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host) override;

    // PsiAccountController
    void setPsiAccountControllingHost(PsiAccountControllingHost *host) override;

    // StanzaFilter
    bool incomingStanza(int accountIndex, const QDomElement &xml) override;
    bool outgoingStanza(int accountIndex, QDomElement &xml) override;

    // ToolbarIconAccessor
    QList<QVariantHash> getButtonParam() override;
    QAction *getAction(QObject *parent, int accountIndex, const QString &contact) override;

    // AccountInfoAccessor
    void setAccountInfoAccessingHost(AccountInfoAccessingHost *host) override;

    // ContactInfoAccessor
    void setContactInfoAccessingHost(ContactInfoAccessingHost *host) override;

    // IconFactoryAccessor
    void setIconFactoryAccessingHost(IconFactoryAccessingHost *host) override;

    // EncryptionMethodAccessor
    void setEncryptionMethodAccessingHost(EncryptionMethodAccessingHost *host) override;

    // Native EncryptionMethodProvider helpers
    /**
     * Applies native OTR processing to an incoming message DOM in place.
     * Protocol-only messages clear @p message. Decrypted application messages
     * replace unauthenticated stanza content with the authenticated OTR payload.
     * XEP-0364 peers are always rendered as plaintext; legacy peers may expose
     * their authenticated OTR payload as normalized XHTML for compatibility.
     */
    bool decryptMessageElement(int account, QDomElement &message, const QString &contact = QString());

    /**
     * Encrypts the message body for the exact OTR endpoint JID in place. On
     * encryption failure @p message is cleared so the caller cannot send the
     * original plaintext accidentally.
     */
    bool encryptMessageElement(int account, QDomElement &message, const QString &contact = QString());

    // OtrCallback
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

    // Plugin helpers
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

    bool isXep0364Peer(int accountIndex, const QString &contact) const;
    void markXep0364Peer(const QString &account, const QString &contact);

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
    QSet<QString> m_xep0364Resources;
    QQueue<QMessageBox *> m_messageBoxList;
};

} // namespace psiotr

#endif
