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

#include "yandexnarodnetman.h"
#include "requestauthdialog.h"
#include "optionaccessinghost.h"
#include "yandexnarodsettings.h"
#include "proxy.h"

yandexnarodNetMan::yandexnarodNetMan(QObject *parent, OptionAccessingHost* host)
	: QObject(parent)
	, psiOptions(host)
{
	netman = new QNetworkAccessManager(this);
	connect(netman, SIGNAL(finished(QNetworkReply*)), this, SLOT(netrpFinished(QNetworkReply*)));

	Proxy p = ProxyManager::instance()->getProxy();
	if(!p.host.isEmpty()) {
		QNetworkProxy np(QNetworkProxy::HttpCachingProxy, p.host, p.port, p.user, p.pass);
		if(p.type != "http")
			np.setType(QNetworkProxy::Socks5Proxy);

		netman->setProxy(np);
	}

	loadSettings();
//	loadCookies();

	auth_flag=0;
}

yandexnarodNetMan::~yandexnarodNetMan()
{
//qDebug()<<"yandexnarodNetMan terminated";
}

void yandexnarodNetMan::startAuthTest(QString login, QString passwd) {
	narodLogin = login;
	narodPasswd = passwd;
	action = "auth_test";
	netmanDo();
}

void yandexnarodNetMan::startGetFilelist() {
	action = "get_filelist";
	filesnum=0;
	fileItems.clear();
	fileids.clear();
//	loadCookies();
	netmanDo();
}

void yandexnarodNetMan::startDelFiles(QList<FileItem> fileItems_) {
	action = "del_files";
	fileItems = fileItems_;
//	loadCookies();
	netmanDo();
}

void yandexnarodNetMan::startUploadFile(QString filearg) {
	filepath = filearg;
	action = "upload";
	nstep=1;
//	loadCookies();
	netmanDo();
}

void yandexnarodNetMan::loadSettings() {
	netreq.setRawHeader("Cache-Control", "no-cache");
	netreq.setRawHeader("Accept", "*/*");
	netreq.setRawHeader("User-Agent", "PsiPlus/0.15 (U; YB/4.2.0; MRA/5.5; en)");
}

//void yandexnarodNetMan::loadCookies() {
//	QNetworkCookieJar *netcookjar = new QNetworkCookieJar(this);
//	int i = 0;
//	QStringList listval = psiOptions->getPluginOption(CONST_COOKIES_VALUES, QStringList()).toStringList();
//	QList<QNetworkCookie> cookieList;
//	foreach (QString cookname, psiOptions->getPluginOption(CONST_COOKIES_NAMES, QStringList()).toStringList()) {
//		if(listval.size() <= i)
//			break;
//		QString cookvalue = listval.at(i++);
//		QNetworkCookie netcook;
//		netcook.setName(cookname.toAscii());
//		netcook.setValue(cookvalue.toAscii());
//		netcook.setDomain(".yandex.ru");
//		netcook.setPath("/");
//		cookieList.append(netcook);
//	}
//	if(!cookieList.isEmpty())
//		netcookjar->setCookiesFromUrl(cookieList, QUrl("http://narod.yandex.ru"));
//	netman->setCookieJar(netcookjar);
//}

//void yandexnarodNetMan::saveCookies() {
//	QNetworkCookieJar *netcookjar = netman->cookieJar();
//	QStringList names, values;
//	foreach (QNetworkCookie netcook, netcookjar->cookiesForUrl(QUrl("http://narod.yandex.ru"))) {
//		names.append(netcook.name());
//		values.append(netcook.value());
//	}
//	psiOptions->setPluginOption(CONST_COOKIES_NAMES, names);
//	psiOptions->setPluginOption(CONST_COOKIES_VALUES, values);
//}

