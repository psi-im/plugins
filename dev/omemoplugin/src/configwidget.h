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

#ifndef PSI_CONFIGWIDGET_H
#define PSI_CONFIGWIDGET_H

#include <QtGui>
#include <QWidget>
#include <QTableView>
#include "omemo.h"

namespace psiomemo {
  class ConfigWidget: public QWidget {
  Q_OBJECT
  public:
    ConfigWidget(OMEMO *omemo);
  };

  class OwnFingerprint: public QWidget {
  Q_OBJECT
  public:
    OwnFingerprint(OMEMO *omemo, QWidget *parent);
  };

  class KnownFingerprints: public QWidget {
  Q_OBJECT
  public:
    KnownFingerprints(OMEMO *omemo, QWidget *parent);
  private:
    QTableView *m_table;
    QStandardItemModel* m_tableModel;
    OMEMO *m_omemo;

    void updateData();
  private slots:
    void trustRevokeFingerprint();
  };
}

#endif //PSI_CONFIGWIDGET_H
