/*
 * boardmodel.cpp - Battleship game plugin
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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "boardmodel.h"

#define MODEL_ROW_COUNT 2 + 10 + 2
#define MODEL_COL_COUNT 2 + 10 + 3 + 10 + 2

BoardModel::BoardModel(QObject *parent)
	: QAbstractTableModel(parent)
	, gameModel_(NULL)
{
}

BoardModel::~BoardModel()
{
}

void BoardModel::init(GameModel *gm)
{
	gameModel_ = gm;
	//
#ifdef HAVE_QT5
	QAbstractTableModel::beginResetModel();
	QAbstractTableModel::endResetModel();
#else
	QAbstractTableModel::reset();
#endif
	connect(gameModel_, SIGNAL(myBoardUpdated(int,int,int,int)), this, SLOT(updateMyBoard(int,int,int,int)));
	connect(gameModel_, SIGNAL(oppBoardUpdated(int,int,int,int)), this, SLOT(updateOppBoard(int,int,int,int)));
}

Qt::ItemFlags BoardModel::flags(const QModelIndex & index) const
{
	Qt::ItemFlags fl = Qt::NoItemFlags | Qt::ItemIsEnabled;
	int row = index.row();
	if (row < 2 || row >= MODEL_ROW_COUNT - 2)
		return fl;
	int col = index.column();
	if (col < 2 || col >= MODEL_COL_COUNT - 2 || (col > 11 && col < 15))
		return fl;
	return (fl | Qt::ItemIsSelectable);
}
QVariant BoardModel::headerData(int /*section*/, Qt::Orientation /*orientation*/, int /*role*/) const
{
	return QVariant();
}

QVariant BoardModel::data(const QModelIndex &/*index*/, int /*role*/) const
{
	return QVariant();
}

int BoardModel::rowCount(const QModelIndex &/*parent*/) const
{
	return MODEL_ROW_COUNT;
}

int BoardModel::columnCount(const QModelIndex &/*parent*/) const
{
	return MODEL_COL_COUNT;
}

void BoardModel::updateMyBoard(int x, int y, int width, int height)
{
	QRect r(x, y, width, height);
	QPoint p1 = myboard2model(r.topLeft());
	QPoint p2 = myboard2model(r.bottomRight());
	emit dataChanged(index(p1.y(), p1.x()), index(p2.y(), p2.x()));
}

void BoardModel::updateOppBoard(int x, int y, int width, int height)
{
	QRect r(x, y, width, height);
	QPoint p1 = oppboard2model(r.topLeft());
	QPoint p2 = oppboard2model(r.bottomRight());
	emit dataChanged(index(p1.y(), p1.x()), index(p2.y(), p2.x()));
}

int BoardModel::model2oppboard(const QPoint &p)
{
	int col = p.x() - 15;
	if (col >= 0 && col < 10)
	{
		int row = p.y() - 2;
		if (row >= 0 && row < 10)
			return row * 10 + col;
	}
	return -1;
}

int BoardModel::model2myboard(const QPoint &p)
{
	int col = p.x() - 2;
	if (col >= 0 && col < 10)
	{
		int row = p.y() - 2;
		if (row >= 0 && row < 10)
			return row * 10 + col;
	}
	return -1;
}

QPoint BoardModel::myboard2model(const QPoint &p) const
{
	return QPoint(p.x() + 2, p.y() + 2);
}

QPoint BoardModel::oppboard2model(const QPoint &p) const
{
	return QPoint(p.x() + 2 + 10 + 3, p.y() + 2);
}
