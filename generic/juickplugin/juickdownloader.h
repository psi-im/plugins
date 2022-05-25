/*
 * juickdownloader.h - plugin
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

#ifndef JUICKDOWNLOADER_H
#define JUICKDOWNLOADER_H

#include <QQueue>
#include <QVariant>

class QNetworkAccessManager;
class QNetworkReply;
class ApplicationInfoAccessingHost;
class QTimer;

struct JuickDownloadItem {
    JuickDownloadItem() { } // need for Q_DECLARE_METATYPE
    JuickDownloadItem(const QString &_path, const QString &_url) : path(_path), url(_url) { }

    QString path;
    QString url;
};

Q_DECLARE_METATYPE(JuickDownloadItem)

class JuickDownloader : public QObject {
    Q_OBJECT

public:
    JuickDownloader(ApplicationInfoAccessingHost *host, QObject *p = nullptr);
    ~JuickDownloader() { }

    void get(const JuickDownloadItem &item);

private slots:
    void requestFinished(QNetworkReply *reply);
    void timeOut();

signals:
    void finished(const QList<QByteArray> &urls);

private:
    void dataReady(const QByteArray &ba, const JuickDownloadItem &it);
    void setProxyHostPort(const QString &host, int port, const QString &username = "", const QString &pass = "",
                          const QString &type = "http");
    void peekNext();

private:
    bool                          inProgress_;
    QNetworkAccessManager        *manager_;
    ApplicationInfoAccessingHost *appInfo_;
    QQueue<JuickDownloadItem>     items_;
    QList<QByteArray>             urls_;
    QTimer                       *waitTimer_;
};

#endif
