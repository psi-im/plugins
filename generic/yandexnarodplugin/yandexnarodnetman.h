/*
	yandexnarodNetMan

	Copyright (c) 2009 by Alexander Kazarin <boiler@co.ru>

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
*/

#ifndef YANDEXNARODNETMAN_H
#define YANDEXNARODNETMAN_H

#include <QNetworkCookie>
#include <QStringList>

class QNetworkAccessManager;
class QNetworkReply;
class QEventLoop;
class QTimer;


class AuthManager : public QObject
{
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
	void uploadFileURL(QString);

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
};

class yandexnarodNetMan : public QObject
{
	Q_OBJECT

public:
	yandexnarodNetMan(QObject *parent);
	~yandexnarodNetMan();

	struct FileItem
	{
		QString fileicon;
		QString fileid;
		QString filename;
		QString fileurl;
		QString token;
		QString size;
		QString date;
	};

	bool startAuth(const QString& login, const QString& pass);
	void startGetFilelist();
	void startDelFiles(const QList<FileItem>& fileItems);
	void startProlongFiles(const QList<FileItem>& fileItems);

private:
	void netmanDo();

	QString action;
	QNetworkAccessManager *netman;
	int nstep;
	//int filesnum;
	QList<FileItem> fileItems;
	//QStringList fileids;


private slots:
	void netrpFinished(QNetworkReply*);

signals:
	void statusText(const QString&);
	void newFileItem(yandexnarodNetMan::FileItem);
	void finished();
};

#endif // YANDEXNARODNETMAN_H
