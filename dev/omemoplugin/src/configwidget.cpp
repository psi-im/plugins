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
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>

namespace psiomemo {
  ConfigWidget::ConfigWidget(OMEMO *omemo) : QWidget() {
    auto mainLayout = new QVBoxLayout(this);
    auto tabWidget = new QTabWidget(this);
    tabWidget->addTab(new KnownFingerprints(omemo, this), "Fingerprints");
    tabWidget->addTab(new OwnFingerprint(omemo, this), "Own Fingerprint");
    mainLayout->addWidget(tabWidget);
    setLayout(mainLayout);
  }

  OwnFingerprint::OwnFingerprint(OMEMO *omemo, QWidget *parent) : QWidget(parent) {
    auto mainLayout = new QVBoxLayout(this);
    auto deviceId = new QLabel("Device ID: " + QString::number(omemo->getDeviceId()), this);
    deviceId->setTextInteractionFlags(Qt::TextSelectableByMouse);
    mainLayout->addWidget(deviceId);
    auto fingerprint = new QLabel(QString("Fingerprint: <code>%1</code>").arg(omemo->getOwnFingerprint()), this);
    fingerprint->setTextInteractionFlags(Qt::TextSelectableByMouse);
    fingerprint->setWordWrap(true);
    mainLayout->addWidget(fingerprint);
    setLayout(mainLayout);
  }

  KnownFingerprints::KnownFingerprints(OMEMO *omemo, QWidget *parent) : QWidget(parent) {
    auto mainLayout = new QVBoxLayout(this);
    m_omemo = omemo;
    m_table = new QTableView(this);
    m_table->setShowGrid(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSortingEnabled(true);

    m_tableModel = new QStandardItemModel(this);
    m_table->setModel(m_tableModel);
    mainLayout->addWidget(m_table);

    auto trustRevokeButton = new QPushButton("Trust/Revoke Selected Fingerprint", this);
    connect(trustRevokeButton, SIGNAL(clicked()), SLOT(trustRevokeFingerprint()));
    mainLayout->addWidget(trustRevokeButton);

    setLayout(mainLayout);
    updateData();
  }

  void KnownFingerprints::updateData() {
    int sortSection = m_table->horizontalHeader()->sortIndicatorSection();
    Qt::SortOrder sortOrder = m_table->horizontalHeader()->sortIndicatorOrder();

    m_tableModel->clear();
    m_tableModel->setColumnCount(3);
    m_tableModel->setHorizontalHeaderLabels({"Contact", "Trust" , "Fingerprint"});
    foreach (auto fingerprint, m_omemo->getKnownFingerprints()) {
      QList<QStandardItem*> row;
      auto contact = new QStandardItem(fingerprint.contact);
      contact->setData(QVariant(fingerprint.deviceId));
      row.append(contact);
      row.append(new QStandardItem(fingerprint.trust == TRUSTED ? "Trusted" : fingerprint.trust == UNTRUSTED ? "Untrusted" : "Undecided"));
      auto fpItem = new QStandardItem(fingerprint.fingerprint);
      fpItem->setData(QColor(fingerprint.trust == TRUSTED ? Qt::darkGreen : fingerprint.trust == UNTRUSTED ? Qt::darkRed : Qt::darkYellow),
                      Qt::ForegroundRole);
      fpItem->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
      row.append(fpItem);
      m_tableModel->appendRow(row);
    }
    m_table->sortByColumn(sortSection, sortOrder);
    m_table->resizeColumnsToContents();
  }

  void KnownFingerprints::trustRevokeFingerprint() {
    if (!m_table->selectionModel()->hasSelection()) {
      return;
    }

    QStandardItem *item = m_tableModel->item(m_table->selectionModel()->selectedRows(0).at(0).row(), 0);
    m_omemo->confirmDeviceTrust(item->text(), item->data().toUInt());
    updateData();
  }
}
