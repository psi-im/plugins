/*
    uploadmanger.cpp

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
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QBuffer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "uploadmanager.h"
#include "common.h"
#include "options.h"
#include "authmanager.h"

static const QString boundary = "AaB03x";


//---------------------------------
//-------HttpDevice----------------
//---------------------------------
class HttpDevice : public QIODevice
{
public:
	HttpDevice(const QString& fileName, QObject *p = 0)
		: QIODevice(p)
		, totalSize(0)
		, ioIndex(0)
		, lastIndex(0)
		, fileName_(fileName)
	{
		QFileInfo fi(fileName_);
		QByteArray mpData;
		mpData.append("--" + boundary + "\r\n");
		mpData.append("Content-Disposition: form-data; name=\"file\"; filename=\"" + fi.fileName().toUtf8() + "\"\r\n");
		mpData.append("Content-Transfer-Encoding: binary\r\n");
		mpData.append("\r\n");

		appendData(mpData);

		QFile *device = new QFile(fileName_, this);

		Range r;
		r.start = totalSize;
		r.end = totalSize + device->size() - 1;

		ioDevices.append(QPair<Range, QIODevice *>(r, device));
		totalSize += device->size();

		appendData("\r\n--" + boundary.toLatin1() + "--\r\n");
	}

	~HttpDevice()
	{
	}

	virtual qint64 size() const
	{
		return totalSize;
	}

	virtual bool seek(qint64 pos)
	{
		if(pos >= totalSize)
			return false;
		ioIndex = pos;
		lastIndex = 0;
		return QIODevice::seek(pos);
	}

	virtual bool open(QIODevice::OpenMode mode)
	{
		if(mode != QIODevice::ReadOnly)
			return false;

		for(int i = 0; i < ioDevices.size(); i++) {
			if(!ioDevices.at(i).second->open(mode))
				return false;
		}

		return QIODevice::open(mode);
	}

protected:
	virtual qint64 readData(char *data, qint64 len)
	{
		if ((len = qMin(len, qint64(totalSize) - ioIndex)) <= 0)
			return qint64(0);

		qint64 totalRead = 0;

		while(len > 0)
		{
			if( ioIndex >= ioDevices[lastIndex].first.start &&
			    ioIndex <= ioDevices[lastIndex].first.end )
			{

			} else {
				for(int i = 0 ; i < ioDevices.count() ; ++i)
				{
					if( ioIndex >= ioDevices[i].first.start &&
					    ioIndex <= ioDevices[i].first.end )
					{
						lastIndex = i;
					}
				}
			}

			QIODevice * chunk = ioDevices[lastIndex].second;

			if(!ioDevices[lastIndex].second->seek(ioIndex - ioDevices[lastIndex].first.start))
			{
				qDebug("HttpDevice: Failed to seek inner device");
				break;
			}

			qint64 bytesLeftInThisChunk = chunk->size() - chunk->pos();
			qint64 bytesToReadInThisRequest = qMin(bytesLeftInThisChunk, len);

			qint64 readLen = chunk->read(data, bytesToReadInThisRequest);
			if( readLen != bytesToReadInThisRequest ) {
				qDebug("HttpDevice: Failed to read requested amount of data");
				break;
			}


			data += bytesToReadInThisRequest;
			len -= bytesToReadInThisRequest;
			totalRead += bytesToReadInThisRequest;
			ioIndex += bytesToReadInThisRequest;
		}

		return totalRead;
	}

	virtual qint64 writeData(const char */*data*/, qint64 /*len*/)
	{
		return -1;
	}

private:
	struct Range {
		int start;
		int end;
	};

	void appendData(const QByteArray& data)
	{
		QBuffer * buffer = new QBuffer(this);
		buffer->setData(data);

		Range r;
		r.start = totalSize;
		r.end = totalSize + data.size() - 1;

		ioDevices.append(QPair<Range, QIODevice *>(r, buffer));
		totalSize += data.size();
	}

private:
	QVector< QPair<Range, QIODevice *> > ioDevices;
	int totalSize;
	qint64 ioIndex;
	int lastIndex;
	QString fileName_;
};




//-----------------------------------------
//-------UploadManager---------------------
//-----------------------------------------
UploadManager::UploadManager(QObject* p)
	: QObject(p)
	, success_(false)
	, hd_(0)
{
	manager_ = newManager(this);
}

UploadManager::~UploadManager()
{
}

