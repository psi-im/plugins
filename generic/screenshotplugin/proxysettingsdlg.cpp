/*
 * proxysettingsdlg.cpp - plugin
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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */


#include "proxysettingsdlg.h"
#include "ui_proxysettingsdlg.h"

ProxySettingsDlg::ProxySettingsDlg(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::ProxySettingsDlg)
{
	ui->setupUi(this);
	ui->cb_type->addItems(QStringList() << "HTTP" << "SOCKS5");
	ui->cb_type->setCurrentIndex(0);
}

ProxySettingsDlg::~ProxySettingsDlg()
{
	delete ui;
}

void ProxySettingsDlg::setProxySettings(const Proxy &p)
{
	proxy_ = p;
	ui->le_host->setText(p.host);
	ui->le_pass->setText(p.pass);
	ui->le_port->setText(QString::number(p.port));
	ui->le_user->setText(p.user);
	if(p.type == "socks") {
		ui->cb_type->setCurrentIndex(1);
	}
}

void ProxySettingsDlg::accept()
{
	if(ui->cb_type->currentText() == "HTTP") {
		proxy_.type = "http";
	}
	else {
		proxy_.type = "socks";
	}
	proxy_.host = ui->le_host->text();
	proxy_.port = ui->le_port->text().toInt();
	proxy_.user = ui->le_user->text();
	proxy_.pass = ui->le_pass->text();

	QDialog::accept();
}
