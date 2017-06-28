/*
 * delegate.cpp - plugin
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

#include "delegate.h"
#include "iconfactoryaccessinghost.h"

#include <QLineEdit>

QSize IconDelegate::sizeHint(const QStyleOptionViewItem& /*option*/, const QModelIndex& index) const
{
	if(index.isValid())	 {
		return QSize (18,18);
	}

	return QSize(0, 0);
}

void IconDelegate::paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
	QRect rect = option.rect;

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

	QPixmap pix;
	if(index.column() == 3) {
		pix = iconHost_->getIcon("psi/browse").pixmap(QSize(16,16));
	} else if(index.column() == 4) {
		pix = iconHost_->getIcon("psi/play").pixmap(QSize(16,16));
	}

	QRect r(rect);
	r.translate(4,5);
	r.setSize(pix.size());
	painter->drawPixmap(r, pix);

	painter->restore();
}


//-----------------------------------
//--------LineEditDelegate-----------
//-----------------------------------
QWidget * LineEditDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex &/*index*/) const
{
	QLineEdit *editor = new QLineEdit(parent);
	return editor;
}

void LineEditDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	QString value = index.model()->data(index, Qt::DisplayRole).toString();
	QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
	lineEdit->setText(value);
}

void LineEditDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
	QString value = lineEdit->text();
	model->setData(index, value, Qt::EditRole);
}
