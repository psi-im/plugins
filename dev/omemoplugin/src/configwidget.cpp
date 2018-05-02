/*
 * OMEMO Plugin for Psi
 * Copyright (C) 2018 Vyacheslav Karpukhin
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

#include "configwidget.h"
#include <QVBoxLayout>
#include <QHeaderView>

namespace psiomemo {
  ConfigWidget::ConfigWidget(OMEMO *omemo, AccountInfoAccessingHost *accountInfo) : QWidget(), m_accountInfo(accountInfo) {
    auto mainLayout = new QVBoxLayout(this);

    int curIndex = 0;
    auto accountBox = new QComboBox(this);
    while (m_accountInfo->getId(curIndex) != "-1") {
      accountBox->addItem(m_accountInfo->getName(curIndex), curIndex);
      curIndex++;
    }
    mainLayout->addWidget(accountBox);

    int account = accountBox->itemData(accountBox->currentIndex()).toInt();

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(new KnownFingerprints(account, omemo, this), "Fingerprints");
    m_tabWidget->addTab(new OwnFingerprint(account, omemo, this), "Own Fingerprint");
    m_tabWidget->addTab(new ManageKeys(account, omemo, this), "Manage Keys");
    mainLayout->addWidget(m_tabWidget);
    setLayout(mainLayout);

    connect(accountBox, SIGNAL(currentIndexChanged(int)), SLOT(currentAccountChanged(int)));
  }

  ConfigWidgetTabWithTable::ConfigWidgetTabWithTable(int account, OMEMO *omemo, QWidget *parent): ConfigWidgetTab(account, omemo, parent) {
    m_table = new QTableView(this);
    m_table->setShowGrid(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSortingEnabled(true);
    m_table->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);

    m_tableModel = new QStandardItemModel(this);
    m_table->setModel(m_tableModel);
  }

  void ConfigWidgetTabWithTable::updateData() {
    int sortSection = m_table->horizontalHeader()->sortIndicatorSection();
    Qt::SortOrder sortOrder = m_table->horizontalHeader()->sortIndicatorOrder();
    m_tableModel->clear();

    doUpdateData();

    m_table->sortByColumn(sortSection, sortOrder);
    m_table->resizeColumnsToContents();
  }

  void ConfigWidget::currentAccountChanged(int index) {
    int account = dynamic_cast<QComboBox *>(sender())->itemData(index).toInt();
    for (int i = 0; i < m_tabWidget->count(); i++) {
      dynamic_cast<ConfigWidgetTab *>(m_tabWidget->widget(i))->setAccount(account);
    }
  }

  OwnFingerprint::OwnFingerprint(int account, OMEMO *omemo, QWidget *parent) : ConfigWidgetTab(account, omemo, parent) {
    auto mainLayout = new QVBoxLayout(this);
    m_deviceLabel = new QLabel(this);
    m_deviceLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    mainLayout->addWidget(m_deviceLabel);
    m_fingerprintLabel = new QLabel(this);
    m_fingerprintLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_fingerprintLabel->setWordWrap(true);
    mainLayout->addWidget(m_fingerprintLabel);
    setLayout(mainLayout);
    updateData();
  }

  void OwnFingerprint::updateData() {
    m_deviceLabel->setText("Device ID: " + QString::number(m_omemo->getDeviceId(m_account)));
    m_fingerprintLabel->setText(QString("Fingerprint: <code>%1</code>").arg(m_omemo->getOwnFingerprint(m_account)));
  }

  KnownFingerprints::KnownFingerprints(int account, OMEMO *omemo, QWidget *parent) : ConfigWidgetTabWithTable(account, omemo, parent) {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_table);

    auto trustRevokeButton = new QPushButton("Trust/Revoke Selected Fingerprint", this);
    connect(trustRevokeButton, SIGNAL(clicked()), SLOT(trustRevokeFingerprint()));
    mainLayout->addWidget(trustRevokeButton);

    setLayout(mainLayout);
    updateData();
  }

  void KnownFingerprints::doUpdateData() {
    m_tableModel->setColumnCount(3);
    m_tableModel->setHorizontalHeaderLabels({"Contact", "Trust" , "Fingerprint"});
    foreach (auto fingerprint, m_omemo->getKnownFingerprints(m_account)) {
      QList<QStandardItem*> row;
      auto contact = new QStandardItem(fingerprint.contact);
      contact->setData(QVariant(fingerprint.deviceId));
      row.append(contact);
      TRUST_STATE state = fingerprint.trust;
      row.append(new QStandardItem(state == TRUSTED ? "Trusted" : state == UNTRUSTED ? "Untrusted" : "Undecided"));
      auto fpItem = new QStandardItem(fingerprint.fingerprint);
      fpItem->setData(QColor(state == TRUSTED ? Qt::darkGreen : state == UNTRUSTED ? Qt::darkRed : Qt::darkYellow),
                      Qt::ForegroundRole);
      fpItem->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
      row.append(fpItem);
      m_tableModel->appendRow(row);
    }
  }

  void KnownFingerprints::trustRevokeFingerprint() {
    if (!m_table->selectionModel()->hasSelection()) {
      return;
    }

    QStandardItem *item = m_tableModel->item(m_table->selectionModel()->selectedRows(0).at(0).row(), 0);
    m_omemo->confirmDeviceTrust(m_account, item->text(), item->data().toUInt());
    updateData();
  }

  ManageKeys::ManageKeys(int account, OMEMO *omemo, QWidget *parent) : ConfigWidgetTabWithTable(account, omemo, parent) {
    m_ourDeviceId = m_omemo->getDeviceId(account);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_table);

    connect(m_table->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), 
            SLOT(selectionChanged(const QItemSelection &, const QItemSelection &)));

    connect(m_omemo, SIGNAL(deviceListUpdated(int)), SLOT(deviceListUpdated(int)));

    m_deleteButton = new QPushButton("Delete", this);
    m_deleteButton->setEnabled(false);
    connect(m_deleteButton, SIGNAL(clicked()), SLOT(deleteDevice()));
    mainLayout->addWidget(m_deleteButton);

    setLayout(mainLayout);
    updateData();
  }
  
  void ManageKeys::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    QModelIndexList selection = selected.indexes();
    if (!selection.isEmpty()) {
      m_deleteButton->setEnabled(selectedDeviceId(selection) != m_ourDeviceId);
    }
  }

  uint32_t ManageKeys::selectedDeviceId(const QModelIndexList &selection) const {
    return m_tableModel->itemFromIndex(selection.first())->data().toUInt();
  }

  void ManageKeys::doUpdateData() {
    m_tableModel->setColumnCount(1);
    m_tableModel->setHorizontalHeaderLabels({"Device ID"});
    foreach (auto deviceId, m_omemo->getOwnDeviceList(m_account)) {
      QStandardItem *item = new QStandardItem(QString::number(deviceId));
      item->setData(deviceId);
      m_tableModel->appendRow(item);
    }
  }

  void ManageKeys::deleteDevice() {
    QModelIndexList selection = m_table->selectionModel()->selectedIndexes();
    if (!selection.isEmpty()) {
      m_omemo->unpublishDevice(m_account, selectedDeviceId(selection));
    }
  }

  void ManageKeys::deviceListUpdated(int account) {
    if (account == m_account) {
      updateData();
    }
  }
}
