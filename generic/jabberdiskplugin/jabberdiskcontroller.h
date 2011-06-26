/*
 * jabberdiskcontroller.h - plugin
 * Copyright (C) 2011  Khryukin Evgeny
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


#ifndef JABBERDISKCONTROLLER_H
#define JABBERDISKCONTROLLER_H

//class QIcon;
class JDMainWin;
struct Session;
class QDomElement;

#include <QObject>

//#include "iconfactoryaccessinghost.h"
#include "stanzasendinghost.h"
#include "accountinfoaccessinghost.h"

class JabberDiskController : public QObject
{
	Q_OBJECT
public:
	static JabberDiskController* instance();
	static void reset();
	virtual ~JabberDiskController();

	//void setIconFactoryAccessingHost(IconFactoryAccessingHost* host);
	void setStanzaSendingHost(StanzaSendingHost *host);
	void setAccountInfoAccessingHost(AccountInfoAccessingHost* host);
	bool incomingStanza(int account, const QDomElement& xml);

	void sendStanza(int account, const QString& to, const QString& message, QString* id);

signals:
	void stanza(int account, const QDomElement& xml);

public slots:
	void initSession();

private slots:
	void viewerDestroyed();

private:
	JabberDiskController();
	static JabberDiskController* instance_;

private:
	StanzaSendingHost* stanzaSender;
	//IconFactoryAccessingHost* iconHost;
	AccountInfoAccessingHost* accInfo;
	QList<Session> sessions_;

};

#endif // JABBERDISKCONTROLLER_H
