/*
    yandexnarodPluginSettings

	Copyright (c) 2009 by Alexander Kazarin <boiler@co.ru>

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
*/

#include "yandexnarodsettings.h"
#include "optionaccessinghost.h"

yandexnarodSettings::yandexnarodSettings(OptionAccessingHost *host)
	: psiOptions(host)
{
	ui.setupUi(this);
	ui.labelStatus->setText(NULL);

	restoreSettings();

	connect(ui.btnTest, SIGNAL(clicked()), this,  SLOT(saveSettings()));
	connect(ui.btnTest, SIGNAL(clicked()), this,  SIGNAL(testclick()));
}

yandexnarodSettings::~yandexnarodSettings()
{	
}

void yandexnarodSettings::saveSettings()
{
	psiOptions->setPluginOption(CONST_LOGIN, ui.editLogin->text());
	psiOptions->setPluginOption(CONST_PASS, ui.editPasswd->text());
	psiOptions->setPluginOption(CONST_TEMPLATE,  ui.textTpl->toPlainText());
}

void yandexnarodSettings::setStatus(QString str)
{
	ui.labelStatus->setText(str);
}

void yandexnarodSettings::restoreSettings()
{
	ui.editLogin->setText(psiOptions->getPluginOption(CONST_LOGIN).toString());
	ui.editPasswd->setText(psiOptions->getPluginOption(CONST_PASS).toString());
	ui.textTpl->setText(psiOptions->getPluginOption(CONST_TEMPLATE, QVariant("File sent: %N (%S bytes)\n%U")).toString());
}
