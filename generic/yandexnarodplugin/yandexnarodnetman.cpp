/*
	yandexnarodNetMan

	Copyright (c) 2008-2009 by Alexander Kazarin <boiler@co.ru>
			2011 by Evgeny Khryukin

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
*/

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QtGui>



#include "yandexnarodnetman.h"
#include "authmanager.h"
#include "options.h"
#include "common.h"


// function needed for tests only
//static void saveData(const QString& text)
//{
//	//qDebug() << text;
//	QFile file(QDir::homePath() + "/page.html");
//	if(file.open(QIODevice::WriteOnly | QIODevice::Truncate) ) {
//		QTextStream str(&file);
//		str << text;
//	}
//}


//-------------------------------------------
//------GetPassDlg---------------------------
//-------------------------------------------
class GetPassDlg : public QDialog
{
	Q_OBJECT
public:
	GetPassDlg(QWidget *p = 0)
		: QDialog(p)
		, lePass(new QLineEdit)
		, leConfirmPass(new QLineEdit)
	{
		setWindowTitle(tr("Set Password"));
		lePass->setEchoMode(QLineEdit::Password);
		leConfirmPass->setEchoMode(QLineEdit::Password);

		QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		QVBoxLayout* l = new QVBoxLayout(this);
		l->addWidget(new QLabel(tr("Password:")));
		l->addWidget(lePass);
		l->addWidget(new QLabel(tr("Confirm password:")));
		l->addWidget(leConfirmPass);
		l->addWidget(bb);

		connect(bb, SIGNAL(rejected()), SLOT(reject()));
		connect(bb, SIGNAL(accepted()), SLOT(okPressed()));

		adjustSize();
		setFixedSize(size());
		show();
	}

	QString password() const
	{
		return lePass->text();
	}

private slots:
	void okPressed()
	{
		if(isPassOk()) {
			accept();
		}
		else {
			QToolTip::showText(pos() + leConfirmPass->pos(), tr("Password does not match"), leConfirmPass);
		}
	}

private:
	bool isPassOk() const
	{
		return lePass->text() == leConfirmPass->text();
	}

private:
	QLineEdit *lePass, *leConfirmPass;
};





//-----------------------------------------
//-------yandexnarodNetMan-----------------
//-----------------------------------------
yandexnarodNetMan::yandexnarodNetMan(QObject *parent)
	: QObject(parent)
{
	netman = newManager(this);
	netman->cookieJar()->setCookiesFromUrl(Options::instance()->loadCookies(), mainUrl);
	connect(netman, SIGNAL(finished(QNetworkReply*)), this, SLOT(netrpFinished(QNetworkReply*)));
}

yandexnarodNetMan::~yandexnarodNetMan()
{
}

bool yandexnarodNetMan::startAuth(const QString& login, const QString& passwd)
{
	AuthManager am;
	emit statusText(O_M(MAuthStart));
	bool auth = am.go(login, passwd);
	if(auth) {
		netman->cookieJar()->setCookiesFromUrl(am.cookies(), mainUrl);
		Options::instance()->saveCookies(am.cookies());
		emit statusText(O_M(MAuthOk));
	}
	else {
		emit statusText(O_M(MAuthError));
	}

	return auth;
}

void yandexnarodNetMan::startGetFilelist()
{
	action = GetFiles;
	netmanDo();
}

void yandexnarodNetMan::startDelFiles(const QList<FileItem>& fileItems_)
{
	if(fileItems_.isEmpty()) {
		emit finished();
		return;
	}
	action = DeleteFiles;
	netmanDo(fileItems_);
}

void yandexnarodNetMan::startProlongFiles(const QList<FileItem>& fileItems_)
{
	if(fileItems_.isEmpty()) {
		emit finished();
		return;
	}
	action = ProlongateFiles;
	netmanDo(fileItems_);
}

void yandexnarodNetMan::startSetPass(const FileItem &item)
{
	if(item.passset) {
		emit finished();
		return;
	}
	action = SetPass;
	netmanDo(QList<FileItem>() << item);
}

void yandexnarodNetMan::startRemovePass(const FileItem &item)
{
	if(!item.passset) {
		emit finished();
		return;
	}
	action = RemovePass;
	netmanDo(QList<FileItem>() << item);
}

