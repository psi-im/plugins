/*
	yandexnarodNetMan

	Copyright (c) 2008-2009 by Alexander Kazarin <boiler@co.ru>

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
*/

#include <QFileDialog>
#include <QNetworkRequest>
#include <QTimer>
#include <QNetworkReply>

#include "yandexnarodnetman.h"
#include "requestauthdialog.h"
#include "optionaccessinghost.h"
#include "yandexnarodsettings.h"
#include "options.h"


static const QUrl mainUrl = QUrl("http://narod.yandex.ru");
static const QUrl authUrl = QUrl("http://passport.yandex.ru/passport?mode=auth");

static QNetworkRequest newRequest()
{
	QNetworkRequest nr;
	nr.setRawHeader("Cache-Control", "no-cache");
	nr.setRawHeader("Accept", "*/*");
	nr.setRawHeader("User-Agent", "PsiPlus/0.15 (U; YB/4.2.0; MRA/5.5; en)");
	return nr;
}

static QNetworkAccessManager* newMnager(QObject* parent)
{
	QNetworkAccessManager* netman = new QNetworkAccessManager(parent);
	if(Options::instance()->useProxy()) {
		netman->setProxy(Options::instance()->getProxy());
	}

	return netman;
}

//-------------------------------------------
//------AuthManager--------------------------
//-------------------------------------------

AuthManager::AuthManager(QObject* p)
	: QObject(p)
	, authorized_(false)
{
	manager_ = newMnager(this);
	connect(manager_, SIGNAL(finished(QNetworkReply*)), SLOT(replyFinished(QNetworkReply*)));

	timer_ = new QTimer(this);
	timer_->setInterval(5000);
	timer_->setSingleShot(true);
	connect(timer_, SIGNAL(timeout()), SLOT(timeout()));

	loop_ = new QEventLoop(this);
}

AuthManager::~AuthManager()
{
	if(timer_->isActive())
		timer_->stop();

	if(loop_->isRunning())
		loop_->exit();
}

bool AuthManager::go(const QString& login, const QString& pass, const QString& captcha)
{
	narodLogin = login;
	narodPass = pass;
	QString narodCaptchaKey = captcha;
	Options *o = Options::instance();

	QByteArray post = "login=" + narodLogin.toLatin1() + "&passwd=" + narodPass.toLatin1();
	if (narodLogin.isEmpty() || narodPass.isEmpty() || !narodCaptchaKey.isEmpty()) {
		requestAuthDialog authdialog;
		authdialog.setLogin(narodLogin);
		authdialog.setPasswd(narodPass);
		if (!narodCaptchaKey.isEmpty()) {
			authdialog.setCaptcha(manager_->cookieJar()->cookiesForUrl(mainUrl),
					      "http://passport.yandex.ru/digits?idkey=" + narodCaptchaKey);
		}
		if (authdialog.exec()) {
			narodLogin = authdialog.getLogin();
			narodPass = authdialog.getPasswd();
			if (authdialog.getRemember()) {
				o->setOption(CONST_LOGIN, narodLogin);
				o->setOption(CONST_PASS, narodPass);
			}
			post = "login=" + narodLogin.toLatin1() + "&passwd=" + narodPass.toLatin1();
		}
		else {
			post.clear();
		}
		if (!post.isEmpty() && !narodCaptchaKey.isEmpty()) {
			post += "&idkey="+narodCaptchaKey.toLatin1()+"&code="+authdialog.getCode();
		}
	}
	if (!post.isEmpty()) {
		QNetworkRequest nr = newRequest();
		nr.setUrl(authUrl);
		manager_->post(nr, post);

		if(!loop_->isRunning()) {
			timer_->start();
			loop_->exec();
		}
	}
	else {
		return false;
	}

	return authorized_;
}

QList<QNetworkCookie> AuthManager::cookies() const
{
	QList<QNetworkCookie> ret;
	if(authorized_)
		ret = manager_->cookieJar()->cookiesForUrl(mainUrl);

	return ret;
}


void AuthManager::timeout()
{
	if(loop_->isRunning()) {
		authorized_ = false;
		loop_->exit();
	}
}

void AuthManager::replyFinished(QNetworkReply* reply)
{
	QString replycookstr = reply->rawHeader("Set-Cookie");
	if (!replycookstr.isEmpty()) {
		QNetworkCookieJar *netcookjar = manager_->cookieJar();
		QList<QNetworkCookie> cooks = netcookjar->cookiesForUrl(mainUrl);
		bool found = false;
		foreach (QNetworkCookie netcook, cooks) {
			if (netcook.name() == "yandex_login" && !netcook.value().isEmpty())
				found = true;
		}
		if (!found) {
			QRegExp rx("<input type=\"?submit\"?[^>]+name=\"no\"");
			QString page = reply->readAll();
			if (rx.indexIn(page) > 0) {
				QRegExp rx1("<input type=\"hidden\" name=\"idkey\" value=\"(\\S+)\"[^>]*>");
				if (rx1.indexIn(page) > 0) {
					QByteArray post = "idkey="+rx1.cap(1).toAscii()+"&no=no";
					QNetworkRequest nr = newRequest();
					nr.setUrl(authUrl);
					manager_->post(nr, post);
					reply->deleteLater();
					return;
				}
			}
			else {
				rx.setPattern("<input type=\"hidden\" name=\"idkey\" value=\"(\\S+)\" />");
				if (rx.indexIn(page) > 0) {
					timer_->stop();
					go(narodLogin, narodPass, rx.cap(1));
					reply->deleteLater();
					return;
				}
				else {
					authorized_ = false;
					loop_->exit();
					reply->deleteLater();
					return;
				}
			}
		}
		else {
			authorized_ = true;
			loop_->exit();
			reply->deleteLater();
			return;
		}
	}

	authorized_ = false;
	loop_->exit();
	reply->deleteLater();
	return;
}





