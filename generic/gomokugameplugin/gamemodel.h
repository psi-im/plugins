/*
 * gamemodel.h - Gomoku Game plugin
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

#ifndef GAMEMODEL_H
#define GAMEMODEL_H

#include <QObject>

#include "gameelement.h"

class GameModel : public QObject {
    Q_OBJECT
public:
    enum GameStatus {
        StatusNone,
        StatusWaitingLocalAction,
        StatusWaitingAccept,
        StatusWaitingOpponent,
        StatusWin,
        StatusLose,
        StatusDraw,
        StatusBreak, // Игра прервана. Например закрытие доски оппонента
        StatusError
    };
    struct TurnInfo {
        int  x;
        int  y;
        bool my;
    };
    GameModel(GameElement::ElementType my, int row_count, int col_count, QObject *parent = nullptr);
    GameModel(const QString &load_str, const bool local, QObject *parent = nullptr);
    ~GameModel();
    bool                     isValid() const { return valid_; }
    GameStatus               gameStatus() const;
    int                      boardSizeX() const { return boardSizeX_; };
    int                      boardSizeY() const { return boardSizeY_; };
    int                      turnsCount() const { return turnsCount_; };
    GameElement::ElementType myElementType() const { return my_el; };
    const GameElement       *getElement(int x, int y) const;
    bool                     doTurn(int x, int y, bool local);
    bool                     doSwitchColor(bool local);
    bool                     accept();
    void                     setErrorStatus();
    QString                  getLastError() const { return lastErrorStr; };
    void                     breakGame();
    void                     setWin();
    void                     setDraw();
    void                     setLose();
    QString                  toString() const;
    int                      lastX() const;
    int                      lastY() const;
    bool                     isLoaded() const;
    QString                  gameInfo() const;
    TurnInfo                 turnInfo(int num) const;

private:
    enum ChksumStatus { ChksumNone, ChksumCorrect, ChksumIncorrect };
    bool                     valid_;
    GameStatus               status_;
    bool                     accepted_;
    int                      turnsCount_;
    int                      blackCount_;
    int                      whiteCount_;
    GameElement::ElementType my_el;
    bool                     switchColor;
    int                      boardSizeX_;
    int                      boardSizeY_;
    int                      loadedTurnsCount;
    ChksumStatus             chksum;
    QString                  lastErrorStr;
    QList<GameElement *>     elementsList;

private:
    bool    selectGameStatus();
    int     getElementIndex(int x, int y) const;
    bool    checkForLose();
    bool    checkForDraw();
    QString statusString() const;

signals:
    void statusUpdated(GameModel::GameStatus);

public slots:
};

#endif // GAMEMODEL_H
