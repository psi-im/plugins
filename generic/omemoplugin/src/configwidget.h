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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef PSI_CONFIGWIDGET_H
#define PSI_CONFIGWIDGET_H

#include <QtGui>
#include <QWidget>
#include <QTableView>
#include <QLabel>
#include <QPushButton>
#include "omemo.h"

namespace psiomemo {
  class ConfigWidgetTab: public QWidget {
  Q_OBJECT
  public:
    ConfigWidgetTab(int account, OMEMO *omemo, QWidget *parent): QWidget(parent), m_account(account), m_omemo(omemo) { }
    void setAccount(int account) {
      m_account = account;
      updateData();
    }
  protected:
    virtual void updateData() = 0;
  protected:
    int m_account;
    OMEMO *m_omemo;
  };

  class ConfigWidgetTabWithTable: public ConfigWidgetTab {
  Q_OBJECT
  public:
    ConfigWidgetTabWithTable(int account, OMEMO *omemo, QWidget *parent);
  protected:
    void updateData() override;
    virtual void doUpdateData() = 0;
    QTableView *m_table;
    QStandardItemModel* m_tableModel;
  };

  class OwnFingerprint: public ConfigWidgetTab {
  Q_OBJECT
  public:
    OwnFingerprint(int account, OMEMO *omemo, QWidget *parent);
  protected:
    void updateData() override;
  private:
    QLabel *m_deviceLabel;
    QLabel *m_fingerprintLabel;
  };

  class KnownFingerprints: public ConfigWidgetTabWithTable {
  Q_OBJECT
  public:
    KnownFingerprints(int account, OMEMO *omemo, QWidget *parent);
  protected:
    void doUpdateData() override;
  private slots:
    void trustRevokeFingerprint();
  };

  class ManageDevices: public ConfigWidgetTabWithTable {
  Q_OBJECT
  public:
    ManageDevices(int account, OMEMO *omemo, QWidget *parent);
  private:
    uint32_t m_ourDeviceId;
    QPushButton *m_deleteButton;
    uint32_t selectedDeviceId(const QModelIndexList &selection) const;
  protected:
    void doUpdateData() override;
  private slots:
    void selectionChanged(const QItemSelection &, const QItemSelection &);
    void deleteDevice();
    void deviceListUpdated(int account);
  };

  class ConfigWidget: public QWidget {
  Q_OBJECT
  public:
    ConfigWidget(OMEMO *omemo, AccountInfoAccessingHost *accountInfo);
  private:
    AccountInfoAccessingHost *m_accountInfo;
    QTabWidget *m_tabWidget;
  private slots:
    void currentAccountChanged(int index);
  };
}

#endif //PSI_CONFIGWIDGET_H
