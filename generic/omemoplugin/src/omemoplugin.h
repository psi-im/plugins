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
#include "optionaccessinghost.h"
#include "optionaccessor.h"
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
                    public OptionAccessor,
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
    Q_PLUGIN_METADATA(IID "com.psi.OmemoPlugin" FILE "psiplugin.json")
    Q_INTERFACES(PsiPlugin StanzaFilter StanzaSender EventCreator AccountInfoAccessor ApplicationInfoAccessor
                     PsiAccountController PluginInfoProvider ToolbarIconAccessor GCToolbarIconAccessor EncryptionSupport
                         OptionAccessor CommandExecutor ContactInfoAccessor)
public:
    OMEMOPlugin() = default;

    // PsiPlugin interface
    QString     name() const override;
    QWidget    *options() override;
    bool        enable() override;
    bool        disable() override;
    void        applyOptions() override;
    void        restoreOptions() override;
    QString     pluginInfo() override;
    QStringList pluginFeatures() override;

    // StanzaFilter interface
    bool incomingStanza(int account, const QDomElement &xml) override;
    bool outgoingStanza(int account, QDomElement &xml) override;

    // EncryptionSupport interface
    bool decryptMessageElement(int account, QDomElement &message) override;
    bool encryptMessageElement(int account, QDomElement &message) override;

    // ApplicationInfoAccessor interface
    void setAccountInfoAccessingHost(AccountInfoAccessingHost *host) override;

    // ApplicationInfoAccessor interface
    void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host) override;

    // StanzaSender interface
    void setStanzaSendingHost(StanzaSendingHost *host) override;

    // EventCreator interface
    void setEventCreatingHost(EventCreatingHost *host) override;

    // PsiAccountController interface
    void setPsiAccountControllingHost(PsiAccountControllingHost *host) override;

    // ContactInfoAccessor interface
    void setContactInfoAccessingHost(ContactInfoAccessingHost *host) override;

    // OptionAccessor interface
    void setOptionAccessingHost(OptionAccessingHost *host) override;
    void optionChanged(const QString &option) override;

    // ToolbarIconAccessor interface
    QList<QVariantHash> getButtonParam() override;
    QAction            *getAction(QObject *parent, int account, const QString &contact) override;

    // GCToolbarIconAccessor interface
    QList<QVariantHash> getGCButtonParam() override;
    QAction            *getGCAction(QObject *parent, int account, const QString &contact) override;

    // CommandExecutor interface
    bool execute(int account, const QHash<QString, QVariant> &args, QHash<QString, QVariant> *result) override;

signals:
    void applyPluginSettings();

private slots:
    void savePluginOptions();
    void enableOMEMOAction(bool);
    void fileDownloadFinished();
    void actionDestroyed(QObject *action);

private:
    QPixmap  getIcon() const;
    QAction *createAction(QObject *parent, int account, const QString &contact, bool isGroup);
    void     updateAction(int account, const QString &user);
    void     processEncryptedFile(int account, QDomElement &xml);
    void     showOwnFingerprint(int account, const QString &jid);
    void     logMuc(QString room, const QString &from, const QString &myJid, const QString &text, QString stamp);

private:
    bool                          m_enabled = false;
    QMultiMap<QString, QAction *> m_actions;
    std::shared_ptr<Crypto>       m_crypto;
    std::unique_ptr<OMEMO>        m_omemo;
    QNetworkAccessManager         m_networkManager;

    AccountInfoAccessingHost     *m_accountInfo       = nullptr;
    ContactInfoAccessingHost     *m_contactInfo       = nullptr;
    ApplicationInfoAccessingHost *m_applicationInfo   = nullptr;
    StanzaSendingHost            *m_stanzaSender      = nullptr;
    EventCreatingHost            *m_eventCreator      = nullptr;
    PsiAccountControllingHost    *m_accountController = nullptr;
    OptionAccessingHost          *m_optionHost        = nullptr;
};
}
#endif // PSIOMEMO_OMEMOPLUGIN_H
