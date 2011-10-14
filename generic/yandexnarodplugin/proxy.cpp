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
#include "proxy.h"
#include "applicationinfoaccessinghost.h"

ProxyManager* ProxyManager::instance_ = 0;


ProxyManager::ProxyManager()
	: QObject(QApplication::instance())
	, appInfo(0)
{
}

ProxyManager::~ProxyManager()
{
}

void ProxyManager::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host)
{
	appInfo = host;
	getProxy();
}

ProxyManager* ProxyManager::instance()
{
	if(!instance_)
		instance_ = new ProxyManager();

	return instance_;
}

void ProxyManager::reset()
{
	delete instance_;
	instance_ = 0;
}

bool ProxyManager::useProxy() const
{
	bool use = false;
	if(appInfo) {
		Proxy p = appInfo->getProxyFor("Yandex Narod Plugin");
		use = !p.host.isEmpty();
	}

	return use;
}

QNetworkProxy ProxyManager::getProxy() const
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

