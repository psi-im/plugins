/*
 * controller.cpp - plugin
 * Copyright (C) 2011  Khryukin Evgeny
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

#include "controller.h"

#include "screenshot.h"
#include "server.h"
#include "options.h"
#include "iconset.h"
#include "defines.h"
#include "applicationinfoaccessinghost.h"

static const QString imageShack = "ImageShack.us&split&http://post.imageshack.us/&split&&split&&split&uploadtype=on&split&fileupload&split&readonly=\"readonly\" class=\"readonly\" value=\"(http://[^\"]+)\" /><span &split&true";
static const QString radikal = "Radikal.ru&split&http://www.radikal.ru/action.aspx&split&&split&&split&upload=yes&split&F&split&<input\\s+id=\"input_link_1\"\\s+value=\"([^\"]+)\"&split&true";
static const QString pixacadem = "Pix.Academ.org&split&http://pix.academ.org/&split&&split&&split&action=upload_image&split&image&split&<div id='link'><a id=\"original\" href=\"(http[^\"]+)\"&split&true";
static const QString kachalka = "Kachalka.com&split&http://www.kachalka.com/upload.php&split&&split&&split&&split&userfile[]&split&name=\"option\" value=\"(http://www.kachalka.com/[^\"]+)\" /></td>&split&true";
static const QString flashtux = "Img.Flashtux.org&split&http://img.flashtux.org/index.php&split&&split&&split&postimg=1&split&filename&split&<br />Link: <a href=\"(http://img.flashtux.org/[^\"]+)\">&split&true";
static const QString smages = "Smages.com&split&http://smages.com/&split&&split&&split&&split&fileup&split&<div class=\"codex\"><a href=\"(http://smages.com/[^\"]+)\" target=\"_blank\">URL:</a></div>&split&true";
static const QString ompldr = "Ompldr.org&split&http://ompldr.org/upload&split&&split&&split&&split&file1&split&<div class=\"left\">File:</div><div class=\"right\"><a href=\"[^\"]+\">(http://ompldr.org/[^\"]+)</a></div>&split&true";
static const QString ipicture = "Ipicture.ru&split&http://ipicture.ru/Upload/&split&&split&&split&method=file&split&userfile&split&value=\"(http://[^\"]+)\">&split&true";

static const QStringList staticHostsList = QStringList() << imageShack << pixacadem << radikal
					 << kachalka << flashtux << smages << ompldr << ipicture;


static bool isListContainsServer(const QString& server, const QStringList& servers)
{
	foreach(QString serv, servers) {
		if(serv.split(Server::splitString()).first() == server.split(Server::splitString()).first())
			return true;
	}
	return false;
}

static void updateServer(QStringList *const servers, const QString& serv)
{
	QStringList::iterator it = servers->begin();
	while(++it != servers->end()) {
		const QStringList tmpOld = (*it).split(Server::splitString());
		const QStringList tmpNew = serv.split(Server::splitString());
		if(tmpOld.first() == tmpNew.first()) {
			*it = serv;
		}
	}
}


Controller::Controller(ApplicationInfoAccessingHost* appInfo)
	: QObject()
	, appInfo_(appInfo)
{
	Options* o = Options::instance();
	QVariant vServers = o->getOption(constServerList);

	if(!vServers.isValid()) { //приложение запущено впервые
		o->setOption(constShortCut, QVariant("Alt+Shift+p"));
		o->setOption(constFormat, QVariant("png"));
		o->setOption(constFileName, QVariant("pic-yyyyMMdd-hhmmss"));
		o->setOption(constDelay, QVariant(0));
		o->setOption(constVersionOption, cVersion);
		o->setOption(constDefaultAction, QVariant(Desktop));
	}

	QStringList servers = vServers.toStringList();
	foreach(const QString& host, staticHostsList) {
		if(!isListContainsServer(host, servers))
			servers.append(host);
	}

	if(o->getOption(constVersionOption).toString() != cVersion) {
		foreach(const QString& host, staticHostsList) {
			updateServer(&servers, host);
		}

		doUpdate();
		o->setOption(constVersionOption, cVersion);
	}

	o->setOption(constServerList, servers); //сохраняем обновленный список серверов
}

Controller::~Controller()
{
	if (screenshot) {
		delete screenshot;
	}

	Options::reset();
	Iconset::reset();
}


void Controller::onShortCutActivated()
{
	if(!screenshot) {
		screenshot = new Screenshot();
		screenshot->setProxy(appInfo_->getProxyFor(constName));
	}

	screenshot->action(Options::instance()->getOption(constDefaultAction).toInt());
}

void Controller::openImage()
{
	if(!screenshot) {
		screenshot = new Screenshot();
		screenshot->setProxy(appInfo_->getProxyFor(constName));
	}

	screenshot->openImage();
}

void Controller::doUpdate()
{
	// do some updates
}
