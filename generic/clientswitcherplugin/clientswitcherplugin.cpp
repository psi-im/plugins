/*
 * clientswitcherplugin.cpp - Client Switcher plugin
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

//#include <QtCore>
//#include <QDomDocument>
#include <QMetaObject>

#include "clientswitcherplugin.h"

#define constPluginShortName "clientswitcher"
#define constPluginName "Client Switcher Plugin"
#define constForAllAcc "for_all_acc"
#define constAccSettingList "accsettlist"

ClientSwitcherPlugin::ClientSwitcherPlugin() :
    psiOptions(nullptr), psiInfo(nullptr), psiAccount(nullptr), psiAccountCtl(nullptr), enabled(false),
    for_all_acc(false), def_os_name(""), def_client_name(""), def_client_version(""), def_caps_node(""),
    def_caps_version("")

{
    settingsList.clear();
    os_presets.clear();
    client_presets.clear();
}

ClientSwitcherPlugin::~ClientSwitcherPlugin()
{
    // disable();
}

QString ClientSwitcherPlugin::name() const { return constPluginName; }

bool ClientSwitcherPlugin::enable()
{
    if (!psiOptions)
        return false;
    enabled = true;

    os_presets = { { "Windows", "95" },
                   { "Windows", "98" },
                   { "Windows", "ME" },
                   { "Windows", "2000" },
                   { "Windows", "XP" },
                   { "Windows", "2003" },
                   { "Windows", "2008" },
                   { "Windows", "Vista" },
                   { "Windows", "7" },
                   { "Windows", "10" },
                   { "Arch" },
                   { "Debian GNU/Linux", "6.0.1 (squeeze)" },
                   { "Ubuntu", "10.04.2 LTS" },
                   { "RFRemix", "14.1 (Laughlin)" },
                   { "openSUSE", "11.4" },
                   { "Gentoo Base System", "2.0.3" },
                   { "Mac OS X" },
                   { "Mac OS X", "10.6" },
                   { "Android OS", "2.3.6 (build XXLB1)" },
                   { "Plan9" },
                   { "Solaris" },
                   { "FreeBSD" },
                   { "NetBSD" },
                   { "OpenBSD" },
                   { "Nokia5130c-2", "07.91" },
                   { "SonyEricssonW580i", "R8BE001" } };

    client_presets.clear();
    client_presets << ClientStruct("Bombus", "0.7.1429M-Zlib", "http://bombus-im.org/java");
    client_presets << ClientStruct("Gajim", "0.12.5", "https://gajim.org");
    client_presets << ClientStruct("Mcabber", "0.9.10", "http://mcabber.com/caps");
    client_presets << ClientStruct("Miranda", "0.9.16.0", "http://miranda-im.org/caps");
    client_presets << ClientStruct("Pidgin", "2.8.0", "http://pidgin.im");
    client_presets << ClientStruct("Psi", "0.14", "https://psi-im.org/caps");
    client_presets << ClientStruct("QIP Infium", "9034", "http://qip.ru/caps");
    client_presets << ClientStruct("qutIM", "0.2", "http://qutim.org");
    client_presets << ClientStruct("Swift", "1.0", "http://swift.im");
    client_presets << ClientStruct("Talisman", "0.1.1.15", "http://jabrvista.net.ru");
    client_presets << ClientStruct("Tkabber", "0.11.1", "http://tkabber.jabber.ru");
    client_presets << ClientStruct("Talkonaut", "1.0.0.84", "http://www.google.com/xmpp/client/caps");
    client_presets << ClientStruct(QString::fromUtf8("Я.Онлайн"), "3.2.0.8873", "yandex-webchat");
    // Флаг "для всех аккаунтов"
    for_all_acc = psiOptions->getPluginOption(constForAllAcc, QVariant(false)).toBool();
    // Получаем настройки аккаунтов
    QStringList sett_list = psiOptions->getPluginOption(constAccSettingList, QVariant()).toStringList();
    // Формируем структуры
    int cnt = sett_list.size();
    for (int i = 0; i < cnt; i++) {
        AccountSettings *as = new AccountSettings(sett_list.at(i));
        if (as) {
            if (as->isValid())
                settingsList.push_back(as);
            else
                delete as;
        }
    }

    return true;
}

bool ClientSwitcherPlugin::disable()
{
    if (!enabled)
        return true;

    while (settingsList.size() != 0) {
        AccountSettings *as = settingsList.takeLast();
        if (as)
            delete as;
    }

    for (int i = 0;; i++) {
        QString id = psiAccount->getId(i);
        if (id == "-1")
            break;
        psiAccountCtl->setClientVersionInfo(i, {});
    }

    enabled = false;
    return true;
}

QWidget *ClientSwitcherPlugin::options()
{
    if (!enabled) {
        return nullptr;
    }
    QWidget *optionsWid = new QWidget();
    ui_options.setupUi(optionsWid);
    // Заполняем виджет пресетов ОС
    ui_options.cb_ospreset->addItem("default", "default");
    ui_options.cb_ospreset->addItem("user defined", "user");
    int cnt = os_presets.size();
    for (int i = 0; i < cnt; i++) {
        ui_options.cb_ospreset->addItem(QString("%1 %2").arg(os_presets.at(i).name, os_presets.at(i).version));
    }
    // Заполняем виджет пресетов клиента
    ui_options.cb_clientpreset->addItem("default", "default");
    ui_options.cb_clientpreset->addItem("user defined", "user");
    cnt = client_presets.size();
    for (int i = 0; i < cnt; i++) {
        ui_options.cb_clientpreset->addItem(client_presets.at(i).name);
    }

    //--
    connect(ui_options.cb_allaccounts, &QCheckBox::stateChanged, this, &ClientSwitcherPlugin::enableAccountsList);
    // TODO: update after stopping support of Ubuntu Xenial:
    connect(ui_options.cb_accounts, SIGNAL(currentIndexChanged(int)), this, SLOT(restoreOptionsAcc(int)));
    connect(ui_options.cmb_lockrequ, SIGNAL(currentIndexChanged(int)), this, SLOT(enableMainParams(int)));
    connect(ui_options.cb_ospreset, SIGNAL(currentIndexChanged(int)), this, SLOT(enableOsParams(int)));
    connect(ui_options.cb_clientpreset, SIGNAL(currentIndexChanged(int)), this, SLOT(enableClientParams(int)));
    restoreOptions();

    return optionsWid;
}

void ClientSwitcherPlugin::applyOptions()
{
    // Аккаунт
    bool for_all_acc_old = for_all_acc;
    for_all_acc          = ui_options.cb_allaccounts->isChecked();
    bool caps_updated    = (for_all_acc != for_all_acc_old);

    int acc_index = ui_options.cb_accounts->currentIndex();
    if (acc_index == -1 && !for_all_acc)
        return;
    QString acc_id = "all";
    if (!for_all_acc)
        acc_id = ui_options.cb_accounts->itemData(acc_index).toString();
    AccountSettings *as = getAccountSetting(acc_id);
    if (!as) {
        as             = new AccountSettings();
        as->account_id = acc_id;
        settingsList.push_back(as);
    }

    // Блокировка запроса версии
    int respMode = ui_options.cmb_lockrequ->currentIndex();
    if (as->response_mode != respMode) {
        caps_updated      = (as->response_mode == AccountSettings::RespAllow || respMode == AccountSettings::RespAllow);
        as->response_mode = respMode;
    }
    // Блокировка запроса времени
    bool tmp_flag = ui_options.cb_locktimerequ->isChecked();
    if (as->lock_time_requ != tmp_flag) {
        as->lock_time_requ = tmp_flag;
        caps_updated       = true;
    }
    // Наименование ОС
    if (ui_options.cb_ospreset->currentIndex() == 0) {
        caps_updated   = !(as->os_name.isEmpty() && as->os_version.isEmpty());
        as->os_name    = QString();
        as->os_version = QString();
    } else {
        auto name      = ui_options.le_osname->text().trimmed();
        auto version   = ui_options.le_osversion->text().trimmed();
        caps_updated   = !(name == as->os_name && version == as->os_version);
        as->os_name    = name;
        as->os_version = version;
    }
    // Клиент
    if (ui_options.cb_clientpreset->currentIndex() == 0) {
        as->client_name    = QString();
        as->client_version = QString();
        if (!as->caps_node.isEmpty()) {
            as->caps_node = QString();
            caps_updated  = true;
        }
    } else {
        // Название клиента
        as->client_name = ui_options.le_clientname->text().trimmed();
        // Версия клиента
        as->client_version = ui_options.le_clientversion->text().trimmed();
        // Caps node клиента
        QString str1 = ui_options.le_capsnode->text().trimmed();
        if (as->caps_node != str1) {
            as->caps_node = str1;
            caps_updated  = true;
        }
    }
    // Сохраняем опции
    psiOptions->setPluginOption(constForAllAcc, QVariant(for_all_acc));
    QStringList sett_list;
    int         cnt = settingsList.size();
    for (int i = 0; i < cnt; i++) {
        AccountSettings *as = settingsList.at(i);
        if (as->isValid() && !as->isEmpty()) {
            QString acc_id = as->account_id;
            if ((!for_all_acc && acc_id != "all") || (for_all_acc && acc_id == "all"))
                sett_list.push_back(as->toString());
        }
    }
    psiOptions->setPluginOption(constAccSettingList, QVariant(sett_list));
    // Если капсы изменились, отсылаем новые пресентсы
    if (caps_updated) {
        if (!for_all_acc && !for_all_acc_old) {
            int acc = getAccountById(acc_id);
            if (acc != -1)
                QMetaObject::invokeMethod(this, "setNewCaps", Qt::QueuedConnection, Q_ARG(int, acc));
        } else {
            // Отправляем новые капсы всем активным аккам
            QMetaObject::invokeMethod(this, "setNewCaps", Qt::QueuedConnection, Q_ARG(int, -1));
        }
    }
}

void ClientSwitcherPlugin::restoreOptions()
{
    // Заполняем флаг "для всех аккаунтов"
    ui_options.cb_allaccounts->setChecked(for_all_acc);
    // Заполняем виджет аккаунтов
    ui_options.cb_accounts->clear();
    if (!psiAccount)
        return;
    int cnt = 0;
    for (int i = 0;; i++) {
        QString id = psiAccount->getId(i);
        if (id == "-1")
            break;
        if (!id.isEmpty()) {
            QString name = psiAccount->getName(i);
            if (name.isEmpty())
                name = "?";
            ui_options.cb_accounts->addItem(QString("%1 (%2)").arg(name, psiAccount->getJid(i)), QVariant(id));
            cnt++;
        }
    }
    int acc_idx = -1;
    if (cnt > 0) {
        if (!for_all_acc)
            acc_idx = 0;
    }
    ui_options.cb_accounts->setCurrentIndex(acc_idx);
    //--
    restoreOptionsAcc(acc_idx);
}

//-- OptionAccessor -------------------------------------------------

void ClientSwitcherPlugin::setOptionAccessingHost(OptionAccessingHost *host) { psiOptions = host; }

void ClientSwitcherPlugin::optionChanged(const QString & /*option*/) { }

