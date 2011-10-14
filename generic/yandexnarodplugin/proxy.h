/*
    proxy.h

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

#ifndef PROXY_H
#define PROXY_H

#include <QNetworkProxy>

class ApplicationInfoAccessingHost;

class ProxyManager : public QObject
{
	Q_OBJECT
public:
	static ProxyManager* instance();
	static void reset();
	void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host);
	QNetworkProxy getProxy() const;
	bool useProxy() const;

private:
	static ProxyManager* instance_;
	ProxyManager();
	virtual ~ProxyManager();

	ApplicationInfoAccessingHost* appInfo;
};

#endif // PROXY_H
