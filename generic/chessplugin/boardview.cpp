/*
 * boardview.cpp - plugin
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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "boardview.h"
#include "boarddelegate.h"
#include "boardmodel.h"

#include <QHeaderView>
#include <QHelpEvent>

using namespace Chess;

BoardView::BoardView(QWidget *parent)
        :QTableView(parent)
{
	QHeaderView *hHeader = horizontalHeader();
	hHeader->setSectionResizeMode(QHeaderView::Fixed);
	hHeader->setSectionsMovable(false);
	hHeader->setSectionsClickable(false);
	hHeader->setDefaultAlignment( Qt::AlignHCenter );
	hHeader->setDefaultSectionSize(50);

	QHeaderView *vHeader = verticalHeader();
	vHeader->setSectionResizeMode(QHeaderView::Fixed);
	vHeader->setSectionsClickable(false);
	vHeader->setSectionsMovable(false);
	vHeader->setDefaultAlignment( Qt::AlignVCenter );
	vHeader->setDefaultSectionSize(50);

	setSelectionMode(QAbstractItemView::SingleSelection);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	setItemDelegate(new BoardDelegate(this));

	setStyleSheet("QHeaderView::section {background-color: #ffffe7; border: 1px solid #74440e; color: black;  }"
		      "QTableCornerButton::section { background-color: #ffffe7; border: 1px solid #74440e; color: black;  }"
		      "QToolTip { background-color: #ffeeaf; padding: 2px; border: 1px solid #74440e; }");
}

void BoardView::mousePressEvent(QMouseEvent *e)
{
	QModelIndex oldIndex = currentIndex();
	BoardModel *model_ = (BoardModel*)model();
	if(!model_->myMove || model_->waitForFigure || model_->gameState_) {
		e->ignore();
		return;
	}
	QTableView::mousePressEvent(e);
	e->accept();
	QModelIndex newIndex = currentIndex();
	if(model_->gameType_ == Figure::BlackPlayer) {
		newIndex = model_->invert(newIndex);
	}
	if(!model_->isYourFigure(newIndex))
		setCurrentIndex(oldIndex);
}

void BoardView::mouseReleaseEvent(QMouseEvent *e)
{
	QModelIndex oldIndex = currentIndex();
	BoardModel *model_ = (BoardModel*)model();
	if(!model_->myMove || model_->waitForFigure || model_->gameState_) {
		e->ignore();
		return;
	}
	QTableView::mousePressEvent(e);
	e->accept();
	QModelIndex newIndex = currentIndex();
	if(model_->gameType_ == Figure::BlackPlayer) {
		oldIndex = model_->invert(oldIndex);
		newIndex = model_->invert(newIndex);
	}

	if(!model_->isYourFigure(newIndex)) {
		if(!model_->moveRequested(oldIndex, newIndex)) {
			if(model_->gameType_ == Figure::BlackPlayer)
				setCurrentIndex(model_->invert(oldIndex));
			else
				setCurrentIndex(oldIndex);
		}
		else {
			if(model_->gameType_ == Figure::BlackPlayer)
				setCurrentIndex(model_->invert(newIndex));
			else
				setCurrentIndex(newIndex);
		}
	}
}

void BoardView::mouseMoveEvent(QMouseEvent *e)
{
	e->ignore();
}

void BoardView::keyPressEvent(QKeyEvent *e)
{
	e->ignore();
}

bool BoardView::event(QEvent *e)
{
	if(e->type() == QEvent::ToolTip) {
		QPoint p = ((QHelpEvent *)e)->pos();
		p.setX(p.x() - verticalHeader()->width());
		p.setY(p.y() - horizontalHeader()->height());
		QModelIndex i = indexAt(p);
		if(i.isValid()) {
			BoardModel *model_ = (BoardModel*)model();
			setToolTip(QString("%1%2").arg(model_->headerData(i.column(), Qt::Horizontal).toString(),
						       model_->headerData(i.row(), Qt::Vertical).toString()));
		}
		else
			setToolTip("");
	}
	return QTableView::event(e);
}
