/*
 * CurrentUpload.h
 *
 *  Created on: 24 Sep 2016
 *      Author: rkfg
 */

#ifndef CURRENTUPLOAD_H
#define CURRENTUPLOAD_H

struct CurrentUpload {
    QString from;
    QString to;
    int account;
    QString getUrl;
    QString type;
    QByteArray aesgcmAnchor;
    QString localFilePath;
    CurrentUpload(): account(-1) {}
};

#endif // CURRENTUPLOAD_H