//-----------------------------------------
//-------UploadManager---------------------
//-----------------------------------------
UploadManager::UploadManager(QObject* p)
	: QObject(p)
	, success_(false)
{
	manager_ = newMnager(this);
}

UploadManager::~UploadManager()
{

}

void UploadManager::go(const QString& file)
{
	if (file.isEmpty()) {
		emit statusText(tr("Canceled"));
		emit uploaded();
		return;
	}

	//manager_->cookieJar()->setCookiesFromUrl(Options::instance()->loadCookies(), mainUrl);

	if(manager_->cookieJar()->cookiesForUrl(mainUrl).isEmpty()) {
		AuthManager am;
		emit statusText(tr("Authorizing..."));
		bool auth = am.go(Options::instance()->getOption(CONST_LOGIN, "").toString(),
				  Options::instance()->getOption(CONST_PASS, "").toString() );
		if(auth) {
			setCookies(am.cookies());
			Options::instance()->saveCookies(am.cookies());
			emit statusText(tr("Authorizing OK"));
		}
		else {
			emit statusText(tr("Authorization failed"));
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
		emit statusText(tr("Error! %1").arg(reply->errorString()));
		emit uploaded();
	}

	reply->deleteLater();
}

void UploadManager::doUpload(const QUrl &url)
{
	QStringList cooks;
	QNetworkCookieJar *netcookjar = manager_->cookieJar();
	QList<QNetworkCookie> cookList = netcookjar->cookiesForUrl(mainUrl);
	foreach (QNetworkCookie netcook, cookList) {
		cooks.append(netcook.name()+"="+netcook.value());
	}

	QNetworkRequest nr = newRequest();
	nr.setUrl(url);
	emit statusText("Opening file...");

	QString boundary = "AaB03x";

	QFile file(fileName_);
	QFileInfo fi(file);

	if (fi.size() == 0) {
		emit statusText(tr("File size is null"));
		emit uploaded();
	}
	else if (file.open(QIODevice::ReadOnly)) {
		emit statusText(tr("Starting upload..."));

		QByteArray mpData;
		mpData.append("--" + boundary + "\r\n");
		mpData.append("Content-Disposition: form-data; name=\"file\"; filename=\"" + fi.fileName().toUtf8() + "\"\r\n");
		mpData.append("Content-Transfer-Encoding: binary\r\n");
		mpData.append("\r\n");
		mpData.append(file.readAll());
		mpData.append("\r\n--" + boundary + "--\r\n");

		file.close();

		nr.setRawHeader("Content-Type", "multipart/form-data, boundary=" + boundary.toLatin1());
		nr.setRawHeader("Content-Length", QString::number(mpData.length()).toLatin1());
		for (int i=0; i<cooks.size(); ++i)
			nr.setRawHeader("Cookie", cooks[i].toLatin1());

		QNetworkReply* netrp;
		netrp = manager_->post(nr, mpData);
		connect(netrp, SIGNAL(uploadProgress(qint64, qint64)), this, SIGNAL(transferProgress(qint64, qint64)));
		connect(netrp, SIGNAL(finished()), SLOT(uploadFinished()));
	}
	else {
		emit statusText(tr("Can't read file"));
		emit uploaded();
	}

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
		emit statusText(tr("Error! %1").arg(reply->errorString()));
		emit uploaded();
	}

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
		emit statusText(tr("Error! %1").arg(reply->errorString()));
	}

	emit uploaded();

	reply->deleteLater();
}



//-----------------------------------------
//-------yandexnarodNetMan-----------------
//-----------------------------------------
yandexnarodNetMan::yandexnarodNetMan(QObject *parent)
	: QObject(parent)
{
	netman = newMnager(this);
	netman->cookieJar()->setCookiesFromUrl(Options::instance()->loadCookies(), mainUrl);
	connect(netman, SIGNAL(finished(QNetworkReply*)), this, SLOT(netrpFinished(QNetworkReply*)));
}

yandexnarodNetMan::~yandexnarodNetMan()
{
}

bool yandexnarodNetMan::startAuth(const QString& login, const QString& passwd)
{
	AuthManager am;
	emit statusText(tr("Authorizing..."));
	bool auth = am.go(login, passwd);
	if(auth) {
		netman->cookieJar()->setCookiesFromUrl(am.cookies(), mainUrl);
		Options::instance()->saveCookies(am.cookies());
		emit statusText(tr("Authorizing OK"));
	}
	else {
		emit statusText(tr("Authorization failed"));
	}

	return auth;
}

