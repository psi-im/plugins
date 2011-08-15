/*
 * PsiOtrConfig.cpp - configuration dialogs for Psi OTR plugin
 * Copyright (C) 2007  Timo Engel (timo-e@freenet.de)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "PsiOtrConfig.hpp"
#include "optionaccessinghost.h"
#include "accountinfoaccessinghost.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QTableView>
#include <QStandardItem>
#include <QMessageBox>
#include <QPushButton>

//-----------------------------------------------------------------------------

namespace psiotr
{

//-----------------------------------------------------------------------------

ConfigDialog::ConfigDialog(OtrMessaging* otr, OptionAccessingHost* optionHost,
                           AccountInfoAccessingHost* accountInfo,
                           QWidget* parent)
    : QWidget(parent),
      m_otr(otr),
      m_optionHost(optionHost),
      m_accountInfo(accountInfo)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QTabWidget* tabWidget = new QTabWidget(this);

    tabWidget->addTab(new FingerprintWidget(m_otr, tabWidget),
                      tr("Known fingerprints"));

    tabWidget->addTab(new PrivKeyWidget(m_accountInfo, m_otr, tabWidget),
                      tr("My private keys"));

    tabWidget->addTab(new ConfigOtrWidget(m_optionHost, m_otr, tabWidget),
                      tr("Configuration"));

    mainLayout->addWidget(tabWidget);
    setLayout(mainLayout);
}

//=============================================================================

ConfigOtrWidget::ConfigOtrWidget(OptionAccessingHost* optionHost,
                                 OtrMessaging* otr,
                                 QWidget* parent)
    : QWidget(parent),
      m_optionHost(optionHost),
      m_otr(otr),
      m_polEnable(0),
      m_polAuto(0),
      m_polRequire(0)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    QGroupBox* policyGroup = new QGroupBox(tr("OTR Policy"), this);
    QVBoxLayout* policyLayout = new QVBoxLayout(policyGroup);

    m_polEnable = new QCheckBox(tr("Enable private messaging"), policyGroup);
    m_polAuto = new QCheckBox(tr("Automatically start private messaging"), policyGroup);
    m_polRequire = new QCheckBox(tr("Require private messaging"), policyGroup);
    policyLayout->addWidget(m_polEnable);
    policyLayout->addWidget(m_polAuto);
    policyLayout->addWidget(m_polRequire);
    policyGroup->setLayout(policyLayout);

    layout->addWidget(policyGroup);
    layout->addStretch();

    setLayout(layout);

    QVariant policyOption = m_optionHost->getPluginOption(PSI_CONFIG_POLICY);
    if (policyOption == QVariant::Invalid)
    {
        policyOption = OTR_POLICY_ENABLED;
    }
    switch (policyOption.toInt())
    {
        case OTR_POLICY_REQUIRE:
            m_polRequire->setCheckState(Qt::Checked);
        case OTR_POLICY_AUTO:
            m_polAuto->setCheckState(Qt::Checked);
        case OTR_POLICY_ENABLED:
            m_polEnable->setCheckState(Qt::Checked);
        case OTR_POLICY_OFF:
            break;
    }
    
    handlePolicyChange();
  
    connect(m_polEnable,  SIGNAL(stateChanged(int)),
            SLOT(handlePolicyChange()));
    connect(m_polAuto,    SIGNAL(stateChanged(int)),
            SLOT(handlePolicyChange()));
    connect(m_polRequire, SIGNAL(stateChanged(int)),
            SLOT(handlePolicyChange()));
}

// ---------------------------------------------------------------------------

void ConfigOtrWidget::handlePolicyChange()
{
    if (m_polEnable->checkState() == Qt::Unchecked)
    {
        m_polAuto->setEnabled(false);
        m_polAuto->setCheckState(Qt::Unchecked);
    }
    if (m_polAuto->checkState() == Qt::Unchecked)
    {
        m_polRequire->setEnabled(false);
        m_polRequire->setCheckState(Qt::Unchecked);
    }
    if (m_polEnable->checkState() == Qt::Checked)
    {
        m_polAuto->setEnabled(true);
    }
    if (m_polAuto->checkState() == Qt::Checked)
    {
        m_polRequire->setEnabled(true);
    }


    OtrPolicy policy = OTR_POLICY_OFF;
    if (m_polRequire->checkState() == Qt::Checked)
    {
        policy = OTR_POLICY_REQUIRE;
    }
    else if (m_polAuto->checkState() == Qt::Checked)
    {
        policy = OTR_POLICY_AUTO;
    }
    else if (m_polEnable->checkState() == Qt::Checked)
    {
        policy = OTR_POLICY_ENABLED;
    }
    
    m_optionHost->setPluginOption(PSI_CONFIG_POLICY, policy);
    m_otr->setPolicy(policy);
}

//=============================================================================

FingerprintWidget::FingerprintWidget(OtrMessaging* otr, QWidget* parent)
    : QWidget(parent),
      m_otr(otr),
      m_table(new QTableView(this)),
      m_tableModel(new QStandardItemModel(this)),
      m_fingerprints()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    m_table->setShowGrid(true);
    m_table->setEditTriggers(0);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(m_table);

    QPushButton* forgetButton = new QPushButton(tr("Forget fingerprint"), this);
    QPushButton* verifyButton = new QPushButton(tr("Verify fingerprint"), this);
    connect(forgetButton,SIGNAL(clicked()),SLOT(forgetFingerprint()));
    connect(verifyButton,SIGNAL(clicked()),SLOT(verifyFingerprint()));
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(forgetButton);
    buttonLayout->addWidget(verifyButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    updateData();
}

//-----------------------------------------------------------------------------

void FingerprintWidget::updateData()
{
    m_tableModel->clear();
    m_tableModel->setColumnCount(5);
    m_tableModel->setHorizontalHeaderLabels(QStringList() << tr("Account")
                                            << tr("User") << tr("Fingerprint")
                                            << tr("Verified") << tr("Status"));

    m_fingerprints = m_otr->getFingerprints();
    QListIterator<Fingerprint> fingerprintIt(m_fingerprints);
    while(fingerprintIt.hasNext())
    {
        QList<QStandardItem*> row;
        Fingerprint fp = fingerprintIt.next();
        row.append(new QStandardItem(m_otr->humanAccount(fp.account)));
        row.append(new QStandardItem(fp.username));
        row.append(new QStandardItem(fp.fingerprintHuman));
        row.append(new QStandardItem(fp.trust));
        row.append(new QStandardItem(fp.messageState));

        m_tableModel->appendRow(row);
    }

    m_table->setModel(m_tableModel);

    m_table->resizeColumnsToContents();
}

//-----------------------------------------------------------------------------
//** slots **

void FingerprintWidget::forgetFingerprint()
{
    if (!m_table->selectionModel()->hasSelection())
    {
        return;
    }
    foreach(QModelIndex selectIndex, m_table->selectionModel()->selectedRows())
    {
        QString msg(tr("Are you sure you want to delete the following fingerprint?") + "\n" +
                    tr("Account: ") + m_otr->humanAccount(m_fingerprints[selectIndex.row()].account) + "\n" +
                    tr("User: ") + m_fingerprints[selectIndex.row()].username + "\n" +
                    tr("Fingerprint: ") + m_fingerprints[selectIndex.row()].fingerprintHuman);

        QMessageBox mb(QMessageBox::Question, tr("Psi OTR"), msg,
                       QMessageBox::Yes | QMessageBox::No, this,
                       Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

        if (mb.exec() == QMessageBox::Yes)
        {
            m_otr->deleteFingerprint(m_fingerprints[selectIndex.row()]);
        }
    }
    updateData();
}

//-----------------------------------------------------------------------------

void FingerprintWidget::verifyFingerprint()
{
    if (!m_table->selectionModel()->hasSelection())
    {
        return;
    }
    foreach(QModelIndex selectIndex, m_table->selectionModel()->selectedRows())
    {
        QString msg(tr("Account: ") + m_otr->humanAccount(m_fingerprints[selectIndex.row()].account) + "\n" +
                    tr("User: ") + m_fingerprints[selectIndex.row()].username + "\n" +
                    tr("Fingerprint: ") + m_fingerprints[selectIndex.row()].fingerprintHuman + "\n\n" +
                    tr("Have you verified that this is in fact the correct fingerprint?"));

        QMessageBox mb(QMessageBox::Question, tr("Psi OTR"), msg,
                       QMessageBox::Yes | QMessageBox::No, this,
                       Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

        if (mb.exec() == QMessageBox::Yes)
        {
            m_otr->verifyFingerprint(m_fingerprints[selectIndex.row()], true);
        }
        else
        {
            m_otr->verifyFingerprint(m_fingerprints[selectIndex.row()], false);
        }
    }
    updateData();
}

//=============================================================================

PrivKeyWidget::PrivKeyWidget(AccountInfoAccessingHost* accountInfo,
                             OtrMessaging* otr, QWidget* parent)
    : QWidget(parent),
      m_accountInfo(accountInfo),
      m_otr(otr),
      m_table(new QTableView(this)),
      m_tableModel(new QStandardItemModel(this)),
      m_keys()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    m_accountBox = new QComboBox(this);

    QString id;
    int accountIndex = 0;
    while ((id = m_accountInfo->getId(accountIndex)) != "-1")
    {
        m_accountBox->addItem(m_accountInfo->getName(accountIndex), QVariant(id));
        accountIndex++;
    }

    QPushButton* generateButton = new QPushButton(tr("Generate Key"), this);
    connect(generateButton,SIGNAL(clicked()),SLOT(generateKey()));

    QHBoxLayout* generateLayout = new QHBoxLayout();
    generateLayout->addWidget(m_accountBox);
    generateLayout->addWidget(generateButton);

    mainLayout->addLayout(generateLayout);
    mainLayout->addWidget(m_table);

    QPushButton* forgetButton = new QPushButton(tr("Forget Key"), this);
    connect(forgetButton,SIGNAL(clicked()),SLOT(forgetKey()));

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(forgetButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    m_table->setShowGrid(true);
    m_table->setEditTriggers(0);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    updateData();
}

//-----------------------------------------------------------------------------

void PrivKeyWidget::updateData()
{
    m_tableModel->clear();
    m_tableModel->setColumnCount(2);
    m_tableModel->setHorizontalHeaderLabels(QStringList() << tr("Account")
                                                          << tr("Fingerprint"));

    m_keys = m_otr->getPrivateKeys();
    QHash<QString, QString>::iterator keyIt;
    for (keyIt = m_keys.begin(); keyIt != m_keys.end(); ++keyIt)
    {
        QList<QStandardItem*> row;
        row.append(new QStandardItem(m_otr->humanAccount(keyIt.key())));
        row.append(new QStandardItem(keyIt.value()));

        m_tableModel->appendRow(row);
    }

    m_table->setModel(m_tableModel);

    m_table->resizeColumnsToContents();
}

//-----------------------------------------------------------------------------

void PrivKeyWidget::forgetKey()
{
    if (!m_table->selectionModel()->hasSelection())
    {
        return;
    }
    foreach(QModelIndex selectIndex, m_table->selectionModel()->selectedRows(1))
    {
        QString fpr(m_tableModel->item(selectIndex.row(), 1)->text());
        QString account(m_keys.key(fpr));
        QString msg(tr("Are you sure you want to delete the following private key?") + "\n" +
                    tr("Account: ") + m_otr->humanAccount(account) + "\n" +
                    tr("Fingerprint: ") + fpr);

        QMessageBox mb(QMessageBox::Question, tr("Psi OTR"), msg,
                       QMessageBox::Yes | QMessageBox::No, this,
                       Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

        if (mb.exec() == QMessageBox::Yes)
        {
            m_otr->deleteKey(account);
        }
    }
    updateData();
}

//-----------------------------------------------------------------------------

void PrivKeyWidget::generateKey()
{
    int accountIndex = m_accountBox->currentIndex();

    QString accountName(m_accountBox->currentText());
    QString accountId(m_accountBox->itemData(accountIndex).toString());

    QString msg(tr("Are you sure you want to generate a new private key "
                   "for account \"%1\"? Existing keys for that account will be "
                   "overwritten.").arg(accountName));

    QMessageBox mb(QMessageBox::Question, tr("Psi OTR"), msg,
                   QMessageBox::Yes | QMessageBox::No, this,
                   Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

    if (mb.exec() == QMessageBox::Yes)
    {
        m_otr->generateKey(accountId);
    }

    updateData();
}

//-----------------------------------------------------------------------------

} // namespace psiotr
