/*
 * viewer.cpp - plugin
 * Copyright (C) 2009-2010  Evgeny Khryukin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "viewer.h"

#include <QFile>
#include <QTextStream>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QFileInfo>


ViewLog::ViewLog(QString filename, IconFactoryAccessingHost *IcoHost, QWidget *parent)
        : QDialog(parent)
        , icoHost_(IcoHost)
        , fileName_(filename)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(fileName_);
	QVBoxLayout *layout = new QVBoxLayout(this);
	textWid = new QTextEdit();
	layout->addWidget(textWid);
	findBar = new Stopspam::TypeAheadFindBar(icoHost_, textWid, tr("Find"), this);
	QPushButton *Close = new QPushButton(icoHost_->getIcon("psi/quit"), tr("Close"));
	QPushButton *Save = new QPushButton(icoHost_->getIcon("psi/save"), tr("Save Changes"));
	QPushButton *Delete = new QPushButton(icoHost_->getIcon("psi/remove"), tr("Delete Log"));
	QPushButton *Update = new QPushButton(icoHost_->getIcon("psi/reload"), tr("Update Log"));
	QHBoxLayout *butLayout = new QHBoxLayout();
	butLayout->addWidget(Delete);
	butLayout->addStretch();
	butLayout->addWidget(Update);
	butLayout->addWidget(Save);
	butLayout->addWidget(Close);
	layout->addWidget(findBar);
	layout->addLayout(butLayout);
	connect(Close, SIGNAL(released()), this, SLOT(close()));
	connect(Delete, SIGNAL(released()), this, SLOT(deleteLog()));
	connect(Save, SIGNAL(released()), this, SLOT(saveLog()));
	connect(Update, SIGNAL(released()), this, SLOT(updateLog()));

	connect(findBar, SIGNAL(firstPage()), this, SLOT(firstPage()));
	connect(findBar, SIGNAL(lastPage()), this, SLOT(lastPage()));
	connect(findBar, SIGNAL(prevPage()), this, SLOT(prevPage()));
	connect(findBar, SIGNAL(nextPage()), this, SLOT(nextPage()));
}

void ViewLog::closeEvent(QCloseEvent *e)
{
	emit onClose(width(), height());
	QDialog::closeEvent(e);
	e->accept();
}

void ViewLog::deleteLog() {
	int ret = QMessageBox::question(this, tr("Delete log file"),
					tr("Are you sure?"), QMessageBox::Yes, QMessageBox::Cancel);
	if(ret == QMessageBox::Cancel) {
		return;
	}
	close();
	QFile file(fileName_);
	if(file.open(QIODevice::ReadWrite)) {
		file.remove();
	}
}

void ViewLog::saveLog() {
	QDateTime Modified = QFileInfo(fileName_).lastModified();
	if(lastModified_ < Modified) {
		QMessageBox msgBox;
		msgBox.setWindowTitle(tr("Save log"));
		msgBox.setText(tr("New messages has been added to log. If you save your changes, you will lose them"));
		msgBox.setInformativeText(tr("Do you want to save your changes?"));
		msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Cancel);
		int ret = msgBox.exec();
		if(ret == QMessageBox::Cancel) {
			return;
		}
	} else {
		int ret = QMessageBox::question(this, tr("Save log"),
						tr("Are you sure?"), QMessageBox::Yes, QMessageBox::Cancel);
		if(ret == QMessageBox::Cancel) {
			return;
		}
	}
	QFile file(fileName_);
	if(file.open(QIODevice::ReadWrite)) {
		file.remove();
	}
	if(file.open(QIODevice::ReadWrite)) {
		QTextStream out(&file);
		out.setCodec("UTF-8");
		QString Text = textWid->toPlainText();
		pages_.insert(currentPage_, Text);
		for(int i = 0; i < pages_.size(); i++) {
			out.setGenerateByteOrderMark(false);
			out << pages_.value(i);
		}
	}
}

void ViewLog::updateLog() {
	pages_.clear();
	init();
}

bool ViewLog::init()
{
	bool b = false;
	QFile file(fileName_);
	if(file.open(QIODevice::ReadOnly)) {
		QString page;
		int numPage = 0;
		QTextStream in(&file);
		in.setCodec("UTF-8");
		while(!in.atEnd()) {
			page = "";
			for(int i = 0; i < 500; i++) {
				if(in.atEnd())
					break;
				page += in.readLine() + "\n";
			}
			pages_.insert(numPage++, page);
		}
		currentPage_ = pages_.size()-1;
		lastModified_ = QDateTime::currentDateTime();
		setPage();
		b = true;
	}
	return b;
}

void ViewLog::setPage()
{
	QString text = pages_.value(currentPage_);
	textWid->setText(text);
	QTextCursor cur = textWid->textCursor();
	cur.setPosition(text.length());
	textWid->setTextCursor(cur);
}

void ViewLog::nextPage()
{
	if(currentPage_ < pages_.size()-1)
		currentPage_++;
	setPage();
}

void ViewLog::prevPage()
{
	if(currentPage_ > 0)
		currentPage_--;
	setPage();
}

void ViewLog::lastPage()
{
	currentPage_ = pages_.size()-1;
	setPage();
}

void ViewLog::firstPage()
{
	currentPage_ = 0;
	setPage();
}
