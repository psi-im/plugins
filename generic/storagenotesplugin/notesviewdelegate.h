/*
 * notesviewdelegate.h - plugin
 * Copyright (C) 2010  Evgeny Khryukin
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

#ifndef NOTESVIEWDELEGATE_H
#define NOTESVIEWDELEGATE_H

#include <QItemDelegate>
#include <QPainter>

class NotesViewDelegate : public QItemDelegate {
    Q_OBJECT

public:
        NotesViewDelegate(QObject * parent = nullptr)  : QItemDelegate(parent) {}
    ~NotesViewDelegate();
    virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
        virtual void drawDisplay(QPainter * /*painter*/, const QStyleOptionViewItem & /*option*/, const QRect & /*rect*/, const QString & /*text*/) const {}
    void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;

};

#endif // NOTESVIEWDELEGATE_H
