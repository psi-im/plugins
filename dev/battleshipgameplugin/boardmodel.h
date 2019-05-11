/*
 * boardmodel.h - Battleship game plugin
 * Copyright (C) 2014  Aleksey Andreev
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

#ifndef BOARDMODEL_H
#define BOARDMODEL_H

#include <QAbstractTableModel>

#include "gamemodel.h"

class BoardModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    BoardModel(QObject *parent = nullptr);
    ~BoardModel();
    void init(GameModel *gm);
    GameModel *gameModel() const { return gameModel_; }
    int model2oppboard(const QPoint &p);
    int model2myboard(const QPoint &p);
    virtual Qt::ItemFlags flags(const QModelIndex & index) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
    QPoint myboard2model(const QPoint &p) const;
    QPoint oppboard2model(const QPoint &p) const;

private:
    GameModel *gameModel_;

private slots:
    void updateMyBoard(int x, int y, int width, int height);
    void updateOppBoard(int x, int y, int width, int height);

};

#endif // BOARDMODEL_H