void UploadManager::go(const QString& file)
{
	if (file.isEmpty()) {
		emit statusText(O_M(MCancel));
		emit uploaded();
		return;
	}

	//manager_->cookieJar()->setCookiesFromUrl(Options::instance()->loadCookies(), mainUrl);

	if(manager_->cookieJar()->cookiesForUrl(mainUrl).isEmpty()) {
		AuthManager am;
		emit statusText(O_M(MAuthStart));
		bool auth = am.go(Options::instance()->getOption(CONST_LOGIN, "").toString(),
				  Options::decodePassword(Options::instance()->getOption(CONST_PASS, "").toString()) );
		if(auth) {
			setCookies(am.cookies());
			Options::instance()->saveCookies(am.cookies());
			emit statusText(O_M(MAuthOk));
		}
		else {
			emit statusText(O_M(MAuthError));
			emit uploaded();
			return;
		}
	}

	fileName_ = file;
	QNetworkRequest nr = newRequest();
	nr.setUrl(QUrl("http://narod.yandex.ru/disk/getstorage/"));
	emit statusText(tr("Getting storage..."));
	QNetworkReply* reply = manager_->get(nr);
	connect(reply, SIGNAL(finished()), SLOT(getStorageFinished()));
}

void UploadManager::setCookies(const QList<QNetworkCookie>& cookies)
{
	manager_->cookieJar()->setCookiesFromUrl(cookies, mainUrl);
}

void UploadManager::getStorageFinished()
{
	QNetworkReply *reply = static_cast<QNetworkReply*>(sender());
	if(reply->error() == QNetworkReply::NoError) {
		QString page = reply->readAll();
		QRegExp rx("\"url\":\"(\\S+)\".+\"hash\":\"(\\S+)\".+\"purl\":\"(\\S+)\"");
		if (rx.indexIn(page) > -1) {
			doUpload(QUrl(rx.cap(1) + "?tid=" + rx.cap(2)));
		}
		else {
			emit statusText(tr("Can't get storage"));
			emit uploaded();
		}
	}
	else {
		emit statusText(O_M(MError).arg(reply->errorString()));
		emit uploaded();
	}

	reply->deleteLater();
}

void UploadManager::doUpload(const QUrl &url)
{
	emit statusText(tr("Starting upload..."));

	hd_ = new HttpDevice(fileName_, this);
	if(!hd_->open(QIODevice::ReadOnly)) {
		       emit statusText(tr("Error opening file!"));
		       emit uploaded();
		       return;
	}

	QNetworkRequest nr = newRequest();
	nr.setUrl(url);
	nr.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/form-data, boundary=" + boundary.toLatin1());
	nr.setHeader(QNetworkRequest::ContentLengthHeader, hd_->size());

	QNetworkReply* netrp = manager_->post(nr, hd_);
	connect(netrp, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(transferProgress(qint64,qint64)));
	connect(netrp, SIGNAL(finished()), this, SLOT(uploadFinished()));
}

void UploadManager::uploadFinished()
{
	QNetworkReply *reply = static_cast<QNetworkReply*>(sender());
	if(reply->error() == QNetworkReply::NoError) {
		emit statusText(tr("Verifying..."));
		QNetworkRequest nr = newRequest();
		nr.setUrl(QUrl("http://narod.yandex.ru/disk/last/"));
		QNetworkReply* netrp = manager_->get(nr);
		connect(netrp, SIGNAL(finished()), SLOT(verifyingFinished()));
	}
	else {
		emit statusText(O_M(MError).arg(reply->errorString()));
		emit uploaded();
	}

	hd_->deleteLater();
	hd_ = 0;
	reply->deleteLater();
}

void UploadManager::verifyingFinished()
{
	QNetworkReply *reply = static_cast<QNetworkReply*>(sender());
	if(reply->error() == QNetworkReply::NoError) {
		QString page = reply->readAll();
		QRegExp rx("<span class='b-fname'><a href=\"(http://narod.ru/disk/\\S+html)\">[^<]+</a></span><br/>");
		if (rx.indexIn(page) != -1) {
			success_ = true;
			emit statusText(tr("Uploaded successfully"));
			QString url = rx.cap(1);
			emit uploadFileURL(url);
		}
		else {
			emit statusText(tr("Verifying failed"));
		}
	}
	else {
		emit statusText(O_M(MError).arg(reply->errorString()));
	}

	emit uploaded();

	reply->deleteLater();
}

//#include "uploadmanager.moc"
