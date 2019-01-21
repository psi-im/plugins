/*
 * gameelement.h - Gomoku Game plugin
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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef GAMEELEMENT_H
#define GAMEELEMENT_H

#include <QPainter>

class GameElement
{
public:
    enum ElementType {
        TypeNone,
        TypeBlack,
        TypeWhite
    };

    GameElement(ElementType type, int x, int y);
    GameElement(const GameElement *from);
    ~GameElement();
    int x() const;
    int y() const;
    ElementType type() const;
    void paint(QPainter *painter, const QRectF &rect) const;

private:
    ElementType type_;
    int posX;
    int posY;
    static int usesCnt;
    static QPixmap *blackstonePixmap;
    static QPixmap *whitestonePixmap;

private:
    QPixmap *getBlackstonePixmap() const;
    QPixmap *getWhitestonePixmap() const;

};

#endif // GAMEELEMENT_H
