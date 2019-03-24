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

#include <QtGui>
#include <QtCore>
#include <QDomElement>

#include "ui_options.h"

#include "psiplugin.h"
#include "optionaccessor.h"
#include "optionaccessinghost.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"
#include "stanzafilter.h"
#include "plugininfoprovider.h"
#include "applicationinfoaccessor.h"
#include "applicationinfoaccessinghost.h"
#include "popupaccessor.h"
#include "popupaccessinghost.h"
#include "accountinfoaccessor.h"
#include "accountinfoaccessinghost.h"
#include "psiaccountcontroller.h"
#include "psiaccountcontrollinghost.h"
#include "contactinfoaccessor.h"
#include "contactinfoaccessinghost.h"
#include "iconfactoryaccessor.h"
#include "iconfactoryaccessinghost.h"

#include "accountsettings.h"

class ClientSwitcherPlugin: public QObject, public PsiPlugin, public OptionAccessor, public StanzaSender, public StanzaFilter, public PluginInfoProvider, public PopupAccessor, public ApplicationInfoAccessor, public AccountInfoAccessor, public PsiAccountController, public ContactInfoAccessor, public IconFactoryAccessor

{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.psi-plus.ClientSwitcherPlugin")
    Q_INTERFACES(PsiPlugin OptionAccessor StanzaSender StanzaFilter PluginInfoProvider PopupAccessor ApplicationInfoAccessor AccountInfoAccessor PsiAccountController ContactInfoAccessor IconFactoryAccessor)

public:
    ClientSwitcherPlugin();
    ~ClientSwitcherPlugin();
    virtual QString name() const;
    virtual QString shortName() const;
    virtual QString version() const;
    virtual bool enable();
    virtual bool disable();
    virtual QWidget* options();
    virtual void applyOptions();
    virtual void restoreOptions();
    virtual QPixmap icon() const;
    // OptionAccessor
    virtual void setOptionAccessingHost(OptionAccessingHost*);
    virtual void optionChanged(const QString&);
    // StanzaSender
    virtual void setStanzaSendingHost(StanzaSendingHost*);
    // StanzaFilter
    virtual bool incomingStanza(int, const QDomElement&);
    virtual bool outgoingStanza(int, QDomElement&);
    // EventFilter
    virtual QString pluginInfo();
    // PopupAccessor
    virtual void setPopupAccessingHost(PopupAccessingHost*);
    // ApplicationInfoAccessor
    virtual void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost*);
    // AccountInfoAccessing
    virtual void setAccountInfoAccessingHost(AccountInfoAccessingHost*);
    // PsiAccountController
    virtual void setPsiAccountControllingHost(PsiAccountControllingHost*);
    // ContactInfoAccessor
    virtual void setContactInfoAccessingHost(ContactInfoAccessingHost*);
    // IconFactoryAccessor
    virtual void setIconFactoryAccessingHost(IconFactoryAccessingHost* host);

private:
    struct OsStruct {
        QString name;
        OsStruct(const QString &n) : name(n) {}
    };

    struct ClientStruct {
        QString name;
        QString version;
        QString caps_node;
        QString caps_version;
        ClientStruct(const QString &n, const QString &v, const QString &cn, const QString &cv) : name(n), version(v), caps_node(cn), caps_version(cv) {}
    };
    Ui::OptionsWidget ui_options;
    StanzaSendingHost* sender_;
    OptionAccessingHost* psiOptions;
    PopupAccessingHost* psiPopup;
    ApplicationInfoAccessingHost* psiInfo;
    AccountInfoAccessingHost* psiAccount;
    PsiAccountControllingHost* psiAccountCtl;
    ContactInfoAccessingHost* psiContactInfo;
    IconFactoryAccessingHost* psiIcon;
    //--
    bool enabled;
    bool for_all_acc;
    QList<AccountSettings*> settingsList;
    //--
    QString def_os_name;
    QString def_client_name;
    QString def_client_version;
    QString def_caps_node;
    QString def_caps_version;
    QList<struct OsStruct> os_presets;
    QList<struct ClientStruct> client_presets;
    QString logsDir;
    int heightLogsView;
    int widthLogsView;
    QString lastLogItem;
    int popupId;
    //--

    int getOsTemplateIndex(const QString &);
    int getClientTemplateIndex(const QString&, const QString&,
                               const QString&, const QString&);
    int getAccountById(const QString&);
    AccountSettings* getAccountSetting(const QString&);
    bool isSkipStanza(AccountSettings*, const int, const QString &);
    QString jidToNick(int account, const QString &jid);
    void showPopup(const QString &nick);
    void showLog(const QString &filename);
    void saveToLog(const int, const QString &, const QString &);

private slots:
    void enableAccountsList(int);
    void restoreOptionsAcc(int);
    void enableMainParams(int);
    void enableOsParams(int);
    void enableClientParams(int);
    void setNewCaps(int);
    void viewFromOpt();
    void onCloseView(int, int);

};

#endif
