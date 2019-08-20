/*
 * juickjidlist.cpp - plugin
 * Copyright (C) 2010  Evgeny Khryukin
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
 *
 */

#include "juickjidlist.h"

#include "ui_juickjidlist.h"

#include <QInputDialog>

JuickJidList::JuickJidList(const QStringList &jids, QWidget *p)
    : QDialog(p)
    , ui_(new Ui::JuickJidDialog)
    , jidList_(jids)
{
    ui_->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    ui_->listWidget->addItems(jidList_);
    ui_->pb_del->setEnabled(false);

    connect(ui_->pb_add, SIGNAL(released()), SLOT(addPressed()));
    connect(ui_->pb_del, SIGNAL(released()), SLOT(delPressed()));
    connect(ui_->pb_ok, SIGNAL(released()), SLOT(okPressed()));
    connect(ui_->listWidget, SIGNAL(clicked(QModelIndex)), SLOT(enableButtons()));
}

JuickJidList::~JuickJidList()
{
    delete ui_;
}

void JuickJidList::enableButtons()
{
    ui_->pb_del->setEnabled(!ui_->listWidget->selectedItems().isEmpty());
}

void JuickJidList::addPressed()
{
    bool ok;
    QString jid = QInputDialog::getText(this, tr("Input JID"),"",QLineEdit::Normal,"", &ok);
    if(ok) {
        jidList_.append(jid);
        ui_->listWidget->addItem(jid);
    }
}

void JuickJidList::delPressed()
{
    QList<QListWidgetItem*> list = ui_->listWidget->selectedItems();
    foreach(QListWidgetItem *i, list) {
        QString jid = i->text();
        jidList_.removeAll(jid);
        ui_->listWidget->removeItemWidget(i);
        delete i;
    }
}

void JuickJidList::okPressed()
{
    emit listUpdated(jidList_);
    close();
}
