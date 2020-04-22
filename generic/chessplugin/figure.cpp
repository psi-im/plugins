/*
 * figure.cpp - plugin
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

#include "figure.h"

Figure::Figure(GameType game, FigureType type, int x, int y, QObject * /*parent*/) :
    isMoved(false), positionX_(x), positionY_(y), type_(type), gameType_(game)
{
}

QPixmap Figure::getPixmap() const
{
    switch (type_) {
    case White_Pawn:
        return QPixmap(":/chessplugin/figures/white_pawn.png");
    case White_King:
        return QPixmap(":/chessplugin/figures/white_king.png");
    case White_Queen:
        return QPixmap(":/chessplugin/figures/white_queen.png");
    case White_Bishop:
        return QPixmap(":/chessplugin/figures/white_bishop.png");
    case White_Knight:
        return QPixmap(":/chessplugin/figures/white_knight.png");
    case White_Castle:
        return QPixmap(":/chessplugin/figures/white_castle.png");
    case Black_Pawn:
        return QPixmap(":/chessplugin/figures/black_pawn.png");
    case Black_King:
        return QPixmap(":/chessplugin/figures/black_king.png");
    case Black_Queen:
        return QPixmap(":/chessplugin/figures/black_queen.png");
    case Black_Bishop:
        return QPixmap(":/chessplugin/figures/black_bishop.png");
    case Black_Knight:
        return QPixmap(":/chessplugin/figures/black_knight.png");
    case Black_Castle:
        return QPixmap(":/chessplugin/figures/black_castle.png");
    case None:
        return QPixmap();
    }
    return QPixmap();
}

QString Figure::typeString() const
{
    switch (type_) {
    case White_Pawn:
        return "Pawn";
    case White_King:
        return "King";
    case White_Queen:
        return "Queen";
    case White_Bishop:
        return "Bishop";
    case White_Knight:
        return "Knight";
    case White_Castle:
        return "Rook";
    case Black_Pawn:
        return "Pawn";
    case Black_King:
        return "King";
    case Black_Queen:
        return "Queen";
    case Black_Bishop:
        return "Bishop";
    case Black_Knight:
        return "Knight";
    case Black_Castle:
        return "Rook";
    case None:
        return QString();
    }
    return QString();
}

int Figure::positionX() const { return positionX_; }

int Figure::positionY() const { return positionY_; }

void Figure::setPosition(int x, int y)
{
    positionX_ = x;
    positionY_ = y;
}

Figure::FigureType Figure::type() const { return type_; }

Figure::GameType Figure::gameType() const { return gameType_; }

void Figure::setType(FigureType type) { type_ = type; }
