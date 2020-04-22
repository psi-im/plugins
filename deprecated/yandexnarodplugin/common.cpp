/*
    common.cpp

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

#include "common.h"

#include "options.h"
#include <QNetworkAccessManager>

QNetworkRequest newRequest()
{
    QNetworkRequest nr;
    nr.setRawHeader("Cache-Control", "no-cache");
    nr.setRawHeader("Accept", "*/*");
    nr.setRawHeader("User-Agent", "PsiPlus/0.15 (U; YB/4.2.0; MRA/5.5; en)");
    return nr;
}

QNetworkAccessManager *newManager(QObject *parent)
{
    QNetworkAccessManager *netman = new QNetworkAccessManager(parent);
    if (Options::instance()->useProxy()) {
        netman->setProxy(Options::instance()->getProxy());
    }

    return netman;
}
