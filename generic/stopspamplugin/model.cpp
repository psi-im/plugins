/*
 * model.cpp - plugin
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

#include "model.h"

Model::Model(QStringList Jids_, QVariantList Sounds_, QObject* parent)
        : QAbstractTableModel(parent)
        , Jids(Jids_)
{
	headers << tr("Enable/Disable") << tr("JID (or part of JID)");

	tmpJids_ = Jids;
	for(int i = Sounds_.size(); i > 0;) {
		bool b = Sounds_.at(--i).toBool();
		if(b)
			selected << Jids.at(i);
	}
}

QVariant Model::headerData ( int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole) {
                if (orientation == Qt::Horizontal) {
                        return headers.at(section);
                } else {
                        return section+1;
                }
        } else
		return QVariant();
}

Qt::ItemFlags Model::flags ( const QModelIndex & index ) const
{
	Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

	int column = index.column();

	if(column == 0)
		flags |= Qt::ItemIsUserCheckable;
	else if(column == 1)
		flags |= Qt::ItemIsEditable;

	return flags;
}

int Model::columnCount(const QModelIndex & /*parent*/) const
{
	return headers.size();
}

int Model::rowCount(const QModelIndex &/* parent*/) const
{
	return tmpJids_.size();
}

QVariant Model::data(const QModelIndex & index, int role) const
{
	if(!index.isValid())
		return QVariant();

	int i = index.column();
	switch(i) {
	case(0):
                if (role == Qt::CheckStateRole)
                        return selected.contains(tmpJids_.at(index.row())) ? 2:0;
                else if (role == Qt::TextAlignmentRole)
                        return (int)(Qt::AlignRight | Qt::AlignVCenter);
                else if (role == Qt::DisplayRole)
                        return QVariant("");
	case(1):
                if (role == Qt::TextAlignmentRole)
                        return (int)(Qt::AlignRight | Qt::AlignVCenter);
                else if (role == Qt::DisplayRole)
			return QVariant(tmpJids_.at(index.row()));

        }
	return QVariant();
}

QString Model::jid(const QModelIndex & index) const
{
	if(!index.isValid())
		return QString();

	return Jids.at(index.row());
}

bool Model::setData(const QModelIndex & index, const QVariant & value, int role)
{
	if(!index.isValid() || role != Qt::EditRole)
		return false;

	int column = index.column();
	if(column == 0) {
		switch(value.toInt()) {
		case(0):
			selected.remove(tmpJids_.at(index.row()));
			break;
		case(2):
			selected << tmpJids_.at(index.row());
			break;
		case(3):
			if( selected.contains(tmpJids_.at(index.row())) )
				selected.remove(tmpJids_.at(index.row()));
			else
				selected << tmpJids_.at(index.row());
			break;
		}
	}
	else if(column == 1)
		tmpJids_.replace(index.row(), value.toString());

	emit dataChanged(index, index);

	return true;
}

void Model::reset()
{
	tmpJids_ = Jids;
}

void Model::addRow()
{    
	beginInsertRows(QModelIndex(), tmpJids_.size(), tmpJids_.size());
	tmpJids_.append("");
	endInsertRows();
}

void Model::deleteRow(int row)
{
	if(tmpJids_.isEmpty() || tmpJids_.size() <= row || row < 0)
		return;

	QString jid_ = tmpJids_.takeAt(row);
	if(selected.contains(jid_))
		selected.remove(jid_);

	emit layoutChanged();
}

void Model::apply()
{
	Jids =  tmpJids_;
}

int Model::indexByJid(const QString& jid) const
{
	return Jids.indexOf(jid);
}

QStringList Model::getJids() const
{
	return Jids;
}

QVariantList Model::enableFor() const
{
	QVariantList list;
	foreach(QString jid, Jids) {
		list.append(selected.contains(jid));
	}
	return list;
}
