/*
 * proxysettingsdlg.h - plugin
 * Copyright (C) 2011  Evgeny Khryukin
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


#ifndef PROXYSETTINGS_H
#define PROXYSETTINGS_H

#include <QDialog>

//for psi+ plugin only
#include "applicationinfoaccessinghost.h"

namespace Ui
{
    class ProxySettingsDlg;
}

class ProxySettingsDlg : public QDialog
{
    Q_OBJECT
public:
    ProxySettingsDlg(QWidget *parent = 0);
    ~ProxySettingsDlg();

    void setProxySettings(const Proxy& p);
    Proxy getSettings() const { return proxy_; };

public slots:
    void accept();

private:
    Ui::ProxySettingsDlg *ui;
    Proxy proxy_;
};

#endif // PROXYSETTINGS_H
