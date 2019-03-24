/*
 * notescontroller.cpp - plugin
 * Copyright (C) 2011  Evgeny Khryukin
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "notescontroller.h"
#include "notes.h"

NotesController::NotesController(StorageNotesPlugin* plugin)
    : QObject(0)
    , plugin_(plugin)
{
}

NotesController::~NotesController()
{
    foreach(Notes* n, notesList_.values()) {
        delete n;
        n = 0;
    }
    notesList_.clear();
}

void NotesController::incomingNotes(int account, const QList<QDomElement>& notes)
{
    if(notesList_.contains(account)) {
        Notes* note = notesList_.value(account);
        if(note)
            note->incomingNotes(notes);
    }
}

void NotesController::start(int account)
{
    QPointer<Notes> note = 0;

    if(notesList_.contains(account)) {
        note = notesList_.value(account);
    }
    if(note) {
        note->load();
        note->raise();
    }
    else {
        note = new Notes(plugin_, account);
        connect(note, SIGNAL(notesDeleted(int)), this, SLOT(notesDeleted(int)));

        notesList_.insert(account, note);
        note->load();
        note->show();
    }
}

void NotesController::error(int account)
{
    if(notesList_.contains(account)) {
        Notes* note = notesList_.value(account);
        if(note)
            note->error();
    }
}

void NotesController::saved(int account)
{
    if(notesList_.contains(account)) {
        Notes* note = notesList_.value(account);
        if(note)
            note->saved();
    }
}

void NotesController::notesDeleted(int account)
{
    if(notesList_.contains(account)) {
        Notes* note = notesList_.value(account);
        note->deleteLater();
        notesList_.remove(account);
    }
}