// ----------------------- StanzaFilter ------------------------------
bool ClientSwitcherPlugin::incomingStanza(int, const QDomElement &) { return false; }

bool ClientSwitcherPlugin::outgoingStanza(int account, QDomElement &stanza)
{
    if (!enabled)
        return false;
    QString          acc_id = (for_all_acc) ? "all" : psiAccount->getId(account);
    AccountSettings *as     = getAccountSetting(acc_id);
    if (!as)
        return false;

    if (stanza.tagName() == "iq" && stanza.attribute("type") == "result") {
        // QString     s_to = stanza.attribute("to");
        QStringList send_ver_list;
        auto        s_child = stanza.firstChildElement();
        while (!s_child.isNull()) {
            if (s_child.tagName() == "time") {
                QString xmlns_str = s_child.namespaceURI();
                if (xmlns_str == "urn:xmpp:time") {
                    if (as->lock_time_requ) {
                        QDomDocument xmldoc = stanza.ownerDocument();
                        // Отклонение запроса, как будто не реализовано
                        stanza.setAttribute("type", "error");
                        QDomNode q_child = s_child.firstChild();
                        while (!q_child.isNull()) {
                            s_child.removeChild(q_child);
                            q_child = s_child.firstChild();
                        }
                        QDomElement err = xmldoc.createElement("error");
                        err.setAttribute("type", "cancel");
                        err.setAttribute("code", "501");
                        stanza.appendChild(err);
                        QDomElement not_imp
                            = xmldoc.createElementNS("urn:ietf:params:xml:ns:xmpp-stanzas", "feature-not-implemented");
                        err.appendChild(not_imp);
                    }
                }
            }
            s_child = s_child.nextSiblingElement();
        }
    }
    return false;
}

