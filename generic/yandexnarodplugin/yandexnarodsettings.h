/*
    yandexnarodSettings

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

#ifndef YANDEXNARODSETTINGS_H
#define YANDEXNARODSETTINGS_H

#include "ui_yandexnarodsettings.h"

#define CONST_LOGIN "login"
#define CONST_PASS "pass"
#define CONST_TEMPLATE "template"
#define CONST_LAST_FOLDER "lastfolder"
#define VERSION "0.0.4"

class yandexnarodSettings  : public QWidget
{
	Q_OBJECT;

public:
	yandexnarodSettings(QWidget *p = 0);
	~yandexnarodSettings();
	QString getLogin() const { return ui.editLogin->text(); }
	QString getPasswd() const { return ui.editPasswd->text(); }
	void btnTest_enabled(bool b) { ui.btnTest->setEnabled(b); }
	void restoreSettings();

public slots:
	void setStatus(const QString& str);
	void saveSettings();

private:
	Ui::yandexnarodSettingsClass ui;


signals:
	void testclick();
	void startManager();

};
#endif
