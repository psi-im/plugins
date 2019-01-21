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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

//#include <QtCore>
//#include <QDomDocument>
#include <QMetaObject>

#include "clientswitcherplugin.h"
#include "viewer.h"

#define cVer                    "0.0.18"
#define constPluginShortName    "clientswitcher"
#define constPluginName         "Client Switcher Plugin"
#define constForAllAcc          "for_all_acc"
#define constAccSettingList     "accsettlist"
#define constShowLogHeight      "showlogheight"
#define constShowLogWidth       "showlogwidth"
#define constLastLogItem        "lastlogview"
#define constPopupDuration      "popupduration"


ClientSwitcherPlugin::ClientSwitcherPlugin() :
    sender_(0),
    psiOptions(0),
    psiPopup(0),
    psiInfo(0),
    psiAccount(0),
    psiAccountCtl(0),
    psiContactInfo(0),
    psiIcon(0),
    enabled(false),
    for_all_acc(false),
    def_os_name(""),
    def_client_name(""),
    def_client_version(""),
    def_caps_node(""),
    def_caps_version(""),
    heightLogsView(500),
    widthLogsView(600),
    lastLogItem(""),
    popupId(0)

{
    settingsList.clear();
    os_presets.clear();
    client_presets.clear();
}

ClientSwitcherPlugin::~ClientSwitcherPlugin() {
    //disable();
}

QString ClientSwitcherPlugin::name() const
{
    return constPluginName;
}

QString ClientSwitcherPlugin::shortName() const
{
    return constPluginShortName;
}

QString ClientSwitcherPlugin::version() const
{
    return cVer;
}

bool ClientSwitcherPlugin::enable()
{
    if (!psiOptions)
        return false;
    enabled = true;
    os_presets.clear();
    os_presets << OsStruct("Windows 98") << OsStruct("Windows ME") << OsStruct("Windows 2000");
    os_presets << OsStruct("Windows XP") << OsStruct("Windows Server 2003") << OsStruct("Windows Server 2008");
    os_presets << OsStruct("Windows Vista") << OsStruct("Windows 7, 32-bit") << OsStruct("Windows 7, 64-bit");
    os_presets << OsStruct("Arch Linux") << OsStruct("Debian GNU/Linux 6.0.1 (squeeze)");
    os_presets << OsStruct("Ubuntu 10.04.2 LTS") << OsStruct("RFRemix release 14.1 (Laughlin)");
    os_presets << OsStruct("openSUSE 11.4") << OsStruct("Gentoo Base System release 2.0.3");
    os_presets << OsStruct("Mac OS X") << OsStruct("Mac OS X 10.6");
    os_presets << OsStruct("Android OS 2.3.6 (build XXLB1)");
    os_presets << OsStruct("Plan9") << OsStruct("Solaris");
    os_presets << OsStruct("FreeBSD") << OsStruct("NetBSD") << OsStruct("OpenBSD");
    os_presets << OsStruct("Nokia5130c-2/07.91") << OsStruct("SonyEricssonW580i/R8BE001");
    client_presets.clear();
    client_presets << ClientStruct("Bombus", "0.7.1429M-Zlib", "http://bombus-im.org/java", "0.7.1429M-Zlib");
    client_presets << ClientStruct("Gajim", "0.12.5", "https://gajim.org", "0.12.5");
    client_presets << ClientStruct("Mcabber", "0.9.10", "http://mcabber.com/caps", "0.9.10");
    client_presets << ClientStruct("Miranda", "0.9.16.0", "http://miranda-im.org/caps", "0.9.16.0");
    client_presets << ClientStruct("Pidgin", "2.8.0", "http://pidgin.im", "2.8.0");
    client_presets << ClientStruct("Psi", "0.14", "https://psi-im.org/caps", "0.14");
    client_presets << ClientStruct("QIP Infium", "9034", "http://qip.ru/caps", "9034");
    client_presets << ClientStruct("qutIM", "0.2", "http://qutim.org", "0.2");
    client_presets << ClientStruct("Swift", "1.0", "http://swift.im", "1.0");
    client_presets << ClientStruct("Talisman", "0.1.1.15", "http://jabrvista.net.ru", "0.1.1.15");
    client_presets << ClientStruct("Tkabber", "0.11.1", "http://tkabber.jabber.ru", "0.11.1");
    client_presets << ClientStruct("Talkonaut", "1.0.0.84", "http://www.google.com/xmpp/client/caps", "1.0.0.84");
    client_presets << ClientStruct(QString::fromUtf8("Я.Онлайн"), "3.2.0.8873", "yandex-webchat", "3.2.0.8873");
    // Флаг "для всех аккаунтов"
    for_all_acc = psiOptions->getPluginOption(constForAllAcc, QVariant(false)).toBool();
    // Получаем настройки аккаунтов
    QStringList sett_list = psiOptions->getPluginOption(constAccSettingList, QVariant()).toStringList();
    // Get and register popup option
    int popup_duration = psiOptions->getPluginOption(constPopupDuration, QVariant(5000)).toInt() / 1000;
    popupId = psiPopup->registerOption(constPluginName, popup_duration, "plugins.options." + shortName() + "." + constPopupDuration);
    // Формируем структуры
    int cnt = sett_list.size();
    for (int i = 0; i < cnt; i++) {
        AccountSettings* ac = new AccountSettings(sett_list.at(i));
        if (ac) {
            if (ac->isValid())
                settingsList.push_back(ac);
            else
                delete ac;
        }
    }
    // Настройки для просмотра логов
    logsDir = psiInfo->appCurrentProfileDir(ApplicationInfoAccessingHost::DataLocation) + "/logs/clientswitcher";
    QDir dir(logsDir);
    if (!dir.exists(logsDir))
        dir.mkpath(logsDir);
    logsDir.append("/");
    heightLogsView = psiOptions->getPluginOption(constShowLogHeight, QVariant(heightLogsView)).toInt();
    widthLogsView = psiOptions->getPluginOption(constShowLogWidth, QVariant(widthLogsView)).toInt();
    lastLogItem = psiOptions->getPluginOption(constLastLogItem, QVariant(widthLogsView)).toString();

    return true;
}

