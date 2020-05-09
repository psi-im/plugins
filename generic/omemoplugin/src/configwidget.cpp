/*
 * OMEMO Plugin for Psi
 * Copyright (C) 2018 Vyacheslav Karpukhin
 * Copyright (C) 2020 Boris Pek
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

#include "configwidget.h"
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

namespace psiomemo {
ConfigWidget::ConfigWidget(OMEMO *omemo, AccountInfoAccessingHost *accountInfo) : QWidget(), m_accountInfo(accountInfo)
{
    auto mainLayout = new QVBoxLayout(this);

    int  curIndex   = 0;
    auto accountBox = new QComboBox(this);
    while (m_accountInfo->getId(curIndex) != "-1") {
        accountBox->addItem(m_accountInfo->getName(curIndex), curIndex);
        curIndex++;
    }
    mainLayout->addWidget(accountBox);

    int account = accountBox->itemData(accountBox->currentIndex()).toInt();

    auto knownFingerprintsTab  = new KnownFingerprints(account, omemo, this);
    auto manageDevicesTab      = new ManageDevices(account, omemo, this);
    auto omemoConfigurationTab = new OmemoConfiguration(account, omemo, this);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(knownFingerprintsTab, tr("Fingerprints"));
    m_tabWidget->addTab(manageDevicesTab, tr("Manage Devices"));
    m_tabWidget->addTab(omemoConfigurationTab, tr("Configuration"));
    mainLayout->addWidget(m_tabWidget);
    setLayout(mainLayout);

    connect(manageDevicesTab, &ManageDevices::updateKnownFingerprints, knownFingerprintsTab,
            &KnownFingerprints::updateData);
    connect(this, &ConfigWidget::applySettings, omemoConfigurationTab, &OmemoConfiguration::saveSettings);

    // TODO: update after stopping support of Ubuntu Xenial:
    connect(accountBox, SIGNAL(currentIndexChanged(int)), SLOT(currentAccountChanged(int)));
}

ConfigWidgetTabWithTable::ConfigWidgetTabWithTable(int account, OMEMO *omemo, QWidget *parent) :
    ConfigWidgetTab(account, omemo, parent)
{
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

void ConfigWidgetTabWithTable::filterContacts(const QString &jid)
{
    m_jid = jid;
    updateData();
}

void ConfigWidgetTabWithTable::updateData()
{
    int           sortSection = m_table->horizontalHeader()->sortIndicatorSection();
    Qt::SortOrder sortOrder   = m_table->horizontalHeader()->sortIndicatorOrder();
    m_tableModel->clear();

    doUpdateData();

    m_table->sortByColumn(sortSection, sortOrder);
    m_table->resizeColumnsToContents();
}

void ConfigWidget::currentAccountChanged(int index)
{
    int account = dynamic_cast<QComboBox *>(sender())->itemData(index).toInt();
    for (int i = 0; i < m_tabWidget->count(); i++) {
        dynamic_cast<ConfigWidgetTab *>(m_tabWidget->widget(i))->setAccount(account);
    }
}

KnownFingerprints::KnownFingerprints(int account, OMEMO *omemo, QWidget *parent) :
    ConfigWidgetTabWithTable(account, omemo, parent)
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_table);

    auto buttonsLayout = new QHBoxLayout(this);
    auto trustButton   = new QPushButton(tr("Trust"), this);
    auto revokeButton  = new QPushButton(tr("Do not trust"), this);
    auto removeButton  = new QPushButton(tr("Delete"), this);

    connect(trustButton, &QPushButton::clicked, this, &KnownFingerprints::trustFingerprint);
    connect(revokeButton, &QPushButton::clicked, this, &KnownFingerprints::revokeFingerprint);
    connect(removeButton, &QPushButton::clicked, this, &KnownFingerprints::removeFingerprint);

    buttonsLayout->addWidget(trustButton);
    buttonsLayout->addWidget(revokeButton);
    buttonsLayout->addWidget(new QLabel(this));
    buttonsLayout->addWidget(removeButton);
    mainLayout->addLayout(buttonsLayout);

    setLayout(mainLayout);
    updateData();
}

void KnownFingerprints::doUpdateData()
{
    m_tableModel->setColumnCount(3);
    m_tableModel->setHorizontalHeaderLabels({ tr("Contact"), tr("Trust"), tr("Fingerprint") });
    for (auto fingerprint : m_omemo->getKnownFingerprints(m_account)) {
        if (!m_jid.isEmpty()) {
            if (fingerprint.contact != m_jid) {
                continue;
            }
        }

        QList<QStandardItem *> row;
        auto                   contact = new QStandardItem(fingerprint.contact);
        contact->setData(QVariant(fingerprint.deviceId));
        row.append(contact);
        TRUST_STATE state = fingerprint.trust;
        row.append(new QStandardItem(state == TRUSTED ? tr("trusted")
                                                      : state == UNTRUSTED ? tr("untrusted") : tr("not decided")));
        auto fpItem = new QStandardItem(fingerprint.fingerprint);
        fpItem->setData(QColor(state == TRUSTED ? Qt::darkGreen : state == UNTRUSTED ? Qt::darkRed : Qt::darkYellow),
                        Qt::ForegroundRole);
        fpItem->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        row.append(fpItem);
        m_tableModel->appendRow(row);
    }
}

void KnownFingerprints::removeFingerprint()
{
    if (!m_table->selectionModel()->hasSelection())
        return;

    QStandardItem *item = m_tableModel->item(m_table->selectionModel()->selectedRows(0).at(0).row(), 0);
    m_omemo->removeDevice(m_account, item->text(), item->data().toUInt());

    updateData();
}

void KnownFingerprints::trustFingerprint()
{
    if (!m_table->selectionModel()->hasSelection())
        return;

    QStandardItem *item = m_tableModel->item(m_table->selectionModel()->selectedRows(0).at(0).row(), 0);
    m_omemo->confirmDeviceTrust(m_account, item->text(), item->data().toUInt());

    const int index    = item->row();
    const int rowCount = m_tableModel->rowCount();
    updateData();

    if (rowCount == m_tableModel->rowCount())
        m_table->selectRow(index);
}

void KnownFingerprints::revokeFingerprint()
{
    if (!m_table->selectionModel()->hasSelection())
        return;

    QStandardItem *item = m_tableModel->item(m_table->selectionModel()->selectedRows(0).at(0).row(), 0);
    m_omemo->revokeDeviceTrust(m_account, item->text(), item->data().toUInt());

    const int index    = item->row();
    const int rowCount = m_tableModel->rowCount();
    updateData();

    if (rowCount == m_tableModel->rowCount())
        m_table->selectRow(index);
}

ManageDevices::ManageDevices(int account, OMEMO *omemo, QWidget *parent) :
    ConfigWidgetTabWithTable(account, omemo, parent)
{
    m_currentDeviceId = m_omemo->getDeviceId(account);

    auto currentDevice = new QGroupBox(tr("Current device"), this);
    auto infoLabel     = new QLabel(tr("Fingerprint: "), currentDevice);
    infoLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_fingerprintLabel = new QLabel(currentDevice);
    m_fingerprintLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_fingerprintLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_fingerprintLabel->setWordWrap(true);

    auto deviceInfoLayout = new QHBoxLayout();
    deviceInfoLayout->addWidget(infoLabel);
    deviceInfoLayout->addWidget(m_fingerprintLabel);

    auto deleteCurrentDeviceButton = new QPushButton(tr("Delete all OMEMO data for current device"), currentDevice);
    connect(deleteCurrentDeviceButton, &QPushButton::clicked, this, &ManageDevices::deleteCurrentDevice);

    auto deleteCurrentDeviceLayout = new QHBoxLayout();
    deleteCurrentDeviceLayout->addWidget(deleteCurrentDeviceButton);
    deleteCurrentDeviceLayout->addWidget(new QLabel(currentDevice));

    auto currentDeviceLayout = new QVBoxLayout(currentDevice);
    currentDeviceLayout->addLayout(deviceInfoLayout);
    currentDeviceLayout->addLayout(deleteCurrentDeviceLayout);
    currentDevice->setLayout(currentDeviceLayout);
    currentDevice->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    auto otherDevices  = new QGroupBox(tr("Other devices"), this);
    auto buttonsLayout = new QHBoxLayout();
    m_deleteButton     = new QPushButton(tr("Delete"), this);
    m_deleteButton->setEnabled(false);
    connect(m_deleteButton, &QPushButton::clicked, this, &ManageDevices::deleteDevice);
    buttonsLayout->addWidget(m_deleteButton);
    buttonsLayout->addWidget(new QLabel(this));
    buttonsLayout->addWidget(new QLabel(this));

    auto otherDevicesLayout = new QVBoxLayout(otherDevices);
    otherDevicesLayout->addWidget(m_table);
    otherDevicesLayout->addLayout(buttonsLayout);
    otherDevices->setLayout(otherDevicesLayout);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(currentDevice);
    mainLayout->addWidget(otherDevices);
    setLayout(mainLayout);

    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ManageDevices::selectionChanged);
    connect(m_omemo, &OMEMO::deviceListUpdated, this, &ManageDevices::deviceListUpdated);

    updateData();
}

void ManageDevices::updateData()
{
    m_currentDeviceId = m_omemo->getDeviceId(m_account);
    m_fingerprintLabel->setText(QString("<code>%1</code>").arg(m_omemo->getOwnFingerprint(m_account)));

    ConfigWidgetTabWithTable::updateData();
}

void ManageDevices::selectionChanged(const QItemSelection &selected, const QItemSelection &)
{
    QModelIndexList selection = selected.indexes();
    if (!selection.isEmpty()) {
        m_deleteButton->setEnabled(selectedDeviceId(selection) != m_currentDeviceId);
    }
}

uint32_t ManageDevices::selectedDeviceId(const QModelIndexList &selection) const
{
    return m_tableModel->itemFromIndex(selection.first())->data().toUInt();
}

void ManageDevices::doUpdateData()
{
    m_tableModel->setColumnCount(1);
    m_tableModel->setHorizontalHeaderLabels({ tr("Device ID"), tr("Fingerprint") });
    auto fingerprintsMap = m_omemo->getOwnFingerprintsMap(m_account);
    for (auto deviceId : m_omemo->getOwnDevicesList(m_account)) {
        if (deviceId == m_currentDeviceId)
            continue;

        QList<QStandardItem *> row;
        auto                   item = new QStandardItem(QString::number(deviceId));
        item->setData(deviceId);
        row.append(item);
        if (fingerprintsMap.contains(deviceId)) {
            row.append(new QStandardItem(fingerprintsMap[deviceId]));
        } else {
            row.append(new QStandardItem());
        }
        m_tableModel->appendRow(row);
    }
}

void ManageDevices::deleteCurrentDevice()
{
    const QString &message = tr("Deleting of all OMEMO data for current device will cause "
                                "to a number of consequences:\n"
                                "1) All started OMEMO sessions will be forgotten.\n"
                                "2) You will lose access to encrypted history stored for "
                                "current device on server side.\n"
                                "3) New device ID and keys pair will be generated.\n"
                                "4) You will need to verify keys for all devices of your "
                                "contacts again.\n"
                                "5) Your contacts will need to verify new device before "
                                "you start receive messages from them.\n")
        + "\n" + tr("Delete current device?");

    QMessageBox messageBox(QMessageBox::Question, QObject::tr("Confirm action"), message);
    messageBox.addButton(QObject::tr("Delete"), QMessageBox::AcceptRole);
    messageBox.addButton(QObject::tr("Cancel"), QMessageBox::RejectRole);

    if (messageBox.exec() != 0)
        return;

    m_omemo->deleteCurrentDevice(m_account, m_currentDeviceId);
    m_omemo->accountConnected(m_account, m_jid);
    updateData();
    emit updateKnownFingerprints();
}

void ManageDevices::deleteDevice()
{
    QModelIndexList selection = m_table->selectionModel()->selectedIndexes();
    if (!selection.isEmpty()) {
        const QString &message = tr("After deleting of device from list of available devices "
                                    "it stops receiving offline messages from your contacts "
                                    "until it will become online and your contacts mark it "
                                    "as trusted.")
            + "\n\n" + tr("Delete selected device?");

        QMessageBox messageBox(QMessageBox::Question, QObject::tr("Confirm action"), message);
        messageBox.addButton(QObject::tr("Delete"), QMessageBox::AcceptRole);
        messageBox.addButton(QObject::tr("Cancel"), QMessageBox::RejectRole);

        if (messageBox.exec() != 0)
            return;

        m_omemo->unpublishDevice(m_account, selectedDeviceId(selection));
    }
}

void ManageDevices::deviceListUpdated(int account)
{
    if (account == m_account) {
        updateData();
    }
}

OmemoConfiguration::OmemoConfiguration(int account, OMEMO *omemo, QWidget *parent) :
    ConfigWidgetTab(account, omemo, parent)
{
    auto omemoPolicy    = new QGroupBox(tr("OMEMO encryption policy"), this);
    m_alwaysEnabled     = new QRadioButton(tr("Always enabled"), omemoPolicy);
    m_enabledByDefault  = new QRadioButton(tr("Enabled by default"), omemoPolicy);
    m_disabledByDefault = new QRadioButton(tr("Disabled by default"), omemoPolicy);

    auto omemoPolicyLayout = new QVBoxLayout(omemoPolicy);
    omemoPolicyLayout->addWidget(m_alwaysEnabled);
    omemoPolicyLayout->addWidget(m_enabledByDefault);
    omemoPolicyLayout->addWidget(m_disabledByDefault);
    omemoPolicy->setLayout(omemoPolicyLayout);
    omemoPolicy->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    m_trustOwnDevices     = new QCheckBox(tr("Automatically mark new own devices as trusted"), this);
    m_trustContactDevices = new QCheckBox(tr("Automatically mark new interlocutors devices as trusted"), this);

    auto spacerLabel = new QLabel(this);
    spacerLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(omemoPolicy);
    mainLayout->addWidget(m_trustOwnDevices);
    mainLayout->addWidget(m_trustContactDevices);
    mainLayout->addWidget(spacerLabel);
    setLayout(mainLayout);

    loadSettings();
}

void OmemoConfiguration::updateData()
{
    ; // Nothing to do...
}

void OmemoConfiguration::loadSettings()
{
    if (m_omemo->isAlwaysEnabled())
        m_alwaysEnabled->setChecked(true);
    else if (m_omemo->isEnabledByDefault())
        m_enabledByDefault->setChecked(true);
    else
        m_disabledByDefault->setChecked(true);

    m_trustOwnDevices->setChecked(m_omemo->trustNewOwnDevices());
    m_trustContactDevices->setChecked(m_omemo->trustNewContactDevices());
}

void OmemoConfiguration::saveSettings()
{
    m_omemo->setAlwaysEnabled(m_alwaysEnabled->isChecked());
    m_omemo->setEnabledByDefault(m_enabledByDefault->isChecked());
    m_omemo->setTrustNewOwnDevices(m_trustOwnDevices->isChecked());
    m_omemo->setTrustNewContactDevices(m_trustContactDevices->isChecked());
    m_omemo->saveSettings();
}

}