void yandexnarodNetMan::netmanDo() {
	QStringList cooks;
	QNetworkCookieJar *netcookjar = netman->cookieJar();
	foreach (QNetworkCookie netcook, netcookjar->cookiesForUrl(QUrl("http://narod.yandex.ru"))) {
		cooks.append(netcook.name()+"="+netcook.value());
	}
	if (cooks.isEmpty() && netreq.url().toString() != "http://passport.yandex.ru/passport?mode=auth") {
		emit statusText(tr("Authorizing..."));
		narodLogin = psiOptions->getPluginOption(CONST_LOGIN).toString();
		narodPasswd = psiOptions->getPluginOption(CONST_PASS).toString();
		QByteArray post = "login=" + narodLogin.toLatin1() + "&passwd=" + narodPasswd.toLatin1();
//qDebug()<<"SEND AUTH";
		if (narodLogin.isEmpty() || narodPasswd.isEmpty() || narodCaptchaKey.length()>0) {
			requestAuthDialog authdialog;
			authdialog.setLogin(narodLogin);
			authdialog.setPasswd(narodPasswd);
			if (narodCaptchaKey.length()>0) {
				authdialog.setCaptcha("http://passport.yandex.ru/digits?idkey="+narodCaptchaKey);
			}
			if (authdialog.exec()) {
				narodLogin = authdialog.getLogin();
				narodPasswd = authdialog.getPasswd();
				if (authdialog.getRemember()) {
					psiOptions->setPluginOption(CONST_LOGIN, narodLogin);
					psiOptions->setPluginOption(CONST_PASS, narodPasswd);
				}
				post = "login=" + narodLogin.toLatin1() + "&passwd=" + narodPasswd.toLatin1();
			}
			else { post.clear(); }
			if (!post.isEmpty() && narodCaptchaKey.length()>0) {
				post += "&idkey="+narodCaptchaKey.toLatin1()+"&code="+authdialog.getCode();
			}
		}
		if (!post.isEmpty()) {
			netreq.setUrl(QUrl("http://passport.yandex.ru/passport?mode=auth"));
			netman->post(netreq, post);
		}
		else {
			emit statusText(tr("Canceled"));
			emit finished();
		}
	}
	else {
//qDebug()<<"SEND Action request"<<action;
		if (action=="auth_test") {
			emit statusText(tr("Authorizing OK"));
			emit finished();
		}
		else if (action=="get_filelist") {
			emit statusText(tr("Downloading filelist..."));
			netreq.setUrl(QUrl("http://narod.yandex.ru/disk/all/page1/?sort=cdate%20desc"));
			netman->get(netreq);
		}
		else if (action=="del_files") {
			emit progressMax(1);
			emit progressValue(0);
			emit statusText(tr("Deleting files..."));
			QByteArray postData;
			postData.append("action=delete");
			foreach (const FileItem& item,  fileItems) {
				postData.append(QString("&fid=%1&token-%1=%2").arg(item.fileid, item.token));
			}
			netreq.setUrl(QUrl("http://narod.yandex.ru/disk/all"));
			netman->post(netreq, postData);
		}
		else if (action=="upload") {
			if (nstep==1) {
				netreq.setUrl(QUrl("http://narod.yandex.ru/disk/getstorage/"));
				emit statusText(tr("Getting storage..."));
				netman->get(netreq);
			}
			else if (nstep==2) {
				QRegExp rx("\"url\":\"(\\S+)\".+\"hash\":\"(\\S+)\".+\"purl\":\"(\\S+)\"");
				if (rx.indexIn(page)>-1) {
					purl = rx.cap(3) + "?tid=" + rx.cap(2);
					netreq.setUrl(QUrl(rx.cap(1) + "?tid=" + rx.cap(2)));
					emit statusText("Opening file...");

					QString boundary = "AaB03x";

					QFile file(filepath);
					fi.setFile(file);
					if (filepath.isEmpty()) {
						emit statusText(tr("Canceled"));
					}
					else if (fi.size()==0) {
						emit statusText(tr("File size is null"));
					}
					else if (file.open(QIODevice::ReadOnly)) {
						lastdir = fi.dir().path();
						QString fName = fi.fileName();

						emit statusText(tr("Starting upload..."));

						QByteArray mpData;
						mpData.append("--" + boundary + "\r\n");
						mpData.append("Content-Disposition: form-data; name=\"file\"; filename=\"" + fName.toUtf8() + "\"\r\n");
						mpData.append("Content-Transfer-Encoding: binary\r\n");
						mpData.append("\r\n");
						mpData.append(file.readAll());
						mpData.append("\r\n--" + boundary + "--\r\n");

						file.close();

						netreq.setRawHeader("Content-Type", "multipart/form-data, boundary=" + boundary.toLatin1());
						netreq.setRawHeader("Content-Length", QString::number(mpData.length()).toLatin1());
						for (int i=0; i<cooks.size(); ++i) netreq.setRawHeader("Cookie", cooks[i].toLatin1());

						emit statusFileName(fName);

						QNetworkReply* netrp;
						netrp = netman->post(netreq, mpData);
						connect(netrp, SIGNAL(uploadProgress(qint64, qint64)), this, SIGNAL(transferProgress(qint64, qint64)));
					}
					else {
						emit statusText(tr("Can't read file"));
						emit finished();
					}
				}
				else {
					emit statusText(tr("Can't get storage"));
					emit finished();
				}
			}
			else if (nstep==3) {
				emit statusText(tr("Verifying..."));
				netreq.setUrl(QUrl(purl));
				netman->get(netreq);
			}
		}
	}
}

