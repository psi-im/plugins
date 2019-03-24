/*
* invitedialog.cpp - Battleship Game plugin
* Copyright (C) 2014  Aleksey Andreev
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

#include "invitedialog.h"


InviteDialog::InviteDialog(const QString &jid, const QStringList &resources, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::InviteDialog)
    , accepted_(false)
    , jid_(jid)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    ui->leJid->setText(jid_);
    ui->cbResource->addItems(resources);
    adjustSize();
    connect(ui->btnFirst, SIGNAL(clicked()), this, SLOT(acceptFirst()));
    connect(ui->btnSecond, SIGNAL(clicked()), this, SLOT(acceptSecond()));
    connect(ui->btnCancel, SIGNAL(clicked()), this, SLOT(close()));
}

InviteDialog::~InviteDialog()
{
    delete ui;
}

void InviteDialog::acceptFirst()
{
    emit acceptGame(jid_ + "/" + ui->cbResource->currentText(), true);
    accepted_ = true;
    accept();
    close();
}

void InviteDialog::acceptSecond()
{
    emit acceptGame(jid_ + "/" + ui->cbResource->currentText(), false);
    accepted_ = true;
    accept();
    close();
}

void InviteDialog::closeEvent(QCloseEvent *event)
{
    if (!accepted_)
        reject();
    event->accept();
}

// ----------------------------------------

InvitationDialog::InvitationDialog(const QString &jid, bool first, QWidget *parent)
    : QDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(false);
    ui_.setupUi(this);

    QString posStr;
    if(first)
        posStr = tr("second", "He wants to play second");
    else
        posStr = tr("first", "He wants to play first");

    ui_.lbl_text->setText(tr("Player %1 invites you \nto play battleship. He wants to play %2.")
                  .arg(jid).arg(posStr));

    connect(ui_.pb_accept, SIGNAL(clicked()), this, SLOT(okPressed()));
    connect(ui_.pb_reject, SIGNAL(clicked()), this, SLOT(close()));

    adjustSize();
    setFixedSize(size());
}

void InvitationDialog::okPressed()
{
    accept();
    close();
}
