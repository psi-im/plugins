/*
 * authmanager.h - plugin
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

#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <QNetworkCookie>

class QEventLoop;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class AuthManager : public QObject {
    Q_OBJECT
public:
    AuthManager(QObject* p = 0);
    ~AuthManager();

    bool go(const QString& login, const QString& pass, const QString& captcha = "");
    QList<QNetworkCookie> cookies() const;

private slots:
    void timeout();
    void replyFinished(QNetworkReply* r);

private:
    bool authorized_;
    QString narodLogin, narodPass;
    QNetworkAccessManager *manager_;
    QEventLoop *loop_;
    QTimer *timer_;
};

#endif // AUTHMANAGER_H
