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

#include "proxy.h"

ProxyManager* ProxyManager::instance_ = 0;


ProxyManager::ProxyManager() :
    QObject()
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

Proxy ProxyManager::getProxy()
{
	return appInfo->getProxyFor("Yandex Narod Plugin");
}