QString ClientSwitcherPlugin::pluginInfo()
{
    return tr("The plugin is intended for substitution of the client version, his name and operating system type.\n"
              "You can specify the version of the client and OS or to select them from the preset list.\n");
}

// ----------------------- ApplicationInfoAccessor ------------------------------
void ClientSwitcherPlugin::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host)
{
    psiInfo = host;
    if (psiInfo) {
        // def_os_name = ;
        def_client_name    = psiInfo->appName();
        def_client_version = psiInfo->appVersion();
        def_caps_node      = psiInfo->appCapsNode();
        def_caps_version   = psiInfo->appCapsVersion();
        def_os_name        = psiInfo->appOsName();
        def_os_version     = psiInfo->appOsVersion();
    }
}

// ----------------------- AccountInfoAccessing ------------------------------
void ClientSwitcherPlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost *host) { psiAccount = host; }

// ----------------------- PsiAccountController ----------------------------------
void ClientSwitcherPlugin::setPsiAccountControllingHost(PsiAccountControllingHost *host)
{
    psiAccountCtl = host;

    psiAccountCtl->subscribeBeforeLogin(this, [this](int accountId) { updateInfo(accountId); });
}

// ----------------------- Private ------------------------------

int ClientSwitcherPlugin::getOsTemplateIndex(const QString &os_name, const QString &os_version)
{
    if (os_name.isEmpty())
        return 0; // default
    int cnt = os_presets.size();
    for (int i = 0; i < cnt; i++) {
        if (os_name == os_presets.at(i).name && os_version == os_presets.at(i).version) {
            i += 2; // Т.к. впереди default и user defined
            return i;
        }
    }
    return 1; // user defined
}