bool ClientSwitcherPlugin::disable()
{
    while (settingsList.size() != 0) {
        AccountSettings* as = settingsList.takeLast();
        if (as)
            delete as;
    }
    enabled = false;
    psiPopup->unregisterOption(constPluginName);
    return true;
}

QWidget* ClientSwitcherPlugin::options()
{
    if (!enabled) {
        return 0;
    }
    QWidget* optionsWid = new QWidget();
    ui_options.setupUi(optionsWid);
    // Заполняем виджет пресетов ОС
    ui_options.cb_ospreset->addItem("default", "default");
    ui_options.cb_ospreset->addItem("user defined", "user");
    int cnt = os_presets.size();
    for (int i = 0; i < cnt; i++) {
        ui_options.cb_ospreset->addItem(os_presets.at(i).name);
    }
    // Заполняем виджет пресетов клиента
    ui_options.cb_clientpreset->addItem("default", "default");
    ui_options.cb_clientpreset->addItem("user defined", "user");
    cnt = client_presets.size();
    for (int i = 0; i < cnt; i++) {
        ui_options.cb_clientpreset->addItem(client_presets.at(i).name);
    }
    // Элементы для просмотра логов
    QDir dir(logsDir);
    int pos = -1;
    foreach(const QString &file, dir.entryList(QDir::Files)) {
        ui_options.cb_logslist->addItem(file);
        ++pos;
        if (file == lastLogItem)
            ui_options.cb_logslist->setCurrentIndex(pos);
    }
    if (pos < 0)
        ui_options.bt_viewlog->setEnabled(false);
    //--
    connect(ui_options.cb_allaccounts, SIGNAL(stateChanged(int)), this, SLOT(enableAccountsList(int)));
    connect(ui_options.cb_accounts, SIGNAL(currentIndexChanged(int)), this, SLOT(restoreOptionsAcc(int)));
    connect(ui_options.cmb_lockrequ, SIGNAL(currentIndexChanged(int)), this, SLOT(enableMainParams(int)));
    connect(ui_options.cb_ospreset, SIGNAL(currentIndexChanged(int)), this, SLOT(enableOsParams(int)));
    connect(ui_options.cb_clientpreset, SIGNAL(currentIndexChanged(int)), this, SLOT(enableClientParams(int)));
    connect(ui_options.bt_viewlog, SIGNAL(released()), this, SLOT(viewFromOpt()));
    restoreOptions();

    return optionsWid;
}

