/*
 * model.cpp - plugin
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

#include "model.h"

Model::Model(const QStringList& watchedJids_, const QStringList& Sounds_, QObject* parent)
        : QAbstractTableModel(parent)
        , watchedJids(watchedJids_)
        , sounds(Sounds_)
{
	headers << tr("")
			<< tr("Watch for JIDs")
			<< tr("Sounds (if empty default sound will be used)")
			<< tr("")
			<< tr("");

	unselectAll();

	tmpWatchedJids_ = watchedJids;
	tmpSounds_ = sounds;
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
	else if(column == 1 || column == 2)
		flags |= Qt::ItemIsEditable;

	return flags;
}

int Model::columnCount(const QModelIndex & /*parent*/) const
{
	return headers.size();
}

int Model::rowCount(const QModelIndex &/* parent*/) const
{
	return tmpWatchedJids_.size();
}

QVariant Model::data(const QModelIndex & index, int role) const
{
	if(!index.isValid())
		return QVariant();

	int i = index.column();
	switch(i) {
	case(0):
                if (role == Qt::CheckStateRole) {
                        return selected.at(index.row()) ? 2:0;
                } else if (role == Qt::TextAlignmentRole) {
                        return (int)(Qt::AlignRight | Qt::AlignVCenter);
                } else if (role == Qt::DisplayRole)
                        return QVariant("");
             case(1):
                if (role == Qt::TextAlignmentRole) {
                        return (int)(Qt::AlignRight | Qt::AlignVCenter);
                } else if (role == Qt::DisplayRole)
			return QVariant(tmpWatchedJids_.at(index.row()));
             case(2):
                if (role == Qt::TextAlignmentRole) {
                        return (int)(Qt::AlignRight | Qt::AlignVCenter);
                } else if (role == Qt::DisplayRole)
                        return QVariant(tmpSounds_.at(index.row()));
             case(3):
		if (role == Qt::TextAlignmentRole) {
                        return (int)(Qt::AlignRight | Qt::AlignVCenter);
                } else if (role == Qt::DisplayRole)
			return QVariant();
             case(4):
		if (role == Qt::TextAlignmentRole) {
                        return (int)(Qt::AlignRight | Qt::AlignVCenter);
                } else if (role == Qt::DisplayRole)
			return QVariant();
        }
	return QVariant();
}

QString Model::jid(const QModelIndex & index) const
{
	if(!index.isValid())
		return QString();

	return watchedJids.at(index.row());
}

QString Model::soundFile(const QModelIndex & index) const
{
	if(!index.isValid())
		return QString();

	return sounds.at(index.row());
}

QString Model::tmpSoundFile(const QModelIndex & index) const
{
	if(!index.isValid())
		return QString();

	return tmpSounds_.at(index.row());
}

bool Model::setData(const QModelIndex & index, const QVariant & value, int role)
{
	if(!index.isValid() || role != Qt::EditRole)
		return false;

	int column = index.column();
	if(column == 0) {
		bool b = selected.at( index.row() );
		switch(value.toInt()) {
		case(0):
			selected.replace( index.row(), false);
			break;
		case(2):
			selected.replace( index.row(), true);
			break;
		case(3):
			selected.replace( index.row(), !b);
			break;
		}
	}
	else if(column == 1)
		tmpWatchedJids_.replace( index.row(), value.toString());

	else if(column == 2)
		tmpSounds_.replace( index.row(), value.toString());

	emit dataChanged(index, index);

	return true;
}

void Model::setStatusForJid(const QString& jid, const QString& status)
{
	statuses.insert(jid, status);
}

void Model::deleteSelected()
{
	emit layoutAboutToBeChanged ();
	QStringList tmpJids, tmpSounds;
	for(int i=0; i<watchedJids.size(); i++) {
		if(!selected.at(i)) {
			tmpJids.append(watchedJids.at(i));
			tmpSounds.append(sounds.at(i));
		}
	}

	tmpWatchedJids_.clear();
	tmpSounds_.clear();
	tmpWatchedJids_ = tmpJids;
	tmpSounds_ = tmpSounds;

	unselectAll();
}

void Model::reset()
{
	tmpWatchedJids_ = watchedJids;
	tmpSounds_ = sounds;
	unselectAll();
}

void Model::selectAll()
{
	selected.clear();

	foreach(QString jid, tmpWatchedJids_) {
		Q_UNUSED(jid);
		selected.append(true);
	}

	emit layoutChanged();
}

void Model::unselectAll()
{
	selected.clear();

	foreach(QString jid, watchedJids) {
		selected.append(false);
	}

	emit layoutChanged();
}

void Model::addRow(const QString& jid)
{
	beginInsertRows(QModelIndex(), tmpWatchedJids_.size(), tmpWatchedJids_.size());
	tmpWatchedJids_ << jid;
	tmpSounds_ << "";

	if(!jid.isEmpty()) {      //вызов происходит из меню контакта
		watchedJids.append(jid);
		sounds.append("");
	}

	selected.append(false);
	endInsertRows();
}

void Model::deleteRow(const QString& jid)
{
	int index = watchedJids.indexOf(QRegExp(jid, Qt::CaseInsensitive));
	if(index == -1)
		return;
	watchedJids.removeAt(index);
	sounds.removeAt(index);
	tmpWatchedJids_.removeAt(index);
	tmpSounds_.removeAt(index);
	selected.removeAt(index);

	emit layoutChanged();
}

void Model::apply()
{
	watchedJids =  tmpWatchedJids_;
	sounds = tmpSounds_;
}

QString Model::statusByJid(const QString& jid) const
{
	return statuses.value(jid, "offline");
}

QString Model::soundByJid(const QString& jid) const
{
	QString sound;
	int index = watchedJids.indexOf(QRegExp(jid, Qt::CaseInsensitive));
	if(index < sounds.size() && index != -1)
		sound = sounds.at( index );

	return sound;
}

int Model::indexByJid(const QString& jid) const
{
	return watchedJids.indexOf(QRegExp(jid, Qt::CaseInsensitive));
}

QStringList Model::getWatchedJids() const
{
	return watchedJids;
}

QStringList Model::getSounds() const
{
	return sounds;
}
