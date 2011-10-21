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
#include <QFileDialog>
#include <QMimeData>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QMenu>

#include "yandexnarodmanage.h"
#include "uploaddialog.h"
#include "optionaccessinghost.h"
#include "yandexnarodsettings.h"
#include "options.h"
#include "ui_yandexnarodmanage.h"



//------------------------------------------
//-------ListWidgetItem---------------------
//------------------------------------------
class ListWidgetItem : public QListWidgetItem
{
public:
	ListWidgetItem(const QIcon& ico, const yandexnarodNetMan::FileItem& fileitem)
		: QListWidgetItem(ico, fileitem.filename)
		, item_(fileitem)
	{
		QString toolTip = QObject::tr("Name: %1\nSize: %2\nDate prolongate: %3\nURL: %4\nPassword: %5")
					.arg(fileitem.filename)
					.arg( QString(fileitem.size).replace("&nbsp;", " ") )
					.arg(fileitem.date)
					.arg(fileitem.fileurl)
					.arg(fileitem.passset ? QObject::tr("Yes") : QObject::tr("No"));
		setToolTip(toolTip);
	}

	const yandexnarodNetMan::FileItem& fileItem() const
	{
		return item_;
	}

private:
	yandexnarodNetMan::FileItem item_;
};




//------------------------------------------
//-------ListWidget-------------------------
//------------------------------------------
ListWidget::ListWidget(QWidget* p)
	: QListWidget(p)
{
}

QStringList ListWidget::mimeTypes() const
{
	return QStringList() << "text/plain";
}

QMimeData* ListWidget::mimeData(const QList<QListWidgetItem *> items) const
{
	if(items.isEmpty())
		return 0;

	QMimeData* d = new QMimeData();
	QString text;
	foreach(QListWidgetItem *i, items) {
		text += static_cast<ListWidgetItem*>(i)->fileItem().fileurl + "\n";
	}
	d->setText(text);

	return d;
}

void ListWidget::mousePressEvent(QMouseEvent *event)
{
	QListWidget::mousePressEvent(event);
	if(event->button() == Qt::RightButton) {
		QListWidgetItem *lwi = itemAt(event->pos());
		if(lwi) {
			ListWidgetItem *it = static_cast<ListWidgetItem*>(lwi);
			emit menu(it->fileItem());
			event->accept();
		}
	}
}




//------------------------------------------
//-------yandexnarodManage------------------
//------------------------------------------
yandexnarodManage::yandexnarodManage(QWidget* p)
	: QDialog(p, Qt::Window)
	, ui_(new Ui::yandexnarodManageClass)
{
	ui_->setupUi(this);
	setWindowTitle(tr("Yandex.Narod file manager"));
	setWindowIcon(QIcon(":/icons/yandexnarodplugin.png"));
	ui_->frameProgress->hide();
	ui_->frameFileActions->hide();
	ui_->listWidget->clear();

	ui_->btnProlong->hide(); // hide cos it doesnt work

	newNetMan();

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

	Options* o = Options::instance();
	int h = o->getOption(CONST_HEIGHT, 200).toInt();
	int w = o->getOption(CONST_WIDTH, 300).toInt();

	resize(w, h);

	setAttribute(Qt::WA_QuitOnClose, false);
	setAttribute(Qt::WA_DeleteOnClose, true);

	connect(ui_->listWidget, SIGNAL(menu(yandexnarodNetMan::FileItem)), SLOT(doMenu(yandexnarodNetMan::FileItem)));
}

yandexnarodManage::~yandexnarodManage()
{
	Options* o = Options::instance();
	o->setOption(CONST_HEIGHT, height());
	o->setOption(CONST_WIDTH, width());

	delete ui_;
}

void yandexnarodManage::newNetMan()
{
	netman = new yandexnarodNetMan(this);
	connect(netman, SIGNAL(statusText(QString)), ui_->labelStatus, SLOT(setText(QString)));
	connect(netman, SIGNAL(newFileItem(yandexnarodNetMan::FileItem)), this, SLOT(newFileItem(yandexnarodNetMan::FileItem)));
	connect(netman, SIGNAL(finished()), this, SLOT(netmanFinished()));
}

