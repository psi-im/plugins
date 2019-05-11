/*
 * figure.h - plugin
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

#ifndef FIGURE_H
#define FIGURE_H

#include <QPixmap>

class Figure
{
public:

    enum GameType {
        NoGame = 0,
        WhitePlayer = 1,
        BlackPlayer = 2
    };

        enum FigureType {
                None = 0,
                White_Pawn = 1,
                White_Castle = 2,
                White_Bishop = 3,
                White_King = 4,
                White_Queen = 5,
                White_Knight = 6,
                Black_Pawn = 7,
                Black_Castle = 8,
                Black_Bishop = 9,
                Black_King = 10,
                Black_Queen = 11,
                Black_Knight = 12
    };

    Figure(GameType game = NoGame, FigureType type = Figure::None, int x = 0, int y = 0, QObject *parent = nullptr);
    QPixmap getPixmap() const;
    void setPosition(int x, int y);
    void setType(FigureType type);
    int positionX() const;
    int positionY() const;
    FigureType type() const;
    GameType gameType() const;
    QString typeString() const;

    bool isMoved;

private:
    int positionX_, positionY_;
    FigureType type_;
    GameType gameType_;

};

#endif // FIGURE_H
