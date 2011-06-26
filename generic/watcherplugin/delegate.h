/*
 * delegate.h - plugin
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

#ifndef DELEGATE_H
#define DELEGATE_H

#include <QItemDelegate>
#include <QPainter>

#include "iconfactoryaccessinghost.h"

class IconDelegate : public QItemDelegate
{
    Q_OBJECT

    public:
            IconDelegate(IconFactoryAccessingHost *iconHost, QObject * parent = 0)  : QItemDelegate(parent), iconHost_(iconHost) {};
            virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
            virtual void drawDisplay(QPainter * /*painter*/, const QStyleOptionViewItem & /*option*/, const QRect & /*rect*/, const QString & /*text*/) const {};
            void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;

    private:
            IconFactoryAccessingHost *iconHost_;

};

class LineEditDelegate : public QItemDelegate
{
    Q_OBJECT

    public:
            LineEditDelegate(QObject * parent = 0)  : QItemDelegate(parent) {};
            QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
            void setEditorData(QWidget *editor, const QModelIndex &index) const;
            void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
           // void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // DELEGATE_H
