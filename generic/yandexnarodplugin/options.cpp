/*
    proxy.cpp

	Copyright (c) 2011 by Evgeny Khryukin

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
*/

#include <QApplication>
#include <QByteArray>
#include "options.h"
#include "applicationinfoaccessinghost.h"
#include "optionaccessinghost.h"


static const QString passwordKey = "yandexnarodpluginkey";


Options * Options ::instance_ = 0;


Options ::Options ()
	: QObject(QApplication::instance())
	, appInfo(0)
	, options(0)
{
}

Options ::~Options()
{
}

void Options ::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host)
{
	appInfo = host;
	getProxy();
}

void Options ::setOptionAccessingHost(OptionAccessingHost *host)
{
	options = host;
}

Options * Options ::instance()
{
	if(!instance_)
		instance_ = new Options();

	return instance_;
}

void Options ::reset()
{
	delete instance_;
	instance_ = 0;
}

bool Options ::useProxy() const
{
	bool use = false;
	if(appInfo) {
		Proxy p = appInfo->getProxyFor("Yandex Narod Plugin");
		use = !p.host.isEmpty();
	}

	return use;
}

QNetworkProxy Options ::getProxy() const
{
	QNetworkProxy np;
	if(appInfo) {
		Proxy p = appInfo->getProxyFor("Yandex Narod Plugin");
		np = QNetworkProxy(QNetworkProxy::HttpCachingProxy, p.host, p.port, p.user, p.pass);
		if(p.type != "http")
			np.setType(QNetworkProxy::Socks5Proxy);
	}

	return np;
}

void Options::setOption(const QString &name, const QVariant &value)
{
	if(options) {
		options->setPluginOption(name, value);
	}
}

QVariant Options::getOption(const QString &name, const QVariant &def)
{
	QVariant ret(def);
	if(options) {
		ret = options->getPluginOption(name, def);
	}

	return ret;
}

void Options::saveCookies(const QList<QNetworkCookie> &cooks)
{
	if(options) {
		QByteArray ba;
		QDataStream ds(&ba, QIODevice::WriteOnly);
		foreach(const QNetworkCookie& cookie, cooks) {
			ds << cookie.toRawForm(QNetworkCookie::NameAndValueOnly);
		}
		options->setPluginOption(CONST_COOKIES, ba);
	}
}

QList<QNetworkCookie> Options::loadCookies()
{
	QList<QNetworkCookie> ret;
	if(options) {
		QByteArray ba = options->getPluginOption(CONST_COOKIES, QByteArray()).toByteArray();
		if(!ba.isEmpty()) {
			QDataStream ds(&ba, QIODevice::ReadOnly);
			QByteArray byte;
			while(!ds.atEnd()) {
				ds >> byte;
				QList<QNetworkCookie> list = QNetworkCookie::parseCookies(byte);
				ret += list;
			}
		}
	}

	return ret;
}

QString Options::message(MessageType type)
{
	switch(type) {
	case MAuthStart:
		return tr("Authorizing...");
	case MAuthOk:
		return tr("Authorizing OK");
	case MAuthError:
		return tr("Authorization failed");
	case MCancel:
		return tr("Canceled");
	case MChooseFile:
		return tr("Choose file");
	case MUploading:
		return tr("Uploading");
	case MError:
		return tr("Error! %1");
	case MRemoveCookie:
		return tr("Cookies are removed");
	}

	return QString();
}

QString Options::encodePassword(const QString &pass)
{
	QString result;
	int n1, n2;

	if (passwordKey.length() == 0) {
		return pass;
	}

	for (n1 = 0, n2 = 0; n1 < pass.length(); ++n1) {
		ushort x = pass.at(n1).unicode() ^ passwordKey.at(n2++).unicode();
		QString hex;
		hex.sprintf("%04x", x);
		result += hex;
		if(n2 >= passwordKey.length()) {
			n2 = 0;
		}
	}
	return result;
}

QString Options::decodePassword(const QString &pass)
{
	QString result;
	int n1, n2;

	if (passwordKey.length() == 0) {
		return pass;
	}

	for(n1 = 0, n2 = 0; n1 < pass.length(); n1 += 4) {
		ushort x = 0;
		if(n1 + 4 > pass.length()) {
			break;
		}
		x += QString(pass.at(n1)).toInt(NULL,16)*4096;
		x += QString(pass.at(n1+1)).toInt(NULL,16)*256;
		x += QString(pass.at(n1+2)).toInt(NULL,16)*16;
		x += QString(pass.at(n1+3)).toInt(NULL,16);
		QChar c(x ^ passwordKey.at(n2++).unicode());
		result += c;
		if(n2 >= passwordKey.length()) {
			n2 = 0;
		}
	}
	return result;
}
