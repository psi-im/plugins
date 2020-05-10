/*
 * addkeydlg.cpp - generating key pair dialog
 *
 * Copyright (C) 2013  Ivan Romanov <drizt@land.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "addkeydlg.h"
#include "datewidget.h"
#include "ui_addkeydlg.h"
#include <QPushButton>

AddKeyDlg::AddKeyDlg(QWidget *parent) : QDialog(parent), ui(new Ui::AddKeyDlg)
{
    ui->setupUi(this);
    adjustSize();
    // By default key expires in a year
    ui->dateExpiration->setDate(QDate::currentDate().addYears(1));
    fillLenght(ui->cmbType->currentText());
    ui->lineName->setFocus();
}

AddKeyDlg::~AddKeyDlg() { delete ui; }

QString AddKeyDlg::name() const { return ui->lineName->text().trimmed(); }

QString AddKeyDlg::email() const { return ui->lineEmail->text().trimmed(); }

QString AddKeyDlg::comment() const { return ui->lineComment->text().trimmed(); }

int AddKeyDlg::type() const { return ui->cmbType->currentIndex(); }

int AddKeyDlg::length() const { return ui->cmbLength->currentText().toInt(); }

QDate AddKeyDlg::expiration() const { return ui->dateExpiration->date(); }

QString AddKeyDlg::pass() const { return ui->linePass->text(); }

void AddKeyDlg::checkPass()
{
    ui->btnBox->button(QDialogButtonBox::Ok)->setEnabled(ui->linePass->text() == ui->linePass2->text());
}

void AddKeyDlg::fillLenght(const QString &type)
{
    QStringList lengths;
    lengths << "1024"
            << "2048"
            << "3072";
    if (!type.contains("DSA")) {
        lengths << "4096";
    }
    ui->cmbLength->clear();
    ui->cmbLength->addItems(lengths);
    ui->cmbLength->setCurrentIndex(1);
}
