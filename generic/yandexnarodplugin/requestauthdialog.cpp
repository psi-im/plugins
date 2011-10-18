/*
    requestAuthDialog

    Copyright (c) 2008-2009 by Alexander Kazarin <boiler@co.ru>
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

#include <QNetworkCookieJar>
#include "requestauthdialog.h"
#include "options.h"

requestAuthDialog::requestAuthDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	setFixedHeight(210);
	ui.frameCaptcha->hide();
	if(Options::instance()->useProxy())
		ui.webCaptcha->page()->networkAccessManager()->setProxy(Options::instance()->getProxy());

	setFixedSize(size());
}


requestAuthDialog::~requestAuthDialog()
{
	
}

void requestAuthDialog::setCaptcha(const QList<QNetworkCookie> &cooks, const QString &url)
{
	ui.frameCaptcha->show();
	ui.webCaptcha->page()->networkAccessManager()->cookieJar()->setCookiesFromUrl(cooks, url);
	ui.webCaptcha->setHtml("<img src=\"" + url + "\" />");
	ui.labelCaptcha->show();
	setFixedHeight(350);
	setFixedSize(size());
}
