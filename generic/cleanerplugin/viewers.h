/*
 * viewers.h - plugin
 * Copyright (C) 2009-2010  Khryukin Evgeny
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef VIEWERS_H
#define VIEWERS_H

#include <QTableView>
#include <QKeyEvent>
#include <QItemDelegate>
#include "iconfactoryaccessinghost.h"

class ClearingViewer : public QTableView
{
    Q_OBJECT

    public:
        ClearingViewer(QWidget *parent = 0) : QTableView(parent) {};
       // virtual ~ClearingViewer() {};
        void init(IconFactoryAccessingHost *iconHost);

    private:
        IconFactoryAccessingHost *iconHost_;

   protected:
        void keyPressEvent(QKeyEvent *e);
        void contextMenuEvent( QContextMenuEvent * e );

   private slots:
        void itemClicked(QModelIndex index);

};



class AvatarDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    AvatarDelegate(QObject *parent) : QItemDelegate(parent) {};
    virtual QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const;
    virtual void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;

};


#endif // VIEWERS_H
