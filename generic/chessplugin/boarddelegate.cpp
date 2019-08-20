/*
 * boarddelegate.cpp - plugin
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

#include "boarddelegate.h"

#include "boardmodel.h"

#include <QPainter>

using namespace Chess;

void BoardDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    BoardModel *model = (BoardModel*)index.model();
    QRect r = option.rect;
    QColor color = ((option.state & QStyle::State_Selected) && model->myMove && !model->gameState_) ?
               QColor("#b5e3ff") : index.data(Qt::BackgroundColorRole).value<QColor>();
    painter->fillRect(r, color);

    QPixmap pix = index.data(Qt::DisplayRole).value<QPixmap>();
    painter->drawPixmap(r, pix);
}