int ClientSwitcherPlugin::getClientTemplateIndex(const QString &cl_name, const QString &cl_ver, const QString &cp_node)
{
    if (cl_name.isEmpty() && cl_ver.isEmpty() && cp_node.isEmpty())
        return 0; // default
    int cnt = client_presets.size();
    for (int i = 0; i < cnt; i++) {
        if (cl_name == client_presets.at(i).name && cl_ver == client_presets.at(i).version) {
            if (cp_node == client_presets.at(i).caps_node) {
                i += 2; // есть еще default и user defined
                return i;
            }
        }
    }
    return 1; // user defined
}

int ClientSwitcherPlugin::getAccountById(const QString &acc_id)
{
    if (!psiAccount || acc_id.isEmpty())
        return -1;
    for (int i = 0;; i++) {
        QString id = psiAccount->getId(i);
        if (id == "-1")
            break;
        if (id == acc_id)
            return i;
    }
    return -1;
}

AccountSettings *ClientSwitcherPlugin::getAccountSetting(const QString &acc_id)
{
    int cnt = settingsList.size();
    for (int i = 0; i < cnt; i++) {
        AccountSettings *as = settingsList.at(i);
        if (as && as->account_id == acc_id)
            return as;
    }
    return nullptr;
}

ClientSwitcherPlugin::UpdateStatus ClientSwitcherPlugin::updateInfo(int account)
{
    if (!enabled || !psiAccount || !psiAccountCtl)
        return UpdateStatus::Disabled;

    QString acc_id = psiAccount->getId(account);
    if (acc_id == "-1" || acc_id.isEmpty())
        return UpdateStatus::NoAccount;

    AccountSettings *as = getAccountSetting(acc_id);
    if (!as || !as->isValid())
        return UpdateStatus::NoSettings;

    QVariantMap info = { { "os-name", as->os_name },
                         { "os-version", as->os_version },
                         { "client-name", as->client_name },
                         { "client-version", as->client_version },
                         { "caps-node", as->caps_node } };
    psiAccountCtl->setClientVersionInfo(account, info);
    return UpdateStatus::Ok;
}

