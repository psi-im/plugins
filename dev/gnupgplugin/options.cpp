/*
 * options.cpp - plugin widget
 *
 * Copyright (C) 2013  Ivan Romanov <drizt@land.ru>
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

#include <QInputDialog>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QItemSelectionModel>
#include <QProgressDialog>
#include "options.h"
#include "ui_options.h"
#include "model.h"
#include "gpgprocess.h"
#include "addkeydlg.h"
#include <QDebug>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>

Options::Options(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::Options)
{
	ui->setupUi(this);

	Model *model = new Model(this);
	ui->keys->setModel(model);

}

Options::~Options()
{
	delete ui;
}

void Options::update()
{
}

void Options::addKey()
{
	AddKeyDlg dlg(this);
	if (dlg.exec() == QDialog::Rejected) {
		return;
	}

	QString key;
	QString type, stype, length, name, comment, email, expiration, pass;
	switch (dlg.type()) {
	case 0: type = stype = "RSA"; break;
	case 1: type = "DSA"; stype = "ELG-E"; break;
	case 2: type = "DSA"; break;
	case 3: type = "RSA"; break;
	}

	length = QString::number(dlg.length());
	name = dlg.name();
	comment = dlg.comment();
	email = dlg.email();
	expiration = dlg.isExpired() ? dlg.expiration().toString(Qt::ISODate) : "0";
	pass = dlg.pass();

	key += QString("Key-Type: %1\n").arg(type);
	key += QString("Key-Length: %2\n").arg(length);
	if (!stype.isEmpty()) {
		key += QString("Subkey-Type: %1\n").arg(stype);
		key += QString("Subkey-Length: %2\n").arg(length);
	}

	if (!name.isEmpty()) {
		key += QString("Name-Real: %1\n").arg(name);
	}

	if (!comment.isEmpty()) {
		key += QString("Name-Comment: %1\n").arg(comment);
	}

	if (!email.isEmpty()) {
		key += QString("Name-Email: %1\n").arg(email);
	}

	key += QString("Expire-Date: %1\n").arg(expiration);

	if (!pass.isEmpty()) {
		key += QString("Passphrase: %1\n").arg(pass);
	}

	key += "%commit\n";

	QString progressText = trUtf8(
"<b>Please wait.</b><br/>"
"We need to generate a lot of random bytes. It is a good idea to perform<br/>"
"some other action (type on the keyboard, move the mouse, utilize the<br/>"
"disks) during the prime generation; this gives the random number<br/>"
"generator a better chance to gain enough entropy.");

	QProgressDialog waitingDlg(progressText, trUtf8("Cancel"), 0, 0, this);
	waitingDlg.setWindowModality(Qt::WindowModal);
	waitingDlg.setWindowTitle(trUtf8("Key pair generating"));
	waitingDlg.show();

	GpgProcess gpg;
	QStringList arguments;
	arguments << "--batch"
			  << "--gen-key";

	gpg.start(arguments);
	gpg.waitForStarted();
	gpg.write(key.toUtf8());
	gpg.closeWriteChannel();
	while (gpg.state() == QProcess::Running) {
		gpg.waitForFinished(1);
		if (waitingDlg.wasCanceled()) {
			gpg.terminate();
			break;
		}
		qApp->processEvents();
	}

	qobject_cast<Model*>(ui->keys->model())->listKeys();
}

void Options::removeKey()
{
	Model *model = qobject_cast<Model*>(ui->keys->model());
	QItemSelectionModel *selModel = ui->keys->selectionModel();

	if (!selModel->hasSelection()) {
		return;
	}

	QModelIndexList indexes = selModel->selectedIndexes();
	QModelIndexList pkeys; // Key IDs
	foreach (QModelIndex index, indexes) {
		// Every selection contains all columns. Need to work only with first
		if (index.column() > 0) {
			continue;
		}

		// Choose only primary keys
		QModelIndex pIndex = index;
		if (index.parent().isValid()) {
			pIndex = index.parent();
		}

		if (pkeys.indexOf(pIndex) < 0) {
			pkeys << pIndex;
		}
	}

	// Remove primary keys
	foreach (QModelIndex key, pkeys) {
		GpgProcess gpg;
		QStringList arguments;
		arguments << "--yes"
				  << "--batch"
				  << "--delete-secret-and-public-key"
				  << "0x" + key.sibling(key.row(), 8).data().toString();

		gpg.start(arguments);
		gpg.waitForFinished();
	}

	model->listKeys();
}

void Options::importKey()
{
	QFileDialog dlg(this);
	dlg.setFileMode(QFileDialog::ExistingFiles);
	QStringList nameFilters;
	nameFilters << trUtf8("ASCII (*.asc)")
				<< trUtf8("All files (*)");
	dlg.setNameFilters(nameFilters);
	if (dlg.exec() == QDialog::Rejected) {
		return;
	}

	QStringList allFiles = dlg.selectedFiles();
	foreach (QString filename, allFiles) {
		GpgProcess gpg;
		QStringList arguments;
		arguments << "--batch"
				  << "--import"
				  << filename;
		gpg.start(arguments);
		gpg.waitForFinished();
	}

	qobject_cast<Model*>(ui->keys->model())->listKeys();
}

void Options::exportKey()
{
	Model *model = qobject_cast<Model*>(ui->keys->model());
	QItemSelectionModel *selModel = ui->keys->selectionModel();

	if (!selModel->hasSelection()) {
		return;
	}

	QModelIndexList indexes = selModel->selectedIndexes();
	QModelIndexList pkeys; // Key IDs
	foreach (QModelIndex index, indexes) {
		// Every selection contains all columns. Need to work only with first
		if (index.column() > 0) {
			continue;
		}

		// Choose only primary keys
		QModelIndex pIndex = index;
		if (index.parent().isValid()) {
			pIndex = index.parent();
		}

		if (pkeys.indexOf(pIndex) < 0) {
			pkeys << pIndex;
		}
	}

	// Remove primary keys
	foreach (QModelIndex key, pkeys) {
		QString filename = key.sibling(key.row(), 1).data().toString() + " " + key.sibling(key.row(), 2).data().toString() + ".asc";
		QFileDialog dlg(this);
		dlg.setFileMode(QFileDialog::AnyFile);
		QStringList nameFilters;
		nameFilters << trUtf8("ASCII (*.asc)");
		dlg.setNameFilters(nameFilters);
		dlg.selectFile(filename);
		if (dlg.exec() == QDialog::Rejected) {
			break;
		}

		filename = dlg.selectedFiles().first();
		if (filename.right(4) != ".asc") {
			filename += ".asc";
		}

		GpgProcess gpg;
		QStringList arguments;
		QString fingerprint = "0x" + key.sibling(key.row(), 8).data().toString();
		arguments << "--output"
				  << filename
				  << "--armor"
				  << "--export"
				  << fingerprint;

		gpg.start(arguments);
		gpg.waitForFinished();
	}

	model->listKeys();
}

void Options::showInfo()
{
	GpgProcess gpg;
	QMessageBox box(QMessageBox::Information, trUtf8("GnuPG info"), gpg.info(), QMessageBox::Ok, this);
	box.exec();
}
