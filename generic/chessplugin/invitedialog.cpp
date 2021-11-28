/*
 * invitedialog.cpp - plugin
 * Copyright (C) 2010-2011  Evgeny Khryukin
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

using namespace Chess;

InviteDialog::InviteDialog(const Request &_r, const QStringList &resources, QWidget *parent) :
    QDialog(parent), resources_(resources), r(_r)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui_.setupUi(this);
    ui_.cb_resource->setEditable(true);
    if (!resources.isEmpty()) {
        ui_.cb_resource->addItems(resources_);
    } else {
        ui_.cb_resource->addItem("Enter resource");
    }

    connect(ui_.pb_black, &QPushButton::pressed, this, &InviteDialog::buttonPressed);
    connect(ui_.pb_white, &QPushButton::pressed, this, &InviteDialog::buttonPressed);

    adjustSize();
    setFixedSize(size());
}

void InviteDialog::buttonPressed()
{
    QString color = "white";
    if (ui_.pb_black->isDown()) {
        color = "black";
    }

    emit play(r, ui_.cb_resource->currentText(), color);
    close();
}

static QString unescape(const QString &escaped)
{
    QString plain = escaped;
    plain.replace("&lt;", "<");
    plain.replace("&gt;", ">");
    plain.replace("&quot;", "\"");
    plain.replace("&amp;", "&");
    return plain;
}

InvitationDialog::InvitationDialog(const QString &jid, QString color, QWidget *parent) : QDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(false);
    ui_.setupUi(this);
    accepted = false;

    if (color == "white")
        color = tr("white");
    else
        color = tr("black");

    ui_.lb_text->setText(tr("Player %1 invites you \nto play chess. He wants to play %2.").arg(unescape(jid), color));

    connect(ui_.pb_accept, &QPushButton::pressed, this, &InvitationDialog::buttonPressed);
    connect(ui_.pb_reject, &QPushButton::pressed, this, &InvitationDialog::close);

    adjustSize();
    setFixedSize(size());
}

void InvitationDialog::buttonPressed()
{
    emit accept();
    accepted = true;
    close();
}

void InvitationDialog::closeEvent(QCloseEvent *e)
{
    if (!accepted)
        emit reject();
    e->accept();
    close();
}