void ClientSwitcherPlugin::applyOptions() {
    bool caps_updated = false;
    // Аккаунт
    bool for_all_acc_old = for_all_acc;
    for_all_acc = ui_options.cb_allaccounts->isChecked();
    if (for_all_acc != for_all_acc_old) {
        caps_updated = true;
    }
    int acc_index = ui_options.cb_accounts->currentIndex();
    if (acc_index == -1 && !for_all_acc)
        return;
    QString acc_id = "all";
    if (!for_all_acc)
        acc_id = ui_options.cb_accounts->itemData(acc_index).toString();
    AccountSettings* as = getAccountSetting(acc_id);
    if (!as) {
        as = new AccountSettings();
        as->account_id = acc_id;
        settingsList.push_back(as);
    }
    // Подмена/блокировка для контактов
    bool tmp_flag = ui_options.cb_contactsenable->isChecked();
    if (as->enable_contacts != tmp_flag) {
        as->enable_contacts = tmp_flag;
        caps_updated = true;
    }
    // Подмена/блокировка для конференций
    tmp_flag = ui_options.cb_conferencesenable->isChecked();
    if (as->enable_conferences != tmp_flag) {
        as->enable_conferences = tmp_flag;
        caps_updated = true;
    }
    // Блокировка запроса версии
    int respMode = ui_options.cmb_lockrequ->currentIndex();
    if (as->response_mode != respMode) {
        if (as->response_mode == AccountSettings::RespAllow || respMode == AccountSettings::RespAllow)
            caps_updated = true;
        as->response_mode = respMode;
    }
    // Блокировка запроса времени
    tmp_flag = ui_options.cb_locktimerequ->isChecked();
    if (as->lock_time_requ != tmp_flag) {
        as->lock_time_requ = tmp_flag;
        caps_updated = true;
    }
    // Уведомления при запросах версии
    as->show_requ_mode = ui_options.cmb_showrequ->currentIndex();
    // Ведение лога
    as->log_mode = ui_options.cmb_savetolog->currentIndex();
    // Наименование ОС
    if (ui_options.cb_ospreset->currentIndex() == 0) {
        as->os_name = "";
    } else {
        as->os_name = ui_options.le_osname->text().trimmed();
    }
    // Клиент
    if (ui_options.cb_clientpreset->currentIndex() == 0) {
        as->client_name = "";
        as->client_version = "";
        if (!as->caps_node.isEmpty()) {
            as->caps_node = "";
            caps_updated = true;
        }
        if (!as->caps_version.isEmpty()) {
            as->caps_version = "";
            caps_updated = true;
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
            caps_updated = true;
        }
        // Caps version клиента
        str1 = ui_options.le_capsversion->text().trimmed();
        if (as->caps_version != str1) {
            as->caps_version = str1;
            caps_updated = true;
        }
    }
    // Сохраняем опции
    psiOptions->setPluginOption(constForAllAcc, QVariant(for_all_acc));
    QStringList sett_list;
    int cnt = settingsList.size();
    for (int i = 0; i < cnt; i++) {
        AccountSettings* as = settingsList.at(i);
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

void ClientSwitcherPlugin::restoreOptions() {
    // Заполняем флаг "для всех аккаунтов"
    ui_options.cb_allaccounts->setChecked(for_all_acc);
    // Заполняем виджет аккаунтов
    ui_options.cb_accounts->clear();
    if (!psiAccount)
        return;
    int cnt = 0;
    for (int i = 0; ; i++) {
        QString id = psiAccount->getId(i);
        if (id == "-1")
            break;
        if (!id.isEmpty()) {
            QString name = psiAccount->getName(i);
            if (name.isEmpty())
                name = "?";
            ui_options.cb_accounts->addItem(QString("%1 (%2)").arg(name).arg(psiAccount->getJid(i)), QVariant(id));
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

QPixmap ClientSwitcherPlugin::icon() const
{
    return QPixmap(":/icons/clientswitcher.png");
}

//-- OptionAccessor -------------------------------------------------

void ClientSwitcherPlugin::setOptionAccessingHost(OptionAccessingHost* host)
{
    psiOptions = host;
}

void ClientSwitcherPlugin::optionChanged(const QString& /*option*/)
{

}

//-- StanzaSender ---------------------------------------------------

void ClientSwitcherPlugin::setStanzaSendingHost(StanzaSendingHost *host)
{
    sender_ = host;
}

// ----------------------- StanzaFilter ------------------------------
bool ClientSwitcherPlugin::incomingStanza(int account, const QDomElement& stanza)
{
    if (!enabled)
        return false;
    QString acc_id = (for_all_acc) ? "all" : psiAccount->getId(account);
    AccountSettings *as = getAccountSetting(acc_id);
    if (!as)
        return false;
    if (!as->enable_contacts && !as->enable_conferences)
        return false;
    int respMode = as->response_mode;
    if (respMode != AccountSettings::RespAllow || as->lock_time_requ || !as->caps_node.isEmpty() || !as->caps_version.isEmpty()) {
        if (stanza.tagName() == "iq" && stanza.attribute("type") == "get") {
            const QString s_to = stanza.attribute("from");
            if (isSkipStanza(as, account, s_to))
                return false;
            QDomNode s_child = stanza.firstChild();
            while (!s_child.isNull()) {
                const QString xmlns = s_child.toElement().attribute("xmlns");
                if (s_child.toElement().tagName() == "query") {
                    if (xmlns == "http://jabber.org/protocol/disco#info") {
                        QString node = s_child.toElement().attribute("node");
                        if (!node.isEmpty()) {
                            QString new_node = def_caps_node;
                            QStringList split_node = node.split("#");
                            if (split_node.size() > 1) {
                                split_node.removeFirst();
                                QString node_ver = split_node.join("#");
                                if (node_ver == ((respMode == AccountSettings::RespAllow) ? as->caps_version : "n/a")) {
                                    node_ver = def_caps_version;
                                }
                                new_node.append("#" + node_ver);
                            }
                            s_child.toElement().setAttribute("node", new_node);
                        }
                    }
                    else if (xmlns == "jabber:iq:version") {
                        if (respMode == AccountSettings::RespIgnore) {
                            // Showing popup if it is necessary
                            if (as->show_requ_mode == AccountSettings::LogAlways)
                                showPopup(jidToNick(account, s_to));
                            // Write log if it is necessary
                            if (as->log_mode == AccountSettings::LogAlways)
                                saveToLog(account, s_to, "ignored");
                            return true;
                        }
                    }
                }
                s_child = s_child.nextSibling();
            }
        }
    }

    return false;
}

bool ClientSwitcherPlugin::outgoingStanza(int account, QDomElement& stanza)
{
    if(!enabled)
        return false;
    QString acc_id = (for_all_acc) ? "all" : psiAccount->getId(account);
    AccountSettings *as = getAccountSetting(acc_id);
    if (!as)
        return false;
    if (as->response_mode != AccountSettings::RespAllow || !as->caps_node.isEmpty() || !as->caps_version.isEmpty()) {
        // --------------- presence ---------------
        if (stanza.tagName() == "presence") {
            if (!as->enable_contacts && !as->enable_conferences)
                return false;
            // Анализ сообщения о присутствии
            bool skipFlag = isSkipStanza(as, account, stanza.attribute("to"));
            if (skipFlag && !as->enable_conferences) // Конференция не определяется при входе в нее, проверим позже
                return false;
            short unsigned int found_flag = 0; // Чтобы исключить перебор лишних элементов
            QDomNode s_child = stanza.firstChild();
            QDomNode caps_node;
            while (!s_child.isNull()) {
                if (((found_flag & 1) == 0) && s_child.toElement().tagName() == "c") {
                    // Подмена будет отложенной
                    caps_node = s_child;
                    found_flag |= 1;
                    if (found_flag == 3)
                        break;
                    s_child = s_child.nextSibling();
                    continue;
                }
                if (((found_flag & 2) == 0) && s_child.toElement().tagName() == "x") {
                    if (s_child.namespaceURI() == "http://jabber.org/protocol/muc") {
                        found_flag |= 2;
                        if (found_flag == 3)
                            break;
                    }
                }
                s_child = s_child.nextSibling();
            }
            if ((found_flag == 1 && as->enable_contacts) || (found_flag == 3 && as->enable_conferences)) {
                // Подменяем капс
                if (as->response_mode != AccountSettings::RespNotImpl)
                {
                    caps_node.toElement().setAttribute("node", as->caps_node);
                    caps_node.toElement().setAttribute("ver", as->caps_version);
                }
                else {
                    caps_node.toElement().setAttribute("node", "unknown");
                    caps_node.toElement().setAttribute("ver", "n/a");
                }
            }
            return false;
        }
    }
    if (stanza.tagName() == "iq" && stanza.attribute("type") == "result") {
        bool is_version_query = false;
        bool is_version_replaced = false;
        QString s_to = stanza.attribute("to");
        QStringList send_ver_list;
        QDomNode s_child = stanza.firstChild();
        int respMode = as->response_mode;
        while (!s_child.isNull()) {
            if (s_child.toElement().tagName() == "query") {
                QString xmlns_str = s_child.namespaceURI();
                if (xmlns_str == "http://jabber.org/protocol/disco#info") {
                    // --- Ответ disco
                    if (isSkipStanza(as, account, s_to))
                        return false;
                    if (respMode == AccountSettings::RespAllow && !as->lock_time_requ && as->caps_node.isEmpty() && as->caps_version.isEmpty())
                        return false;
                    // Подменяем ноду, если она есть
                    QString node = s_child.toElement().attribute("node");
                    if (!node.isEmpty()) {
                        QString new_node = (respMode == AccountSettings::RespAllow) ? as->caps_node : "unknown";
                        QStringList split_node = node.split("#");
                        if (split_node.size() > 1) {
                            split_node.removeFirst();
                            QString new_ver = split_node.join("#");
                            if (new_ver == def_caps_version)
                                new_ver = (respMode == AccountSettings::RespAllow) ? as->caps_version : "n/a";
                            new_node.append("#" + new_ver);
                        }
                        s_child.toElement().setAttribute("node", new_node);
                    }
                    // Подменяем identity и удаляем feature для версии, если есть блокировка
                    int update = 0;
                    if (respMode == AccountSettings::RespAllow)
                        ++update;
                    if (!as->lock_time_requ)
                        ++update;
                    QDomNode ver_domnode;
                    QDomNode time_domnode;
                    QDomNode q_child = s_child.firstChild();
                    while (!q_child.isNull()) {
                        QString tag_name = q_child.toElement().tagName();
                        if (tag_name == "feature") {
                            if (respMode != AccountSettings::RespAllow && q_child.toElement().attribute("var") == "jabber:iq:version") {
                                ver_domnode = q_child;
                                if (++update >= 3)
                                    break;
                            } else if (as->lock_time_requ && q_child.toElement().attribute("var") == "urn:xmpp:time") {
                                time_domnode = q_child;
                                if (++update >= 3)
                                    break;
                            }
                        } else if (tag_name == "identity") {
                            if (!q_child.toElement().attribute("name").isEmpty())
                                q_child.toElement().setAttribute("name", (respMode == AccountSettings::RespAllow) ? as->client_name : "unknown");
                            if (++update >= 3)
                                break;
                        }
                        q_child = q_child.nextSibling();
                    }
                    if (!s_child.isNull()) {
                        if (!ver_domnode.isNull())
                            s_child.removeChild(ver_domnode);
                        if (!time_domnode.isNull())
                            s_child.removeChild(time_domnode);
                    }
                } else if (xmlns_str == "jabber:iq:version") {
                    // Ответ version
                    is_version_query = true;
                    bool skip_stanza = isSkipStanza(as, account, s_to);
                    if (skip_stanza && as->log_mode != AccountSettings::LogAlways)
                        break;
                    QDomDocument xmldoc = stanza.ownerDocument();
                    if (respMode == AccountSettings::RespAllow) {
                        // Подменяем ответ
                        bool f_os_name = false;
                        bool f_client_name = false;
                        bool f_client_ver = false;
                        QDomNode q_child = s_child.firstChild();
                        while(!q_child.isNull()) {
                            QString tag_name = q_child.toElement().tagName().toLower();
                            if (tag_name == "os") {
                                QString str1;
                                if (!skip_stanza && !as->os_name.isEmpty()) {
                                    str1 = as->os_name;
                                    q_child.toElement().replaceChild(xmldoc.createTextNode(str1), q_child.firstChild());
                                    is_version_replaced = true;
                                } else {
                                    str1 = q_child.toElement().text();
                                }
                                send_ver_list.push_back("OS: " + str1);
                                f_os_name = true;
                            } else if (tag_name == "name") {
                                QString str1;
                                if (!skip_stanza && !as->client_name.isEmpty()) {
                                    str1 = as->client_name;
                                    q_child.toElement().replaceChild(xmldoc.createTextNode(str1), q_child.firstChild());
                                    is_version_replaced = true;
                                } else {
                                    str1 = q_child.toElement().text();
                                }
                                send_ver_list.push_back("Client name: " + str1);
                                f_client_name = true;
                            } else if (tag_name == "version") {
                                QString str1;
                                if (!skip_stanza && !as->caps_version.isEmpty()) {
                                    str1 = as->client_version;
                                    q_child.toElement().replaceChild(xmldoc.createTextNode(str1), q_child.firstChild());
                                    is_version_replaced = true;
                                } else {
                                    str1 = q_child.toElement().text();
                                }
                                send_ver_list.push_back("Client version: " + str1);
                                f_client_ver = true;
                            }
                            q_child = q_child.nextSibling();
                        }
                        // Создаем теги, если их нет в ответе
                        QDomDocument doc;
                        if (!f_client_name && !as->client_name.isEmpty()) {
                            doc = s_child.ownerDocument();
                            QDomElement cl_name = doc.createElement("name");
                            cl_name.appendChild(doc.createTextNode(as->client_name));
                            s_child.appendChild(cl_name);
                        }
                        if (!f_client_ver && !as->client_version.isEmpty()) {
                            if (doc.isNull())
                                doc = s_child.ownerDocument();
                            QDomElement cl_ver = doc.createElement("version");
                            cl_ver.appendChild(doc.createTextNode(as->client_version));
                            s_child.appendChild(cl_ver);
                        }
                        if (!f_os_name && !as->os_name.isEmpty()) {
                            if (doc.isNull())
                                doc = s_child.ownerDocument();
                            QDomElement os_name = doc.createElement("os");
                            os_name.appendChild(doc.createTextNode(as->os_name));
                            s_child.appendChild(os_name);
                        }
                    } else if (respMode == AccountSettings::RespNotImpl) {
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
                        QDomElement not_imp = xmldoc.createElement("feature-not-implemented");
                        not_imp.setAttribute("xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas");
                        err.appendChild(not_imp);
                        send_ver_list.push_back("feature-not-implemented");
                    }
                }
                break;
            } else if (s_child.toElement().tagName() == "time") {
                QString xmlns_str = s_child.namespaceURI();
                if (xmlns_str == "urn:xmpp:time") {
                    // Ответ time
                    if (isSkipStanza(as, account, s_to))
                        break;
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
                        QDomElement not_imp = xmldoc.createElement("feature-not-implemented");
                        not_imp.setAttribute("xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas");
                        err.appendChild(not_imp);
                    }
                }
            }
            s_child = s_child.nextSibling();
        }

        // Showing popup if it is necessary
        if (is_version_query && as->show_requ_mode != AccountSettings::LogNever
            && (as->show_requ_mode == AccountSettings::LogAlways || is_version_replaced))
            showPopup(jidToNick(account, s_to));

        // Write log if it is necessary
        if (is_version_query && as->log_mode != AccountSettings::LogNever
            && (as->log_mode == AccountSettings::LogAlways || is_version_replaced))
            saveToLog(account, s_to, send_ver_list.join(", "));
    }
    return false;
}

QString ClientSwitcherPlugin::pluginInfo() {
    return tr("Authors: ") +  "Liuch\n\n"
         + trUtf8("The plugin is intended for substitution of the client version, his name and operating system type.\n"
                  "You can specify the version of the client and OS or to select them from the preset list.\n");
}

void ClientSwitcherPlugin::setPopupAccessingHost(PopupAccessingHost* host)
{
    psiPopup = host;
}

// ----------------------- ApplicationInfoAccessor ------------------------------
void ClientSwitcherPlugin::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host)
{
    psiInfo = host;
    if (psiInfo) {
        // def_os_name = ;
        def_client_name = psiInfo->appName();
        def_client_version = psiInfo->appVersion();
        def_caps_node = psiInfo->appCapsNode();
        def_caps_version = psiInfo->appCapsVersion();
        def_os_name = psiInfo->appOsName();
    }
}

// ----------------------- AccountInfoAccessing ------------------------------
void ClientSwitcherPlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost* host)
{
    psiAccount = host;
}

// ----------------------- PsiAccountController ----------------------------------
void ClientSwitcherPlugin::setPsiAccountControllingHost(PsiAccountControllingHost* host)
{
    psiAccountCtl = host;
}

// ----------------------- ContactInfoAccessor -----------------------
void ClientSwitcherPlugin::setContactInfoAccessingHost(ContactInfoAccessingHost* host)
{
    psiContactInfo = host;
}

// ----------------------- IconFactoryAccessor -----------------------
void ClientSwitcherPlugin::setIconFactoryAccessingHost(IconFactoryAccessingHost* host)
{
    psiIcon = host;
}

// ----------------------- Private ------------------------------

int ClientSwitcherPlugin::getOsTemplateIndex(QString &os_name)
{
    if (os_name.isEmpty())
        return 0; // default
    int cnt = os_presets.size();
    for (int i = 0; i < cnt; i++) {
        if (os_name == os_presets.at(i).name) {
            i += 2; // Т.к. впереди default и user defined
            return i;
        }
    }
    return 1; // user defined
}

int ClientSwitcherPlugin::getClientTemplateIndex(QString &cl_name, QString &cl_ver, QString &cp_node, QString &cp_ver)
{
    if (cl_name.isEmpty() && cl_ver.isEmpty() && cp_node.isEmpty() && cp_ver.isEmpty())
        return 0; // default
    int cnt = client_presets.size();
    for (int i = 0; i < cnt; i++) {
        if (cl_name == client_presets.at(i).name && cl_ver == client_presets.at(i).version) {
            if (cp_node == client_presets.at(i).caps_node && cp_ver == client_presets.at(i).caps_version) {
                i += 2; // есть еще default и user defined
                return i;
            }
        }
    }
    return 1; // user defined
}

int ClientSwitcherPlugin::getAccountById(QString &acc_id)
{
    if (!psiAccount || acc_id.isEmpty())
        return -1;
    for (int i = 0; ; i++) {
        QString id = psiAccount->getId(i);
        if (id == "-1")
            break;
        if (id == acc_id)
            return i;
    }
    return -1;
}

AccountSettings* ClientSwitcherPlugin::getAccountSetting(const QString &acc_id)
{
    int cnt = settingsList.size();
    for (int i = 0; i < cnt; i++) {
        AccountSettings* as = settingsList.at(i);
        if (as && as->account_id == acc_id)
            return as;
    }
    return NULL;
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
    for (; ; acc++) {
        QString acc_id = psiAccount->getId(acc);
        if (acc_id == "-1")
            break;
        if (!acc_id.isEmpty()) {
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

bool ClientSwitcherPlugin::isSkipStanza(AccountSettings* as, int account, QString to)
{
    if (to.isEmpty()) { // Широковещательный
        if (!as->enable_contacts)
            return true;
    } else { // Адресный
        QString to_jid = to.split("/").takeFirst();
        if (!to_jid.contains("@")) {
            if (as->enable_contacts) {
                if (!to.contains("/"))
                    return false; // Предполагаем что это сервер
                return true; // Ошибочный запрос
            }
        }
        if (psiContactInfo->isConference(account, to_jid) || psiContactInfo->isPrivate(account, to)) {
            //if (to.contains("conference.")) {
            if (!as->enable_conferences)
                return true;
        } else {
            if (!as->enable_contacts)
                return true;
        }
    }
    return false;
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
            AccountSettings* as = getAccountSetting(acc_id);
            if (!as) {
                as = new AccountSettings(); // Сразу заполняется дефолтными настройками
                as->account_id = acc_id;
                settingsList.push_back(as);
            }
            // Подмена/блокировка для контактов
            ui_options.cb_contactsenable->setChecked(as->enable_contacts);
            // Подмена/блокировка для конференций
            ui_options.cb_conferencesenable->setChecked(as->enable_conferences);
            // Блокировка запроса версии
            ui_options.cmb_lockrequ->setCurrentIndex(as->response_mode);
            // Блокировка запроса времени
            ui_options.cb_locktimerequ->setChecked(as->lock_time_requ);
            // Уведомления при запросах версии
            ui_options.cmb_showrequ->setCurrentIndex(as->show_requ_mode);
            // Ведение лога
            ui_options.cmb_savetolog->setCurrentIndex(as->log_mode);
            // Виджет шаблона ОС
            QString os_name = as->os_name;
            int os_templ = getOsTemplateIndex(os_name);
            ui_options.cb_ospreset->setCurrentIndex(os_templ);
            // Название ОС
            ui_options.le_osname->setText(os_name);
            // Виджет шаблона клиента
            QString cl_name = as->client_name;
            QString cl_ver = as->client_version;
            QString cp_node = as->caps_node;
            QString cp_ver = as->caps_version;
            int cl_templ = getClientTemplateIndex(cl_name, cl_ver, cp_node, cp_ver);
            ui_options.cb_clientpreset->setCurrentIndex(cl_templ);
            // Название клиента
            ui_options.le_clientname->setText(cl_name);
            // Версия клиента
            ui_options.le_clientversion->setText(cl_ver);
            // Caps node клиента
            ui_options.le_capsnode->setText(cp_node);
            // Caps version клиента
            ui_options.le_capsversion->setText(cp_ver);
            // Блокировка/снятие блокировки виджетов
            ui_options.gb_enablefor->setEnabled(true);
            ui_options.cmb_lockrequ->setEnabled(true);
            enableMainParams(as->response_mode);
            enableOsParams(os_templ);
            enableClientParams(cl_templ);
            return;
        }
    }
    // Блокировка виджетов при отсутствии валидного акка
    ui_options.cb_contactsenable->setChecked(false);
    ui_options.cb_conferencesenable->setChecked(false);
    ui_options.gb_enablefor->setEnabled(false);
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
    } else {
        if (mode == 0) { // default os
            ui_options.le_osname->setText(def_os_name);
        } else {
            int pres_index = mode - 2;
            if (pres_index >= 0 && pres_index < os_presets.size()) {
                ui_options.le_osname->setText(os_presets.at(pres_index).name);
            }
        }
        ui_options.le_osname->setEnabled(false);
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
                ui_options.le_capsversion->setText(client_presets.at(pres_index).caps_version);
            }
        }
        ui_options.le_clientname->setEnabled(false);
        ui_options.le_clientversion->setEnabled(false);
        ui_options.le_capsnode->setEnabled(false);
        ui_options.le_capsversion->setEnabled(false);
    }
}

