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

#ifndef YANDEXNARODMANAGE_H
#define YANDEXNARODMANAGE_H

#include "yandexnarodnetman.h"
#include "ui_yandexnarodmanage.h"

class OptionAccessingHost;

class yandexnarodManage : public QDialog, public Ui::yandexnarodManageClass
{
	Q_OBJECT

public:
	yandexnarodManage(OptionAccessingHost* host);
	~yandexnarodManage();

private:
	yandexnarodNetMan *netman;
	//yandexnarodNetMan *upnetman;
	QStringList cooks;
	QList<QIcon> fileicons;
	QHash<QString, int> fileiconstyles;
	void netmanPrepare();
	OptionAccessingHost* psiOptions;

	QList<yandexnarodNetMan::FileItem> fileitems;

public slots:
	void setCookies(QStringList incooks) { cooks = incooks; }

private slots:
	void setCooks(QStringList /*incooks*/) { /*if (incooks.count()>0) { cooks = incooks; emit cookies(incooks); }*/ }
	void newFileItem(yandexnarodNetMan::FileItem);
	void on_btnDelete_clicked();
	void on_btnClipboard_clicked();
	void on_listWidget_pressed(QModelIndex index);
	void on_btnReload_clicked();
	void on_btnUpload_clicked();
	void removeUploadWidget();
	void netmanFinished();

signals:
	void cookies(QStringList);

};
#endif

