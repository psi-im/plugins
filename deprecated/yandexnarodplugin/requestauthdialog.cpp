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
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include "requestauthdialog.h"
#include "options.h"

requestAuthDialog::requestAuthDialog(QWidget *parent)
    : QDialog(parent)
    , manager_(0)
{
    ui.setupUi(this);
    setFixedHeight(210);
    ui.frameCaptcha->hide();
    setFixedSize(size());
}


requestAuthDialog::~requestAuthDialog()
{
    
}

void requestAuthDialog::setCaptcha(const QList<QNetworkCookie> &cooks, const QString &url)
{
    if(!manager_) {
        manager_ = new QNetworkAccessManager(this);
        if(Options::instance()->useProxy())
            manager_->setProxy(Options::instance()->getProxy());

        connect(manager_, SIGNAL(finished(QNetworkReply*)), SLOT(reply(QNetworkReply*)));
    }
    manager_->cookieJar()->setCookiesFromUrl(cooks, url);
    manager_->get(QNetworkRequest(QUrl(url)));
}

void requestAuthDialog::reply(QNetworkReply *r)
{
    if(r->error() == QNetworkReply::NoError) {
        ui.frameCaptcha->show();
        ui.labelCaptcha->show();
        QPixmap pix = QPixmap::fromImage(QImage::fromData(r->readAll()));
        ui.webCaptcha->setPixmap(pix);
        setFixedHeight(350);
        setFixedSize(size());
    }

    r->deleteLater();
}