void yandexnarodNetMan::netmanDo(QList<FileItem> fileItems)
{
	QNetworkCookieJar *netcookjar = netman->cookieJar();
	QList<QNetworkCookie> cookList = netcookjar->cookiesForUrl(mainUrl);
	if (cookList.isEmpty()) {
		bool auth = startAuth(Options::instance()->getOption(CONST_LOGIN, "").toString(),
				      Options::decodePassword(Options::instance()->getOption(CONST_PASS, "").toString()) );

		if(!auth)
			return;
	}

	switch(action) {
		case GetFiles:
		{
			emit statusText(tr("Downloading filelist..."));
			QNetworkRequest nr = newRequest();
			nr.setUrl(QUrl("http://narod.yandex.ru/disk/all/page1/?sort=cdate%20desc"));
			netman->get(nr);
			break;
		}
		case DeleteFiles:
		case ProlongateFiles:
		{
			emit statusText((action == DeleteFiles) ? tr("Deleting files...") : tr("Prolongate files..."));
			QByteArray postData;
			postData.append((action == DeleteFiles) ? "action=delete" : "action=prolongate");
			foreach (const FileItem& item,  fileItems) {
				postData.append(QString("&fid=%1&token-%1=%2").arg(item.fileid, item.token));
			}
			QNetworkRequest nr = newRequest();
			nr.setUrl(QUrl("http://narod.yandex.ru/disk/all"));
			nr.setHeader(QNetworkRequest::ContentLengthHeader, postData.length());
			nr.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
			netman->post(nr, postData);
			break;
		}
		case SetPass:
		{
			GetPassDlg gpd;
			if(gpd.exec() == QDialog::Accepted) {
				QNetworkRequest nr = newRequest();
				nr.setUrl(QUrl("http://narod.yandex.ru/disk/setpasswd/" + fileItems.first().fileid));
				const QString pass = gpd.password();
				QByteArray post;
				post.append("passwd=" + pass);
				post.append("&token=" + fileItems.first().passtoken);
				nr.setHeader(QNetworkRequest::ContentLengthHeader, post.length());
				nr.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
				netman->post(nr, post);
			}
			else {
				emit statusText(O_M(MCancel));
				emit finished();
			}
			break;
		}
		case RemovePass:
		{
			QNetworkRequest nr = newRequest();
			nr.setUrl(QUrl("http://narod.yandex.ru/disk/setpasswd/" + fileItems.first().fileid));
			QByteArray post;
			post.append("passwd=&token=" + fileItems.first().passtoken);
			nr.setHeader(QNetworkRequest::ContentLengthHeader, post.length());
			nr.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
			netman->post(nr, post);
			break;
		}
		default:
			break;
	}
}

void yandexnarodNetMan::netrpFinished(QNetworkReply* reply)
{
	if(reply->error() == QNetworkReply::NoError)
	{
		QString page = reply->readAll();

		switch(action) {
			case GetFiles:
			{
				static bool firstTry = true;
				if(page.isEmpty())
				{
					if(firstTry) {
						firstTry = false;
						emit statusText(tr("Cookies are obsolete!\nReathorization..."));
						QNetworkCookieJar *jar = netman->cookieJar();
						delete jar;
						jar = new QNetworkCookieJar(netman);
						netman->setCookieJar(jar);
						netmanDo();
					}
					else {
						emit statusText(tr("Can't get files!\nTry remove cookies."));
					}
					emit finished();
					reply->deleteLater();
					return;
				}
				else {
					firstTry = true;
					page.replace("<wbr/>", "");
					int cpos = 0;
					static int count = 0;
					QRegExp rx("class=\"\\S+icon\\s(\\S+)\"[^<]+<img[^<]+</i[^<]+</td[^<]+<td[^<]+<input[^v]+value=\"(\\d+)\" data-token=\"(\\S+)\""
						   "[^<]+</td[^<]+<td[^<]+<span\\sclass='b-fname'><a\\shref=\"(\\S+)\">([^<]+)</a>.*"
						   "<td class=\"size\">(\\S+)</td>.*data-token=\"(\\S+)\".*<i class=\"([^\"]+)\".*"
						   "<td class=\"date prolongate\"><nobr>([^>]+)</nobr></td>");
					rx.setMinimal(true);
					cpos = rx.indexIn(page);
					while (cpos != -1) {
						FileItem fileitem;
						QTextDocument doc;
						doc.setHtml(QString::fromUtf8(rx.cap(5).toLatin1()));
						fileitem.filename = doc.toPlainText();
						fileitem.fileid = rx.cap(2);
						fileitem.token = rx.cap(3);
						fileitem.fileurl = rx.cap(4);
						fileitem.fileicon = rx.cap(1);
						fileitem.size = QString::fromUtf8(rx.cap(6).toLatin1());
						fileitem.passtoken = rx.cap(7);
						fileitem.passset = (rx.cap(8) == "b-old-icon b-old-icon-pwd-on");
						fileitem.date = QString::fromUtf8(rx.cap(9).toLatin1());
						emit newFileItem(fileitem);
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
				break;
			}

			case DeleteFiles:
			{
				emit statusText(tr("File(s) deleted"));
				emit finished();
				break;
			}

			case ProlongateFiles:
			{
				emit statusText(tr("File(s) prolongated"));
				emit finished();
				break;
			}

			case SetPass:
			{
				emit statusText(tr("Password is set"));
				emit finished();
				break;
			}

			case RemovePass:
			{
				emit statusText(tr("Password is deleted"));
				emit finished();
				break;
			}

			default:
				emit finished();
				break;
		}
	}
	else {
		emit statusText(O_M(MError).arg(reply->errorString()));
		emit finished();
	}

	reply->deleteLater();
}

#include "yandexnarodnetman.moc"
