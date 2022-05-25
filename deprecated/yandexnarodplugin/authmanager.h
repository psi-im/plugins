/*
    authmanger.h

    Copyright (c) 2011 by Evgeny Khryukin

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
*/

#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <QNetworkCookie>

class QNetworkReply;
class QNetworkAccessManager;
class QEventLoop;
class QTimer;

class AuthManager : public QObject {
    Q_OBJECT
public:
    AuthManager(QObject *p = 0);
    ~AuthManager();

    bool                  go(const QString &login, const QString &pass, const QString &captcha = "");
    QList<QNetworkCookie> cookies() const;

private slots:
    void timeout();
    void replyFinished(QNetworkReply *r);

private:
    bool                   authorized_;
    QString                narodLogin, narodPass;
    QNetworkAccessManager *manager_;
    QEventLoop            *loop_;
    QTimer                *timer_;
};

#endif // AUTHMANAGER_H
