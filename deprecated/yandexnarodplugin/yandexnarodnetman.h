/*
 * yandexnarodnetman.h - plugin
 * Copyright (C) 2009  Alexander Kazarin <boiler@co.ru>
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

#ifndef YANDEXNARODNETMAN_H
#define YANDEXNARODNETMAN_H

#include <QStringList>

class QNetworkAccessManager;
class QNetworkReply;

class yandexnarodNetMan : public QObject {
    Q_OBJECT

public:
    yandexnarodNetMan(QObject *parent);
    ~yandexnarodNetMan();

    struct FileItem
    {
        int prolong() const
        {
            int d = 1;
            QRegExp re("(\\d+) \\S+");
            if(re.indexIn(date) != -1) {
                d = re.cap(1).toInt();
            }
            return d;
        }

        QString fileicon;
        QString fileid;
        QString filename;
        QString fileurl;
        QString token;
        QString size;
        QString date;
        QString passtoken;
        bool passset = false;
        bool deleted = false;
    };

    bool startAuth(const QString& login, const QString& pass);
    void startGetFilelist();
    void startDelFiles(const QList<FileItem>& fileItems);
    void startProlongFiles(const QList<FileItem>& fileItems);
    void startSetPass(const FileItem& item);
    void startRemovePass(const FileItem& item);

private:
    enum Actions { NoAction = 0, GetFiles, DeleteFiles, ProlongateFiles, SetPass, RemovePass };

    void netmanDo(QList<FileItem> fileItems = QList<FileItem>());

private slots:
    void netrpFinished(QNetworkReply*);

signals:
    void statusText(const QString&);
    void newFileItem(yandexnarodNetMan::FileItem);
    void finished();

private:
    Actions action;
    QNetworkAccessManager *netman;
};

#endif // YANDEXNARODNETMAN_H
