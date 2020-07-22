/*
 * accountsettings.cpp - Client Switcher plugin
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

#include "accountsettings.h"

AccountSettings::AccountSettings()
{
    init();
    return;
}

AccountSettings::AccountSettings(const QString &set_str) { fromString(set_str); }

AccountSettings::~AccountSettings() { }

bool AccountSettings::isValid() { return !account_id.isEmpty(); }

bool AccountSettings::isEmpty()
{
    if (response_mode == RespAllow && !lock_time_requ) {
        if (os_name.isNull() && client_name.isEmpty() && caps_node.isEmpty()) {
            return true;
        }
    }
    return false;
}

void AccountSettings::fromString(const QString &set_str)
{
    QStringList  set_list;
    int          len    = set_str.length();
    unsigned int sl_cnt = 0;
    int          st_pos = 0;
    for (int i = 0; i < len; i++) {
        QChar ch = set_str.at(i);
        if (ch == '\\') {
            sl_cnt++;
        } else if (ch == ';') {
            if ((sl_cnt & 1) == 0) { // четное кол-во слешей
                set_list.push_back(set_str.mid(st_pos, i - st_pos));
                st_pos = i + 1;
            }
            sl_cnt = 0;
        } else {
            sl_cnt = 0;
        }
    }
    if (st_pos < len) {
        set_list.push_back(set_str.mid(st_pos));
    }
    len = set_list.size();
    for (int i = 0; i < len; i++) {
        QStringList param_list = set_list.at(i).split("=");
        if (param_list.size() >= 2) {
            QString param_name  = param_list.takeFirst();
            QString param_value = param_list.join("=");
            if (param_name == "acc_id") {
                account_id = stripSlashes(param_value);
            } else if (param_name == "l_req") {
                if (param_value == "true")
                    response_mode = RespNotImpl;
                else if (param_value == "ignore")
                    response_mode = RespIgnore;
                else
                    response_mode = RespAllow;
            } else if (param_name == "l_treq") {
                lock_time_requ = (param_value == "true") ? true : false;
            } else if (param_name == "os_nm") {
                os_name = stripSlashes(param_value);
            } else if (param_name == "os_ver") {
                os_version = stripSlashes(param_value);
            } else if (param_name == "cl_nm") {
                client_name = stripSlashes(param_value);
            } else if (param_name == "cl_ver") {
                client_version = stripSlashes(param_value);
            } else if (param_name == "cp_nd") {
                caps_node = stripSlashes(param_value);
            }
        }
    }
}

QString AccountSettings::toString()
{
    QString s_res = "acc_id=" + addSlashes(account_id);
    QString str1;
    if (response_mode == RespNotImpl)
        str1 = "true";
    else if (response_mode == RespIgnore)
        str1 = "ignore";
    else
        str1 = "false";
    s_res.append(";l_req=").append(str1);
    s_res.append(";l_treq=").append((lock_time_requ) ? "true" : "false");
    if (!os_name.isNull())
        s_res.append(";os_nm=").append(addSlashes(os_name));
    if (!os_version.isNull())
        s_res.append(";os_ver=").append(addSlashes(os_version));
    if (!client_name.isNull())
        s_res.append(";cl_nm=").append(addSlashes(client_name));
    if (!client_version.isNull())
        s_res.append(";cl_ver=").append(addSlashes(client_version));
    if (!caps_node.isNull())
        s_res.append(";cp_nd=").append(addSlashes(caps_node));
    return s_res;
}

void AccountSettings::init()
{
    account_id     = "";
    response_mode  = RespAllow;
    lock_time_requ = false;
    os_name        = "";
    os_version     = "";
    client_name    = "";
    client_version = "";
    caps_node      = "";
}

QString AccountSettings::addSlashes(QString &str)
{
    return str.replace("\\", "\\\\", Qt::CaseSensitive).replace(";", "\\;");
}

QString AccountSettings::stripSlashes(QString &str) { return str.replace("\\;", ";").replace("\\\\", "\\"); }
