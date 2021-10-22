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
#include "accountinfoaccessinghost.h"
#include "addkeydlg.h"
#include "gpgprocess.h"
#include "model.h"
#include "optionaccessinghost.h"
#include "pgpkeydlg.h"
#include "pgputil.h"
#include "psiaccountcontrollinghost.h"
#include "showtextdlg.h"
#include "ui_options.h"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
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
#include <QStringList>
#include <QTimer>
#include <QUrl>

using OpenPgpPluginNamespace::GpgProcess;

Options::Options(QWidget *parent) : QWidget(parent), m_ui(new Ui::Options)
{
    m_ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    {
        m_allKeysTableModel = new Model(this);
        m_ui->allKeysTable->setModel(m_allKeysTableModel);
        connect(m_allKeysTableModel, &Model::keysListUpdated, this, &Options::allKeysTableModelUpdated);

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
        m_ui->knownKeysTable->setShowGrid(true);
        m_ui->knownKeysTable->setEditTriggers(QAbstractItemView::EditTriggers());
        m_ui->knownKeysTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_ui->knownKeysTable->setSortingEnabled(true);

        m_ui->knownKeysTable->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_ui->knownKeysTable, &QTableView::customContextMenuRequested, this, &Options::contextMenuKnownKeys);

        m_knownKeysTableModel = new QStandardItemModel(this);
        m_ui->knownKeysTable->setModel(m_knownKeysTableModel);

        connect(m_ui->deleteKnownKey, &QPushButton::clicked, this, &Options::deleteKnownKey);
    }
    {
        m_ui->ownKeysTable->setShowGrid(true);
        m_ui->ownKeysTable->setEditTriggers(QAbstractItemView::EditTriggers());
        m_ui->ownKeysTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_ui->ownKeysTable->setSortingEnabled(true);

        m_ui->ownKeysTable->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_ui->ownKeysTable, &QTableView::customContextMenuRequested, this, &Options::contextMenuOwnKeys);

        m_ownKeysTableModel = new QStandardItemModel(this);
        m_ui->ownKeysTable->setModel(m_ownKeysTableModel);

        connect(m_ui->chooseKey, &QPushButton::clicked, this, &Options::chooseKey);
        connect(m_ui->deleteOwnKey, &QPushButton::clicked, this, &Options::deleteOwnKey);
    }
    {
        connect(m_ui->openGpgAgentConfig, &QPushButton::clicked, this, &Options::openGpgAgentConfig);
    }

    m_ui->tabWidget->setCurrentWidget(m_ui->knownKeysTab);
}

Options::~Options() { delete m_ui; }

void Options::setOptionAccessingHost(OptionAccessingHost *host) { m_optionHost = host; }

void Options::setAccountInfoAccessingHost(AccountInfoAccessingHost *host)
{
    m_accountInfo = host;

    updateAccountsList();
    updateKnownKeys();
    updateOwnKeys();
}

void Options::setPsiAccountControllingHost(PsiAccountControllingHost *host) { m_accountHost = host; }

void Options::loadSettings()
{
    { // Encryption policy
        m_ui->alwaysEnabled->setChecked(m_optionHost->getGlobalOption("options.pgp.always-enabled").toBool());
        m_ui->enabledByDefault->setChecked(m_optionHost->getGlobalOption("options.pgp.enabled-by-default").toBool());
        m_ui->disabledByDefault->setChecked(!m_ui->enabledByDefault->isChecked());
    }
    m_ui->autoAssign->setChecked(m_optionHost->getGlobalOption("options.pgp.auto-assign").toBool());
    m_ui->showPgpInfoInTooltips->setChecked(
        m_optionHost->getGlobalOption("options.ui.contactlist.tooltip.pgp").toBool());
    m_ui->autoImportPgpKeyFromMessage->setChecked(m_optionHost->getPluginOption("auto-import", true).toBool());
    m_ui->hideMessagesWithPgpKeys->setChecked(m_optionHost->getPluginOption("hide-key-message", true).toBool());
    m_ui->doNotSignPresence->setChecked(!m_optionHost->getPluginOption("sign-presence", true).toBool());

    loadGpgAgentConfigData();
}

