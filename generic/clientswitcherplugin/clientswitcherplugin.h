/*
 * clientswitcherplugin.h - Client Switcher plugin
 * Copyright (C) 2010  Aleksey Andreev
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * You can also redistribute and/or modify this program under the
 * terms of the Psi License, specified in the accompanied COPYING
 * file, as published by the Psi Project; either dated January 1st,
 * 2005, or (at your option) any later version.
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

#ifndef PLUGIN_MAIN_H
#define PLUGIN_MAIN_H

#include <QDomElement>
#include <QtCore>
#include <QtGui>

#include "ui_options.h"

#include "accountinfoaccessinghost.h"
#include "accountinfoaccessor.h"
#include "applicationinfoaccessinghost.h"
#include "applicationinfoaccessor.h"
#include "contactinfoaccessinghost.h"
#include "contactinfoaccessor.h"
#include "iconfactoryaccessinghost.h"
#include "iconfactoryaccessor.h"
#include "optionaccessinghost.h"
#include "optionaccessor.h"
#include "plugininfoprovider.h"
#include "psiaccountcontroller.h"
#include "psiaccountcontrollinghost.h"
#include "psiplugin.h"
#include "stanzafilter.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"

#include "accountsettings.h"

class ClientSwitcherPlugin : public QObject,
                             public PsiPlugin,
                             public OptionAccessor,
                             public StanzaFilter,
                             public PluginInfoProvider,
                             public ApplicationInfoAccessor,
                             public AccountInfoAccessor,
                             public PsiAccountController

{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.psi-plus.ClientSwitcherPlugin" FILE "psiplugin.json")
    Q_INTERFACES(PsiPlugin OptionAccessor StanzaFilter PluginInfoProvider ApplicationInfoAccessor AccountInfoAccessor
                     PsiAccountController)

public:
    ClientSwitcherPlugin();
    ~ClientSwitcherPlugin();
    virtual QString  name() const;
    virtual bool     enable();
    virtual bool     disable();
    virtual QWidget *options();
    virtual void     applyOptions();
    virtual void     restoreOptions();
    // OptionAccessor
    virtual void setOptionAccessingHost(OptionAccessingHost *);
    virtual void optionChanged(const QString &);
    // StanzaFilter
    virtual bool incomingStanza(int, const QDomElement &);
    virtual bool outgoingStanza(int, QDomElement &);
    // EventFilter
    virtual QString pluginInfo();
    // ApplicationInfoAccessor
    virtual void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *);
    // AccountInfoAccessing
    virtual void setAccountInfoAccessingHost(AccountInfoAccessingHost *);
    // PsiAccountController
    virtual void setPsiAccountControllingHost(PsiAccountControllingHost *);

private:
    struct OsStruct {
        QString name;
        QString version;
        OsStruct(const QString &n, const QString &v = QString()) : name(n), version(v) { }
    };

    struct ClientStruct {
        QString name;
        QString version;
        QString caps_node;
        ClientStruct(const QString &n, const QString &v, const QString &cn) : name(n), version(v), caps_node(cn) { }
    };

    Ui::OptionsWidget             ui_options;
    OptionAccessingHost *         psiOptions;
    ApplicationInfoAccessingHost *psiInfo;
    AccountInfoAccessingHost *    psiAccount;
    PsiAccountControllingHost *   psiAccountCtl;
    //--
    bool                     enabled;
    bool                     for_all_acc;
    QList<AccountSettings *> settingsList;
    //--
    QString                    def_os_name;
    QString                    def_os_version;
    QString                    def_client_name;
    QString                    def_client_version;
    QString                    def_caps_node;
    QString                    def_caps_version;
    QList<struct OsStruct>     os_presets;
    QList<struct ClientStruct> client_presets;

    int              getOsTemplateIndex(const QString &, const QString &os_version);
    int              getClientTemplateIndex(const QString &, const QString &, const QString &);
    int              getAccountById(const QString &);
    AccountSettings *getAccountSetting(const QString &);

private:
    bool updateInfo(int account);

private slots:
    void enableAccountsList(int);
    void restoreOptionsAcc(int);
    void enableMainParams(int);
    void enableOsParams(int);
    void enableClientParams(int);
    void setNewCaps(int);
};

#endif
