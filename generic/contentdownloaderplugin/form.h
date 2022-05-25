/*
 * form.h - options widget of Content Downloader Plugin
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

#ifndef FORM_H
#define FORM_H

#include <QModelIndex>
#include <QNetworkProxy>
#include <QWidget>

class QNetworkAccessManager;
class ContentItem;
class OptionAccessingHost;
class QNetworkReply;
class QAuthenticator;
class QDialog;

namespace Ui {
class Form;
}

class Form : public QWidget {
    Q_OBJECT
public:
    Form(QWidget *parent = nullptr);
    ~Form();

    void setDataDir(const QString &path);
    void setCacheDir(const QString &path);
    void setResourcesDir(const QString &path);
    void setPsiOption(OptionAccessingHost *host);
    void setProxy(const QNetworkProxy &proxy);

public slots:
    void onBtnInstallClicked();
    void onBtnLoadListClicked();

protected:
    void changeEvent(QEvent *e);

private:
    void parseContentList(const QString &text);
    void startDownload();

    Ui::Form              *ui;
    QNetworkAccessManager *nam_;
    QString                dataDir_;
    QString                tmpDir_;
    QString                listUrl_;
    QList<ContentItem *>   toDownload_;
    OptionAccessingHost   *psiOptions_;
    QNetworkReply         *replyLastHtml_;

private slots:
    void downloadContentListProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadContentListFinished();

    void downloadContentProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadContentFinished();

    void downloadHtmlFinished();
    void downloadImgFinished();

    void modelSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void modelSelectedItem();
};

#endif // FORM_H
