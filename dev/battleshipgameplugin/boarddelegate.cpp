/*
 * boarddelegate.cpp - Battleship game plugin
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

#include "boarddelegate.h"

BoardDelegate::BoardDelegate(BoardModel *model, QObject *parent)
    : QItemDelegate(parent)
    , model_(model)
{
}

void BoardDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid())
        return;
    QPoint point(index.column(), index.row());
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    QRectF r(option.rect);
    int opos = model_->model2oppboard(point);
    int mpos = -1;
    if (opos == -1)
        mpos = model_->model2myboard(point);
    if (opos != -1 || mpos != -1)
    {
        GameBoard::CellStatus st;
        if (opos != -1)
            st = model_->gameModel()->oppBoard().cell(opos).status;
        else
            st = model_->gameModel()->myBoard().cell(mpos).status;
        if (st == GameBoard::CellOccupied || st == GameBoard::CellHit)
        {
            QRectF r2 = r.adjusted(1.0, 1.0, -1.0, -1.0);
            QPen pen(Qt::black);
            pen.setWidthF(2.0);
            pen.setJoinStyle(Qt::MiterJoin);
            painter->setPen(pen);
            painter->drawRect(r2);
            if (st == GameBoard::CellHit)
            {
                r2.adjust(1.0, 1.0, -1.0, -1.0);
                pen.setCapStyle(Qt::RoundCap);
                painter->drawLine(r2.topLeft(), r2.bottomRight());
                painter->drawLine(r2.topRight(), r2.bottomLeft());
            }
        }
        else
        {
            QRectF r2 = r.adjusted(0.5, 0.5, -0.5, -0.5);
            setGridPen(painter);
            painter->drawRect(r2);
            if (st == GameBoard::CellMiss || st == GameBoard::CellMargin)
            {
                qreal w = r.width() * 0.2;
                QPen pen(Qt::black);
                pen.setWidthF(w);
                pen.setCapStyle(Qt::RoundCap);
                painter->setPen(pen);
                painter->drawPoint(r.center());
            }
        }
    }
    else
    {
        // displaying coordinates
        if ((point.x() == 1 || point.x() == model_->columnCount() - 2)
                && point.y() >= 2 && point.y() < model_->rowCount() - 2)
        {
            // Numbers
            QString text = QString::number(point.y() - 1);
            painter->drawText(r, Qt::AlignCenter, text, 0);
            QRectF r2 = r.adjusted(0.5, 0.5, -0.5, -0.5);
            setGridPen(painter);
            if (point.x() == 1)
                painter->drawLine(r2.topRight(), r2.bottomRight());
            else
                painter->drawLine(r2.topLeft(), r2.bottomLeft());
        }
        else if ((point.y() == 1 || point.y() == model_->rowCount() - 2)
            && point.x() >= 2 && point.x() < model_->columnCount() - 2
            && (point.x() <= 11 || point.x() >= 15))
        {
            // letters
            static const QString letters("ABCDEFGHJK");
            QString text;
            if (point.x() <= 11)
                text = letters.mid(point.x() - 2, 1);
            else
                text = letters.mid(point.x() - 15, 1);
            painter->drawText(r, Qt::AlignCenter, text, 0);
            QRectF r2 = r.adjusted(0.5, 0.5, -0.5, -0.5);
            setGridPen(painter);
            if (point.y() == 1)
                painter->drawLine(r2.bottomLeft(), r2.bottomRight());
            else
                painter->drawLine(r2.topLeft(), r2.topRight());
        }
    }
    painter->restore();
}

void BoardDelegate::setGridPen(QPainter *painter)
{
    QPen pen(Qt::blue);
    pen.setWidthF(0.5);
    pen.setJoinStyle(Qt::MiterJoin);
    pen.setCapStyle(Qt::SquareCap);
    painter->setPen(pen);
}
