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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <QAuthenticator>
#include <QDebug>
#include <QDomDocument>
#include <QFile>
#include <QFileSystemModel>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkProxyFactory>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>

#include "applicationinfoaccessinghost.h"
#include "cditemmodel.h"
#include "contentitem.h"
#include "form.h"
#include "ui_form.h"

#define LIST_URL "https://raw.githubusercontent.com/psi-im/contentdownloader/master/content.list"

Form::Form(QWidget *parent) : QWidget(parent), ui(new Ui::Form), listUrl_(LIST_URL)
{
    ui->setupUi(this);
    ui->progressBar->hide();
    nam_ = new QNetworkAccessManager(this);

    CDItemModel *model = new CDItemModel(this);
    ui->treeView->setModel(model);

    connect(ui->treeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &Form::modelSelectionChanged);
    connect(model, &CDItemModel::dataChanged, this, [this]() { modelSelectedItem(); });
    connect(ui->btnLoadList, &QPushButton::clicked, this, &Form::onBtnLoadListClicked);
    connect(ui->btnInstall, &QPushButton::clicked, this, &Form::onBtnInstallClicked);
    ui->widgetContent->hide();
}

Form::~Form()
{
    toDownload_.clear();
    delete ui;
}

void Form::setDataDir(const QString &path)
{
    dataDir_           = path;
    CDItemModel *model = static_cast<CDItemModel *>(ui->treeView->model());
    model->setDataDir(path);
}

void Form::setCacheDir(const QString &path)
{
    // Create directory for caching if not exist
    tmpDir_ = QDir::toNativeSeparators(QString("%1/tmp-contentdownloader").arg(path));

    QDir dir(tmpDir_);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // Setup disk cache
    QNetworkDiskCache *diskCache = new QNetworkDiskCache(this);
    diskCache->setCacheDirectory(dir.path());
    nam_->setCache(diskCache);
}

void Form::setResourcesDir(const QString &path)
{
    CDItemModel *model = static_cast<CDItemModel *>(ui->treeView->model());
    model->setResourcesDir(path);
}

void Form::setPsiOption(OptionAccessingHost *host) { psiOptions_ = host; }

void Form::setProxy(const QNetworkProxy &proxy)
{
    if (!proxy.hostName().isEmpty()) {
        nam_->setProxy(proxy);
    }
}

void Form::onBtnInstallClicked() { startDownload(); }

void Form::onBtnLoadListClicked()
{
    ui->btnLoadList->setEnabled(false);
    toDownload_.clear();
    ui->btnInstall->setEnabled(false);
    QString         url(LIST_URL);
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Content Downloader Plugin (Psi)");
    QNetworkReply *reply = nam_->get(request);

    // State of progress
    connect(reply, &QNetworkReply::downloadProgress, this, &Form::downloadContentProgress);

    // Content list have beign downloaded
    connect(reply, &QNetworkReply::finished, this, &Form::downloadContentListFinished);

    ui->progressBar->show();
    ui->progressBar->setFormat(url.section(QDir::separator(), -1) + " %v Kb (%p%)");
    ui->progressBar->setMaximum(int(reply->size()));
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
    CDItemModel *model    = static_cast<CDItemModel *>(ui->treeView->model());
    int          position = 0;

    QStringList        list;
    QRegularExpression rx("\\[([^\\]]*)\\]([^\\[]*)");

    while (position < text.length()) {
        auto match = rx.match(text);
        position   = match.capturedStart(); // rx.indexIn(text, position);
        if (position == -1) {
            break;
        }

        QString group;
        QString name;
        QString url;
        QString html;

        group = match.captured(1);
        list  = match.captured(2).split("\n", Qt::SkipEmptyParts);
        for (int i = list.size() - 1; i >= 0; i--) {
            QString left, right;
            left  = list[i].section("=", 0, 0).trimmed();
            right = list[i].section("=", 1, 1).trimmed();
            if (left == "name") {
                name = right;
            } else if (left == "url") {
                url = right;
            } else if (left == "html") {
                html = right;
            }
        }
        if (name.isEmpty() || group.isEmpty()) {
            continue;
        }

        model->addRecord(group, name, url, html);
        position += match.capturedLength(0);
    }
}

void Form::downloadContentListProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    ui->progressBar->setMaximum(int(bytesTotal / 1024));
    ui->progressBar->setValue(int(bytesReceived / 1024));
}

void Form::downloadContentListFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    ui->progressBar->hide();

    // Occurs erros
    if (reply->error() != QNetworkReply::NoError) {
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
    CDItemModel *model = static_cast<CDItemModel *>(ui->treeView->model());
    model->update();
    ui->treeView->reset();
}

void Form::downloadContentProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    ui->progressBar->setMaximum(int(bytesTotal / 1024));
    ui->progressBar->setValue(int(bytesReceived / 1024));
}

