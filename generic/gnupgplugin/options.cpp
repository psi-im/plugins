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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "options.h"
#include "addkeydlg.h"
#include "gpgprocess.h"
#include "model.h"
#include "optionaccessinghost.h"
#include "ui_options.h"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QProgressBar>
#include <QProgressDialog>
#include <QStandardItem>
#include <QStandardItemModel>

Options::Options(QWidget *parent) : QWidget(parent), ui(new Ui::Options)
{
    ui->setupUi(this);

    Model *model = new Model(this);
    ui->keys->setModel(model);
    updateKeys();

    // Import key
    QAction *action;
    QMenu *  menu = new QMenu(this);

    action = menu->addAction(tr("from file"));
    connect(action, SIGNAL(triggered()), SLOT(importKeyFromFile()));

    action = menu->addAction(tr("from clipboard"));
    connect(action, SIGNAL(triggered()), SLOT(importKeyFromClipboard()));

    ui->btnImport->setMenu(menu);

    // Export key

    menu   = new QMenu(this);
    action = menu->addAction(tr("to file"));
    connect(action, SIGNAL(triggered()), SLOT(exportKeyToFile()));
    ui->btnExport->addAction(action);

    action = menu->addAction(tr("to clipboard"));
    connect(action, SIGNAL(triggered()), SLOT(exportKeyToClipboard()));

    ui->btnExport->setMenu(menu);
}

Options::~Options() { delete ui; }

void Options::update() { }

void Options::loadSettings()
{
    ui->chkAutoImport->setChecked(_optionHost->getPluginOption("auto-import", true).toBool());
    ui->chkHideKeyMessage->setChecked(_optionHost->getPluginOption("hide-key-message", true).toBool());
}

void Options::saveSettings()
{
    _optionHost->setPluginOption("auto-import", ui->chkAutoImport->isChecked());
    _optionHost->setPluginOption("hide-key-message", ui->chkHideKeyMessage->isChecked());
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
    case 0:
        type = stype = "RSA";
        break;
    case 1:
        type  = "DSA";
        stype = "ELG-E";
        break;
    case 2:
        type = "DSA";
        break;
    case 3:
        type = "RSA";
        break;
    }

    length     = QString::number(dlg.length());
    name       = dlg.name();
    comment    = dlg.comment();
    email      = dlg.email();
    expiration = dlg.expiration().isValid() ? dlg.expiration().toString(Qt::ISODate) : "0";
    pass       = dlg.pass();

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

    QProgressDialog waitingDlg("", tr("Cancel"), 0, 0, this);

    QLabel progressTextLabel(tr("<b>Please wait!</b><br/>"
                                "We need to generate a lot of random bytes. It is a good idea to perform "
                                "some other action (type on the keyboard, move the mouse, utilize the "
                                "disks) during the prime generation; this gives the random number "
                                "generator a better chance to gain enough entropy."),
                             &waitingDlg);
    progressTextLabel.setAlignment(Qt::AlignHCenter);
    progressTextLabel.setWordWrap(true);

    waitingDlg.setLabel(&progressTextLabel);

    QProgressBar progressBar(&waitingDlg);
    progressBar.setAlignment(Qt::AlignHCenter);
    progressBar.setMinimum(0);
    progressBar.setMaximum(0);

    waitingDlg.setBar(&progressBar);

    waitingDlg.setWindowModality(Qt::WindowModal);
    waitingDlg.setWindowTitle(tr("Key pair generating"));
    waitingDlg.show();

    GpgProcess  gpg;
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

    updateKeys();
}

void Options::removeKey()
{
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

    if (!pkeys.isEmpty()) {
        if (QMessageBox::question(this, tr("Delete"), tr("Do you want to delete the selected keys?"),
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
            == QMessageBox::No) {
            return;
        }
    }

    // Remove primary keys
    foreach (QModelIndex key, pkeys) {
        GpgProcess  gpg;
        QStringList arguments;
        arguments << "--yes"
                  << "--batch"
                  << "--delete-secret-and-public-key"
                  << "0x" + key.sibling(key.row(), Model::Fingerprint).data().toString();

        gpg.start(arguments);
        gpg.waitForFinished();
    }

    updateKeys();
}

void Options::importKeyFromFile()
{
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::ExistingFiles);
    QStringList nameFilters;
    nameFilters << tr("ASCII (*.asc)") << tr("All files (*)");
    dlg.setNameFilters(nameFilters);
    if (dlg.exec() == QDialog::Rejected) {
        return;
    }

    QStringList allFiles = dlg.selectedFiles();
    foreach (QString filename, allFiles) {
        GpgProcess  gpg;
        QStringList arguments;
        arguments << "--batch"
                  << "--import" << filename;
        gpg.start(arguments);
        gpg.waitForFinished();
    }

    updateKeys();
}

void Options::exportKeyToFile()
{
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
        QString filename
            = key.sibling(key.row(), 1).data().toString() + " " + key.sibling(key.row(), 2).data().toString() + ".asc";
        QFileDialog dlg(this);
        dlg.setAcceptMode(QFileDialog::AcceptSave);
        dlg.setFileMode(QFileDialog::AnyFile);
        QStringList nameFilters;
        nameFilters << tr("ASCII (*.asc)");
        dlg.setNameFilters(nameFilters);
        dlg.selectFile(filename);
        if (dlg.exec() == QDialog::Rejected) {
            break;
        }

        filename = dlg.selectedFiles().first();
        if (filename.right(4) != ".asc") {
            filename += ".asc";
        }

        GpgProcess  gpg;
        QStringList arguments;
        QString     fingerprint = "0x" + key.sibling(key.row(), 8).data().toString();
        arguments << "--output" << filename << "--armor"
                  << "--export" << fingerprint;

        gpg.start(arguments);
        gpg.waitForFinished();
    }
}

void Options::importKeyFromClipboard()
{
    QClipboard *clipboard = QApplication::clipboard();
    QString     key       = clipboard->text().trimmed();

    if (!key.startsWith("-----BEGIN PGP PUBLIC KEY BLOCK-----")
        || !key.endsWith("-----END PGP PUBLIC KEY BLOCK-----")) {
        return;
    }

    GpgProcess  gpg;
    QStringList arguments;
    arguments << "--batch"
              << "--import";
    gpg.start(arguments);
    gpg.waitForStarted();
    gpg.write(key.toUtf8());
    gpg.closeWriteChannel();
    gpg.waitForFinished();

    updateKeys();
}

void Options::exportKeyToClipboard()
{
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
    QString strKey = "";
    foreach (QModelIndex key, pkeys) {
        GpgProcess  gpg;
        QStringList arguments;
        QString     fingerprint = "0x" + key.sibling(key.row(), 8).data().toString();
        arguments << "--armor"
                  << "--export" << fingerprint;

        gpg.start(arguments);
        gpg.waitForFinished();

        strKey += QString::fromUtf8(gpg.readAllStandardOutput());
    }

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(strKey.toUtf8().trimmed());
}

void Options::showInfo()
{
    GpgProcess        gpg;
    QString           info;
    QMessageBox::Icon icon;
    if (gpg.info(info)) {
        icon = QMessageBox::Information;
    } else {
        icon = QMessageBox::Critical;
    }
    QMessageBox box(icon, tr("GnuPG info"), info, QMessageBox::Ok, this);
    box.exec();
}

void Options::updateKeys()
{
    qobject_cast<Model *>(ui->keys->model())->listKeys();

    int columns = ui->keys->model()->columnCount();
    for (int i = 0; i < columns; i++) {
        ui->keys->resizeColumnToContents(i);
    }
}
