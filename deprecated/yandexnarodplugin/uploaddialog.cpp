/*
    uploadDialog

    Copyright (c) 2008-2009 by Alexander Kazarin <boiler@co.ru>
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

#include <QFileInfo>

#include "uploaddialog.h"
#include "uploadmanager.h"
#include "options.h"

uploadDialog::uploadDialog(QWidget *p)
    : QDialog(p, Qt::MSWindowsFixedSizeDialogHint)
{
    ui.setupUi(this);
    ui.progressBar->setValue(0);
    connect(ui.btnUploadCancel, SIGNAL(clicked()), this, SIGNAL(canceled()));
    connect(ui.btnUploadCancel, SIGNAL(clicked()), this, SLOT(close()));
    setAttribute(Qt::WA_DeleteOnClose, true);

    netman = new UploadManager(this);
    connect(netman, SIGNAL(statusText(QString)), this, SLOT(setStatus(QString)));
    connect(netman, SIGNAL(transferProgress(qint64,qint64)),this, SLOT(progress(qint64,qint64)));
    connect(netman, SIGNAL(uploadFileURL(QString)), this, SIGNAL(fileUrl(QString)));
    connect(netman, SIGNAL(uploaded()), this, SLOT(setDone()));
    connect(netman, SIGNAL(uploadFileURL(QString)), this, SLOT(setLink(QString)));
}

uploadDialog::~uploadDialog() 
{
    
}

void uploadDialog::setFilename(const QString& str)
{
    ui.labelFile->setText(tr("File: ") + str);
    setWindowTitle(O_M(MUploading) + " - " + str);
}

void uploadDialog::start(const QString& fileName)
{
    QFileInfo fi(fileName);
    setFilename(fi.fileName());

    ui.progressBar->setValue(0);
    ui.labelLink->setVisible(false);
    utime.start();
    netman->go(fileName);
}

void uploadDialog::progress(qint64 cBytes, qint64 totalBytes)
{
    ui.labelStatus->setText(O_M(MUploading)+"...");
    ui.labelProgress->setText(tr("Progress: ")  + QString::number(cBytes) + " /  " + QString::number(totalBytes));
    ui.progressBar->setMaximum(totalBytes);
    ui.progressBar->setValue(cBytes);

    setWindowTitle("[" + ui.progressBar->text() + "] - " + O_M(MUploading)+"...");
    
    QTime etime(0,0,0);
    int elapsed = utime.elapsed();
    etime = etime.addMSecs(elapsed);
    ui.labelETime->setText(tr("Elapsed time: ") + etime.toString("hh:mm:ss") );
    
    float speed_kbsec = (elapsed == 0) ? 0 : (cBytes / (((float)elapsed)/1000))/1024;
    if (speed_kbsec > 0)
        ui.labelSpeed->setText(tr("Speed: ")+QString::number(speed_kbsec)+tr(" kb/sec"));
    
    if (cBytes == totalBytes)
        ui.labelStatus->setText(tr("Upload completed. Waiting for verification."));
}

void uploadDialog::setDone()
{
    if(netman->success())
        ui.btnUploadCancel->setText(tr("Done"));
    else
        ui.btnUploadCancel->setText(tr("Close"));

    emit finished();
}

void uploadDialog::setLink(const QString &link)
{
    ui.labelLink->setVisible(true);
    ui.labelLink->setText(tr("Link: <a href=\"%1\">%2</a>").arg(link, link.left(30)+"..."));
}
