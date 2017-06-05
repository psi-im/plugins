/*
 * common.cpp - plugin
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

#include "common.h"

void Utils::requestMail(AccountSettings *as, StanzaSendingHost *stanzaSender, AccountInfoAccessingHost *accInfo)
{
	int acc = as->account;
	if(!checkAccount(acc, accInfo))
		return;

	if(!as->isMailEnabled || !as->isMailSupported)
		return;

	QString time, tid;
	if (!as->notifyAllUnread) {
		time = as->lastMailTime;
		tid = as->lastMailTid;
	}
	if (!time.isEmpty()) {
		time = QString("newer-than-time='%1'").arg(time);
	}
	if (!tid.isEmpty()) {
		tid = QString("newer-than-tid='%1'").arg(tid);
	}
	QString id = stanzaSender->uniqueId(acc);
	QString reply = QString("<iq type='get' to='%1' id='%4'>"\
				"<query xmlns='google:mail:notify' %2 %3/></iq>")
				.arg(as->jid, time, tid, id);
	stanzaSender->sendStanza(acc, reply);
}

void Utils::requestSharedStatusesList(AccountSettings *as, StanzaSendingHost *stanzaSender, AccountInfoAccessingHost *accInfo)
{
	int acc = as->account;
	if(!checkAccount(acc, accInfo))
		return;

	if(!as->isSharedStatusEnabled || !as->isSharedStatusSupported)
		return;

	QString id = stanzaSender->uniqueId(acc);
	QString str = QString("<iq type='get' to='%1' id='%2' >"
			      "<query xmlns='google:shared-status' version='2'/></iq>")
			.arg(as->jid, id);
	stanzaSender->sendStanza(acc, str);
}

void Utils::requestExtendedContactAttributes(AccountSettings *as, StanzaSendingHost *stanzaSender, AccountInfoAccessingHost *accInfo)
{
	int acc = as->account;
	if(!checkAccount(acc, accInfo))
		return;

	if(!as->isAttributesEnabled || !as->isAttributesSupported)
		return;

	QString id = stanzaSender->uniqueId(acc);
	QString str = QString("<iq type='get' id='%1'>"
			      "<query xmlns='jabber:iq:roster' xmlns:gr='google:roster' gr:ext='2'/></iq>")
			.arg(id);
	stanzaSender->sendStanza(acc, str);
}

void Utils::updateSharedStatus(AccountSettings *as, StanzaSendingHost *stanzaSender, AccountInfoAccessingHost *accInfo)
{
	int acc = as->account;
	if(!checkAccount(acc, accInfo))
		return;

	if(!as->isSharedStatusSupported || !as->isSharedStatusEnabled)
		return;

	QString id = stanzaSender->uniqueId(acc);
	QString str = QString("<iq type='set' to='%1' id='%2'>"
			      "<query xmlns='google:shared-status' version='2'>"
			      "<status>%3</status><show>%4</show>")
			.arg(as->jid, id)
			.arg(as->message, as->status.replace("online", "default"));
	foreach(QString status, as->sharedStatuses.keys()) {
		str += QString("<status-list show='%1'>").arg(QString(status).replace("online", "default"));
		foreach(QString message, as->sharedStatuses.value(status)) {
			str += QString("<status>%1</status>").arg(message);
		}
		str += "</status-list>";
	}
	str += "<invisible value='false'/></query></iq>";

	stanzaSender->sendStanza(acc, str);
}

void Utils::updateSettings(AccountSettings *as, StanzaSendingHost *stanzaSender, AccountInfoAccessingHost *accInfo)
{
	int acc = as->account;
	if(!checkAccount(acc, accInfo))
		return;

	QString reply = QString("<iq type=\"set\" to=\"%1\" id=\"%2\">"\
			"<usersetting xmlns=\"google:setting\">"\
			"<mailnotifications value=\"%3\" />"\
			"<archivingenabled value=\"%4\" />"\
			"<autoacceptsuggestions value=\"%5\" />"\
			"</usersetting></iq>")
			.arg(as->jid, stanzaSender->uniqueId(acc))
			.arg(as->isMailEnabled ? "true" : "false")
			.arg(as->isArchivingEnabled ? "true" : "false")
			.arg(as->isSuggestionsEnabled ? "true" : "false");

	stanzaSender->sendStanza(acc, reply);
}

void Utils::getUserSettings(AccountSettings *as, StanzaSendingHost *stanzaSender, AccountInfoAccessingHost *accInfo)
{
	int acc = as->account;
	if(!checkAccount(acc, accInfo))
		return;

	QString id = stanzaSender->uniqueId(acc);
	//id_.append(id);
	QString mes = QString("<iq type='get' to='%1' id='%2'><usersetting xmlns='google:setting' /></iq>")
		      .arg(as->jid).arg(id);
	stanzaSender->sendStanza(acc, mes);
}

void Utils::updateNoSaveState(AccountSettings *as, StanzaSendingHost *stanzaSender, AccountInfoAccessingHost *accInfo)
{
	int acc = as->account;
	if(!checkAccount(acc, accInfo) || !as->isNoSaveSupported)
		return;

	QString id = stanzaSender->uniqueId(acc);
	QString mes = QString("<iq type='get' to='%1' id='%2'>"
			    "<query xmlns='google:nosave' /></iq>")
			.arg(as->jid, id);

	stanzaSender->sendStanza(acc, mes);
}

bool Utils::checkAccount(int acc, AccountInfoAccessingHost *accInfo)
{
	if(acc == -1)
		return false;

	if(accInfo->getStatus(acc) == "offline")
		return false;

	return true;
}


