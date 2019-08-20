/*
 * boarddelegate.h - plugin
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

#ifndef BOARDDELEGATE_H
#define BOARDDELEGATE_H

#include <QItemDelegate>

namespace Chess {
class BoardDelegate : public QItemDelegate {
    Q_OBJECT

public:
    BoardDelegate(QObject *parent) : QItemDelegate(parent) {};
    virtual void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;

};
} // namespace Chess

#endif // BOARDDELEGATE_H
