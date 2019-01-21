/*
 * jd_view.cpp - plugin
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
#include "jd_view.h"
#include <QMouseEvent>

JDView::JDView(QWidget* p)
    : QTreeView(p)
{
}

JDView::~JDView()
{
}

void JDView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QTreeView::currentChanged(current, previous);
    emit newIndex(current);
}

void JDView::mousePressEvent(QMouseEvent *e)
{
    QTreeView::mousePressEvent(e);
    if(e->button() == Qt::RightButton) {
        emit contextMenu(currentIndex());
    }
}

//void JDView::dragEnterEvent(QDragEnterEvent *event)
//{
//    if(event->mimeData()->hasFormat(JDItem::mimeType()))
//        event->acceptProposedAction();
//}
//
//void JDView::dropEvent(QDropEvent *event)
//{
//    QTreeView::dropEvent(event);
//}
