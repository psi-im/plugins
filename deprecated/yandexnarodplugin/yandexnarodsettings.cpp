/*
    yandexnarodPluginSettings

    Copyright (c) 2009 by Alexander Kazarin <boiler@co.ru>
            2011 by Evgeny Khryukin

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
#include "options.h"

yandexnarodSettings::yandexnarodSettings(QWidget *p) : QWidget(p)
{
    ui.setupUi(this);
    ui.labelStatus->setText(NULL);

    restoreSettings();

    connect(ui.btnTest, SIGNAL(clicked()), this, SLOT(saveSettings()));
    connect(ui.btnTest, SIGNAL(clicked()), this, SIGNAL(testclick()));
    connect(ui.pb_startManager, SIGNAL(clicked()), this, SIGNAL(startManager()));
}

yandexnarodSettings::~yandexnarodSettings() { }

void yandexnarodSettings::saveSettings()
{
    Options *o = Options::instance();
    o->setOption(CONST_LOGIN, ui.editLogin->text());
    o->setOption(CONST_PASS, Options::encodePassword(ui.editPasswd->text()));
    o->setOption(CONST_TEMPLATE, ui.textTpl->toPlainText());
}

void yandexnarodSettings::setStatus(const QString &str) { ui.labelStatus->setText(str); }

void yandexnarodSettings::restoreSettings()
{
    Options *o = Options::instance();
    ui.editLogin->setText(o->getOption(CONST_LOGIN).toString());
    ui.editPasswd->setText(Options::decodePassword(o->getOption(CONST_PASS).toString()));
    ui.textTpl->setText(o->getOption(CONST_TEMPLATE, QVariant("File sent: %N (%S bytes)\n%U")).toString());
}

void yandexnarodSettings::on_btnClearCookies_clicked()
{
    Options::instance()->saveCookies(QList<QNetworkCookie>());
    setStatus(O_M(MRemoveCookie));
}
