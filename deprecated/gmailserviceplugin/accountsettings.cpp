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

#include "accountsettings.h"

static const QString splitString = "&split&";

AccountSettings::AccountSettings(const int acc, const QString &j)
    : account(acc)
    , jid(j)
    , fullJid(j)
    , isMailEnabled(false)
    , isMailSupported(false)
    , isArchivingEnabled(false)
    , isSuggestionsEnabled(false)
    , notifyAllUnread(false)
    , lastMailTime("")
    , lastMailTid("")
    , isSharedStatusEnabled(false)
    , isSharedStatusSupported(false)
    , isAttributesSupported(false)
    , isAttributesEnabled(true)
    , status("offline")
    , message("")
    , listMax(3)
    , listContentsMax(5)
    , statusMax(512)
    , isNoSaveSupported(false)
    , isNoSaveEnbaled(false)
{
}

void AccountSettings::fromString(const QString& settings)
{
    QStringList list = settings.split(splitString);
    if(!list.isEmpty())
        jid = list.takeFirst();
    if(!list.isEmpty())
        lastMailTime = list.takeFirst();
    if(!list.isEmpty())
        lastMailTid = list.takeFirst();
    if(!list.isEmpty())
        notifyAllUnread = (list.takeFirst() == "true");
    if(!list.isEmpty())
        isSharedStatusEnabled = (list.takeFirst() == "true");
    if(!list.isEmpty())
        isNoSaveEnbaled = (list.takeFirst() == "true");
    if(!list.isEmpty())
        isAttributesEnabled = (list.takeFirst() == "true");
}

QString AccountSettings::toString() const
{
    QStringList list;
    list <<  jid
         << lastMailTime
         << lastMailTid
         << (notifyAllUnread ? "true" : "false")
         << (isSharedStatusEnabled ? "true" : "false")
         << (isNoSaveEnbaled ? "true" : "false")
         << (isAttributesEnabled ? "true" : "false");
         return list.join(splitString);
}