void Options::saveSettings()
{
    { // Encryption policy
        m_optionHost->setGlobalOption("options.pgp.always-enabled", m_ui->alwaysEnabled->isChecked());
        m_optionHost->setGlobalOption("options.pgp.enabled-by-default", m_ui->enabledByDefault->isChecked());
    }
    m_optionHost->setGlobalOption("options.pgp.auto-assign", m_ui->autoAssign->isChecked());
    m_optionHost->setGlobalOption("options.ui.contactlist.tooltip.pgp", m_ui->showPgpInfoInTooltips->isChecked());
    m_optionHost->setPluginOption("auto-import", m_ui->autoImportPgpKeyFromMessage->isChecked());
    m_optionHost->setPluginOption("hide-key-message", m_ui->hideMessagesWithPgpKeys->isChecked());
    m_optionHost->setPluginOption("sign-presence", !m_ui->doNotSignPresence->isChecked());

    updateGpgAgentConfig(m_ui->pwdExpirationTime->value());
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
    QItemSelectionModel *selModel = m_ui->allKeysTable->selectionModel();

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
        const auto result = QMessageBox::question(this, tr("Delete"), tr("Do you want to delete the selected keys?"),
                                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (result == QMessageBox::No) {
            return;
        }
    }

    // Remove primary keys
    for (auto key : qAsConst(pkeys)) {
        const QStringList &&arguments = { "--yes", "--batch", "--delete-secret-and-public-key",
                                          "0x" + key.sibling(key.row(), Model::Fingerprint).data().toString() };

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
    for (const auto &filename : allFiles) {
        const QStringList &&arguments = { "--batch", "--import", filename };

        GpgProcess gpg;
        gpg.start(arguments);
        gpg.waitForFinished();
    }

    updateAllKeys();
}

void Options::exportKeyToFile()
{
    QItemSelectionModel *selModel = m_ui->allKeysTable->selectionModel();

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
    for (auto key : qAsConst(pkeys)) {
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

        filename = dlg.selectedFiles().constFirst();
        if (filename.right(4) != ".asc") {
            filename += ".asc";
        }

        const QString &&    fingerprint = "0x" + key.sibling(key.row(), 8).data().toString();
        const QStringList &&arguments   = { "--output", filename, "--armor", "--export", fingerprint };

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
    QItemSelectionModel *selModel = m_ui->allKeysTable->selectionModel();

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
    for (auto key : qAsConst(pkeys)) {
        const QString &&    fingerprint = "0x" + key.sibling(key.row(), 8).data().toString();
        const QStringList &&arguments   = { "--armor", "--export", fingerprint };

        GpgProcess gpg;
        gpg.start(arguments);
        gpg.waitForFinished();

        strKey += QString::fromUtf8(gpg.readAllStandardOutput());
    }

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(strKey.toUtf8().trimmed());
}

void Options::showInfo() { PGPKeyDlg::showInfoDialog(this); }

void Options::updateAllKeys() { m_allKeysTableModel->updateAllKeys(); }

void Options::allKeysTableModelUpdated()
{
    const int columns = m_ui->allKeysTable->model()->columnCount();
    for (int i = 0; i < columns; ++i) {
        m_ui->allKeysTable->resizeColumnToContents(i);
    }
}

void Options::updateAccountsList()
{
    if (!m_accountInfo)
        return;

    QString currAccount;
    if (m_ui->accounts->count() > 0) {
        currAccount = m_ui->accounts->currentText();
        m_ui->accounts->clear();
    }

    for (int idx = 0; m_accountInfo->getId(idx) != "-1"; ++idx) {
        m_ui->accounts->addItem(m_accountInfo->getName(idx), QVariant(idx));
    }

    if (!currAccount.isEmpty()) {
        m_ui->accounts->setCurrentText(currAccount);
    } else {
        m_ui->accounts->setCurrentIndex(0);
    }
}

void Options::updateKnownKeys()
{
    if (!m_accountInfo)
        return;

    const int           sortSection = m_ui->knownKeysTable->horizontalHeader()->sortIndicatorSection();
    const Qt::SortOrder sortOrder   = m_ui->knownKeysTable->horizontalHeader()->sortIndicatorOrder();

    {
        const QStringList &&headerLabels
            = { tr("Account"), tr("User"), tr("Key ID"), tr("User ID"), tr("Fingerprint") };

        m_knownKeysTableModel->clear();
        m_knownKeysTableModel->setColumnCount(5);
        m_knownKeysTableModel->setHorizontalHeaderLabels(headerLabels);
    }

    for (int idx = 0; m_accountInfo->getId(idx) != "-1"; ++idx) {
        const auto knownKeysMap = m_accountInfo->getKnownPgpKeys(idx);
        if (knownKeysMap.isEmpty())
            continue;

        const auto users = knownKeysMap.keys();
        for (const QString &user : users) {
            QStandardItem *accItem = new QStandardItem(m_accountInfo->getName(idx));
            accItem->setData(QVariant(idx));

            QStandardItem *userItem = new QStandardItem(user);
            QStandardItem *keyItem  = new QStandardItem(knownKeysMap[user]);

            const QString &&userId     = PGPUtil::getUserId(knownKeysMap[user]);
            QStandardItem * userIdItem = new QStandardItem(userId);

            const QString &&fingerprint     = PGPUtil::getFingerprint(knownKeysMap[user]);
            QStandardItem * fingerprintItem = new QStandardItem(fingerprint);

            const QList<QStandardItem *> &&row = { accItem, userItem, keyItem, userIdItem, fingerprintItem };
            m_knownKeysTableModel->appendRow(row);
        }
    }

    m_ui->knownKeysTable->sortByColumn(sortSection, sortOrder);
    m_ui->knownKeysTable->resizeColumnsToContents();
}

void Options::updateOwnKeys()
{
    if (!m_accountInfo)
        return;

    const int           sortSection = m_ui->ownKeysTable->horizontalHeader()->sortIndicatorSection();
    const Qt::SortOrder sortOrder   = m_ui->ownKeysTable->horizontalHeader()->sortIndicatorOrder();

    {
        const QStringList &&headerLabels = { tr("Account"), tr("Key ID"), tr("User ID"), tr("Fingerprint") };

        m_ownKeysTableModel->clear();
        m_ownKeysTableModel->setColumnCount(4);
        m_ownKeysTableModel->setHorizontalHeaderLabels(headerLabels);
    }

    for (int idx = 0; m_accountInfo->getId(idx) != "-1"; ++idx) {
        const QString &&keyId = m_accountInfo->getPgpKey(idx);
        if (keyId.isEmpty())
            continue;

        QStandardItem *accItem = new QStandardItem(m_accountInfo->getName(idx));
        accItem->setData(QVariant(idx));

        QStandardItem *keyItem = new QStandardItem(keyId);

        const QString &&userId     = PGPUtil::getUserId(keyId);
        QStandardItem * userIdItem = new QStandardItem(userId);

        const QString &&fingerprint     = PGPUtil::getFingerprint(keyId);
        QStandardItem * fingerprintItem = new QStandardItem(fingerprint);

        const QList<QStandardItem *> &&row = { accItem, keyItem, userIdItem, fingerprintItem };
        m_ownKeysTableModel->appendRow(row);
    }

    m_ui->ownKeysTable->sortByColumn(sortSection, sortOrder);
    m_ui->ownKeysTable->resizeColumnsToContents();
}

void Options::deleteKnownKey()
{
    if (!m_accountInfo || !m_accountHost)
        return;

    if (!m_ui->knownKeysTable->selectionModel()->hasSelection())
        return;

    bool       keyRemoved = false;
    const auto indexes    = m_ui->knownKeysTable->selectionModel()->selectedRows(0);
    for (auto selectIndex : indexes) {
        const QVariant &accountId = m_knownKeysTableModel->item(selectIndex.row(), 0)->data();
        if (accountId.isNull())
            continue;

        const QString &jid = m_knownKeysTableModel->item(selectIndex.row(), 1)->text();
        if (jid.isEmpty())
            continue;

        const QString &accountName = m_knownKeysTableModel->item(selectIndex.row(), 0)->text();
        const QString &userName    = m_knownKeysTableModel->item(selectIndex.row(), 1)->text();
        const QString &fingerprint = m_knownKeysTableModel->item(selectIndex.row(), 4)->text();

        const QString msg(tr("Are you sure you want to delete the following key?") + "\n\n" + tr("Account: ")
                          + accountName + "\n" + tr("User: ") + userName + "\n" + tr("Fingerprint: ") + fingerprint);

        QMessageBox mb(QMessageBox::Question, tr("Confirm action"), msg, QMessageBox::Yes | QMessageBox::No, this);
        if (mb.exec() == QMessageBox::Yes) {
            m_accountHost->removeKnownPgpKey(accountId.toInt(), jid);
            keyRemoved = true;
        }
    }

    if (keyRemoved)
        updateKnownKeys();
}

void Options::deleteOwnKey()
{
    if (!m_accountInfo || !m_accountHost)
        return;

    if (!m_ui->ownKeysTable->selectionModel()->hasSelection())
        return;

    bool       keyRemoved = false;
    const auto indexes    = m_ui->ownKeysTable->selectionModel()->selectedRows(0);
    for (auto selectIndex : indexes) {
        const QVariant &accountId = m_ownKeysTableModel->item(selectIndex.row(), 0)->data().toString();
        if (accountId.isNull())
            continue;

        const QString &accountName = m_ownKeysTableModel->item(selectIndex.row(), 0)->text();
        const QString &fingerprint = m_ownKeysTableModel->item(selectIndex.row(), 3)->text();

        const QString msg(tr("Are you sure you want to delete the following key?") + "\n\n" + tr("Account: ")
                          + accountName + "\n" + tr("Fingerprint: ") + fingerprint);

        QMessageBox mb(QMessageBox::Question, tr("Confirm action"), msg, QMessageBox::Yes | QMessageBox::No, this);
        if (mb.exec() == QMessageBox::Yes) {
            m_accountHost->setPgpKey(accountId.toInt(), QString());
            keyRemoved = true;
        }
    }

    if (keyRemoved)
        updateOwnKeys();
}

void Options::chooseKey()
{
    if (!m_accountInfo || !m_accountHost)
        return;

    const QVariant &accountId = m_ui->accounts->currentData();
    if (accountId.isNull())
        return;

    const int idx = accountId.toInt();
    if (m_accountInfo->getId(idx) == "-1")
        return;

    const QString &&pgpKeyId = m_accountInfo->getPgpKey(idx);
    const QString &&newKeyId = PGPUtil::chooseKey(PGPKeyDlg::Secret, pgpKeyId, tr("Choose Secret Key"));

    if (newKeyId.isEmpty())
        return;

    m_accountHost->setPgpKey(idx, newKeyId);
    updateOwnKeys();
}

void Options::copyKnownFingerprint()
{
    if (!m_ui->knownKeysTable->selectionModel()->hasSelection())
        return;

    copyFingerprintFromTable(m_knownKeysTableModel, m_ui->knownKeysTable->selectionModel()->selectedRows(4), 4);
}

void Options::copyOwnFingerprint()
{
    if (!m_ui->ownKeysTable->selectionModel()->hasSelection())
        return;

    copyFingerprintFromTable(m_ownKeysTableModel, m_ui->ownKeysTable->selectionModel()->selectedRows(3), 3);
}

void Options::contextMenuKnownKeys(const QPoint &pos)
{
    QModelIndex index = m_ui->knownKeysTable->indexAt(pos);
    if (!index.isValid())
        return;

    QMenu *menu = new QMenu(this);

    // TODO: update after stopping support of Ubuntu Xenial:
    menu->addAction(QIcon::fromTheme("edit-delete"), tr("Delete"), this, SLOT(deleteKnownKey()));
    menu->addAction(QIcon::fromTheme("edit-copy"), tr("Copy fingerprint"), this, SLOT(copyKnownFingerprint()));

    menu->exec(QCursor::pos());
}

void Options::contextMenuOwnKeys(const QPoint &pos)
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

void Options::openGpgAgentConfig() { QDesktopServices::openUrl(QUrl::fromLocalFile(GpgProcess().gpgAgentConfig())); }

void Options::loadGpgAgentConfigData()
{
    const QString &&configText = PGPUtil::readGpgAgentConfig();
    if (configText.isEmpty())
        return;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    const auto lines = configText.split("\n", Qt::SkipEmptyParts);
#else
    const auto lines = configText.split("\n", QString::SkipEmptyParts);
#endif
    for (const QString &line : lines) {
        if (line.contains("default-cache-ttl")) {
            QString str(line);
            str.replace("default-cache-ttl", "");
            str.replace(" ", "");
            str.replace("\t", "");
            str.replace("\r", "");
            const int value = str.toInt();
            if (value >= 60) {
                m_ui->pwdExpirationTime->setValue(value);
            }
            return;
        }
    }
}

void Options::updateGpgAgentConfig(const int pwdExpirationTime)
{
    QString configText = PGPUtil::readGpgAgentConfig();
    if (!configText.contains("default-cache-ttl")) {
        configText = PGPUtil::readGpgAgentConfig(true);
    }

    QStringList lines = configText.split("\n");
    for (QString &line : lines) {
        if (line.contains("default-cache-ttl")) {
            line = QString("default-cache-ttl ") + QString::number(pwdExpirationTime);
        } else if (line.contains("max-cache-ttl")) {
            line = QString("max-cache-ttl ") + QString::number(pwdExpirationTime);
        }
    }
    if (PGPUtil::saveGpgAgentConfig(lines.join("\n"))) {
        if (!GpgProcess().reloadGpgAgentConfig()) {
            const QString &&message = tr("Attempt to reload gpg-agent config is failed. "
                                         "You need to restart your system to see changes "
                                         "in gpg-agent settings.");
            QMessageBox     msgbox(QMessageBox::Warning, tr("Warning"), message, QMessageBox::Ok, nullptr);
            msgbox.exec();
        }
    } else {
        const QString &&message = tr("Attempt to save gpg-agent config is failed! "
                                     "Check that you have write permission for file:\n%1")
                                      .arg(GpgProcess().gpgAgentConfig());
        QMessageBox msgbox(QMessageBox::Warning, tr("Warning"), message, QMessageBox::Ok, nullptr);
        msgbox.exec();
    }
}

void Options::copyFingerprintFromTable(QStandardItemModel *tableModel, const QModelIndexList &indexesList,
                                       const int column)
{
    QString text;
    for (auto selectIndex : indexesList) {
        if (!text.isEmpty()) {
            text += "\n";
        }
        text += tableModel->item(selectIndex.row(), column)->text();
    }
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(text);
}
