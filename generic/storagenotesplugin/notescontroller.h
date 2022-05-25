/*
 * notescontroller.h - plugin
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

#ifndef NOTESCONTROLLER_H
#define NOTESCONTROLLER_H

#include <QHash>
#include <QPointer>

class QDomElement;
class StorageNotesPlugin;
class Notes;

class NotesController : public QObject {
    Q_OBJECT
public:
    NotesController(StorageNotesPlugin *plugin);
    ~NotesController();

    void incomingNotes(int account, const QList<QDomElement> &notes);
    void start(int account);
    void error(int account);
    void saved(int account);

private slots:
    void notesDeleted(int account);

private:
    QHash<int, QPointer<Notes>> notesList_;
    StorageNotesPlugin         *plugin_;
};

#endif // NOTESCONTROLLER_H
