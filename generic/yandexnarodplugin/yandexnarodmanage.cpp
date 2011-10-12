/*
    yandexnarodManage

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

#include <QClipboard>
#include "yandexnarodmanage.h"
#include "uploaddialog.h"
#include "optionaccessinghost.h"
#include "yandexnarodsettings.h"

#define CONST_WIDTH "width"
#define CONST_HEIGHT "height"

yandexnarodManage::yandexnarodManage(OptionAccessingHost *host)
	: QDialog()
	, psiOptions(host)
{
	setupUi(this);
	this->setWindowTitle(tr("Yandex.Narod file manager"));
	this->setWindowIcon(QIcon(":/icons/yandexnarodplugin.png"));
	frameProgress->hide();
	frameFileActions->hide();
	listWidget->clear();

	netman = new yandexnarodNetMan(this, psiOptions);
	connect(netman, SIGNAL(statusText(QString)), labelStatus, SLOT(setText(QString)));
	connect(netman, SIGNAL(progressMax(int)), progressBar, SLOT(setMaximum(int)));
	connect(netman, SIGNAL(progressValue(int)), progressBar, SLOT(setValue(int)));
	connect(netman, SIGNAL(newFileItem(yandexnarodNetMan::FileItem)), this, SLOT(newFileItem(yandexnarodNetMan::FileItem)));
	connect(netman, SIGNAL(finished()), this, SLOT(netmanFinished()));

	QPixmap iconimage(":/icons/yandexnarod-icons-files.png");
	for (int i=0; i<(iconimage.width()/16); i++) {
		QIcon icon(iconimage.copy((i*16),0,16,16));
		fileicons.append(icon);
	}

	fileiconstyles["b-icon-music"] = 0;
	fileiconstyles["b-icon-video"] = 1;
	fileiconstyles["b-icon-arc"] = 2;
	fileiconstyles["b-icon-doc"] = 3;
	fileiconstyles["b-icon-soft"] = 4;
	fileiconstyles["b-icon-unknown"] = 5;
	fileiconstyles["b-icon-picture"] = 14;

	int h = psiOptions->getPluginOption(CONST_HEIGHT, 200).toInt();
	int w = psiOptions->getPluginOption(CONST_WIDTH, 300).toInt();

	resize(w, h);

	setAttribute(Qt::WA_QuitOnClose, false);
	setAttribute(Qt::WA_DeleteOnClose, true);
}

yandexnarodManage::~yandexnarodManage()
{
	psiOptions->setPluginOption(CONST_HEIGHT, height());
	psiOptions->setPluginOption(CONST_WIDTH, width());
}

void yandexnarodManage::newFileItem(yandexnarodNetMan::FileItem fileitem)
{
	int iconnum = 5;
	QString fileiconname = fileitem.fileicon.replace("-old", "");
	if (fileiconstyles.contains(fileiconname))
		iconnum = fileiconstyles[fileiconname];

	QListWidgetItem *listitem = new QListWidgetItem(fileicons[iconnum], fileitem.filename);
	listWidget->addItem(listitem);

	fileitems.append(fileitem);
}

void yandexnarodManage::netmanPrepare()
{
	progressBar->setValue(0);
	frameProgress->show();
	labelStatus->clear();
	frameFileActions->hide();
	btnReload->setEnabled(false);
}

void yandexnarodManage::netmanFinished()
{
	btnReload->setEnabled(true);
	progressBar->setValue(progressBar->maximum());
}

void yandexnarodManage::on_btnReload_clicked()
{
	listWidget->clear();
	fileitems.clear();

	netmanPrepare();
	netman->startGetFilelist();
}

void yandexnarodManage::on_btnDelete_clicked()
{
	progressBar->setMaximum(1);
	netmanPrepare();

	QList<yandexnarodNetMan::FileItem> delfileids;
	for (int i = 0; i < listWidget->count(); i++) {
		if (listWidget->item(i)->isSelected()) {
			listWidget->item(i)->setIcon(fileicons[15]);
			delfileids.append(fileitems[i]);
		}
	}

	netman->startDelFiles(delfileids);
}

void yandexnarodManage::on_listWidget_pressed(QModelIndex)
{
	if (progressBar->value() == progressBar->maximum())
		frameProgress->hide();

	if (frameFileActions->isHidden())
		frameFileActions->show();
}

void yandexnarodManage::on_btnClipboard_clicked()
{
	QString text;
	for (int i=0; i<listWidget->count(); i++) {
		if (listWidget->item(i)->isSelected()) {
			text += fileitems[i].fileurl+"\n";
		}
	}
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(text);
}


void yandexnarodManage::on_btnUpload_clicked()
{
	QString filepath = QFileDialog::getOpenFileName(this, tr("Choose file"), psiOptions->getPluginOption(CONST_LAST_FOLDER).toString());

	if (filepath.length() > 0) {
		uploadDialog* uploadwidget = new uploadDialog(this);
		connect(uploadwidget, SIGNAL(canceled()), this, SLOT(removeUploadWidget()));
		uploadwidget->show();
		uploadwidget->start();

		QFileInfo fi(filepath);
		psiOptions->setPluginOption(CONST_LAST_FOLDER, fi.dir().path());
		yandexnarodNetMan* upnetman = new yandexnarodNetMan(uploadwidget, psiOptions);
		connect(upnetman, SIGNAL(statusText(QString)), uploadwidget, SLOT(setStatus(QString)));
		connect(upnetman, SIGNAL(statusFileName(QString)), uploadwidget, SLOT(setFilename(QString)));
		connect(upnetman, SIGNAL(transferProgress(qint64,qint64)), uploadwidget, SLOT(progress(qint64,qint64)));
		connect(upnetman, SIGNAL(uploadFinished()), uploadwidget, SLOT(setDone()));
		connect(upnetman, SIGNAL(finished()), this, SLOT(netmanFinished()));
		upnetman->startUploadFile(filepath);
	}
}

void yandexnarodManage::removeUploadWidget()
{
	//uploadDialog* uploadwidget = static_cast<uploadDialog*>(sender());
	netmanFinished();
	//uploadwidget->deleteLater();
}
