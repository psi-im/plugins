/*
 * notesviewdelegate.cpp - plugin
 * Copyright (C) 2010  Khryukin Evgeny
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

#include "notesviewdelegate.h"
#include "tagsmodel.h"

NotesViewDelegate::~NotesViewDelegate()
{
}

QSize NotesViewDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	if(index.isValid())  {
		QSize size = QItemDelegate::sizeHint(option, index);
		size.setWidth(size.width()/2);
		return size;
	}

	return QSize(0, 0);
}

void NotesViewDelegate::paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
	const QRect rect = option.rect;
	const QString text = index.data(NoteModel::NoteRole).toString();
	const QString title = index.data(NoteModel::TitleRole).toString();
	const QString tags = index.data(NoteModel::TagRole).toString();
	painter->save();

	QPalette palette = option.palette;
	QColor c = (option.state & QStyle::State_Selected) ?
		   palette.color(QPalette::Highlight) : palette.color(QPalette::Base);

	painter->fillRect(rect, c);

	QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                                  ? QPalette::Normal : QPalette::Disabled;


	if (option.state & QStyle::State_Selected) {
                painter->setPen(palette.color(cg, QPalette::HighlightedText));
	}
	else {
                painter->setPen(palette.color(cg, QPalette::Text));
	}
	QRect r(rect);

	const QFontMetrics fm = option.fontMetrics;
	QFont font = option.font;

	if(!title.isEmpty()) {
		r.setHeight(fm.height());
		font.setBold(true);
		painter->setFont(font);
		painter->drawText(r, Qt::AlignLeft, title);
		r.moveTo(r.bottomLeft());
	}

	if(!tags.isEmpty()) {		
		r.setHeight(fm.height());
		font.setBold(false);
		font.setItalic(true);
		font.setUnderline(true);
		painter->setFont(font);
		painter->drawText(r, Qt::AlignLeft, tags);
		r.moveTo(r.bottomLeft());
	}

	if(!title.isEmpty() || !tags.isEmpty()) {
		r.setBottom(rect.bottom());
	}

	font.setBold(false);
	font.setItalic(false);
	font.setUnderline(false);
	painter->setFont(font);
	painter->drawText(r, Qt::AlignLeft, text);

	painter->drawLine(rect.topRight(), rect.topLeft());
	painter->drawLine(rect.bottomRight(), rect.bottomLeft());

	painter->restore();
}
