/*
    uploadmanger.h

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

#ifndef UPLOADMANAGER_H
#define UPLOADMANAGER_H


#include <QNetworkCookie>

class QNetworkAccessManager;
class QFile;
class HttpDevice;

class UploadManager : public QObject
{
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
