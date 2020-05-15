/*
 * options.cpp - plugin widget
 *
 * Copyright (C) 2013  Ivan Romanov <drizt@land.ru>
 * Copyright (C) 2020  Boris Pek <tehnick-8@yandex.ru>
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
#include "showtextdlg.h"
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
#include <QTimer>

Options::Options(QWidget *parent) : QWidget(parent), m_ui(new Ui::Options)
{
    m_ui->setupUi(this);

    {
        Model *model = new Model(this);
        m_ui->keys->setModel(model);

        // Delayed init
        QTimer::singleShot(500, this, &Options::updateAllKeys);

        // Import key
        QAction *action;
        QMenu *  menu = new QMenu(this);

        action = menu->addAction(tr("from file"));
        connect(action, &QAction::triggered, this, &Options::importKeyFromFile);

        action = menu->addAction(tr("from clipboard"));
        connect(action, &QAction::triggered, this, &Options::importKeyFromClipboard);

        m_ui->btnImport->setMenu(menu);

        // Export key

        menu   = new QMenu(this);
        action = menu->addAction(tr("to file"));
        connect(action, &QAction::triggered, this, &Options::exportKeyToFile);
        m_ui->btnExport->addAction(action);

        action = menu->addAction(tr("to clipboard"));
        connect(action, &QAction::triggered, this, &Options::exportKeyToClipboard);

        m_ui->btnExport->setMenu(menu);
    }
    {
        m_ui->ownKeysTable->setShowGrid(true);
        m_ui->ownKeysTable->setEditTriggers(nullptr);
        m_ui->ownKeysTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_ui->ownKeysTable->setSortingEnabled(true);

        m_ui->ownKeysTable->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_ui->ownKeysTable, &QTableView::customContextMenuRequested, this, &Options::contextMenu);

        m_ownKeysTableModel = new QStandardItemModel(this);
        m_ui->ownKeysTable->setModel(m_ownKeysTableModel);

        updateOwnKeys();
    }

    m_ui->tabWidget->removeTab(1); // Temporary!!!
}

Options::~Options()
{
    delete m_ui;
}

void Options::updateOwnKeys()
{
    ; // TODO
}

void Options::setOptionAccessingHost(OptionAccessingHost *host) { m_optionHost = host; }

void Options::loadSettings()
{
    {   // Encryption policy
        m_ui->alwaysEnabled->setChecked(m_optionHost->getGlobalOption("options.pgp.always-enabled").toBool());
        m_ui->enabledByDefault->setChecked(m_optionHost->getGlobalOption("options.pgp.enabled-by-default").toBool());
        m_ui->disabledByDefault->setChecked(!m_ui->enabledByDefault->isChecked());
    }
    m_ui->autoAssign->setChecked(m_optionHost->getGlobalOption("options.pgp.auto-assign").toBool());
    m_ui->showPgpInfoInTooltips->setChecked(m_optionHost->getGlobalOption("options.ui.contactlist.tooltip.pgp").toBool());
    m_ui->autoImportPgpKeyFromMessage->setChecked(m_optionHost->getPluginOption("auto-import", true).toBool());
    m_ui->hideMessagesWithPgpKeys->setChecked(m_optionHost->getPluginOption("hide-key-message", true).toBool());
}

void Options::saveSettings()
{
    {   // Encryption policy
        m_optionHost->setGlobalOption("options.pgp.always-enabled", m_ui->alwaysEnabled->isChecked());
        m_optionHost->setGlobalOption("options.pgp.enabled-by-default", m_ui->enabledByDefault->isChecked());
    }
    m_optionHost->setGlobalOption("options.pgp.auto-assign", m_ui->autoAssign->isChecked());
    m_optionHost->setGlobalOption("options.ui.contactlist.tooltip.pgp", m_ui->showPgpInfoInTooltips->isChecked());
    m_optionHost->setPluginOption("auto-import", m_ui->autoImportPgpKeyFromMessage->isChecked());
    m_optionHost->setPluginOption("hide-key-message", m_ui->hideMessagesWithPgpKeys->isChecked());
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

    const QStringList &&arguments = { "--batch", "--gen-key" };

    GpgProcess gpg;
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

    updateAllKeys();
}

void Options::deleteKey()
{
    QItemSelectionModel *selModel = m_ui->keys->selectionModel();

    if (!selModel->hasSelection()) {
        return;
    }

    QModelIndexList indexes = selModel->selectedIndexes();
    QModelIndexList pkeys; // Key IDs
    for (auto index : indexes) {
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
    for (auto key : pkeys) {
        const QStringList &&arguments = {
            "--yes",
            "--batch",
            "--delete-secret-and-public-key",
            "0x" + key.sibling(key.row(), Model::Fingerprint).data().toString()
        };

        GpgProcess gpg;
        gpg.start(arguments);
        gpg.waitForFinished();
    }

    updateAllKeys();
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
    for (auto filename : allFiles) {
        const QStringList &&arguments = { "--batch", "--import", filename };

        GpgProcess gpg;
        gpg.start(arguments);
        gpg.waitForFinished();
    }

    updateAllKeys();
}

void Options::exportKeyToFile()
{
    QItemSelectionModel *selModel = m_ui->keys->selectionModel();

    if (!selModel->hasSelection()) {
        return;
    }

    QModelIndexList indexes = selModel->selectedIndexes();
    QModelIndexList pkeys; // Key IDs
    for (auto index : indexes) {
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
    for (auto key : pkeys) {
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


        const QString &&fingerprint = "0x" + key.sibling(key.row(), 8).data().toString();
        const QStringList &&arguments = {
            "--output",
            filename,
            "--armor",
            "--export",
            fingerprint
        };

        GpgProcess gpg;
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

    const QStringList &&arguments = { "--batch", "--import" };

    GpgProcess gpg;
    gpg.start(arguments);
    gpg.waitForStarted();
    gpg.write(key.toUtf8());
    gpg.closeWriteChannel();
    gpg.waitForFinished();

    updateAllKeys();
}

void Options::exportKeyToClipboard()
{
    QItemSelectionModel *selModel = m_ui->keys->selectionModel();

    if (!selModel->hasSelection()) {
        return;
    }

    QModelIndexList indexes = selModel->selectedIndexes();
    QModelIndexList pkeys; // Key IDs
    for (auto index : indexes) {
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
    for (auto key : pkeys) {
        const QString &&fingerprint = "0x" + key.sibling(key.row(), 8).data().toString();
        const QStringList &&arguments = {
            "--armor",
            "--export",
            fingerprint
        };

        GpgProcess gpg;
        gpg.start(arguments);
        gpg.waitForFinished();

        strKey += QString::fromUtf8(gpg.readAllStandardOutput());
    }

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(strKey.toUtf8().trimmed());
}

void Options::showInfo()
{
    GpgProcess gpg;
    QString    info;

    gpg.info(info);
    ShowTextDlg *w = new ShowTextDlg(info, true, false, this);
    w->setWindowTitle(tr("GnuPG info"));
    w->resize(560, 240);
    w->show();
}

void Options::updateAllKeys()
{
    qobject_cast<Model *>(m_ui->keys->model())->listKeys();

    int columns = m_ui->keys->model()->columnCount();
    for (int i = 0; i < columns; i++) {
        m_ui->keys->resizeColumnToContents(i);
    }
}

void Options::deleteOwnKey()
{
    if (!m_ui->ownKeysTable->selectionModel()->hasSelection())
        return;

    ; // TODO

    updateOwnKeys();
}

void Options::copyOwnFingerprint()
{
    if (!m_ui->ownKeysTable->selectionModel()->hasSelection())
        return;

    QString text;
    for (auto selectIndex : m_ui->ownKeysTable->selectionModel()->selectedRows(1)) {
        if (!text.isEmpty()) {
            text += "\n";
        }
        text += m_ownKeysTableModel->item(selectIndex.row(), 1)->text();
    }
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(text);
}

void Options::contextMenu(const QPoint &pos)
{
    QModelIndex index = m_ui->ownKeysTable->indexAt(pos);
    if (!index.isValid())
        return;

    QMenu *menu = new QMenu(this);

    // TODO: update after stopping support of Ubuntu Xenial:
    menu->addAction(QIcon::fromTheme("edit-delete"), tr("Delete"), this, SLOT(deleteOwnKey()));
    menu->addAction(QIcon::fromTheme("edit-copy"), tr("Copy fingerprint"), this, SLOT(copyOwnFingerprint()));

    menu->exec(QCursor::pos());
}

