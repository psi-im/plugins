/*
 * http.h - plugin
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

#ifndef HTTP_H
#define HTTP_H
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QUrl>
#include <QTimer>


class Http: public QObject
{
	Q_OBJECT

public:
	Http(QObject *p = 0);
	virtual ~Http() {};
	QByteArray get(const QString& path);
	void setProxyHostPort(const QString& host, int port, const QString& username = "", const QString& pass = "", const QString& type = "http");
	void setHost(const QString& host);

private slots:
	void requestFinished(QNetworkReply *reply);
	void timeout();

private:
	QNetworkAccessManager *manager_;
	QEventLoop *eloop_;
	QUrl url_;
	QByteArray ba_;
	QTimer *timer_;
}; 


#endif