void ClientSwitcherPlugin::viewFromOpt() {
    lastLogItem = ui_options.cb_logslist->currentText();
    if (lastLogItem.isEmpty())
        return;
    psiOptions->setPluginOption(constLastLogItem, QVariant(lastLogItem));
    showLog(lastLogItem);
}

QString ClientSwitcherPlugin::jidToNick(int account, const QString &jid)
{
    QString nick;
    if (psiContactInfo)
        nick = psiContactInfo->name(account, jid);
    if (nick.isEmpty())
        nick = jid;
    return nick;
}

void ClientSwitcherPlugin::showPopup(const QString &nick)
{
    int msecs = psiPopup->popupDuration(constPluginName);
    if (msecs > 0)
        psiPopup->initPopup(tr("%1 has requested your version").arg(sender_->escape(nick)), constPluginName, "psi/headline", popupId);
}

void ClientSwitcherPlugin::showLog(QString filename)
{
    QString fullname = logsDir + filename;
    Viewer *v = new Viewer(fullname, psiIcon);
    v->resize(widthLogsView, heightLogsView);
    if(!v->init()) {
        delete(v);
        return;
    }
    connect(v, SIGNAL(onClose(int,int)), this, SLOT(onCloseView(int,int)));
    v->show();
}

void ClientSwitcherPlugin::saveToLog(int account, QString to_jid, QString ver_str)
{
    QString acc_jid = psiAccount->getJid(account);
    if (acc_jid.isEmpty() || acc_jid == "-1")
        return;
    QFile file(logsDir + acc_jid.replace("@", "_at_") + ".log");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QString time_str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        QTextStream out(&file);
        out.setCodec("UTF-8");
        out.setGenerateByteOrderMark(false);
        out << time_str << "  " << to_jid << " <-- " << ver_str << endl;
    }
}

void ClientSwitcherPlugin::onCloseView(int w, int h) {
    if (widthLogsView != w)
    {
        widthLogsView = w;
        psiOptions->setPluginOption(constShowLogWidth, QVariant(w));
    }
    if (heightLogsView != h)
    {
        heightLogsView = h;
        psiOptions->setPluginOption(constShowLogHeight, QVariant(h));
    }
}
