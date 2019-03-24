/*
 * boardview.cpp - Battleship game plugin
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

#include <QHeaderView>

#include "boardview.h"

BoardView::BoardView(QWidget *parent)
    : QTableView(parent)
    , bmodel_(NULL)
{
}

void BoardView::setModel(BoardModel *model)
{
    QTableView::setModel(model);
    bmodel_ = model;
}

void BoardView::resizeEvent(QResizeEvent */*event*/)
{
    setCellsSize();
}

void BoardView::mouseReleaseEvent(QMouseEvent */*event*/)
{
    QModelIndex index = currentIndex();
    if (index.isValid())
    {
        int pos = bmodel_->model2oppboard(QPoint(index.column(), index.row()));
        if (pos != -1)
            bmodel_->gameModel()->localTurn(pos);
    }
}

void BoardView::setCellsSize()
{
    if (!bmodel_)
        return;
    int rowCnt = model()->rowCount() - 2;
    int colCnt = model()->columnCount() - 3;
    int boardWidth = width() - verticalHeader()->width() - (lineWidth() + midLineWidth()) * 2;
    int boardHeight = height() - horizontalHeader()->height() - (lineWidth() + midLineWidth()) * 2;
    boardWidth -= 4; boardHeight -= 4; // Запас для гарантии отсутствия прокрутки
    int cellWidth = boardWidth / colCnt - 1;
    int cellHeight = boardHeight / rowCnt - 1;
    int cellSize = qMin(cellWidth, cellHeight);
    int hMargin = boardHeight - cellSize * rowCnt;
    if (hMargin < 0)
        hMargin = 0;
    hMargin /= 2;
    int vMargin = boardWidth - cellSize * colCnt;
    if (vMargin < 0)
        vMargin = 0;
    vMargin /= 3;
    horizontalHeader()->setDefaultSectionSize(cellSize);
    verticalHeader()->setDefaultSectionSize(cellSize);
    horizontalHeader()->resizeSection(0, vMargin);
    horizontalHeader()->resizeSection(colCnt / 2 + 1, vMargin);
    horizontalHeader()->resizeSection(colCnt + 2, vMargin);
    verticalHeader()->resizeSection(0, hMargin);
    verticalHeader()->resizeSection(rowCnt + 1, hMargin);
}
