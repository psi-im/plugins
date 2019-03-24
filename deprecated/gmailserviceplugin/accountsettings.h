/*
 * accountsettings.cpp - plugin
 * Copyright (C) 2010 Evgeny Khryukin
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

#ifndef ACCOUNTSETTINGS_H
#define ACCOUNTSETTINGS_H

#include <QMap>
#include <QStringList>
#include <QMetaType>

struct Attributes
{
//    int mc;
//    int emc;
//    int w;
//    bool rejected;
    QString t;
//    bool autosub;
//    QString alias_for;
//    QCString inv;
};


class AccountSettings
{
public:
    AccountSettings(const int acc = -1, const QString &j = QString());
    ~AccountSettings() {}
    void fromString(const QString& settings);
    QString toString() const;

    int account;
    QString jid;
    QString fullJid;
    bool isMailEnabled;
    bool isMailSupported;
    bool isArchivingEnabled;
    bool isSuggestionsEnabled;
    bool notifyAllUnread;
    QString lastMailTime;
    QString lastMailTid;
    bool isSharedStatusEnabled;
    bool isSharedStatusSupported;
    bool isAttributesSupported;
    bool isAttributesEnabled;
    QString status;
    QString message;
    QMap<QString, QStringList> sharedStatuses; // < status, list of status messages >
    int listMax;
    int listContentsMax;
    int statusMax;
    bool isNoSaveSupported;
    bool isNoSaveEnbaled;
    QMap<QString, bool> noSaveList; // < jid, is no-save enabled >
    QMap<QString, Attributes> attributes; //jid, Attributes
};

Q_DECLARE_METATYPE(AccountSettings*)

#endif // ACCOUNTSETTINGS_H
