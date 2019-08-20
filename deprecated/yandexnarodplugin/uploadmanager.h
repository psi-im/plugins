/*
 * uploadmanager.h - plugin
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

#ifndef UPLOADMANAGER_H
#define UPLOADMANAGER_H

#include <QNetworkCookie>

class HttpDevice;
class QFile;
class QNetworkAccessManager;

class UploadManager : public QObject {
    Q_OBJECT
public:
    UploadManager(QObject* p = 0);
    ~UploadManager();
    void go(const QString& file);
    void setCookies(const QList<QNetworkCookie>& cookies);
    bool success() const { return success_; };

signals:
    void transferProgress(qint64, qint64);
    void uploaded();
    void statusText(const QString&);
    void uploadFileURL(const QString&);

private slots:
    void getStorageFinished();
    void uploadFinished();
    void verifyingFinished();

private:
    void doUpload(const QUrl& url);

private:
    QNetworkAccessManager* manager_;
    QString fileName_;
    bool success_;
    HttpDevice *hd_;
};

#endif // UPLOADMANAGER_H
