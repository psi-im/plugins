/*
 * boardmodel.cpp - Gomoku Game plugin
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

#include "boardmodel.h"
#include "common.h"

using namespace GomokuGame;

BoardModel::BoardModel(QObject *parent) :
    QAbstractTableModel(parent), selectX(-1), selectY(-1), columnCount_(0), rowCount_(0), gameModel(nullptr)
{
}

BoardModel::~BoardModel()
{
    if (gameModel)
        delete gameModel;
}

void BoardModel::init(GameModel *gm)
{
    if (gameModel)
        delete gameModel;
    gameModel = gm;
    selectX   = -1;
    selectY   = -1;
    setHeaders();
    beginResetModel();
    endResetModel();
    connect(gameModel, &GameModel::statusUpdated, this, &BoardModel::changeGameStatus);
    emit changeGameStatus(gm->gameStatus());
}

void BoardModel::setHeaders()
{
    rowCount_    = gameModel->boardSizeY() + 4;
    columnCount_ = gameModel->boardSizeX() + 4;
}

Qt::ItemFlags BoardModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::NoItemFlags | Qt::ItemIsEnabled;
    int           row   = index.row();
    if (row < 2 || row > rowCount_ - 2) {
        return flags;
    }
    int col = index.column();
    if (col < 2 || col > columnCount_ - 2) {
        return flags;
    }
    return (flags | Qt::ItemIsSelectable);
}

QVariant BoardModel::headerData(int /*section*/, Qt::Orientation /*orientation*/, int /*role*/) const
{
    return QVariant();
}

QVariant BoardModel::data(const QModelIndex & /*index*/, int /*role*/) const { return QVariant(); }

bool BoardModel::setData(const QModelIndex &index, const QVariant & /*value*/, int role)
{
    if (index.isValid() && role == Qt::DisplayRole) {
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

int BoardModel::rowCount(const QModelIndex & /*parent*/) const { return rowCount_; }

int BoardModel::columnCount(const QModelIndex & /*parent*/) const { return columnCount_; }

const GameElement *BoardModel::getGameElement(int x, int y) { return gameModel->getElement(x - 2, y - 2); }

/**
 * Был щелчек мышью по доске
 */
bool BoardModel::clickToBoard(QModelIndex index)
{
    if (index.isValid()) {
        int x = index.column() - 2;
        int y = index.row() - 2;
        if (setElementToBoard(x, y, true)) {
            emit setupElement(x, y);
            return true;
        }
    }
    return false;
}

/**
 * Пришел ход от оппонента
 */
bool BoardModel::opponentTurn(int x, int y)
{
    if (setElementToBoard(x, y, false)) {
        // После хода оппонента в модели производится проверка на проигрыш и ничью.
        GameModel::GameStatus status = gameModel->gameStatus();
        if (status == GameModel::StatusLose)
            emit lose(); // Сообщаем оппоненту что мы проиграли
        else if (status == GameModel::StatusDraw)
            emit draw(); // Сообщаем оппоненту об ничьей
        return true;
    }
    gameModel->setErrorStatus();
    return false;
}

void BoardModel::setAccept() { gameModel->accept(); }

void BoardModel::setError() { gameModel->setErrorStatus(); }

void BoardModel::setClose() { gameModel->breakGame(); }

void BoardModel::setWin() { gameModel->setWin(); }

void BoardModel::opponentDraw() { gameModel->setDraw(); }

/**
 * Мы сдались. Сообщаем оппоненту и самому себе.
 */
void BoardModel::setResign()
{
    emit lose();
    gameModel->setLose();
}

/**
 * Добавляет новый элемент (в данном случае камень) в игровую модель и обновляет игровую доску
 * Возвращает true в случае удачи.
 */
bool BoardModel::setElementToBoard(int x, int y, bool local)
{
    if (gameModel->doTurn(x, y, local)) {
        const QModelIndex mi = index(y + 2, x + 2); // Offset for margin
        emit              dataChanged(mi, mi);
        return true;
    }
    const QString msg = gameModel->getLastError();
    if (!msg.isEmpty())
        emit doPopup(msg);
    return false;
}

/**
 * Возвращает номер  ожидаемого хода
 */
int BoardModel::turnNum() { return (gameModel->turnsCount() + 1); }

/**
 * Переключает цвет игрока (предусмотрено международными правилами)
 */
bool BoardModel::doSwitchColor(bool local)
{
    if (gameModel->doSwitchColor(local)) {
        if (local)
            emit switchColor();
        return true;
    }
    return false;
}

/**
 * Сохраняет состояние игры в строку
 */
QString BoardModel::saveToString() const { return gameModel->toString(); }

/**
 * Устанавливает новые координаты подсвечиваемого объекта и дает команды на перисовку
 */
void BoardModel::setSelect(int x, int y)
{
    int old_x = selectX;
    int old_y = selectY;
    selectX   = x + 2;
    selectY   = y + 2;
    if (old_x != selectX || old_y != selectY) {
        if (old_x != -1 && old_y != -1) {
            QModelIndex idx = index(old_y, old_x);
            emit        dataChanged(idx, idx);
        }
        QModelIndex idx = index(selectY, selectX);
        emit        dataChanged(idx, idx);
    }
}

GameElement::ElementType BoardModel::myElementType() const
{
    if (gameModel) {
        return gameModel->myElementType();
    }
    return GameElement::TypeNone;
}
