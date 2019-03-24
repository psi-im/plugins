/*
 * boardview.cpp - Gomoku Game plugin
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

#include <QHeaderView>

#include "boardview.h"

using namespace GomokuGame;

BoardView::BoardView(QWidget *parent) :
    QTableView(parent),
    model_(NULL)
{
}

void BoardView::setModel(QAbstractItemModel *model)
{
    QTableView::setModel(model);
    model_ = static_cast<BoardModel *>(model);
}

void BoardView::resizeEvent(QResizeEvent * /*event*/)
{
    setCellsSize();
}

void BoardView::mouseReleaseEvent(QMouseEvent * /*event*/)
{
    QModelIndex index = currentIndex();
    if (index.isValid()) {
        model_->clickToBoard(index);
    }
}

void BoardView::setCellsSize()
{
    if (!model_)
        return;
    int row_cnt = model()->rowCount() - 2;
    int col_cnt = model()->columnCount() - 2;
    int board_width = width() - verticalHeader()->width() - (lineWidth() + midLineWidth()) * 2;
    int board_height = height() - horizontalHeader()->height() - (lineWidth() + midLineWidth()) * 2;
    board_width -= 4; board_height -= 4; // Запас для гарантии отсутствия прокрутки
    int cell_width = board_width / (row_cnt) - 1;
    int cell_heigt = board_height / (col_cnt) - 1;
    int cell_size = qMin(cell_width, cell_heigt);
    int h_margin = board_width - cell_size * col_cnt;
    if (h_margin < 0)
        h_margin = 0;
    h_margin /= 2;
    int v_margin = board_height - cell_size * row_cnt;
    if (v_margin < 0)
        v_margin = 0;
    v_margin /= 2;
    horizontalHeader()->setDefaultSectionSize(cell_size);
    verticalHeader()->setDefaultSectionSize(cell_size);
    horizontalHeader()->resizeSection(0, h_margin);
    horizontalHeader()->resizeSection(col_cnt + 1, h_margin);
    verticalHeader()->resizeSection(0, v_margin);
    verticalHeader()->resizeSection(row_cnt + 1, v_margin);
}
