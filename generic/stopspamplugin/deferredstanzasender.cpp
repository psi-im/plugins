/*
 * deferredstanzasender.cpp - plugin
 * Copyright (C) 2011  Evgeny Khryukin
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


#include "deferredstanzasender.h"

#define DELAY 500

DefferedStanzaSender::DefferedStanzaSender(StanzaSendingHost *host, QObject* p)
	: QObject(p)
	, stanzaSender_(host)
	, timer_(new QTimer(this))
{
	timer_->setInterval(DELAY);
	connect(timer_, SIGNAL(timeout()), SLOT(timeout()));
}

void DefferedStanzaSender::sendStanza(int account, const QDomElement& xml)
{
	XmlStanzaItem item(account, xml);
	items_.append(Item(Xml, item));
	timer_->start();  // стартуем или рестартуем таймер. рестарт нужен, чтобы быть уверенным, что интервал будет соблюден
}

void DefferedStanzaSender::sendStanza(int account, const QString& xml)
{
	StringStanzaItem item(account, xml);
	items_.append(Item(String, item));
	timer_->start();
}

void DefferedStanzaSender::sendMessage(int account, const QString& to, const QString& body, const QString& subject, const QString& type)
{
	MessageItem item(account, to, body, subject, type);
	items_.append(Item(Message, item));
	timer_->start();
}

QString DefferedStanzaSender::uniqueId(int account) const
{
	return stanzaSender_->uniqueId(account);
}

void DefferedStanzaSender::timeout()
{
	if(!items_.isEmpty()) {
		Item i = items_.takeFirst();
		switch(i.type) {
		case Xml:
			stanzaSender_->sendStanza(i.xmlItem.first, i.xmlItem.second);
			break;
		case String:
			stanzaSender_->sendStanza(i.stringItem.first, i.stringItem.second);
			break;
		case Message:
		{
			MessageItem mi = i.messageItem;
			stanzaSender_->sendMessage(mi.account, mi.to, mi.body, mi.subject, mi.type);
			break;
		}
		default:
			break;
		}
	}
	else {
		timer_->stop();
	}
};
