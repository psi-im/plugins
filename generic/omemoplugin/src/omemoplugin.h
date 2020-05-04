/*
 * OMEMO Plugin for Psi
 * Copyright (C) 2018 Vyacheslav Karpukhin
 * Copyright (C) 2020 Boris Pek
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

#ifndef PSIOMEMO_OMEMOPLUGIN_H
#define PSIOMEMO_OMEMOPLUGIN_H

#include <QObject>
#include <QtNetwork/QtNetwork>

#include "accountinfoaccessor.h"
#include "applicationinfoaccessor.h"
#include "commandexecutor.h"
#include "contactinfoaccessor.h"
#include "crypto.h"
#include "encryptionsupport.h"
#include "eventcreator.h"
#include "gctoolbariconaccessor.h"
#include "omemo.h"
#include "plugininfoprovider.h"
#include "psiaccountcontroller.h"
#include "psiplugin.h"
#include "stanzafilter.h"
#include "stanzasender.h"
#include "storage.h"
#include "toolbariconaccessor.h"

namespace psiomemo {
class OMEMOPlugin : public QObject,
                    public PsiPlugin,
                    public StanzaFilter,
                    public StanzaSender,
                    public EventCreator,
                    public AccountInfoAccessor,
                    public ApplicationInfoAccessor,
                    public PsiAccountController,
                    public PluginInfoProvider,
                    public ToolbarIconAccessor,
                    public GCToolbarIconAccessor,
                    public EncryptionSupport,
                    public CommandExecutor,
                    public ContactInfoAccessor {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.psi.OmemoPlugin")
    Q_INTERFACES(PsiPlugin StanzaFilter StanzaSender EventCreator AccountInfoAccessor ApplicationInfoAccessor
                     PsiAccountController PluginInfoProvider ToolbarIconAccessor GCToolbarIconAccessor EncryptionSupport
                         CommandExecutor ContactInfoAccessor)
public:
    QString  name() const override;
    QString  shortName() const override;
    QString  version() const override;
    QWidget *options() override;
    bool     enable() override;
    bool     disable() override;
    QPixmap  icon() const override;
    void     applyOptions() override;
    void     restoreOptions() override;

    QString pluginInfo() override;

    bool        incomingStanza(int account, const QDomElement &xml) override;
    bool        outgoingStanza(int account, QDomElement &xml) override;
    bool        decryptMessageElement(int account, QDomElement &message) override;
    bool        encryptMessageElement(int account, QDomElement &message) override;
    void        setAccountInfoAccessingHost(AccountInfoAccessingHost *host) override;
    void        setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host) override;
    void        setStanzaSendingHost(StanzaSendingHost *host) override;
    void        setEventCreatingHost(EventCreatingHost *host) override;
    void        setPsiAccountControllingHost(PsiAccountControllingHost *host) override;
    void        setContactInfoAccessingHost(ContactInfoAccessingHost *host) override;
    QStringList pluginFeatures() override;

    QList<QVariantHash> getButtonParam() override;
    QAction *           getAction(QObject *parent, int account, const QString &contact) override;
    QList<QVariantHash> getGCButtonParam() override;
    QAction *           getGCAction(QObject *parent, int account, const QString &contact) override;

    bool execute(int account, const QHash<QString, QVariant> &args, QHash<QString, QVariant> *result) override;

private:
    bool                          m_enabled;
    QMultiMap<QString, QAction *> m_actions;
    OMEMO                         m_omemo;
    QNetworkAccessManager         m_networkManager;

    AccountInfoAccessingHost *    m_accountInfo;
    ContactInfoAccessingHost *    m_contactInfo;
    ApplicationInfoAccessingHost *m_applicationInfo;
    EventCreatingHost *           m_eventCreator;

    QPixmap  getIcon() const;
    QAction *createAction(QObject *parent, int account, const QString &contact, bool isGroup);
    void     updateAction(int account, const QString &user);
    void     processEncryptedFile(int account, QDomElement &xml);
    void     showOwnFingerprint(int account, const QString &jid);

private slots:
    void onEnableOMEMOAction(bool);
    void onFileDownloadFinished();
    void onActionDestroyed(QObject *action);
};
}
#endif // PSIOMEMO_OMEMOPLUGIN_H
