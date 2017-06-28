/*
 * server.cpp - plugin
 * Copyright (C) 2009-2011  Evgeny Khryukin
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

#include "server.h"


Server::Server(QListWidget *parent)
	: QListWidgetItem(parent)	
	, displayName_("server")
	, url_("")
	, userName_("")
	, password_("")
	, servPostdata_("")
	, servFileinput_("")
	, servRegexp_("")
{
}

void Server::setDisplayName(QString n)
{
	displayName_ = n;
}

void Server::setServerData(const QString& post, const QString& fInput, const QString& reg/*, QString fFilter*/)
{
	servPostdata_ = post;
	servFileinput_ = fInput;
	servRegexp_ = reg;
	//servFilefilter_ = fFilter;
}

void Server::setServer(const QString& url, const QString& user, const QString& pass)
{
	url_ = url;
	userName_ = user;
	password_ = pass;
}

QString Server::settingsToString() const
{
	QStringList l = QStringList() << displayName_ << url_ << userName_ << password_;
	l << servPostdata_ << servFileinput_ << servRegexp_/* << servFilefilter_*/;
	l << (useProxy_ ? "true" : "false");
	return l.join(splitString());
}

void Server::setFromString(const QString& settings)
{
	QStringList l = settings.split(splitString());
	if(l.size() == 11) {
		processOltSettingsString(l);
		return;
	}
	if(!l.isEmpty())
		displayName_ = l.takeFirst();
	if(!l.isEmpty())
		url_ = l.takeFirst();
	if(!l.isEmpty())
		userName_ = l.takeFirst();
	if(!l.isEmpty())
		password_ = l.takeFirst();
	if(!l.isEmpty())
		servPostdata_ = l.takeFirst();
	if(!l.isEmpty())
		servFileinput_ = l.takeFirst();
	if(!l.isEmpty())
		servRegexp_ = l.takeFirst();
	/*if(!l.isEmpty())
		servFilefilter_ = l.takeFirst();*/
	if(!l.isEmpty())
		useProxy_ = (l.takeFirst() == "true");
}

void Server::processOltSettingsString(QStringList l)
{
	displayName_ = l.takeFirst();
	url_ = l.takeFirst();
	userName_ = l.takeFirst();
	password_ = l.takeFirst();

	//remove old useless proxy settings
	l.takeFirst();
	l.takeFirst();
	l.takeFirst();
	l.takeFirst();

	servPostdata_ = l.takeFirst();
	servFileinput_ = l.takeFirst();
	servRegexp_ = l.takeFirst();
}

QString Server::splitString()
{
	return QString("&split&");
}
