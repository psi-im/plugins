/*
 * jabberdiskcontroller.cpp - plugin
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

#include <QDomElement>
#include <QAction>

#include "jabberdiskcontroller.h"
#include "jd_mainwin.h"

struct Session {
	Session(int a, const QString& j, JDMainWin* w = 0)
		: account(a)
		, jid(j)
		, window(w)
	{
	}

	int account;
	QString jid;
	JDMainWin* window;

	bool operator==(const Session& s)
	{
		return account == s.account && jid == s.jid;
	}
};

JabberDiskController::JabberDiskController()
	: QObject(0)
	, stanzaSender(0)
//	, iconHost(0)
	, accInfo(0)
{
}

JabberDiskController::~JabberDiskController()
{
	while(!sessions_.isEmpty()) {
		delete sessions_.takeFirst().window;
	}
}

JabberDiskController* JabberDiskController::instance()
{
	if(instance_)
		return instance_;
	return instance_ = new JabberDiskController();
}

void JabberDiskController::reset()
{
	delete instance_;
	instance_ = 0;
}

void JabberDiskController::viewerDestroyed()
{
	JDMainWin *w = static_cast<JDMainWin*>(sender());
	for(int i = 0; i < sessions_.size(); i++) {
		Session s = sessions_.at(i);
		if(s.window == w) {
			sessions_.removeAt(i);
			break;
		}
	}
}

void JabberDiskController::initSession()
{
	QAction* act = dynamic_cast<QAction*>(sender());
	if(act) {
		int account = act->property("account").toInt();
		const QString jid = act->property("jid").toString();
		Session s(account, jid);
		if(sessions_.contains(s)) {
			sessions_.at(sessions_.indexOf(s)).window->raise();
		}
		else {
			s.window = new JDMainWin(accInfo->getJid(account), jid, account);
			connect(s.window, SIGNAL(destroyed()), SLOT(viewerDestroyed()));
			sessions_.append(s);
		}
	}
}

void JabberDiskController::sendStanza(int account, const QString &to, const QString &message, QString* id)
{
	*id = stanzaSender->uniqueId(account);
	QString txt = QString("<message from=\"%1\" id=\"%3\" type=\"chat\" to=\"%2\">"
			  "<body>%4</body></message>")
			.arg(accInfo->getJid(account))
			.arg(to)
			.arg(*id)
#ifdef HAVE_QT5
			.arg(message.toHtmlEscaped());
#else
			.arg(Qt::escape(message));
#endif
	stanzaSender->sendStanza(account, txt);
}

//void JabberDiskController::setIconFactoryAccessingHost(IconFactoryAccessingHost* host)
//{
//	iconHost = host;
//}

void JabberDiskController::setStanzaSendingHost(StanzaSendingHost *host)
{
	stanzaSender = host;
}

void JabberDiskController::setAccountInfoAccessingHost(AccountInfoAccessingHost* host)
{
	accInfo = host;
}

bool JabberDiskController::incomingStanza(int account, const QDomElement& xml)
{
	Session s(account, xml.attribute("from").split("/").at(0).toLower());
	if(sessions_.contains(s)) {
		emit stanza(account, xml);
		return true;
	}
	return false;
}


JabberDiskController* JabberDiskController::instance_ = 0;
