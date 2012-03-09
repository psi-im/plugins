/*
 * tagsmodel.cpp - plugin
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

#include "tagsmodel.h"

static const QString allTags = QObject::tr("All Tags");

TagModel::TagModel(QObject *parent)
	: QAbstractItemModel(parent)
{
	stringList.clear();
	pIndex = createIndex(0, 0, -1);
}

int TagModel::rowCount(const QModelIndex &parent) const
{
	if(parent == QModelIndex())
		return 1;
	else if(parent == createAllTagsIndex())
		return stringList.count();

	return 0;
}

QVariant TagModel::headerData(int /*section*/, Qt::Orientation /*orientation*/, int /*role*/) const
{
	return QVariant();
}

QModelIndex TagModel::index(int row, int column, const QModelIndex& parent) const
{
	if(row > stringList.size() || column != 0)
		return QModelIndex();

	if(parent == QModelIndex()) {
		if(row == 0)
			return createAllTagsIndex();
		else
			return QModelIndex();
	}
	else if(parent == createAllTagsIndex())
		return createIndex(row, column, row);

	return QModelIndex();
}

QModelIndex TagModel::parent(const QModelIndex& index) const
{
	if(!index.isValid())
		return QModelIndex();
	if(index.internalId() == -1)
		return QModelIndex();
	if(index.row() == index.internalId())
		return createAllTagsIndex();
	return QModelIndex();
}


QVariant TagModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (role != Qt::DisplayRole)
		return QVariant();

	if(index.internalId() == -1)
		return allTags;

	if (index.row() >= stringList.size() || index.row() != index.internalId())
		return QVariant();

	return stringList.at(index.row());

}

void TagModel::addTag(const QString& tag_)
{
	const QString tag = tag_.toLower();
	if(stringList.contains(tag))
		return;
	beginInsertRows(createAllTagsIndex(), stringList.size(), stringList.size());
	stringList.append(tag);
	stringList.sort();
	endInsertRows();
}

void TagModel::removeTag(const QString& tag_)
{
	const QString tag = tag_.toLower();
	int i = stringList.indexOf(tag);
	if(i == -1)
		return;

	beginRemoveRows(QModelIndex(), i, i);
	stringList.removeAt(i);
	endRemoveRows();
}

void TagModel::clear()
{
	stringList.clear();
	reset();
}

QModelIndex TagModel::indexByTag(const QString& tag) const
{
	int row = stringList.indexOf(tag);
	if(row == -1)
		return QModelIndex();

	return index(row,0, createAllTagsIndex());
}

QModelIndex TagModel::createAllTagsIndex() const
{
	return pIndex;
}

QString TagModel::allTagsName()
{
	return allTags;
}




////--------NoteModel---------------------
NoteModel::NoteModel(QObject *parent) : QAbstractListModel(parent)
{  
}

int NoteModel::rowCount(const QModelIndex &/*parent*/) const
{
	return notesList.count();
}

QVariant NoteModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (index.row() >= notesList.size())
		return QVariant();

	if(role == Qt::DisplayRole){
		QDomElement note = notesList.at(index.row());
		QString textNote;
		QString tag = note.attribute("tags");
		QString text = note.firstChildElement("text").text();
		QString title = note.firstChildElement("title").text();
		if(!title.isEmpty())
			textNote += tr("Title: %1").arg(title);
		if(!tag.isEmpty())
			textNote += tr("\nTags: %1").arg(tag);
		if(!text.isEmpty())
			textNote += "\n"+text;

		return textNote.isEmpty() ? QVariant() : QVariant(textNote);
	}
	else if (role == NoteRole) {
		QDomElement note = notesList.at(index.row());
		return QVariant(note.firstChildElement("text").text());
	}
	else if(role == TagRole) {
		QDomElement note = notesList.at(index.row());
		return QVariant(note.attribute("tags"));
	}
	else if(role == TitleRole) {
		QDomElement note = notesList.at(index.row());
		return QVariant(note.firstChildElement("title").text());
	}
	else
		return QVariant();
}

void NoteModel::clear()
{
	notesList.clear();
	reset();
}

void NoteModel::editNote(const QDomElement& note, const QModelIndex &index)
{
	delNote(index);
	insertNote(note, index);
}

void NoteModel::addNote(const QDomElement& note)
{
	beginInsertRows(QModelIndex(), notesList.size(), notesList.size());
	notesList.append(note);
	endInsertRows();
}

void NoteModel::insertNote(const QDomElement& note, const QModelIndex &index)
{
	if (!index.isValid())
		return;

	beginInsertRows(QModelIndex(), index.row(), index.row());
	notesList.insert(index.row(), note);
	endInsertRows();
}

void NoteModel::delNote(const QModelIndex &index)
{
	if (!index.isValid())
		return;

	if (index.row() >= notesList.size())
		return;

	beginRemoveRows(QModelIndex(), index.row(), index.row());
	notesList.removeAt(index.row());
	endRemoveRows();
}

QList<QDomElement> NoteModel::getAllNotes() const
{
	return notesList;
}

QStringList NoteModel::getAllTags() const
{
	QStringList tagsList;
	foreach(const QDomElement& note, notesList) {
		QStringList tags = note.attribute("tags").split(" ", QString::SkipEmptyParts);
		tagsList += tags;
	}
	return tagsList;
}



/////-------------ProxyModel---------------

bool ProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &parent) const
{
	QModelIndex index = sourceModel()->index(sourceRow, 0, parent);
	QString filter = filterRegExp().pattern();
	if(allTags.contains(filter))
		return true;

	QStringList tags = index.data(NoteModel::TagRole).toString().split(" ");
	return  tags.contains(filter, Qt::CaseInsensitive);
}