void yandexnarodNetMan::startGetFilelist()
{
	action = "get_filelist";
	fileItems.clear();
	//fileids.clear();
	netmanDo();
}

void yandexnarodNetMan::startDelFiles(const QList<FileItem>& fileItems_)
{
	if(fileItems_.isEmpty()) {
		emit finished();
		return;
	}
	action = "del_files";
	fileItems = fileItems_;
	netmanDo();
}

void yandexnarodNetMan::startProlongFiles(const QList<FileItem>& fileItems_)
{
	if(fileItems_.isEmpty()) {
		emit finished();
		return;
	}
	action = "prolong_files";
	fileItems = fileItems_;
	netmanDo();
}

void yandexnarodNetMan::netmanDo()
{
	QNetworkCookieJar *netcookjar = netman->cookieJar();
	QList<QNetworkCookie> cookList = netcookjar->cookiesForUrl(mainUrl);
	if (cookList.isEmpty()/* && netreq.url().toString() != "http://passport.yandex.ru/passport?mode=auth"*/) {
		bool auth = startAuth(Options::instance()->getOption(CONST_LOGIN, "").toString(),
			      Options::instance()->getOption(CONST_PASS, "").toString() );

		if(!auth)
			return;
	}

	if (action == "get_filelist") {
		emit statusText(tr("Downloading filelist..."));
		QNetworkRequest nr = newRequest();
		nr.setUrl(QUrl("http://narod.yandex.ru/disk/all/page1/?sort=cdate%20desc"));
		netman->get(nr);
	}
	else if (action == "del_files" || action == "prolong_files") {
		emit statusText((action == "del_files") ? tr("Deleting files...") : tr("Prolongate files..."));
		QByteArray postData;
		postData.append((action == "del_files") ? "action=delete" : "action=prolongate");
		foreach (const FileItem& item,  fileItems) {
			postData.append(QString("&fid=%1&token-%1=%2").arg(item.fileid, item.token));
		}
		QNetworkRequest nr = newRequest();
		nr.setUrl(QUrl("http://narod.yandex.ru/disk/all"));
		netman->post(nr, postData);
	}
}

void yandexnarodNetMan::netrpFinished( QNetworkReply* reply )
{
	if(reply->error() == QNetworkReply::NoError) {
		QString page = reply->readAll();
		//qDebug()<<"PAGE"<<page;

		if (action == "get_filelist") {
			page.replace("<wbr/>", "");
//			QRegExp rxfn("<span\\sclass=\"num\">\\((\\d+)\\)</span>");
//			if (rxfn.indexIn(page)>-1) {
//				filesnum=rxfn.cap(1).toInt();
//				emit progressMax(filesnum);
//			}
			int cpos = 0;
			static int count = 0;
			QRegExp rx("class=\"\\S+icon\\s(\\S+)\"[^<]+<img[^<]+</i[^<]+</td[^<]+<td[^<]+<input[^v]+value=\"(\\d+)\" data-token=\"(\\S+)\""
				   "[^<]+</td[^<]+<td[^<]+<span\\sclass='b-fname'><a\\shref=\"(\\S+)\">([^<]+)</a>.*"
				   "<td class=\"size\">(\\S+)</td>.*<td class=\"date prolongate\"><nobr>(\\S+ \\S+)</nobr></td>");
			rx.setMinimal(true);
			cpos = rx.indexIn(page);
			while (cpos != -1) {
				FileItem fileitem;
				fileitem.filename = QString::fromUtf8(rx.cap(5).toLatin1());
				fileitem.fileid = rx.cap(2);
				fileitem.token = rx.cap(3);
				fileitem.fileurl = rx.cap(4);
				fileitem.fileicon = rx.cap(1);
				fileitem.size = QString::fromUtf8(rx.cap(6).toLatin1());
				fileitem.date = QString::fromUtf8(rx.cap(7).toLatin1());
				emit newFileItem(fileitem);
				//fileids.append(rx.cap(2));
				cpos = rx.indexIn(page, cpos+1);
				++count;
			}
			QRegExp rxnp("<a\\sid=\"next_page\"\\shref=\"([^\"]+)\"");
			cpos = rxnp.indexIn(page);
			if (cpos > 0 && rxnp.capturedTexts()[1].length()) {
				QNetworkRequest nr = newRequest();
				nr.setUrl(QUrl("http://narod.yandex.ru"+rxnp.cap(1)));
				netman->get(nr);
			}
			else {
				emit statusText(QString(tr("Filelist downloaded\n(%1 files)")).arg(QString::number(count)));
				emit finished();
				count = 0;
			}
		}
		else if (action == "del_files") {
			emit statusText(tr("File(s) deleted"));
			emit finished();
		}
		else if (action == "prolong_files") {
			emit statusText(tr("File(s) prolongated"));
			emit finished();
		}
	}
	else {
		emit statusText(tr("Error! %1").arg(reply->errorString()));
		emit finished();
	}

	reply->deleteLater();
}
