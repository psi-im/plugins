/*
    uploadDialog

    Copyright (c) 2008 by Alexander Kazarin <boiler@co.ru>
          2011 Evgeny Khryukin


 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
*/

#ifndef UPLOADDIALOG_H
#define UPLOADDIALOG_H

#include "ui_uploaddialog.h"
#include <QDialog>
#include <QTime>

class UploadManager;

class uploadDialog : public QDialog {
    Q_OBJECT

public:
    uploadDialog(QWidget *p = 0);
    ~uploadDialog();
    void start(const QString &fileName);

private:
    void                  setFilename(const QString &str);
    Ui::uploadDialogClass ui;
    QTime                 utime;
    UploadManager        *netman;

signals:
    void canceled();
    void finished();
    void fileUrl(const QString &);

private slots:
    void progress(qint64, qint64);
    void setStatus(const QString &str) { ui.labelStatus->setText(str); }
    void setDone();
    void setLink(const QString &link);
};
#endif
