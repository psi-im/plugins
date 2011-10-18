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
#include "options.h"
#include "applicationinfoaccessinghost.h"
#include "optionaccessinghost.h"

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
