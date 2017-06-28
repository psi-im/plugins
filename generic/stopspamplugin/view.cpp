/*
 * view.cpp - plugin
 * Copyright (C) 2009-2010  Evgeny Khryukin
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

#include "view.h"

#include <QHeaderView>

void Viewer::init()
{
        setSelectionBehavior(QAbstractItemView::SelectRows);
        resizeColumnsToContents();
#ifdef HAVE_QT5
        horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
        horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#endif
        horizontalHeader()->setStretchLastSection(true);
        verticalHeader()->setDefaultAlignment( Qt::AlignHCenter );

        connect(this, SIGNAL(clicked(QModelIndex)), this, SLOT(itemClicked(QModelIndex)));
}

void Viewer::itemClicked(QModelIndex index)
{
	if(index.column() == 0)
		model()->setData(index, 3); //invert
}
