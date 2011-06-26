/*
 * tagsmodel.h - plugin
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

#ifndef TAGSMODEL_H
#define TAGSMODEL_H

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QDomElement>
#include <QStringList>

class TagModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	TagModel(QObject *parent = 0);
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &/*parent*/ = QModelIndex()) const { return 1; };
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	QModelIndex index(int row, int column = 0, const QModelIndex& parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex& index) const;
	QVariant data(const QModelIndex &index, int role) const;
	void addTag(const QString& tag);
	void removeTag(const QString& tag);
	void clear();
	QModelIndex indexByTag(const QString& tag) const;

private:
	QModelIndex createAllTagsIndex() const;

	QStringList stringList;
	QModelIndex pIndex;
};



class NoteModel : public QAbstractListModel
{
	Q_OBJECT

public:

	enum RoleType {
		NoteRole = Qt::DisplayRole + 1,
		TagRole = Qt::DisplayRole + 2,
		TitleRole = Qt::DisplayRole + 3
	};

	NoteModel(QObject *parent = 0);
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role) const;
	void clear();
	void addNote(const QDomElement& note);
	void delNote(const QModelIndex &index);
	void insertNote(const QDomElement& note, const QModelIndex &index);
	void editNote(const QDomElement& note, const QModelIndex &index);
	QList<QDomElement> getAllNotes() const;
	QStringList getAllTags() const;

private:
	QList<QDomElement> notesList;

};

class ProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

public:
	ProxyModel(QObject *parent = 0) : QSortFilterProxyModel(parent) {};
	bool filterAcceptsRow(int sourceRow, const QModelIndex &parent) const;
};
#endif // TAGSMODEL_H
