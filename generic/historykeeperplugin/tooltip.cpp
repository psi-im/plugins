/*
 * tooltip.cpp - plugin
 * Copyright (C) 2010  Khryukin Evgeny
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

#include "tooltip.h"

#include <QHBoxLayout>
#include <QCursor>

KeeperToolTip::KeeperToolTip(QString jid, bool checked, QObject* /*parent*/)
            : QFrame(0, Qt::ToolTip)
            , jid_(jid)
{
    setAttribute(Qt::WA_DeleteOnClose);
    QHBoxLayout *layout = new QHBoxLayout(this);
    box_ = new QCheckBox(tr("Remove history on exit"));
    box_->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    layout->addWidget(box_);

    QPoint pos = QCursor::pos();
    pos.setX(pos.x() - 20);
    pos.setY(pos.y() - 20);
    move(pos);

    connect(box_, SIGNAL(stateChanged(int)), this, SLOT(stateChanged(int)));
}

void KeeperToolTip::stateChanged(int state)
{
    if(state)
        emit check(jid_, true);
    else
        emit check(jid_, false);
}

void KeeperToolTip::leaveEvent(QEvent *e)
{
    e->accept();
    close();
}
