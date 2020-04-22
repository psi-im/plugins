/*
 * boarddelegate.h - Gomoku Game plugin
 * Copyright (C) 2011  Aleksey Andreev
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * You can also redistribute and/or modify this program under the
 * terms of the Psi License, specified in the accompanied COPYING
 * file, as published by the Psi Project; either dated January 1st,
 * 2005, or (at your option) any later version.
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
#include <QPainter>

#include "boardmodel.h"

namespace GomokuGame {

class BoardPixmaps : public QObject {

public:
    BoardPixmaps(QObject *parent = nullptr);
    ~BoardPixmaps();
    void     clearPix();
    QPixmap *getBoardPixmap(int, int, double, double);

private:
    QPixmap *             boardPixmap;
    double                width;
    double                height;
    int                   w_cnt;
    int                   h_cnt;
    QHash<int, QPixmap *> scaledPixmap;
};

class BoardDelegate : public QItemDelegate {
    Q_OBJECT
public:
    explicit BoardDelegate(BoardModel *model, QObject *parent = nullptr);
    void         setSkin(int skin_num);
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    BoardModel *  model_;
    int           skin;
    BoardPixmaps *pixmaps;
};
}

#endif // BOARDDELEGATE_H
