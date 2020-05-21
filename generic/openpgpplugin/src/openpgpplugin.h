/*
 * openpgpplugin.h - plugin main class
 *
 * Copyright (C) 2013  Ivan Romanov <drizt@land.ru>
 * Copyright (C) 2020  Boris Pek <tehnick-8@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "accountinfoaccessor.h"
#include "activetabaccessor.h"
#include "applicationinfoaccessinghost.h"
#include "optionaccessor.h"
#include "plugininfoprovider.h"
#include "psiaccountcontroller.h"
#include "psiplugin.h"
#include "stanzafilter.h"
#include "stanzasender.h"
#include <QAction>

class OpenPgpMessaging;
class Options;
class QMenu;

class OpenPgpPlugin : public QObject,
                public PsiPlugin,
                public PluginInfoProvider,
                public StanzaFilter,
                public PsiAccountController,
                public OptionAccessor,
                public StanzaSender,
                public ActiveTabAccessor,
                public AccountInfoAccessor {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.psi-plus.OpenPGP" FILE "psiplugin.json")
    Q_INTERFACES(PsiPlugin PluginInfoProvider StanzaFilter PsiAccountController
                     OptionAccessor StanzaSender
                     ActiveTabAccessor AccountInfoAccessor)

public:
    OpenPgpPlugin();
    ~OpenPgpPlugin();

    // from PsiPlugin
    QString version() const override { return "1.5"; }
    QString shortName() const override { return "openpgp"; }
    QString name() const override { return "OpenPGP Plugin"; }

    QWidget *options() override;
    bool     enable() override;
    bool     disable() override;
    void     applyOptions() override;
    void     restoreOptions() override;
    QPixmap  icon() const override;

    // from PluginInfoProvider
    QString pluginInfo() override;

    // from StanzaSender
    void setStanzaSendingHost(StanzaSendingHost *host) override;

    // from StanzaFilter
    bool incomingStanza(int account, const QDomElement &stanza) override;
    bool outgoingStanza(int account, QDomElement &stanza) override;

    // from PsiAccountController
    void setPsiAccountControllingHost(PsiAccountControllingHost *host) override;

    // from OptionAccessor
    void setOptionAccessingHost(OptionAccessingHost *host) override;
    void optionChanged(const QString &option) override;

    // from ToolbarIconAccessor
    QList<QVariantHash> getButtonParam();
    QAction *           getAction(QObject *parent, int account, const QString &contact);

    // from ActiveTabAccessor
    void setActiveTabAccessingHost(ActiveTabAccessingHost *host) override;

    // from AccountInfoAccessor
    void setAccountInfoAccessingHost(AccountInfoAccessingHost *host) override;

private slots:
    void actionActivated();
    void sendPublicKey();
    void actionDestroyed(QObject *);
    void optionsDestroyed(QObject *);

private:
    bool isEnabled() const;

private:
    OpenPgpMessaging          *m_pgpMessaging  = nullptr;
    QAction *                  m_action        = nullptr;
    Options *                  m_optionsDialog = nullptr;
    PsiAccountControllingHost *m_accountHost   = nullptr;
    OptionAccessingHost *      m_optionHost    = nullptr;
    QMenu *                    m_menu          = nullptr;
    ActiveTabAccessingHost *   m_activeTab     = nullptr;
    AccountInfoAccessingHost * m_accountInfo   = nullptr;
};
