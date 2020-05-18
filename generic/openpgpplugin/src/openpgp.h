/*
 * openpgp.h - plugin main class
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

class Options;
class QMenu;

class OpenPGP : public QObject,
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
    Q_INTERFACES(PsiPlugin PluginInfoProvider StanzaFilter PsiAccountController OptionAccessor StanzaSender
                     ActiveTabAccessor AccountInfoAccessor)

public:
    OpenPGP();
    ~OpenPGP();

    // from PsiPlugin
    QString version() const { return "1.3"; }
    QString shortName() const { return "openpgp"; }
    QString name() const { return "OpenPGP Plugin"; }

    QWidget *options();
    bool     enable();
    bool     disable();
    void     applyOptions();
    void     restoreOptions();
    QPixmap  icon() const;

    // from PluginInfoProvider
    QString pluginInfo();

    // from StanzaSender
    void setStanzaSendingHost(StanzaSendingHost *host) { m_stanzaSending = host; }

    // from StanzaFilter
    bool incomingStanza(int account, const QDomElement &stanza);
    bool outgoingStanza(int /*account*/, QDomElement & /*stanza*/) { return false; }

    // from PsiAccountController
    void setPsiAccountControllingHost(PsiAccountControllingHost *host) { m_accountHost = host; }

    // from OptionAccessor
    void setOptionAccessingHost(OptionAccessingHost *host) { m_optionHost = host; }
    void optionChanged(const QString & /*option*/) { }

    // from ToolbarIconAccessor
    QList<QVariantHash> getButtonParam();
    QAction *           getAction(QObject *parent, int account, const QString &contact);

    // from ActiveTabAccessor
    void setActiveTabAccessingHost(ActiveTabAccessingHost *host) { m_activeTab = host; }

    // from AccountInfoAccessor
    void setAccountInfoAccessingHost(AccountInfoAccessingHost *host) { m_accountInfo = host; }

private slots:
    void actionActivated();
    void sendPublicKey();
    void actionDestroyed(QObject *action);

private:
    bool isEnabled() const;

private:
    QAction *                  m_action        = nullptr;
    Options *                  m_optionsForm   = nullptr;
    PsiAccountControllingHost *m_accountHost   = nullptr;
    OptionAccessingHost *      m_optionHost    = nullptr;
    QMenu *                    m_menu          = nullptr;
    StanzaSendingHost *        m_stanzaSending = nullptr;
    ActiveTabAccessingHost *   m_activeTab     = nullptr;
    AccountInfoAccessingHost * m_accountInfo   = nullptr;
};
