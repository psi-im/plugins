/*
 * boarddelegate.cpp - Gomoku Game plugin
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

#include <QStyleOptionViewItem>

#include "boarddelegate.h"
#include "common.h"

using namespace GomokuGame;

BoardPixmaps::BoardPixmaps(QObject *parent) :
    QObject(parent),
    width(-1), height(-1),
    w_cnt(1), h_cnt(1)
{
    boardPixmap = new QPixmap(":/gomokugameplugin/goban1");
}

BoardPixmaps::~BoardPixmaps()
{
    clearPix();
    delete boardPixmap;
}

void BoardPixmaps::clearPix()
{
    QList<QPixmap *> values = scaledPixmap.values();
    while (!values.isEmpty()) {
        delete values.takeLast();
    }
    scaledPixmap.clear();
}

QPixmap *BoardPixmaps::getBoardPixmap(int x, int y, double w, double h)
{
    if (w != width || h != height) {
        width = w;
        height = h;
        clearPix();
    }
    QPixmap *scPixmap = scaledPixmap.value(0, NULL);
    if (scPixmap == nullptr) {
        // Масштабирование картинки под целое количество единиц ширины и высоты
        scPixmap = new QPixmap();
        w_cnt = boardPixmap->width() / w;
        h_cnt = boardPixmap->height() / h;
        // Тут можно ограничить максимальное количество кусков
        //--
        *scPixmap = boardPixmap->scaled(QSize(w_cnt * w, h_cnt * h), Qt::IgnoreAspectRatio, Qt::FastTransformation);
        scaledPixmap[0] = scPixmap;
    }
    int curr_key = (x % w_cnt) * 100 + (y % h_cnt) + 1;
    QPixmap *scPixmap2 = scaledPixmap.value(curr_key, NULL);
    if (scPixmap2 == nullptr) {
        // Вырезаем необходимый кусок картинки
        scPixmap2 = new QPixmap();
        int xpixpos = (x % w_cnt) * w;
        int ypixpos = (y % h_cnt) * h;
        *scPixmap2 = scPixmap->copy(xpixpos, ypixpos, w, h);
        scaledPixmap[curr_key] = scPixmap2;
    }
    return scPixmap2;
}

// -----------------------------------------------------------------------------

BoardDelegate::BoardDelegate(BoardModel *model, QObject *parent) :
    QItemDelegate(parent),
    model_(model),
    skin(0),
    pixmaps(nullptr)
{
}

void BoardDelegate::setSkin(int skin_num)
{
    if (skin != skin_num) {
        skin = skin_num;
        if (skin == 0) {
            if (pixmaps != nullptr) {
                delete pixmaps;
                pixmaps = nullptr;
            }
        } else {
            if (pixmaps == nullptr) {
                pixmaps    = new BoardPixmaps(this);
            }
        }
    }
}

void BoardDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // Проверки
    if (!index.isValid())
        return;
    int row = index.row();
    if (row <= 0 || row >= model_->rowCount() - 1)
        return;
    int col = index.column();
    if (col <= 0 || col >= model_->columnCount() - 1)
        return;
    painter->save();
    QRectF rect(option.rect);
    // Отрисовка фона
    if (skin == 0) {
        QBrush fill_brush(QColor(220, 179, 92, 255), Qt::SolidPattern);
        painter->fillRect(rect, fill_brush);
    } else {
        QPixmap *pixmap = pixmaps->getBoardPixmap(col - 1, row - 1, rect.width(), rect.height());
        painter->drawPixmap(rect, *pixmap, pixmap->rect());
    }
    QBrush brush1(Qt::SolidPattern);
    int row_min = 2;
    int row_max = model_->rowCount() - 3;
    int col_min = 2;
    int col_max = model_->columnCount() - 3;
    if (row >= row_min && row <= row_max && col >= col_min && col <= col_max) {
        // Отрисовка центральных линий
        qreal x = rect.left() + rect.width() / 2.0;
        qreal y = rect.top() + rect.height() / 2.0;
        painter->setPen(Qt::darkGray);
        painter->drawLine(rect.left(), y - 1, rect.right(), y - 1);
        painter->drawLine(x - 1, rect.top(), x - 1, rect.bottom());
        painter->setPen(Qt::lightGray);
        painter->drawLine(rect.left(), y, rect.right(), y);
        painter->drawLine(x, rect.top(), x, rect.bottom());
        // Отрисовка разделителя
        if (row == row_min || col == col_min || row == row_max || col == col_max) {
            painter->setPen(Qt::black);
            if (row == row_min) {
                painter->drawLine(rect.topLeft(), rect.topRight());
            } else if (row == row_max) {
                QPointF p1 = rect.bottomLeft();
                p1.setY(p1.y() - 1.0);
                QPointF p2 = rect.bottomRight();
                p2.setY(p2.y() - 1.0);
                painter->drawLine(p1, p2);
            }
            if (col == col_min) {
                painter->drawLine(rect.topLeft(), rect.bottomLeft());
            } else if (col == col_max) {
                QPointF p1 = rect.topRight();
                p1.setX(p1.x() - 1);
                QPointF p2 = rect.bottomRight();
                p2.setX(p2.x() - 1);
                painter->drawLine(p1, p2);
            }
        }
        if (model_->selectX == col && model_->selectY == row) {
            brush1.setColor(QColor(0, 255, 0, 64));
            painter->fillRect(rect, brush1);
        }
        // Отрисовка если курсор мыши над клеткой
        if (option.state & QStyle::State_MouseOver) {
            brush1.setColor(QColor(0, 0, 0, 32));
            painter->fillRect(rect, brush1);
        }
        rect.setWidth(rect.width() - 1);
        rect.setHeight(rect.height() - 1);
        // Отрисовка если клетка выбрана
        if (option.state & QStyle::State_Selected) {
            QRectF rect2(rect);
            rect2.setLeft(rect2.left() + 1);
            rect2.setTop(rect2.top() + 1);
            rect2.setWidth(rect2.width() - 1);
            rect2.setHeight(rect2.height() - 1);
            painter->setPen(Qt::gray);
            painter->drawRect(rect2);
        }
        // Отрисовка элемента
        const GameElement *el = model_->getGameElement(col, row);
        if (el) {
            el->paint(painter, rect);
        }
    } else {
        // Рисуем координаты
        if ((row == 1 || row == model_->columnCount() - 2) && col >= 2 && col <= model_->columnCount() - 3) {
            // Буквы
            QString text = horHeaderString.at(col - 2);
            painter->drawText(rect, Qt::AlignCenter,text, nullptr);
        } else if ((col == 1 || model_->rowCount() - 2) && row >= 2 && row <= model_->rowCount() - 3) {
            // Цифры
            QString text = QString::number(row - 1);
            painter->drawText(rect, Qt::AlignCenter,text, nullptr);
        }
    }
    painter->restore();
}
