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

#ifndef PSI_CONFIGWIDGET_H
#define PSI_CONFIGWIDGET_H

#include "omemo.h"
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QTableView>
#include <QWidget>
#include <QtGui>

namespace psiomemo {
class ConfigWidgetTab : public QWidget {
    Q_OBJECT

public:
    ConfigWidgetTab(int account, OMEMO *omemo, QWidget *parent) :
        QWidget(parent), m_account(account), m_omemo(omemo) { }

    void setAccount(int account)
    {
        m_account = account;
        updateData();
    }

protected:
    virtual void updateData() = 0;

protected:
    int    m_account;
    OMEMO *m_omemo;
};

class ConfigWidgetTabWithTable : public ConfigWidgetTab {
    Q_OBJECT

public:
    ConfigWidgetTabWithTable(int account, OMEMO *omemo, QWidget *parent);
    void filterContacts(const QString &jid);
    void updateData() override;
    void copyFingerprintFromTable(QStandardItemModel *tableModel,
                                  const QModelIndexList &indexesList,
                                  const int column);

protected:
    virtual void        doUpdateData() = 0;
    QTableView *        m_table;
    QStandardItemModel *m_tableModel;
    QString             m_jid;
};

class KnownFingerprints : public ConfigWidgetTabWithTable {
    Q_OBJECT
public:
    KnownFingerprints(int account, OMEMO *omemo, QWidget *parent);

protected:
    void doUpdateData() override;

private slots:
    void removeKnownKey();
    void trustKnownKey();
    void revokeKnownKey();
    void contextMenuKnownKeys(const QPoint &pos);
    void copyKnownFingerprint();
};

class ManageDevices : public ConfigWidgetTabWithTable {
    Q_OBJECT

public:
    ManageDevices(int account, OMEMO *omemo, QWidget *parent);

signals:
    void updateKnownFingerprints();

protected:
    void updateData() override;

private:
    QLabel *     m_fingerprintLabel;
    QLabel *     m_deviceIdLabel;
    uint32_t     m_currentDeviceId;
    QPushButton *m_deleteButton;

protected:
    void doUpdateData() override;

private slots:
    void deleteCurrentDevice();
    void deleteDevice();
    void deviceListUpdated(int account);
    void contextMenuOwnDevices(const QPoint &pos);
    void copyOwnFingerprint();
};

class OmemoConfiguration : public ConfigWidgetTab {
    Q_OBJECT

public:
    OmemoConfiguration(int account, OMEMO *omemo, QWidget *parent);

    void updateData() override;
    void loadSettings();
    void saveSettings();

private:
    QRadioButton *m_alwaysEnabled;
    QRadioButton *m_enabledByDefault;
    QRadioButton *m_disabledByDefault;
    QCheckBox *   m_trustOwnDevices;
    QCheckBox *   m_trustContactDevices;
};

class ConfigWidget : public QWidget {
    Q_OBJECT

public:
    ConfigWidget(OMEMO *omemo, AccountInfoAccessingHost *accountInfo);

signals:
    void applySettings();

private slots:
    void currentAccountChanged(int index);

private:
    AccountInfoAccessingHost *m_accountInfo;
    QTabWidget *              m_tabWidget;
};
}

#endif // PSI_CONFIGWIDGET_H
