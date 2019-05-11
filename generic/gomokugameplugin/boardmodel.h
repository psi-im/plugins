/*
 * boardmodel.h - Gomoku Game plugin
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

#ifndef BOARDMODEL_H
#define BOARDMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

#include "gameelement.h"
#include "gamemodel.h"

namespace GomokuGame {

class BoardModel : public QAbstractTableModel
{
Q_OBJECT
public:
    explicit BoardModel(QObject *parent = nullptr);
    ~BoardModel();
    void init(GameModel *gameModel);
    virtual Qt::ItemFlags flags(const QModelIndex & index) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const;
    // --
    const GameElement *getGameElement(int x, int y);
    bool clickToBoard(QModelIndex index);
    bool opponentTurn(int x, int y);
    void setAccept();
    void setError();
    void setClose();
    void setWin();
    void opponentDraw();
    void setResign();
    int  turnNum();
    bool doSwitchColor(bool local);
    QString saveToString() const;
    void setSelect(int x, int y);
    GameElement::ElementType myElementType() const;

public:
    int selectX;
    int selectY;

private:
    int columnCount_;
    int rowCount_;
    GameModel *gameModel;

private:
    void setHeaders();
    bool setElementToBoard(int x, int y, bool my_element);

signals:
    void changeGameStatus(GameModel::GameStatus);
    void setupElement(int x, int y);
    void lose();
    void draw();
    void switchColor();
    void doPopup(const QString);
    void playSound(const QString);

};
}

#endif // BOARDMODEL_H