/**
 * Отсылка пресентса с новыми капсами от имени указанного аккаунта
 * Если account == -1, то капсы посылаются всем активным аккаунтам
 */
void ClientSwitcherPlugin::setNewCaps(int account)
{
    if (!enabled || !psiAccount || !psiAccountCtl)
        return;
    int acc = account;
    if (acc == -1)
        acc = 0;
    for (;; acc++) {
        auto ret = updateInfo(acc);
        if (ret == UpdateStatus::NoAccount)
            break;
        if (ret == UpdateStatus::Ok) {
            QString acc_status = psiAccount->getStatus(acc);
            if (!acc_status.isEmpty() && acc_status != "offline" && acc_status != "invisible") {
                // Отсылаем тот же статус, а капсы заменим в outgoingStanza()
                psiAccountCtl->setStatus(acc, acc_status, psiAccount->getStatusMessage(acc));
            }
        }
        if (account != -1)
            break;
    }
}

// ----------------------- Slots ------------------------------
void ClientSwitcherPlugin::enableAccountsList(int all_acc_mode)
{
    bool all_acc_flag = (all_acc_mode == Qt::Checked) ? true : false;
    ui_options.cb_accounts->setEnabled(!all_acc_flag);
    ui_options.cb_accounts->setCurrentIndex(-1);
    restoreOptionsAcc(-1);
}

