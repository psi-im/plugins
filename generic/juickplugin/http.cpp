/*
 * http.cpp - plugin
 * Copyright (C) 2009-2010 Kravtsov Nikolai, Khryukin Evgeny
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <QNetworkProxy>
#include <QNetworkRequest>
#include "qdebug.h"

#include "http.h"

const int DOWNLOAD_TIMEOUT = 60000;

Http::Http(QObject *p)
	:QObject(p)
{	
	manager_ = new QNetworkAccessManager(this);
	eloop_ = new QEventLoop(this);
	url_ = QUrl();
	ba_.clear();
	timer_ = new QTimer(this);

	connect(manager_, SIGNAL(finished(QNetworkReply*)), SLOT(requestFinished(QNetworkReply*)));
	connect(timer_, SIGNAL(timeout()), SLOT(timeout()));
}

void Http::setHost(const QString &host)
{
	url_.setHost(host);
	url_.setScheme("http");
}

QByteArray Http::get(const QString &path)
{
	url_.setPath(path);
	QNetworkRequest request;
	request.setUrl(url_);
	request.setRawHeader("User-Agent", "Juick Plugin (PSI+)");
	manager_->get(request);

	timer_->start(DOWNLOAD_TIMEOUT);
	eloop_->exec();

	return ba_;
}

void Http::timeout()
{
	timer_->stop();
	eloop_->quit();
}

void Http::setProxyHostPort(const QString& host, int port, const QString& username, const QString& pass, const QString& type)
{
	if(host.isEmpty())
		return;

	QNetworkProxy prx;
	prx.setType(QNetworkProxy::HttpCachingProxy);
	if(type == "socks")
		prx.setType(QNetworkProxy::Socks5Proxy);
	prx.setPort(port);
	prx.setHostName(host);
	if(!username.isEmpty()) {
		prx.setUser(username);
		prx.setPassword(pass);
	}
	manager_->setProxy(prx);
}

void Http::requestFinished(QNetworkReply *reply)
{
	if (reply->error() == QNetworkReply::NoError ) {
		ba_ = reply->readAll();
	}
	else
		qDebug() << reply->errorString();

	timer_->stop();
	reply->deleteLater();
	eloop_->quit();
}
