/*
    yandexnarodPlugin

	Copyright (c) 2008-2009 by Alexander Kazarin <boiler@co.ru>
		      2011 Evgeny Khryukin

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
*/

#include "yandexnarod.h"
#include "requestauthdialog.h"
#include "yandexnarodmanage.h"
#include "yandexnarodsettings.h"
#include "uploaddialog.h"
#include "proxy.h"


yandexnarodPlugin::yandexnarodPlugin()
	: psiOptions(0)
	, psiIcons(0)
	, stanzaSender(0)
	, appInfo(0)
	, enabled(false)
	, currentAccount(-1)
{
}

QString yandexnarodPlugin::name() const
{
	return "Yandex Narod Plugin";
}
QString yandexnarodPlugin::shortName() const
{
	return "yandexnarod";
}

QString yandexnarodPlugin::version() const
{
	return VERSION;
}

QWidget* yandexnarodPlugin::options()
{
	if(!enabled) {
		return 0;
	}

	settingswidget = new yandexnarodSettings(psiOptions);
	connect(settingswidget, SIGNAL(testclick()), this,  SLOT(on_btnTest_clicked()));
	return settingswidget;
}

bool yandexnarodPlugin::enable()
{
	enabled = true;
	QFile file(":/icons/yandexnarodplugin.png");
	file.open(QIODevice::ReadOnly);
	QByteArray image = file.readAll();
	psiIcons->addIcon("yandexnarod/logo",image);
	file.close();

	ProxyManager::instance()->setApplicationInfoAccessingHost(appInfo);

	return enabled;
}

bool yandexnarodPlugin::disable()
{
	enabled = false;
	if(manageDialog)
		delete manageDialog;

	if(uploadwidget)
		delete uploadwidget;

	ProxyManager::reset();

	return true;
}

void yandexnarodPlugin::setOptionAccessingHost(OptionAccessingHost* host)
{
	psiOptions = host;
}

void yandexnarodPlugin::setIconFactoryAccessingHost(IconFactoryAccessingHost *host)
{
	psiIcons = host;
}

void yandexnarodPlugin::setStanzaSendingHost(StanzaSendingHost *host)
{
	stanzaSender = host;
}

void yandexnarodPlugin::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host)
{
	appInfo = host;
}

void yandexnarodPlugin::applyOptions()
{
	if(settingswidget)
		settingswidget->saveSettings();
}

void yandexnarodPlugin::restoreOptions()
{
	if(settingswidget)
		settingswidget->restoreSettings();
}

QList < QVariantHash > yandexnarodPlugin::getAccountMenuParam()
{
	QList < QVariantHash > list;
	QVariantHash hash;
	hash["icon"] = QVariant(QString("yandexnarod/logo"));
	hash["name"] = QVariant(tr("Open Yandex Narod Manager"));
	hash["reciver"] = qVariantFromValue(qobject_cast<QObject *>(this));
	hash["slot"] = QVariant(SLOT(manage_clicked()));

	list.append(hash);
	return list;
}

QList < QVariantHash > yandexnarodPlugin::getContactMenuParam()
{
	QList < QVariantHash > list;
	QVariantHash hash;
	hash["icon"] = QVariant(QString("yandexnarod/logo"));
	hash["name"] = QVariant(tr("Send file via Yandex Narod"));
	hash["reciver"] = qVariantFromValue(qobject_cast<QObject *>(this));
	hash["slot"] = QVariant(SLOT(actionStart()));

	list.append(hash);
	return list;
}

void yandexnarodPlugin::manage_clicked()
{
	if(!manageDialog) {
		manageDialog = new yandexnarodManage(psiOptions);
		manageDialog->show();
	}
	else {
		manageDialog->raise();
		manageDialog->activateWindow();
	}
}

void yandexnarodPlugin::on_btnTest_clicked()
{
	if(!settingswidget)
		return;

	yandexnarodNetMan* testnetman = new yandexnarodNetMan(settingswidget, psiOptions);
	connect(testnetman, SIGNAL(statusText(QString)), settingswidget, SLOT(setStatus(QString)));
	connect(testnetman, SIGNAL(finished()), this , SLOT(on_TestFinished()));
	testnetman->startAuthTest(settingswidget->getLogin(), settingswidget->getPasswd());
}

void yandexnarodPlugin::on_TestFinished()
{
	yandexnarodNetMan* testnetman = static_cast<yandexnarodNetMan*>(sender());
	delete testnetman;
	restoreOptions();
}

void yandexnarodPlugin::actionStart()
{
	currentJid = sender()->property("jid").toString();
	currentAccount = sender()->property("account").toInt();
	QString filepath = QFileDialog::getOpenFileName(uploadwidget, tr("Choose file"),
							psiOptions->getPluginOption(CONST_LAST_FOLDER).toString());

	if (filepath.length() > 0) {
		uploadwidget = new uploadDialog();
		//connect(uploadwidget, SIGNAL(canceled()), uploadwidget, SLOT(close()));
		uploadwidget->show();
		uploadwidget->start();

		fi = QFileInfo(filepath);
		psiOptions->setPluginOption(CONST_LAST_FOLDER, fi.dir().path());

		yandexnarodNetMan* netman = new yandexnarodNetMan(uploadwidget, psiOptions);
		connect(netman, SIGNAL(statusText(QString)), uploadwidget, SLOT(setStatus(QString)));
		connect(netman, SIGNAL(statusFileName(QString)), uploadwidget, SLOT(setFilename(QString)));
		connect(netman, SIGNAL(transferProgress(qint64,qint64)), uploadwidget, SLOT(progress(qint64,qint64)));
		connect(netman, SIGNAL(uploadFileURL(QString)), this, SLOT(onFileURL(QString)));
		connect(netman, SIGNAL(uploadFinished()), uploadwidget, SLOT(setDone()));
		netman->startUploadFile(filepath);
	}
}

void yandexnarodPlugin::onFileURL(const QString& url)
{
	QString sendmsg = psiOptions->getPluginOption(CONST_TEMPLATE).toString();
	sendmsg.replace("%N", fi.fileName());
	sendmsg.replace("%U", url);
	sendmsg.replace("%S", QString::number(fi.size()));
	uploadwidget->deleteLater();

	if(currentAccount != -1 && !currentJid.isEmpty()) {
		stanzaSender->sendMessage(currentAccount, currentJid, stanzaSender->escape(sendmsg), "", "chat");
	}

	currentJid.clear();
	currentAccount = -1;
}

QString yandexnarodPlugin::pluginInfo()
{
	return tr("Ported from QutIM Yandex.Narod plugin\nhttp://qutim.org/forum/viewtopic.php?f=62&t=711");
}

Q_EXPORT_PLUGIN(yandexnarodPlugin);