void ClientSwitcherPlugin::restoreOptionsAcc(int acc_index)
{
    // Блокировка виджета акка, при установленной галке "для всех акков"
    bool all_acc_flag = ui_options.cb_allaccounts->isChecked();
    ui_options.cb_accounts->setEnabled(!all_acc_flag);
    //--
    if (all_acc_flag || acc_index >= 0) {
        QString acc_id;
        if (!all_acc_flag) {
            acc_id = ui_options.cb_accounts->itemData(acc_index).toString();
        } else {
            acc_id = "all";
        }
        if (!acc_id.isEmpty()) {
            AccountSettings *as = getAccountSetting(acc_id);
            if (!as) {
                as = new AccountSettings(); // Сразу заполняется дефолтными настройками
                as->account_id = acc_id;
                settingsList.push_back(as);
            }
            // Блокировка запроса версии
            ui_options.cmb_lockrequ->setCurrentIndex(as->response_mode);
            // Блокировка запроса времени
            ui_options.cb_locktimerequ->setChecked(as->lock_time_requ);
            // Виджет шаблона ОС
            QString os_name    = as->os_name;
            QString os_version = as->os_version;
            int     os_templ   = getOsTemplateIndex(os_name, os_version);
            ui_options.cb_ospreset->setCurrentIndex(os_templ);
            // Название ОС
            ui_options.le_osname->setText(os_name);
            ui_options.le_osversion->setText(os_version);
            // Виджет шаблона клиента
            QString cl_name  = as->client_name;
            QString cl_ver   = as->client_version;
            QString cp_node  = as->caps_node;
            int     cl_templ = getClientTemplateIndex(cl_name, cl_ver, cp_node);
            ui_options.cb_clientpreset->setCurrentIndex(cl_templ);
            // Название клиента
            ui_options.le_clientname->setText(cl_name);
            // Версия клиента
            ui_options.le_clientversion->setText(cl_ver);
            // Caps node клиента
            ui_options.le_capsnode->setText(cp_node);
            // Блокировка/снятие блокировки виджетов
            ui_options.cmb_lockrequ->setEnabled(true);
            enableMainParams(as->response_mode);
            enableOsParams(os_templ);
            enableClientParams(cl_templ);
            return;
        }
    }
    // Блокировка виджетов при отсутствии валидного акка
    ui_options.cmb_lockrequ->setCurrentIndex(AccountSettings::RespAllow);
    ui_options.cmb_lockrequ->setEnabled(false);
    ui_options.cb_ospreset->setCurrentIndex(0);
    ui_options.gb_os->setEnabled(false);
    ui_options.cb_clientpreset->setCurrentIndex(0);
    ui_options.gb_client->setEnabled(false);
    enableOsParams(0);
    enableClientParams(0);
}

void ClientSwitcherPlugin::enableMainParams(int lock_mode)
{
    bool enableFlag = (lock_mode == AccountSettings::RespAllow);
    ui_options.gb_os->setEnabled(enableFlag);
    ui_options.gb_client->setEnabled(enableFlag);
}

void ClientSwitcherPlugin::enableOsParams(int mode)
{
    if (mode == 1) { // user defined
        ui_options.le_osname->setEnabled(true);
        ui_options.le_osversion->setEnabled(true);
    } else {
        if (mode == 0) { // default os
            ui_options.le_osname->setText(def_os_name);
            ui_options.le_osversion->setText(def_os_version);
        } else {
            int pres_index = mode - 2;
            if (pres_index >= 0 && pres_index < os_presets.size()) {
                ui_options.le_osname->setText(os_presets.at(pres_index).name);
                ui_options.le_osversion->setText(os_presets.at(pres_index).version);
            }
        }
        ui_options.le_osname->setEnabled(false);
        ui_options.le_osversion->setEnabled(false);
    }
}

void ClientSwitcherPlugin::enableClientParams(int mode)
{
    if (mode == 1) { // user defined
        ui_options.le_clientname->setEnabled(true);
        ui_options.le_clientversion->setEnabled(true);
        ui_options.le_capsnode->setEnabled(true);
        ui_options.le_capsversion->setEnabled(true);
    } else {
        if (mode == 0) { // default client
            ui_options.le_clientname->setText(def_client_name);
            ui_options.le_clientversion->setText(def_client_version);
            ui_options.le_capsnode->setText(def_caps_node);
            ui_options.le_capsversion->setText(def_caps_version);
        } else {
            int pres_index = mode - 2;
            if (pres_index >= 0 && pres_index < client_presets.size()) {
                ui_options.le_clientname->setText(client_presets.at(pres_index).name);
                ui_options.le_clientversion->setText(client_presets.at(pres_index).version);
                ui_options.le_capsnode->setText(client_presets.at(pres_index).caps_node);
            }
        }
        ui_options.le_clientname->setEnabled(false);
        ui_options.le_clientversion->setEnabled(false);
        ui_options.le_capsnode->setEnabled(false);
        ui_options.le_capsversion->setEnabled(false);
    }
}
