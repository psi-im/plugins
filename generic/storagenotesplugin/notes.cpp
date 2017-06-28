/*
 * notes.cpp - plugin
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

#include "notes.h"
#include "editnote.h"
#include "notesviewdelegate.h"
#include "storagenotesplugin.h"

#include <QMessageBox>


Notes::Notes(StorageNotesPlugin *storageNotes, int acc, QWidget *parent)
	: QDialog(parent, Qt::Window)
	, account_(acc)
	, storageNotes_(storageNotes)
	, tagModel_(new TagModel(this))
	, noteModel_(new NoteModel(this))
	, proxyModel_(new ProxyModel(this))
	, updateTagsTimer_(new QTimer(this))
	, newNotes(false)
	, waitForSave(false)

{
	setModal(false);
	ui_.setupUi(this);

	setWindowTitle(tr("Notebook") + " - " + storageNotes_->accInfo->getJid(account_));

	setWindowIcon(storageNotes_->iconHost->getIcon("storagenotes/storagenotes"));
	ui_.pb_add->setIcon(storageNotes_->iconHost->getIcon("psi/action_templates_edit"));
	ui_.pb_delete->setIcon(storageNotes_->iconHost->getIcon("psi/remove"));
	ui_.pb_edit->setIcon(storageNotes_->iconHost->getIcon("psi/options"));
	ui_.pb_load->setIcon(storageNotes_->iconHost->getIcon("psi/reload"));
	ui_.pb_save->setIcon(storageNotes_->iconHost->getIcon("psi/save"));
	ui_.pb_close->setIcon(storageNotes_->iconHost->getIcon("psi/cancel"));

	ui_.tv_tags->setModel(tagModel_);	
	proxyModel_->setSourceModel(noteModel_);
	ui_.lv_notes->setResizeMode(QListView::Adjust);
	ui_.lv_notes->setItemDelegate(new NotesViewDelegate(this));
	ui_.lv_notes->setModel(proxyModel_);

	connect(ui_.tv_tags, SIGNAL(clicked(QModelIndex)), this, SLOT(selectTag()));
	connect(ui_.lv_notes, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(edit()));
	connect(ui_.pb_save, SIGNAL(released()), this, SLOT(save()));
	connect(ui_.pb_close, SIGNAL(released()), this, SLOT(close()));
	connect(ui_.pb_load, SIGNAL(released()), this, SLOT(load()));
	connect(ui_.pb_add, SIGNAL(released()), this, SLOT(add()));
	connect(ui_.pb_delete, SIGNAL(released()), this, SLOT(del()));
	connect(ui_.pb_edit, SIGNAL(released()), this, SLOT(edit()));

	ui_.tv_tags->installEventFilter(this);

	updateTagsTimer_->setSingleShot(true);
	updateTagsTimer_->setInterval(100);
	connect(updateTagsTimer_, SIGNAL(timeout()), this, SLOT(updateTags()));
}

Notes::~Notes()
{
}

void Notes::closeEvent(QCloseEvent *e)
{
	if(newNotes) {
		int rez = QMessageBox::question(this, tr("Notebook"),
						tr("Some changes are not saved. Are you sure you want to quit?"),
						QMessageBox::Ok | QMessageBox::Cancel);
		if(rez == QMessageBox::Cancel) {
			e->ignore();
			return;
		}
	}

	emit notesDeleted(account_);
	e->ignore();
}

void Notes::keyPressEvent(QKeyEvent *e)
{
	if(e->key() == Qt::Key_Escape) {
		e->ignore();
		close();
	}
	else {
		QDialog::keyPressEvent(e);
		e->accept();
	}
}

bool Notes::eventFilter(QObject *obj, QEvent *e)
{
	if(obj == ui_.tv_tags && e->type() == QEvent::KeyPress)	{
		QTimer::singleShot(0, this, SLOT(selectTag()));
	}

	return QDialog::eventFilter(obj, e);
}

void Notes::save()
{
	QList<QDomElement> notesList = noteModel_->getAllNotes();
	QString notes;
	foreach(const QDomElement& note, notesList) {
		QString tag = note.attribute("tags");
		QString text = note.firstChildElement("text").text();
		QString title = note.firstChildElement("title").text();
		tag = replaceSymbols(tag);
		text = replaceSymbols(text);
		title = replaceSymbols(title);
		notes+=QString("<note tags=\"%1\"><title>%2</title><text>%3</text></note>")
				.arg(tag)
				.arg(title)
				.arg(text);
	}
	QString xml = QString("<iq type=\"set\" id=\"%2\"><query xmlns=\"jabber:iq:private\"><storage xmlns=\"http://miranda-im.org/storage#notes\">%1</storage></query></iq>")
			.arg(notes)
			.arg(NOTES_ID);

	storageNotes_->stanzaSender->sendStanza(account_, xml);

	newNotes = false;
	waitForSave = true;
}

void Notes::add()
{
	QString tag = ui_.tv_tags->currentIndex().data(Qt::DisplayRole).toString();
	if(tag == TagModel::allTagsName())
		tag = QString();

	EditNote *editNote = new EditNote(this, tag);
	connect(editNote, SIGNAL(newNote(QDomElement)), this, SLOT(addNote(QDomElement)));
	editNote->show();

	newNotes = true;
}

void Notes::del()
{    
	noteModel_->delNote(proxyModel_->mapToSource(ui_.lv_notes->currentIndex()));
	updateTagsTimer_->start();

	newNotes = true;
}

void Notes::updateTags()
{
	QStringList tags = noteModel_->getAllTags();
	QString currentTag = ui_.tv_tags->currentIndex().data(Qt::DisplayRole).toString();

	tagModel_->clear();

	foreach(const QString& tag, tags) {
		if(!tag.isEmpty())
			tagModel_->addTag(tag);
	}

	QModelIndex ind = tagModel_->indexByTag(currentTag);
	if(ind.isValid())
		ui_.tv_tags->setCurrentIndex(tagModel_->indexByTag(currentTag));
	else
		ui_.tv_tags->setCurrentIndex(tagModel_->index(0));
	selectTag();
	ui_.tv_tags->expandToDepth(2);
}

void Notes::edit()
{
	QModelIndex index = proxyModel_->mapToSource(ui_.lv_notes->currentIndex());
	if(!index.isValid())
		return;

	QString text = index.data(NoteModel::NoteRole).toString();
	QString title = index.data(NoteModel::TitleRole).toString();
	QString tags = index.data(NoteModel::TagRole).toString();

	EditNote *editNote = new EditNote( this, tags, title, text, index);
	connect(editNote, SIGNAL(editNote(QDomElement, QModelIndex)), this, SLOT(noteEdited(QDomElement, QModelIndex)));
	editNote->show();
}

void Notes::noteEdited(const QDomElement& note, const QModelIndex& index)
{
	noteModel_->editNote(note, index);
	updateTagsTimer_->start();

	newNotes = true;
}

void Notes::load()
{
	if(storageNotes_->accInfo->getStatus(account_) == "offline")
		return;

	if(newNotes) {
		int rez = QMessageBox::question(this, tr("Notebook"),
						tr("Some changes are not saved. Are you sure you want to continue?"),
						QMessageBox::Ok | QMessageBox::Cancel);
		if(rez == QMessageBox::Cancel) {
			return;
		}
	}

	tagModel_->clear();
	ui_.tv_tags->setCurrentIndex(tagModel_->index(0));
	selectTag();
	noteModel_->clear();
	QString str = QString("<iq type=\"get\" id=\"%1\"><query xmlns=\"jabber:iq:private\"><storage xmlns=\"%2\" /></query></iq>")
			.arg(NOTES_ID).arg("http://miranda-im.org/storage#notes");
	storageNotes_->stanzaSender->sendStanza(account_, str);

	newNotes = false;
}

void Notes::incomingNotes(const QList<QDomElement>& notes)
{
	foreach(const QDomElement& note, notes) {
		addNote(note);
	}
}

void Notes::addNote(const QDomElement& note)
{
	QString tag = note.attribute("tags");
	noteModel_->addNote(note);
	updateTagsTimer_->start();
}

void Notes::selectTag()
{    
	proxyModel_->setFilterFixedString(ui_.tv_tags->currentIndex().data(Qt::DisplayRole).toString());
}

void Notes::error()
{
	storageNotes_->popup->initPopup(tr("Error! Perhaps the function is not implemented on the server."),
					tr("Storage Notes Plugin"), "storagenotes/storagenotes", 7); // 7 - AlertHeadline
	close();
}

void Notes::saved()
{
	if(!waitForSave)
		return;

	storageNotes_->popup->initPopup(tr("Notes has been saved."),
					tr("Storage Notes Plugin"), "storagenotes/storagenotes", 7); // 7 - AlertHeadline

	waitForSave = false;
}

QString Notes::replaceSymbols(const QString& str)
{
	return storageNotes_->stanzaSender->escape(str);
}
