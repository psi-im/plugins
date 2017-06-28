/*
 * notes.h - plugin
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

#ifndef NOTES_H
#define NOTES_H

#include "ui_notes.h"
#include "tagsmodel.h"

#include <QKeyEvent>

class StorageNotesPlugin;
class QTimer;

class Notes : public QDialog
{
        Q_OBJECT
public:
	Notes(StorageNotesPlugin *storageNotes, int acc, QWidget *parent = 0);
	~Notes();
	void incomingNotes(const QList<QDomElement>& notes);
	void error();
	void saved();

private:
	QString replaceSymbols(const QString& str);

signals:
	void notesDeleted(int);

public slots:	
	void load();

private slots:
        void save();
        void add();
        void del();
        void edit();
	void addNote(const QDomElement& note);
	void noteEdited(const QDomElement& note, const QModelIndex& index);
	void selectTag();
	void updateTags();

protected:
        void closeEvent(QCloseEvent * event);
        void keyPressEvent(QKeyEvent *e);
	bool eventFilter(QObject *obj, QEvent *e);

private:
	Ui::Notes ui_;
	int account_;
	StorageNotesPlugin *storageNotes_;
	TagModel *tagModel_;
	NoteModel *noteModel_;
	ProxyModel *proxyModel_;
	QTimer* updateTagsTimer_;
	bool newNotes;
	bool waitForSave;
};


#endif // NOTES_H
