/*
 * requestauthdialog.cpp - plugin
 * Copyright (C) 2008-2009  Alexander Kazarin <boiler@co.ru>
 * Copyright (C) 2011  Evgeny Khryukin
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "requestauthdialog.h"

#include "options.h"

#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QNetworkRequest>

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
