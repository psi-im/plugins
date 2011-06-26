/*
 * common.h - plugin
 * Copyright (C) 2010 Khryukin Evgeny
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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef COMMON_H
#define COMMON_H

#include "accountsettings.h"
#include "stanzasendinghost.h"
#include "accountinfoaccessinghost.h"

class Utils
{
public:
	static void requestMail(AccountSettings* as, StanzaSendingHost* stanzaSender, AccountInfoAccessingHost* accInfo);
	static void requestSharedStatusesList(AccountSettings* as, StanzaSendingHost* stanzaSender, AccountInfoAccessingHost* accInfo);
	static void updateSettings(AccountSettings* as, StanzaSendingHost* stanzaSender, AccountInfoAccessingHost* accInfo);
	static void updateSharedStatus(AccountSettings* as, StanzaSendingHost* stanzaSender, AccountInfoAccessingHost* accInfo);
	static void getUserSettings(AccountSettings* as, StanzaSendingHost* stanzaSender, AccountInfoAccessingHost* accInfo);
	static void updateNoSaveState(AccountSettings* as, StanzaSendingHost* stanzaSender, AccountInfoAccessingHost* accInfo);

	static bool checkAccount(int account, AccountInfoAccessingHost* accInfo);
};

#endif // COMMON_H
