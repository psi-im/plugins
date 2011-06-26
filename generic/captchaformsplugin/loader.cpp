/*
 * loader.cpp - plugin
 * Copyright (C) 2010  Khryukin Evgeny
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

#include "loader.h"

#include <QNetworkProxy>


Loader::Loader(QString id, QObject *p)
	: QObject(p)
	, id_(id)

{
	manager_ = new QNetworkAccessManager(this);
}

Loader::~Loader()
{
}

void Loader::setProxy(QString host, int port, QString user, QString pass)
{
	if(host.isEmpty())
		return;

	QNetworkProxy proxy(QNetworkProxy::HttpCachingProxy,host,port,user,pass);
	manager_->setProxy(proxy);
}

void Loader::start(const QString& url)
{	
	manager_->get(QNetworkRequest(QUrl(url)));
	connect(manager_,SIGNAL(finished(QNetworkReply*)), SLOT(onRequestFinish(QNetworkReply*)));
}

void Loader::onRequestFinish(QNetworkReply *reply)
{
	if(reply->error() == QNetworkReply::NoError) {
		QByteArray ba = reply->readAll();
		emit data(id_, ba);
	}
	else
		emit error(id_);

	deleteLater();
}

