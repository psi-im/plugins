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

#include <QtNetwork>
#include <QObject>
#include <QFileDialog>

class QNetworkAccessManager;
class QNetworkRequest;
class QNetworkReply;
class OptionAccessingHost;

#define CONST_COOKIES_NAMES "cookiesnames"
#define CONST_COOKIES_VALUES "cookiesvalues"

class yandexnarodNetMan : public QObject
{
	Q_OBJECT

public:
	yandexnarodNetMan(QObject *parent, OptionAccessingHost* host);
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

	void setFilepath (QString arg) { filepath = arg; }
	void startAuthTest(QString, QString);
	void startGetFilelist();
	void startDelFiles(QList<FileItem> fileItems);
	void startProlongFiles(QList<FileItem> fileItems);
	void startUploadFile(QString);

private:
	void netmanDo();
	QString narodCaptchaKey;
	QString action;
	QString page;
	QNetworkAccessManager *netman;
	QNetworkRequest netreq;
	int nstep;
	int filesnum;
	QString purl;
	QList<FileItem> fileItems;
	QStringList fileids;
	QString filepath;
	QString lastdir;
	QFileInfo fi;
	void loadSettings();
//	void loadCookies();
//	void saveCookies();
	QString narodLogin;
	QString narodPasswd;
	int auth_flag;
	OptionAccessingHost* psiOptions;

private slots:
	void netrpFinished(QNetworkReply*);
//	void netmanTransferProgress(qint64, qint64);

signals:
	void statusText(QString);
	void statusFileName(QString);
	void progressMax(int);
	void progressValue(int);
	void newFileItem(yandexnarodNetMan::FileItem);
	void uploadFileURL(QString);
	void transferProgress(qint64, qint64);
	void uploadFinished();
	void finished();
};

#endif // YANDEXNARODNETMAN_H
