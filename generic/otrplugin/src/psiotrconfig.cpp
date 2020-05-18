/*
 * psiotrconfig.cpp - Configuration dialogs
 *
 * Off-the-Record Messaging plugin for Psi
 * Copyright (C) 2007-2011  Timo Engel (timo-e@freenet.de)
 *                    2011  Florian Fieber
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "psiotrconfig.h"
#include "accountinfoaccessinghost.h"
#include "optionaccessinghost.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QModelIndex>
#include <QPoint>
#include <QPushButton>
#include <QRadioButton>
#include <QStandardItem>
#include <QTableView>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

//-----------------------------------------------------------------------------

namespace psiotr {

//-----------------------------------------------------------------------------

ConfigDialog::ConfigDialog(OtrMessaging *otr, OptionAccessingHost *optionHost, AccountInfoAccessingHost *accountInfo,
                           QWidget *parent) :
    QWidget(parent),
    m_otr(otr), m_optionHost(optionHost), m_accountInfo(accountInfo)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QTabWidget * tabWidget  = new QTabWidget(this);

    tabWidget->addTab(new FingerprintWidget(m_otr, tabWidget), tr("Known Keys"));
    tabWidget->addTab(new PrivKeyWidget(m_accountInfo, m_otr, tabWidget), tr("Own Keys"));
    tabWidget->addTab(new ConfigOtrWidget(m_optionHost, m_otr, tabWidget), tr("Configuration"));

    mainLayout->addWidget(tabWidget);
    setLayout(mainLayout);
}

//=============================================================================

ConfigOtrWidget::ConfigOtrWidget(OptionAccessingHost *optionHost, OtrMessaging *otr, QWidget *parent) :
    QWidget(parent), m_optionHost(optionHost), m_otr(otr)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QGroupBox *  policyGroup  = new QGroupBox(tr("OTR encryption policy"), this);
    QVBoxLayout *policyLayout = new QVBoxLayout(policyGroup);

    m_policy = new QButtonGroup(policyGroup);

    QRadioButton *polDisable = new QRadioButton(tr("Disable private messaging"), policyGroup);
    QRadioButton *polEnable  = new QRadioButton(tr("Manually start private messaging"), policyGroup);
    QRadioButton *polAuto    = new QRadioButton(tr("Automatically start private messaging"), policyGroup);
    QRadioButton *polRequire = new QRadioButton(tr("Require private messaging"), policyGroup);

    m_endWhenOffline = new QCheckBox(tr("End session when contact goes offline"), this);

    m_policy->addButton(polDisable, OTR_POLICY_OFF);
    m_policy->addButton(polEnable, OTR_POLICY_ENABLED);
    m_policy->addButton(polAuto, OTR_POLICY_AUTO);
    m_policy->addButton(polRequire, OTR_POLICY_REQUIRE);

    policyLayout->addWidget(polDisable);
    policyLayout->addWidget(polEnable);
    policyLayout->addWidget(polAuto);
    policyLayout->addWidget(polRequire);
    policyGroup->setLayout(policyLayout);

    auto verticalSpacer = new QLabel(this);
    verticalSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    layout->addWidget(policyGroup);
    layout->addWidget(m_endWhenOffline);
    layout->addWidget(verticalSpacer);

    setLayout(layout);

    int policyOption = m_optionHost->getPluginOption(OPTION_POLICY, DEFAULT_POLICY).toInt();
    if ((policyOption < OTR_POLICY_OFF) || (policyOption > OTR_POLICY_REQUIRE)) {
        policyOption = static_cast<int>(OTR_POLICY_ENABLED);
    }

    bool endWhenOfflineOption
        = m_optionHost->getPluginOption(OPTION_END_WHEN_OFFLINE, DEFAULT_END_WHEN_OFFLINE).toBool();

    m_policy->button(policyOption)->setChecked(true);

    m_endWhenOffline->setChecked(endWhenOfflineOption);

    updateOptions();

    // TODO: update after stopping support of Ubuntu Xenial:
    connect(m_policy, SIGNAL(buttonClicked(int)), this, SLOT(updateOptions()));

    connect(m_endWhenOffline, &QCheckBox::stateChanged, this, &ConfigOtrWidget::updateOptions);
}

// ---------------------------------------------------------------------------

void ConfigOtrWidget::updateOptions()
{
    OtrPolicy policy = static_cast<OtrPolicy>(m_policy->checkedId());

    m_optionHost->setPluginOption(OPTION_POLICY, policy);
    m_optionHost->setPluginOption(OPTION_END_WHEN_OFFLINE, m_endWhenOffline->checkState() == Qt::Checked);
    m_otr->setPolicy(policy);
}

//=============================================================================

FingerprintWidget::FingerprintWidget(OtrMessaging *otr, QWidget *parent) :
    QWidget(parent), m_otr(otr), m_table(new QTableView(this)), m_tableModel(new QStandardItemModel(this)),
    m_fingerprints()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_table->setShowGrid(true);
    m_table->setEditTriggers(nullptr);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    m_table->setSortingEnabled(true);

    connect(m_table, &QTableView::customContextMenuRequested, this, &FingerprintWidget::contextMenu);

    mainLayout->addWidget(m_table);

    auto trustButton  = new QPushButton(tr("Trust"), this);
    auto revokeButton = new QPushButton(tr("Do not trust"), this);
    auto deleteButton = new QPushButton(tr("Delete"), this);
    connect(trustButton, &QPushButton::clicked, this, &FingerprintWidget::verifyKnownKey);
    connect(revokeButton, &QPushButton::clicked, this, &FingerprintWidget::revokeKnownKey);
    connect(deleteButton, &QPushButton::clicked, this, &FingerprintWidget::deleteKnownKey);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(trustButton);
    buttonLayout->addWidget(revokeButton);
    buttonLayout->addWidget(new QLabel(this));
    buttonLayout->addWidget(deleteButton);

    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);
    updateData();
}

//-----------------------------------------------------------------------------

void FingerprintWidget::updateData()
{
    int           sortSection = m_table->horizontalHeader()->sortIndicatorSection();
    Qt::SortOrder sortOrder   = m_table->horizontalHeader()->sortIndicatorOrder();

    {
        m_tableModel->clear();
        m_tableModel->setColumnCount(5);
        const QStringList &headerLabels
            = { tr("Account"), tr("User"), tr("Fingerprint"), tr("Verified"), tr("Status") };
        m_tableModel->setHorizontalHeaderLabels(headerLabels);
    }

    m_fingerprints = m_otr->getFingerprints();
    QListIterator<Fingerprint> fingerprintIt(m_fingerprints);
    int                        fpIndex = 0;
    while (fingerprintIt.hasNext()) {
        QList<QStandardItem *> row;
        Fingerprint            fp = fingerprintIt.next();

        QStandardItem *item = new QStandardItem(m_otr->humanAccount(fp.account));
        item->setData(QVariant(fpIndex));

        row.append(item);
        row.append(new QStandardItem(fp.username));
        row.append(new QStandardItem(fp.fingerprintHuman));
        row.append(new QStandardItem(fp.trust));
        row.append(new QStandardItem(m_otr->getMessageStateString(fp.account, fp.username)));

        m_tableModel->appendRow(row);

        fpIndex++;
    }

    m_table->setModel(m_tableModel);

    m_table->sortByColumn(sortSection, sortOrder);
    m_table->resizeColumnsToContents();
}

//-----------------------------------------------------------------------------
//** slots **

void FingerprintWidget::deleteKnownKey()
{
    if (!m_table->selectionModel()->hasSelection()) {
        return;
    }
    for (auto selectIndex : m_table->selectionModel()->selectedRows()) {
        int fpIndex = m_tableModel->item(selectIndex.row(), 0)->data().toInt();

        QString msg(tr("Are you sure you want to delete the following key?") + "\n\n" + tr("Account: ")
                    + m_otr->humanAccount(m_fingerprints[fpIndex].account) + "\n" + tr("User: ")
                    + m_fingerprints[fpIndex].username + "\n" + tr("Fingerprint: ")
                    + m_fingerprints[fpIndex].fingerprintHuman);

        QMessageBox mb(QMessageBox::Question, tr("Confirm action"), msg, QMessageBox::Yes | QMessageBox::No, this,
                       Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

        if (mb.exec() == QMessageBox::Yes) {
            m_otr->deleteFingerprint(m_fingerprints[fpIndex]);
        }
    }
    updateData();
}

//-----------------------------------------------------------------------------

void FingerprintWidget::verifyKnownKey()
{
    if (!m_table->selectionModel()->hasSelection()) {
        return;
    }

    bool itemChanged = false;
    for (auto selectIndex : m_table->selectionModel()->selectedRows()) {
        const int fpIndex = m_tableModel->item(selectIndex.row(), 0)->data().toInt();

        QString msg(tr("Have you verified that this is in fact the correct fingerprint?") + "\n\n" + tr("Account: ")
                    + m_otr->humanAccount(m_fingerprints[fpIndex].account) + "\n" + tr("User: ")
                    + m_fingerprints[fpIndex].username + "\n" + tr("Fingerprint: ")
                    + m_fingerprints[fpIndex].fingerprintHuman);

        QMessageBox mb(QMessageBox::Question, tr("Confirm action"), msg, QMessageBox::Yes | QMessageBox::No, this,
                       Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

        if (mb.exec() == QMessageBox::Yes) {
            m_otr->verifyFingerprint(m_fingerprints[fpIndex], true);
            itemChanged = true;
        }
    }

    if (itemChanged)
        updateData();
}

void FingerprintWidget::revokeKnownKey()
{
    if (!m_table->selectionModel()->hasSelection()) {
        return;
    }

    for (auto selectIndex : m_table->selectionModel()->selectedRows()) {
        const int fpIndex = m_tableModel->item(selectIndex.row(), 0)->data().toInt();
        m_otr->verifyFingerprint(m_fingerprints[fpIndex], false);
    }
    updateData();
}

//-----------------------------------------------------------------------------

void FingerprintWidget::copyFingerprint()
{
    if (!m_table->selectionModel()->hasSelection()) {
        return;
    }
    QString text;
    for (auto selectIndex : m_table->selectionModel()->selectedRows(1)) {
        int fpIndex = m_tableModel->item(selectIndex.row(), 0)->data().toInt();

        if (!text.isEmpty()) {
            text += "\n";
        }
        text += m_fingerprints[fpIndex].fingerprintHuman;
    }
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(text);
}

//-----------------------------------------------------------------------------

void FingerprintWidget::contextMenu(const QPoint &pos)
{
    QModelIndex index = m_table->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    QMenu *menu = new QMenu(this);

    menu->addAction(QIcon::fromTheme("edit-delete"), tr("Delete"), this, SLOT(deleteKnownKey()));
    menu->addAction(QIcon(":/otrplugin/otr_unverified.png"), tr("Verify fingerprint"), this, SLOT(verifyKnownKey()));
    menu->addAction(QIcon::fromTheme("edit-copy"), tr("Copy fingerprint"), this, SLOT(copyFingerprint()));

    menu->exec(QCursor::pos());
}

//=============================================================================

PrivKeyWidget::PrivKeyWidget(AccountInfoAccessingHost *accountInfo, OtrMessaging *otr, QWidget *parent) :
    QWidget(parent), m_accountInfo(accountInfo), m_otr(otr), m_table(new QTableView(this)),
    m_tableModel(new QStandardItemModel(this)), m_keys()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_accountBox = new QComboBox(this);

    QString id;
    int     accountIndex = 0;
    while ((id = m_accountInfo->getId(accountIndex)) != "-1") {
        m_accountBox->addItem(m_accountInfo->getName(accountIndex), QVariant(id));
        accountIndex++;
    }

    QPushButton *generateButton = new QPushButton(tr("Generate new key"), this);
    connect(generateButton, &QPushButton::clicked, this, &PrivKeyWidget::generateNewKey);

    QHBoxLayout *generateLayout = new QHBoxLayout();
    generateLayout->addWidget(m_accountBox);
    generateLayout->addWidget(generateButton);

    mainLayout->addLayout(generateLayout);
    mainLayout->addWidget(m_table);

    QPushButton *deleteButton = new QPushButton(tr("Delete"), this);
    connect(deleteButton, &QPushButton::clicked, this, &PrivKeyWidget::deleteOwnKey);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addWidget(new QLabel(this));
    buttonLayout->addWidget(new QLabel(this));

    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    m_table->setShowGrid(true);
    m_table->setEditTriggers(nullptr);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSortingEnabled(true);

    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, &QTableView::customContextMenuRequested, this, &PrivKeyWidget::contextMenu);

    updateData();
}

//-----------------------------------------------------------------------------

void PrivKeyWidget::updateData()
{
    int           sortSection = m_table->horizontalHeader()->sortIndicatorSection();
    Qt::SortOrder sortOrder   = m_table->horizontalHeader()->sortIndicatorOrder();

    {
        m_tableModel->clear();
        m_tableModel->setColumnCount(2);
        const QStringList &headerLabels = { tr("Account"), tr("Fingerprint") };
        m_tableModel->setHorizontalHeaderLabels(headerLabels);
    }

    m_keys = m_otr->getPrivateKeys();
    QHash<QString, QString>::iterator keyIt;
    for (keyIt = m_keys.begin(); keyIt != m_keys.end(); ++keyIt) {
        QList<QStandardItem *> row;

        QStandardItem *accItem = new QStandardItem(m_otr->humanAccount(keyIt.key()));
        accItem->setData(QVariant(keyIt.key()));

        row.append(accItem);
        row.append(new QStandardItem(keyIt.value()));

        m_tableModel->appendRow(row);
    }

    m_table->setModel(m_tableModel);

    m_table->sortByColumn(sortSection, sortOrder);
    m_table->resizeColumnsToContents();
}

//-----------------------------------------------------------------------------

void PrivKeyWidget::deleteOwnKey()
{
    if (!m_table->selectionModel()->hasSelection()) {
        return;
    }

    bool keyRemoved = false;
    for (auto selectIndex : m_table->selectionModel()->selectedRows(1)) {
        const QString fpr(m_tableModel->item(selectIndex.row(), 1)->text());
        const QString account(m_tableModel->item(selectIndex.row(), 0)->data().toString());

        QString msg(tr("Are you sure you want to delete the following key?") + "\n\n" + tr("Account: ")
                    + m_otr->humanAccount(account) + "\n" + tr("Fingerprint: ") + fpr);

        QMessageBox mb(QMessageBox::Question, tr("Confirm action"), msg, QMessageBox::Yes | QMessageBox::No, this,
                       Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

        if (mb.exec() == QMessageBox::Yes) {
            m_otr->deleteKey(account);
            keyRemoved = true;
        }
    }

    if (keyRemoved)
        updateData();
}

//-----------------------------------------------------------------------------

void PrivKeyWidget::generateNewKey()
{
    int accountIndex = m_accountBox->currentIndex();

    if (accountIndex == -1) {
        return;
    }

    QString accountName(m_accountBox->currentText());
    QString accountId(m_accountBox->itemData(accountIndex).toString());

    if (m_keys.contains(accountId)) {
        QString msg(tr("Are you sure you want to overwrite the following key?") + "\n\n" + tr("Account: ") + accountName
                    + "\n" + tr("Fingerprint: ") + m_keys.value(accountId));

        QMessageBox mb(QMessageBox::Question, tr("Confirm action"), msg, QMessageBox::Yes | QMessageBox::No, this,
                       Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

        if (mb.exec() == QMessageBox::No) {
            return;
        }
    }

    m_otr->generateKey(accountId);

    updateData();
}

//-----------------------------------------------------------------------------

void PrivKeyWidget::copyFingerprint()
{
    if (!m_table->selectionModel()->hasSelection()) {
        return;
    }
    QString text;
    for (auto selectIndex : m_table->selectionModel()->selectedRows(1)) {
        if (!text.isEmpty()) {
            text += "\n";
        }
        text += m_tableModel->item(selectIndex.row(), 1)->text();
    }
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(text);
}

//-----------------------------------------------------------------------------

void PrivKeyWidget::contextMenu(const QPoint &pos)
{
    QModelIndex index = m_table->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    QMenu *menu = new QMenu(this);

    menu->addAction(QIcon::fromTheme("edit-delete"), tr("Delete"), this, SLOT(deleteOwnKey()));
    menu->addAction(QIcon::fromTheme("edit-copy"), tr("Copy fingerprint"), this, SLOT(copyFingerprint()));

    menu->exec(QCursor::pos());
}

//-----------------------------------------------------------------------------

} // namespace psiotr