void yandexnarodManage::newFileItem(yandexnarodNetMan::FileItem fileitem)
{
	int iconnum = 5;
	QString fileiconname = fileitem.fileicon.replace("-old", "");
	if (fileiconstyles.contains(fileiconname))
		iconnum = fileiconstyles[fileiconname];

	QListWidgetItem *listitem = new ListWidgetItem(fileicons[iconnum], fileitem);
	ui_->listWidget->addItem(listitem);
}

void yandexnarodManage::netmanPrepare()
{
	ui_->frameProgress->show();
	ui_->labelStatus->clear();
	ui_->frameFileActions->hide();
	ui_->btnReload->setEnabled(false);
}

void yandexnarodManage::netmanFinished()
{
	ui_->btnReload->setEnabled(true);
}

void yandexnarodManage::on_btnReload_clicked()
{
	ui_->listWidget->clear();
	netmanPrepare();
	netman->startGetFilelist();
}

void yandexnarodManage::on_btnDelete_clicked()
{
	netmanPrepare();
	foreach(QListWidgetItem* i, ui_->listWidget->selectedItems()) {
		i->setIcon(fileicons[15]);
	}

	netman->startDelFiles(selectedItems());
}

void yandexnarodManage::on_btnProlong_clicked()
{
	netmanPrepare();
	netman->startProlongFiles(selectedItems());
}

void yandexnarodManage::on_btnClearCookies_clicked()
{
	netman->disconnect();
	netman->deleteLater();

	Options::instance()->saveCookies(QList<QNetworkCookie>());

	newNetMan();
	ui_->frameProgress->show();
	ui_->labelStatus->setText(tr("Cookies are removed"));
}

QList<yandexnarodNetMan::FileItem> yandexnarodManage::selectedItems() const
{
	QList<yandexnarodNetMan::FileItem> items;
	foreach(QListWidgetItem* i, ui_->listWidget->selectedItems()) {
		items.append(static_cast<ListWidgetItem*>(i)->fileItem());
	}

	return items;
}

void yandexnarodManage::on_listWidget_pressed(QModelIndex)
{
	if (ui_->frameFileActions->isHidden())
		ui_->frameFileActions->show();
}

void yandexnarodManage::on_btnClipboard_clicked()
{
	QString text;
	foreach(QListWidgetItem* i, ui_->listWidget->selectedItems()) {
		text += static_cast<ListWidgetItem*>(i)->fileItem().fileurl + "\n";
	}
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(text);
}


void yandexnarodManage::on_btnUpload_clicked()
{
	QString filepath = QFileDialog::getOpenFileName(this, tr("Choose file"), Options::instance()->getOption(CONST_LAST_FOLDER).toString());

	if (!filepath.isEmpty()) {
		QFileInfo fi(filepath);
		Options::instance()->setOption(CONST_LAST_FOLDER, fi.dir().path());

		uploadDialog* uploadwidget = new uploadDialog(this);
		connect(uploadwidget, SIGNAL(canceled()), this, SLOT(netmanFinished()));
		connect(uploadwidget, SIGNAL(finished()), this, SLOT(netmanFinished()));
		uploadwidget->show();
		uploadwidget->start(filepath);
	}
}
void yandexnarodManage::doMenu(const yandexnarodNetMan::FileItem &it)
{
	QMenu m;
	QList<QAction*> actions;
	QAction *act = new QAction(tr("Set password"), &m);
	act->setVisible(!it.passset);
	act->setData(1);
	actions << act;
	act = new QAction(tr("Remove password"), &m);
	act->setVisible(it.passset);
	act->setData(2);
	actions << act;
//	act = new QAction(tr("Copy URL"), &m);
//	act->setData(3);
//	actions << act;
	m.addActions(actions);
	QAction* ret = m.exec(QCursor::pos());
	if(ret) {
		switch(ret->data().toInt()) {
		case 1:
			netman->startSetPass(it);
			break;
		case 2:
			netman->startRemovePass(it);
			break;
		case 3:
			on_btnClipboard_clicked();
			break;
		default:
			break;
		}
	}
}

