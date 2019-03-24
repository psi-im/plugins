/*
 * accountsettings.h - Client Switcher plugin
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

#ifndef ACCOUNTSETTINGS_H
#define ACCOUNTSETTINGS_H

#include <QtCore>

class AccountSettings
{

public:
    enum {RespAllow = 0, RespNotImpl = 1, RespIgnore = 2}; // как номер индекса в combobox
    enum {LogNever = 0, LogIfReplace = 1, LogAlways = 2};

    AccountSettings();
    AccountSettings(const QString&);
    ~AccountSettings();
    bool isValid();
    bool isEmpty();
    void fromString(const QString&);
    QString toString();
    //--
    QString account_id;
    bool enable_contacts;
    bool enable_conferences;
    int  response_mode;
    bool lock_time_requ;
    int  show_requ_mode;
    QString os_name;
    QString client_name;
    QString client_version;
    QString caps_node;
    QString caps_version;
    int  log_mode;

private:
    //--
    void init();
    QString addSlashes(QString&);
    QString stripSlashes(QString&);

};

#endif // ACCOUNTSETTINGS_H
