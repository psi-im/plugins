/*
 * form.cpp - options widget of Content Downloader Plugin
 * Copyright (C) 2010  Ivan Romanov <drizt@land.ru>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include <QDebug>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkProxyFactory>
#include <QAuthenticator>
#include <QNetworkDiskCache>
#include <QFile>
#include <QRegExp>
#include <QUrl>
#include <QDomDocument>
#include <QFileSystemModel>
#include <QNetworkProxyFactory>

#include "form.h"
#include "cditemmodel.h"
#include "contentitem.h"
#include "applicationinfoaccessinghost.h"
#include "ui_form.h"

#define LIST_URL "https://raw.githubusercontent.com/psi-plus/contentdownloader/master/content.list"

Form::Form(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::Form)
	, listUrl_(LIST_URL)
{
	ui->setupUi(this);
	ui->progressBar->hide();
	nam_ = new QNetworkAccessManager(this);

	CDItemModel *model = new CDItemModel(this);
	ui->treeView->setModel(model);

	connect(ui->treeView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), SLOT(modelSelectionChanged(const QModelIndex&, const QModelIndex&)));
	connect(model, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), SLOT(modelSelectedItem()));
	ui->widgetContent->hide();
}

Form::~Form()
{
	toDownload_.clear();
	delete ui;
}

void Form::setDataDir(const QString &path)
{
	dataDir_ = path;
	CDItemModel *model = qobject_cast<CDItemModel*>(ui->treeView->model());
	model->setDataDir(path);
}

void Form::setCacheDir(const QString &path)
{
	// Create directory for caching if not exist
	tmpDir_ = QDir::toNativeSeparators(QString("%1/tmp-contentdownloader").
									   arg(path));

	QDir dir(tmpDir_);
	if(!dir.exists()) {
		dir.mkpath(".");
	}

	// Setup disk cache
	QNetworkDiskCache *diskCache = new QNetworkDiskCache(this);
	diskCache->setCacheDirectory(dir.path());
	nam_->setCache(diskCache);
}

void Form::setResourcesDir(const QString &path)
{
	CDItemModel *model = qobject_cast<CDItemModel*>(ui->treeView->model());
	model->setResourcesDir(path);
}

void Form::setPsiOption(OptionAccessingHost *host)
{
	psiOptions_ = host;
}

void Form::setProxy(const QNetworkProxy &proxy)
{
	if(!proxy.hostName().isEmpty()) {
		nam_->setProxy(proxy);
	}
}

void Form::on_btnInstall_clicked()
{
	startDownload();
}

void Form::on_btnLoadList_clicked()
{
	ui->btnLoadList->setEnabled(false);
	toDownload_.clear();
	ui->btnInstall->setEnabled(false);
	QString url(LIST_URL);
	QNetworkRequest request(url);
	request.setRawHeader("User-Agent", "Content Downloader Plugin (Psi+)");
	QNetworkReply *reply = nam_->get(request);

	// State of progress 
	connect(reply, SIGNAL(downloadProgress(qint64, qint64)), SLOT(downloadContentListProgress(qint64, qint64)));

	// Content list have beign downloaded
	connect(reply, SIGNAL(finished()), SLOT(downloadContentListFinished()));
  
	ui->progressBar->show();
	ui->progressBar->setFormat(url.section(QDir::separator(), -1) + " %v Kb (%p%)");
	ui->progressBar->setMaximum(reply->size());
}

void Form::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void Form::parseContentList(const QString &text)
{
	CDItemModel *model = qobject_cast<CDItemModel*>(ui->treeView->model());
	int position = 0;

	QStringList list;
	QRegExp rx("\\[([^\\]]*)\\]([^\\[]*)");
	
	while(position < text.length()) {
		position = rx.indexIn(text, position);
		if(position == -1) {
			break;
		}
		
		QString group;
		QString name;
		QString url;
		QString html;

		group = rx.cap(1);
		list = rx.cap(2).split("\n", QString::SkipEmptyParts);
		for(int i = list.size() - 1; i >= 0; i--) {
			QString left, right;
			left = list[i].section("=", 0, 0).trimmed();
			right = list[i].section("=", 1, 1).trimmed();
			if(left == "name") {
				name = right;
			} else if(left == "url") {
				url = right;
			} else if(left == "html") {
				html = right;
			}
		}
		if(name.isEmpty() || group.isEmpty()) {
			continue;
		}
		
		model->addRecord(group, name, url, html);
		position += rx.matchedLength();
	}
	
}

void Form::downloadContentListProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	ui->progressBar->setMaximum(bytesTotal / 1024);
	ui->progressBar->setValue(bytesReceived / 1024);
}

void Form::downloadContentListFinished()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
	ui->progressBar->hide();
 
	// Occurs erros
	if(reply->error() != QNetworkReply::NoError) {
		qDebug() << "Content Downloader Plugin:" << reply->errorString();
		ui->progressBar->hide();
		ui->btnInstall->setEnabled(false);
		reply->close();
		return;
	}

	// No error
	ui->widgetContent->show();
	ui->widgetLoadList->hide();
	parseContentList(reply->readAll());
	reply->close();
	ui->btnInstall->setEnabled(false);
	CDItemModel *model = qobject_cast<CDItemModel*>(ui->treeView->model());
	model->update();
	ui->treeView->reset();
}

void Form::downloadContentProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	ui->progressBar->setMaximum(bytesTotal / 1024);
	ui->progressBar->setValue(bytesReceived / 1024);
}

void Form::downloadContentFinished()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
	
	// Occurs erros
	if(reply->error() != QNetworkReply::NoError) {
		qDebug() << "Content Downloader Plugin:" << reply->errorString();

		ui->progressBar->hide();
		reply->close();
		toDownload_.removeFirst();
		startDownload();
		return;
	}

	// No error
	ContentItem *item = toDownload_.first();
	QString filename = item->url().section("/", -1);
	toDownload_.removeFirst();
	if(filename.endsWith(".jisp")) {
		QDir dir(QDir::toNativeSeparators(QString("%1/%2").
										  arg(dataDir_).
										  arg(item->group())));

		if(!dir.exists()) {
			dir.mkpath(".");
		}

		QString fullFileName = QDir::toNativeSeparators(QString("%1/%2").
														arg(dir.absolutePath()).
														arg(filename));
 
		QFile fd(fullFileName);
		
		if(!fd.open(QIODevice::WriteOnly) || fd.write(reply->readAll()) == -1) {
			qDebug() << "Content Downloader Plugin:" << fd.errorString() << fullFileName;
		}
		
		fd.close();
		CDItemModel *model = qobject_cast<CDItemModel*>(ui->treeView->model());
		model->update();
	}
	
	reply->close();
	startDownload();
}

void Form::downloadHtmlFinished()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
	
	// Occurs erros
	if(reply->error() != QNetworkReply::NoError) {
		qDebug() << "Content Downloader Plugin:" << reply->errorString();
		reply->close();
		return;
	}

	if(reply == replyLastHtml_) {
		QString html = QString::fromUtf8(reply->readAll());
		QDomDocument doc("html");

		QString errorMsg;
		int errorLine, errorColumn;
		if(doc.setContent(html, &errorMsg, &errorLine, &errorColumn)) {
			QString imgsdir = tmpDir_ + QDir::separator() + "imgs";
			QDir dir(imgsdir);
			QFileSystemModel *model = new QFileSystemModel();
			if(model->index(dir.path()).isValid()) {
				model->remove(model->index(dir.path()));
			}
			delete model;
			dir.mkpath(".");
	  
			QDomNodeList imgs = doc.elementsByTagName("img");
			QDomNode node;
			QVector<QStringList> text;
	
			for(int i = 0; i < imgs.size(); i++) {
				QDomElement el = imgs.at(i).toElement();
				QString urlStr(el.attribute("src"));
				if(!urlStr.isEmpty() && !(urlStr.startsWith("http://") || urlStr.startsWith("http://"))) {
					urlStr = reply->url().toString().section('/', 0, -2) + '/' + urlStr;
				}

				QUrl url(urlStr);
				if(!url.isValid()) {
					continue;
				}
				
				QString filename = url.toString().section("/", -1);
				el.setAttribute("src", imgsdir + QDir::separator() + filename);
				QNetworkRequest request(url);
				request.setRawHeader("User-Agent", "Content Downloader Plugin (Psi+)");
				request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
				QNetworkReply *reply = nam_->get(request);
				connect(reply, SIGNAL(finished()), SLOT(downloadImgFinished()));
			}
			html = doc.toString();
		} else {
			// Error occurs in parse of yandex.xml
			qDebug() << "Content Downloader Plugin:" << " line: " << errorLine << ", column: " << errorColumn;
		}
		
		ui->textEdit->setHtml(html);
	}
	
	reply->close();
}

void Form::downloadImgFinished()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
	
	// Occurs erros
	if(reply->error() != QNetworkReply::NoError) {
		qDebug() << "Content Downloader Plugin:" << reply->errorString();
		reply->close();
		return;
	}
	
	QString filename = reply->url().toString().section("/", -1);
	QString fullFileName = QDir::toNativeSeparators(QString("%1/imgs/%2").
													arg(tmpDir_).
													arg(filename));

	QFile fd(fullFileName);
	if(!fd.open(QIODevice::WriteOnly) || fd.write(reply->readAll()) == -1) {
		qDebug() << "Content Downloader Plugin:" << fd.errorString();
	}

	ui->textEdit->setHtml(ui->textEdit->toHtml());
}

void Form::startDownload()
{
	if(toDownload_.isEmpty()) {
		ui->btnInstall->setEnabled(true);
		ui->progressBar->hide();
		return;
	}

	ui->btnInstall->setEnabled(false);
	QNetworkRequest request;
	request.setUrl(QUrl(toDownload_.first()->url()));
	request.setRawHeader("User-Agent", "Content Downloader Plugin (Psi+)");
	QNetworkReply *reply = nam_->get(request);

	// State of progress 
	connect(reply, SIGNAL(downloadProgress(qint64, qint64)), SLOT(downloadContentProgress(qint64, qint64)));

	// Content list have beign downloaded
	connect(reply, SIGNAL(finished()), SLOT(downloadContentFinished()));
  
	ui->progressBar->show();
	ui->progressBar->setFormat(toDownload_.first()->url().section("/", -1) + " %v Kb (%p%)");
	ui->progressBar->setMaximum(reply->size());
}

void Form::modelSelectionChanged(const QModelIndex &current, const QModelIndex &previous)
{
	Q_UNUSED(previous);

	ContentItem *item = (ContentItem*)current.internalPointer();
	ui->textEdit->setHtml("");
	QUrl url(item->html());

	if(url.isValid()) {
		QNetworkRequest request(url);
		request.setRawHeader("User-Agent", "Content Downloader Plugin (Psi+)");
		request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
		replyLastHtml_ = nam_->get(request);

		// Content list have beign downloaded
		connect(replyLastHtml_, SIGNAL(finished()), SLOT(downloadHtmlFinished()));
   }
}

void Form::modelSelectedItem()
{
	CDItemModel *model = qobject_cast<CDItemModel*>(ui->treeView->model());
	toDownload_ = model->getToInstall();

	if(toDownload_.isEmpty()) {
		ui->btnInstall->setEnabled(false);
	} else {
		ui->btnInstall->setEnabled(true);
	}
}
