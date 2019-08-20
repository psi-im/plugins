/*
 * authmanger.cpp - plugin
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

#include "authmanager.h"

#include "common.h"
#include "options.h"
#include "requestauthdialog.h"

#include <QNetworkReply>
#include <QTimer>

AuthManager::AuthManager(QObject* p)
    : QObject(p)
    , authorized_(false)
{
    manager_ = newManager(this);
    connect(manager_, SIGNAL(finished(QNetworkReply*)), SLOT(replyFinished(QNetworkReply*)));

    timer_ = new QTimer(this);
    timer_->setInterval(10000);
    timer_->setSingleShot(true);
    connect(timer_, SIGNAL(timeout()), SLOT(timeout()));

    loop_ = new QEventLoop(this);
}

AuthManager::~AuthManager()
{
    if(timer_->isActive())
        timer_->stop();

    if(loop_->isRunning())
        loop_->exit();
}

bool AuthManager::go(const QString& login, const QString& pass, const QString& captcha)
{
    narodLogin = login;
    narodPass = pass;
    QString narodCaptchaKey = captcha;
    Options *o = Options::instance();

    QByteArray post = "login=" + narodLogin.toLatin1() + "&passwd=" + narodPass.toLatin1();
    if (narodLogin.isEmpty() || narodPass.isEmpty() || !narodCaptchaKey.isEmpty()) {
        requestAuthDialog authdialog;
        authdialog.setLogin(narodLogin);
        authdialog.setPasswd(narodPass);
        if (!narodCaptchaKey.isEmpty()) {
            authdialog.setCaptcha(manager_->cookieJar()->cookiesForUrl(mainUrl),
                          "http://passport.yandex.ru/digits?idkey=" + narodCaptchaKey);
        }
        if (authdialog.exec()) {
            narodLogin = authdialog.getLogin();
            narodPass = authdialog.getPasswd();
            if (authdialog.getRemember()) {
                o->setOption(CONST_LOGIN, narodLogin);
                o->setOption(CONST_PASS, Options::encodePassword(narodPass));
            }
            post = "login=" + narodLogin.toLatin1() + "&passwd=" + narodPass.toLatin1();
        }
        else {
            post.clear();
        }
        if (!post.isEmpty() && !narodCaptchaKey.isEmpty()) {
            post += "&idkey="+narodCaptchaKey.toLatin1()+"&code="+authdialog.getCode();
        }
    }
    if (!post.isEmpty()) {
        post += "&twoweeks=yes";
        QNetworkRequest nr = newRequest();
        nr.setUrl(authUrl);
        nr.setHeader(QNetworkRequest::ContentLengthHeader, post.length());
        nr.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        manager_->post(nr, post);

        if(!loop_->isRunning()) {
            timer_->start();
            loop_->exec();
        }
    }
    else {
        return false;
    }

    return authorized_;
}

QList<QNetworkCookie> AuthManager::cookies() const
{
    QList<QNetworkCookie> ret;
    if(authorized_)
        ret = manager_->cookieJar()->cookiesForUrl(mainUrl);

    return ret;
}

void AuthManager::timeout()
{
    if(loop_->isRunning()) {
        authorized_ = false;
        loop_->exit();
    }
}

void AuthManager::replyFinished(QNetworkReply* reply)
{
    QVariant cooks = reply->header(QNetworkRequest::SetCookieHeader);
    if (!cooks.isNull()) {
        bool found = false;
        foreach (const QNetworkCookie& netcook, qVariantValue< QList<QNetworkCookie> >(cooks)) {
            if (netcook.name() == "yandex_login" && !netcook.value().isEmpty()) {
                found = true;
                break;
            }
        }
        if (!found) {
            QRegExp rx("<input type=\"?submit\"?[^>]+name=\"no\"");
            QString page = reply->readAll();
            if (rx.indexIn(page) > 0) {
                QRegExp rx1("<input type=\"hidden\" name=\"idkey\" value=\"(\\S+)\"[^>]*>");
                if (rx1.indexIn(page) > 0) {
                    QByteArray post = "idkey=" + rx1.cap(1).toAscii() + "&filled=yes";
                    QNetworkRequest nr = newRequest();
                    nr.setUrl(authUrl);
                    nr.setHeader(QNetworkRequest::ContentLengthHeader, post.length());
                    nr.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
                    manager_->post(nr, post);
                    reply->deleteLater();
                    return;
                }
            }
            else {
                rx.setPattern("<input type=\"hidden\" name=\"idkey\" value=\"(\\S+)\" />");
                if (rx.indexIn(page) > 0) {
                    timer_->stop();
                    go(narodLogin, narodPass, rx.cap(1));
                    reply->deleteLater();
                    return;
                }
                else {
                    authorized_ = false;
                    loop_->exit();
                    reply->deleteLater();
                    return;
                }
            }
        }
        else {
            authorized_ = true;
            loop_->exit();
            reply->deleteLater();
            return;
        }
    }

    authorized_ = false;
    loop_->exit();
    reply->deleteLater();
    return;
}
