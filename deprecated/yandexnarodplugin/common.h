/*
    common.h

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

#ifndef COMMON_H
#define COMMON_H

#include <QNetworkRequest>
class QNetworkAccessManager;

const QUrl mainUrl = QUrl("http://narod.yandex.ru");
const QUrl authUrl = QUrl("http://passport.yandex.ru/passport?mode=auth");

QNetworkRequest        newRequest();
QNetworkAccessManager *newManager(QObject *parent);

#endif // COMMON_H
