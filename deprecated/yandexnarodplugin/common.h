/*
 * common.h - plugin
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

#ifndef COMMON_H
#define COMMON_H

#include <QNetworkRequest>

class QNetworkAccessManager;

const QUrl mainUrl = QUrl("http://narod.yandex.ru");
const QUrl authUrl = QUrl("http://passport.yandex.ru/passport?mode=auth");

QNetworkRequest newRequest();
QNetworkAccessManager* newManager(QObject* parent);

#endif // COMMON_H
