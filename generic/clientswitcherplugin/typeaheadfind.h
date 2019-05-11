/*
 * typeaheadfind.h - plugin
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef TYPEAHEADFIND_H
#define TYPEAHEADFIND_H

#include <QToolBar>
#include <QTextEdit>

#include "iconfactoryaccessinghost.h"

namespace ClientSwitcher {

class TypeAheadFindBar : public QToolBar
{
    Q_OBJECT
public:
    TypeAheadFindBar(IconFactoryAccessingHost *IcoHost, QTextEdit *textedit, const QString &title, QWidget *parent = nullptr);

    ~TypeAheadFindBar();
    void init();

signals:
    void firstPage();
    void lastPage();
    void nextPage();
    void prevPage();

private slots:
    void textChanged(const QString &);
    void findNext();
    void findPrevious();
    void caseToggled();

private:
    class Private;
    Private *d;
    IconFactoryAccessingHost *icoHost_;
};

}
#endif
