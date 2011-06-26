/*
 * loader.h - plugin
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

#ifndef LOADER_H
#define LOADER_H

#include <QNetworkAccessManager>
#include <QNetworkReply>

class Loader : public QObject
{
	Q_OBJECT
public:
	Loader(QString id, QObject *p);
	~Loader();
	void start(const QString& url);
	void setProxy(QString host, int port, QString user = QString(), QString pass = QString());

private slots:
	void onRequestFinish(QNetworkReply*);

signals:
	void error(QString);
	void data(QString, QByteArray);

private:
	QNetworkAccessManager* manager_;
	QString id_;
};

#endif // LOADER_H
