/*
 * juickdownloader.cpp - plugin
 * Copyright (C) 2012 Evgeny Khryukin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "juickdownloader.h"
#include "applicationinfoaccessinghost.h"
#include "defines.h"

#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QTimer>

static void save(const QString &path, const QByteArray &img)
{
    QFile file(path);

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(img);
    } else
        QMessageBox::warning(nullptr, QObject::tr("Warning"),
                             QObject::tr("Cannot write to file %1:\n%2.").arg(file.fileName(), file.errorString()));
}

JuickDownloader::JuickDownloader(ApplicationInfoAccessingHost *host, QObject *p) :
    QObject(p), inProgress_(false), manager_(new QNetworkAccessManager(this)), appInfo_(host),
    waitTimer_(new QTimer(this))
{
    connect(manager_, &QNetworkAccessManager::finished, this, &JuickDownloader::requestFinished);

    waitTimer_->setSingleShot(true);
    waitTimer_->setInterval(1000);
    connect(waitTimer_, &QTimer::timeout, this, &JuickDownloader::timeOut);

    //    qRegisterMetaType<JuickDownloadItem>("JuickDownloadItem");
}

void JuickDownloader::get(const JuickDownloadItem &item)
{
    if (waitTimer_->isActive())
        waitTimer_->stop();

    items_.enqueue(item);
    Proxy prx = appInfo_->getProxyFor(constPluginName);
    setProxyHostPort(prx.host, prx.port, prx.user, prx.pass, prx.type);
    if (!inProgress_) {
        peekNext();
    }
}

void JuickDownloader::setProxyHostPort(const QString &host, int port, const QString &username, const QString &pass,
                                       const QString &type)
{
    QNetworkProxy prx;

    if (!host.isEmpty()) {
        prx.setType(QNetworkProxy::HttpCachingProxy);
        if (type == "socks")
            prx.setType(QNetworkProxy::Socks5Proxy);
        prx.setPort(port);
        prx.setHostName(host);
        if (!username.isEmpty()) {
            prx.setUser(username);
            prx.setPassword(pass);
        }
    }

    manager_->setProxy(prx);
}

void JuickDownloader::peekNext()
{
    if (items_.isEmpty()) {
        inProgress_ = false;
        waitTimer_->start();
    } else {
        inProgress_          = true;
        JuickDownloadItem it = items_.dequeue();
        QNetworkRequest   request;
        request.setUrl(QUrl(it.url));
        request.setRawHeader("User-Agent", "Juick Plugin (Psi)");
        QNetworkReply *reply = manager_->get(request);
        QVariant       v;
        v.setValue(it);
        reply->setProperty("jdi", v);
    }
}

void JuickDownloader::requestFinished(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray        ba = reply->readAll();
        JuickDownloadItem it = reply->property("jdi").value<JuickDownloadItem>();
        dataReady(ba, it);
    } else {
        qDebug() << reply->errorString();
    }

    reply->deleteLater();
    peekNext();
}

void JuickDownloader::timeOut()
{
    emit finished(urls_);
    urls_.clear();
}

void JuickDownloader::dataReady(const QByteArray &ba, const JuickDownloadItem &it)
{
    urls_.append(QUrl::fromLocalFile(it.path).toEncoded());
    save(it.path, ba);
}