void Form::downloadContentFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    // Occurs erros
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Content Downloader Plugin:" << reply->errorString();

        ui->progressBar->hide();
        reply->close();
        toDownload_.removeFirst();
        startDownload();
        return;
    }

    // No error
    ContentItem *item     = toDownload_.first();
    QString      filename = item->url().section("/", -1);
    toDownload_.removeFirst();
    if (filename.endsWith(".jisp", Qt::CaseInsensitive)) {
        QDir dir(QDir::toNativeSeparators(QString("%1/%2").arg(dataDir_, item->group())));

        if (!dir.exists()) {
            dir.mkpath(".");
        }

        QString fullFileName = QDir::toNativeSeparators(QString("%1/%2").arg(dir.absolutePath(), filename));

        QFile fd(fullFileName);

        if (!fd.open(QIODevice::WriteOnly) || fd.write(reply->readAll()) == -1) {
            qDebug() << "Content Downloader Plugin:" << fd.errorString() << fullFileName;
        }

        fd.close();
        CDItemModel *model = static_cast<CDItemModel *>(ui->treeView->model());
        model->update();
    }

    reply->close();
    startDownload();
}

void Form::downloadHtmlFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    // Occurs erros
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Content Downloader Plugin:" << reply->errorString();
        reply->close();
        return;
    }

    if (reply == replyLastHtml_) {
        QString      html = QString::fromUtf8(reply->readAll());
        QDomDocument doc("html");

        QString errorMsg;
        int     errorLine, errorColumn;
        if (doc.setContent(html, &errorMsg, &errorLine, &errorColumn)) {
            QString           imgsdir = tmpDir_ + QDir::separator() + "imgs";
            QDir              dir(imgsdir);
            QFileSystemModel *model = new QFileSystemModel();
            if (model->index(dir.path()).isValid()) {
                model->remove(model->index(dir.path()));
            }
            delete model;
            dir.mkpath(".");

            QDomNodeList imgs = doc.elementsByTagName("img");
            // QDomNode             node;
            // QVector<QStringList> text;

            for (int i = 0; i < imgs.size(); i++) {
                QDomElement       el = imgs.at(i).toElement();
                QString           urlStr(el.attribute("src"));
                const QStringList protocols({ "https://", "http://" });
                for (const QString &protocol : protocols) {
                    if (!urlStr.isEmpty() && !(urlStr.startsWith(protocol) || urlStr.startsWith(protocol))) {
                        urlStr = reply->url().toString().section('/', 0, -2) + '/' + urlStr;
                        break;
                    }
                }

                QUrl url(urlStr);
                if (!url.isValid()) {
                    continue;
                }

                QString filename = url.toString().section("/", -1);
                el.setAttribute("src", imgsdir + QDir::separator() + filename);
                QNetworkRequest request(url);
                request.setRawHeader("User-Agent", "Content Downloader Plugin (Psi)");
                request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
                QNetworkReply *reply = nam_->get(request);
                connect(reply, &QNetworkReply::finished, this, &Form::downloadImgFinished);
            }
            html = doc.toString();
        } else {
            // Error occurs in parse of yandex.xml
            qDebug() << "Content Downloader Plugin:"
                     << " line: " << errorLine << ", column: " << errorColumn << "msg: " << errorMsg;
        }

        ui->textEdit->setHtml(html);
    }

    reply->close();
}

void Form::downloadImgFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    // Occurs erros
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Content Downloader Plugin:" << reply->errorString();
        reply->close();
        return;
    }

    QString filename     = reply->url().toString().section("/", -1);
    QString fullFileName = QDir::toNativeSeparators(QString("%1/imgs/%2").arg(tmpDir_, filename));

    QFile fd(fullFileName);
    if (!fd.open(QIODevice::WriteOnly) || fd.write(reply->readAll()) == -1) {
        qDebug() << "Content Downloader Plugin:" << fd.errorString();
    }

    ui->textEdit->setHtml(ui->textEdit->toHtml());
}

void Form::startDownload()
{
    if (toDownload_.isEmpty()) {
        ui->btnInstall->setEnabled(true);
        ui->progressBar->hide();
        return;
    }

    ui->btnInstall->setEnabled(false);
    QNetworkRequest request;
    request.setUrl(QUrl(toDownload_.first()->url()));
    request.setRawHeader("User-Agent", "Content Downloader Plugin (Psi)");
    QNetworkReply *reply = nam_->get(request);

    // State of progress
    connect(reply, &QNetworkReply::downloadProgress, this, &Form::downloadContentProgress);

    // Content list have beign downloaded
    connect(reply, &QNetworkReply::finished, this, &Form::downloadContentFinished);

    ui->progressBar->show();
    ui->progressBar->setFormat(toDownload_.first()->url().section("/", -1) + " %v Kb (%p%)");
    ui->progressBar->setMaximum(int(reply->size()));
}

void Form::modelSelectionChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);

    ContentItem *item = static_cast<ContentItem *>(current.internalPointer());
    ui->textEdit->setHtml("");
    QUrl url(item->html());

    if (url.isValid()) {
        QNetworkRequest request(url);
        request.setRawHeader("User-Agent", "Content Downloader Plugin (Psi)");
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
        replyLastHtml_ = nam_->get(request);

        // Content list have beign downloaded
        connect(replyLastHtml_, &QNetworkReply::finished, this, &Form::downloadHtmlFinished);
    }
}

void Form::modelSelectedItem()
{
    CDItemModel *model = static_cast<CDItemModel *>(ui->treeView->model());
    toDownload_        = model->getToInstall();

    if (toDownload_.isEmpty()) {
        ui->btnInstall->setEnabled(false);
    } else {
        ui->btnInstall->setEnabled(true);
    }
}
