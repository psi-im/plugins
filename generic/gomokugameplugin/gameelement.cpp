/*
 * gameelement.cpp - Gomoku Game plugin
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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "gameelement.h"

GameElement::GameElement(ElementType type, int x, int y)
	: type_(type)
	, posX(x)
	, posY(y)
{
	++GameElement::usesCnt;
}

GameElement::~GameElement()
{
	--GameElement::usesCnt;
	if (!GameElement::usesCnt) {
		if (GameElement::blackstonePixmap) {
			delete GameElement::blackstonePixmap;
			GameElement::blackstonePixmap = NULL;
		}
		if (GameElement::whitestonePixmap) {
			delete GameElement::whitestonePixmap;
			GameElement::whitestonePixmap = NULL;
		}
	}
}

int GameElement::x() const
{
	return posX;
}

int GameElement::y() const
{
	return posY;
}

GameElement::ElementType GameElement::type() const
{
	return type_;
}

/*void GameElement::setType(ElementType tp)
{
	type_ = tp;
}*/

void GameElement::paint(QPainter *painter, const QRectF &rect)
{
	if (type_ == TypeNone)
		return;
	painter->save();
	painter->setRenderHint(QPainter::Antialiasing, true);
	painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
	QPixmap *pixmap;
	if (type_ == TypeBlack) {
		pixmap = getBlackstonePixmap();
	} else {
		pixmap = getWhitestonePixmap();
	}
	if (pixmap) {
		painter->drawPixmap(rect, *pixmap, pixmap->rect());
	}
	painter->restore();
}

int GameElement::usesCnt = 0;

QPixmap *GameElement::blackstonePixmap = NULL;

QPixmap *GameElement::getBlackstonePixmap()
{
	if (!GameElement::blackstonePixmap) {
		GameElement::blackstonePixmap = new QPixmap(":/gomokugameplugin/black-stone");
	}
	return GameElement::blackstonePixmap;
}

QPixmap *GameElement::whitestonePixmap = NULL;

QPixmap *GameElement::getWhitestonePixmap()
{
	if (!GameElement::whitestonePixmap) {
		GameElement::whitestonePixmap = new QPixmap(":/gomokugameplugin/white-stone");
	}
	return GameElement::whitestonePixmap;
}