void yandexnarodNetMan::netrpFinished( QNetworkReply* reply ) {
	page = reply->readAll();
//qDebug()<<"PAGE"<<page;

	bool stop = false;

	QString replycookstr = reply->rawHeader("Set-Cookie");
	if (!replycookstr.isEmpty()) {
		QNetworkCookieJar *netcookjar = netman->cookieJar();
		foreach (QNetworkCookie netcook, netcookjar->cookiesForUrl(QUrl("http://narod.yandex.ru"))) {
//qDebug()<<"Cookie"<<netcook.name()<<netcook.value();
			if (netcook.name()=="yandex_login") {
				if (netcook.value().isEmpty()) {
					if (reply->url().toString()=="http://passport.yandex.ru/passport?mode=auth") {
						QRegExp rx("<input type=\"?submit\"?[^>]+name=\"no\"");
						if (rx.indexIn(page)>0) {
							QRegExp rx1("<input type=\"hidden\" name=\"idkey\" value=\"(\\S+)\"[^>]*>");
							if (rx1.indexIn(page)>0) {
//qDebug()<<"Confirmation send";
								QByteArray post = "idkey="+rx1.cap(1).toAscii()+"&no=no";
								netman->post(netreq, post);
								stop=true;
							}
						}
						else {
							rx.setPattern("<img\\ssrc=\"\\S+\\?idkey=(\\S+)\"\\sname=\"captcha\"");
							if (rx.indexIn(page)>0) {
								emit statusText(tr("Authorization captcha request"));
								narodCaptchaKey = rx.cap(1);
								netreq.setUrl(QUrl("http://narod.yandex.ru")); //hack
								netmanDo(); stop=true;
							}
							else {
								auth_flag = -1;
							}
						}
					}
				}
				else auth_flag = 1;
			}

		}
	}

	if (!stop && reply->url().toString()=="http://passport.yandex.ru/passport?mode=auth") {
		if (auth_flag>0) {
			netmanDo();
			stop=true;
		}
		else
			auth_flag = -1;
	}

	if (!stop && auth_flag < 0) {
		emit statusText(tr("Authorization failed"));
	}

	if (!stop && auth_flag>-1) {
		if (action == "auth_test") {
			netmanDo();
		}
		else if (action == "get_filelist") {
			page.replace("<wbr/>", "");
			QRegExp rxfn("<span\\sclass=\"num\">\\((\\d+)\\)</span>");
			if (rxfn.indexIn(page)>-1) {
				filesnum=rxfn.cap(1).toInt();
				emit progressMax(filesnum);
			}
			int cpos=0;
			QRegExp rx("class=\"\\S+icon\\s(\\S+)\"[^<]+<img[^<]+</i[^<]+</td[^<]+<td[^<]+<input[^v]+value=\"(\\d+)\" data-token=\"(\\S+)\"[^<]+</td[^<]+<td[^<]+<span\\sclass='b-fname'><a\\shref=\"(\\S+)\">([^<]+)");
			cpos = rx.indexIn(page);
			while (cpos > 0) {
				FileItem fileitem;
				fileitem.filename = QString::fromUtf8(rx.cap(5).toLatin1());
				fileitem.fileid = rx.cap(2);
				fileitem.token = rx.cap(3);
				fileitem.fileurl = rx.cap(4);
				fileitem.fileicon = rx.cap(1);
				emit newFileItem(fileitem);
				fileids.append(rx.cap(2));
				emit progressValue(fileids.count());
				cpos = rx.indexIn(page, cpos+1);
			}
			QRegExp rxnp("<a\\sid=\"next_page\"\\shref=\"([^\"]+)\"");
			cpos = rxnp.indexIn(page);
			if (cpos>0 && rxnp.capturedTexts()[1].length()) {
				netreq.setUrl(QUrl("http://narod.yandex.ru"+rxnp.cap(1)));
				netman->get(netreq);
			}
			else {
				emit statusText(QString(tr("Filelist downloaded\n(%1 files)")).arg(QString::number(filesnum)));
				emit finished();
			}
		}
		else if (action=="del_files") {
			emit statusText(tr("File(s) deleted"));
			emit progressValue(1);
			emit finished();
		}
		else if (action=="upload") {
			if (nstep==1 || nstep==2) {
				nstep++; netmanDo();
			}
			else if (nstep==3) {
				emit finished();
				QRegExp rx("\"status\":\\s*\"done\".+\"fid\":\\s*\"(\\d+)\"\\,\\s*\"hash\":\\s*\"(\\S+)\"\\,\\s*\"name\":\\s*\"(\\S+)\"");
				if (rx.indexIn(page)>0) {
					emit statusText(tr("Uploaded successfully"));
					QString url = "http://narod.ru/disk/"+rx.cap(2)+"/"+rx.cap(3)+".html";
					emit uploadFileURL(url);
					emit uploadFinished();
				}
				else {
					//qDebug()<<"page"<<page;
					emit statusText(tr("Verifying failed"));
					emit finished();
				}
			}
		}
	}
}
