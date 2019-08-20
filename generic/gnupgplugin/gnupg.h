/*
 * gnupg.h - plugin main class
 *
 * Copyright (C) 2013  Ivan Romanov <drizt@land.ru>
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

#ifndef GNUPG_H
#define GNUPG_H

#include "accountinfoaccessor.h"
#include "activetabaccessor.h"
#include "applicationinfoaccessinghost.h"
#include "iconfactoryaccessor.h"
#include "optionaccessor.h"
#include "plugininfoprovider.h"
#include "psiaccountcontroller.h"
#include "psiplugin.h"
#include "stanzafilter.h"
#include "stanzasender.h"
#include "toolbariconaccessor.h"

class Options;
class QMenu;

class GnuPG:
        public AccountInfoAccessor,
        public ActiveTabAccessor,
        public IconFactoryAccessor,
        public OptionAccessor,
        public PluginInfoProvider,
        public PsiAccountController,
        public PsiPlugin,
        public QObject,
        public StanzaFilter,
        public StanzaSender,
        public ToolbarIconAccessor {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.psi-plus.GnuPG")
    Q_INTERFACES(PsiPlugin
                 PluginInfoProvider
                 StanzaFilter
                 PsiAccountController
                 OptionAccessor
                 ToolbarIconAccessor
                 IconFactoryAccessor
                 StanzaSender
                 ActiveTabAccessor
                 AccountInfoAccessor)

public:
    GnuPG();
    ~GnuPG();

    // from PsiPlugin
    QString name() const { return "GnuPG Key Manager"; }
    QString shortName() const { return "gnupg"; }
    QString version() const { return "0.3.9"; }

    QWidget *options();
    bool enable();
    bool disable();
    void applyOptions();
    void restoreOptions();
    QPixmap icon() const;

    // from PluginInfoProvider
    QString pluginInfo();

    // from StanzaSender
    void setStanzaSendingHost(StanzaSendingHost *host) { _stanzaSending = host; }

    // from StanzaFilter
    bool incomingStanza(int account, const QDomElement &stanza);
    bool outgoingStanza(int /*account*/, QDomElement &/*stanza*/) { return false; }

    // from PsiAccountController
    void setPsiAccountControllingHost(PsiAccountControllingHost *host) { _accountHost = host; }

    // from OptionAccessor
    void setOptionAccessingHost(OptionAccessingHost *host) { _optionHost = host; }
    void optionChanged(const QString &/*option*/) { }

    // from ToolbarIconAccessor
    QList<QVariantHash> getButtonParam();
    QAction* getAction(QObject * /*parent*/, int /*account*/, const QString &/*contact*/) { return nullptr; }

    // from IconFactoryAccessor
    void setIconFactoryAccessingHost(IconFactoryAccessingHost *host) { _iconFactory = host; }

    // from ActiveTabAccessor
    void setActiveTabAccessingHost(ActiveTabAccessingHost* host) { _activeTab = host; }

    // from AccountInfoAccessor
    void setAccountInfoAccessingHost(AccountInfoAccessingHost* host) { _accountInfo = host; }

private slots:
    void actionActivated();
    void sendPublicKey();

private:
    bool _enabled;
    Options *_optionsForm;
    PsiAccountControllingHost *_accountHost;
    OptionAccessingHost *_optionHost;
    IconFactoryAccessingHost *_iconFactory;
    QMenu *_menu;
    StanzaSendingHost *_stanzaSending;
    ActiveTabAccessingHost *_activeTab;
    AccountInfoAccessingHost *_accountInfo;
};

#endif // GNUPG_H
