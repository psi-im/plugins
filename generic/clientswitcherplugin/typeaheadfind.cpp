/*
 * typeaheadfind.cpp  - plugin
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

#include "typeaheadfind.h"

#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>

using namespace ClientSwitcher;

class TypeAheadFindBar::Private
{
public:
    void doFind(bool backward = false)    {
        QTextDocument::FindFlags options;
        if (caseSensitive)
            options |= QTextDocument::FindCaseSensitively;
        if (backward) {
            options |= QTextDocument::FindBackward;
            QTextCursor cursor = te->textCursor();
            cursor.setPosition(cursor.selectionStart());
            cursor.movePosition(QTextCursor::Left);
            te->setTextCursor(cursor);
        }
        if (find(text, options)) {
            le_find->setStyleSheet("");
        }
        else {
            le_find->setStyleSheet("QLineEdit { background: #ff6666; color: #ffffff }");
        }
    }

    bool find(const QString &str, QTextDocument::FindFlags options, QTextCursor::MoveOperation start = QTextCursor::NoMove)    {
        Q_UNUSED(str);
        if (start != QTextCursor::NoMove) {
            QTextCursor cursor = te->textCursor();
            cursor.movePosition(start);
            te->setTextCursor(cursor);
        }
        bool found = te->find(text, options);
        if (!found) {
            if (start == QTextCursor::NoMove)
                return find(text, options, options & QTextDocument::FindBackward ? QTextCursor::End : QTextCursor::Start);
            return false;
        }
        return true;
    }

    QString text;
    bool caseSensitive;
    QTextEdit *te;
    QLineEdit *le_find;
    QPushButton *but_next;
    QPushButton *but_prev;
    QPushButton *first_page, *next_page, *last_page, *prev_page;
    QCheckBox *cb_case;
};

TypeAheadFindBar::TypeAheadFindBar(IconFactoryAccessingHost *IcoHost, QTextEdit *textedit, const QString &title, QWidget *parent)
        : QToolBar(title, parent)
        , icoHost_(IcoHost)
{
    d = new Private();
    d->te = textedit;
    init();
}

void TypeAheadFindBar::init() {
    d->caseSensitive = false;
    d->text = "";
    addWidget(new QLabel(tr("Search: "), this));

    d->le_find = new QLineEdit(this);
    d->le_find->setMaximumWidth(128);
    connect(d->le_find, SIGNAL(textEdited(const QString &)), SLOT(textChanged(const QString &)));
    addWidget(d->le_find);

    d->but_prev = new QPushButton(this);
    d->but_prev->setFixedSize(25,25);
    d->but_prev->setIcon(icoHost_->getIcon("psi/arrowUp"));
    d->but_prev->setEnabled(false);
    connect(d->but_prev, SIGNAL(released()), SLOT(findPrevious()));
    addWidget(d->but_prev);

    d->but_next = new QPushButton(this);
    d->but_next->setFixedSize(25,25);
    d->but_next->setIcon(icoHost_->getIcon("psi/arrowDown"));
    d->but_next->setEnabled(false);
    connect(d->but_next, SIGNAL(released()), SLOT(findNext()));
    addWidget(d->but_next);

    d->cb_case = new QCheckBox(tr("&Case sensitive"), this);
    connect(d->cb_case, SIGNAL(clicked()), SLOT(caseToggled()));
    addWidget(d->cb_case);

    addSeparator();

    d->first_page = new QPushButton(this);
    d->first_page->setToolTip(tr("First page"));
    connect(d->first_page, SIGNAL(released()), SIGNAL(firstPage()));
    d->first_page->setFixedSize(25,25);
    d->first_page->setIcon(icoHost_->getIcon("psi/doubleBackArrow"));
    addWidget(d->first_page);

    d->prev_page = new QPushButton(this);
    d->prev_page->setToolTip(tr("Previous page"));
    connect(d->prev_page, SIGNAL(released()), SIGNAL(prevPage()));
    d->prev_page->setFixedSize(25,25);
    d->prev_page->setIcon(icoHost_->getIcon("psi/arrowLeft"));
    addWidget(d->prev_page);

    d->next_page = new QPushButton(this);
    d->next_page->setToolTip(tr("Next page"));
    connect(d->next_page, SIGNAL(released()), SIGNAL(nextPage()));
    d->next_page->setFixedSize(25,25);
    d->next_page->setIcon(icoHost_->getIcon("psi/arrowRight"));
    addWidget(d->next_page);

    d->last_page = new QPushButton(this);
    d->last_page->setToolTip(tr("Last page"));
    connect(d->last_page, SIGNAL(released()), SIGNAL(lastPage()));
    d->last_page->setFixedSize(25,25);
    d->last_page->setIcon(icoHost_->getIcon("psi/doubleNextArrow"));
    addWidget(d->last_page);
}

TypeAheadFindBar::~TypeAheadFindBar() {
    delete d;
    d = nullptr;
}

void TypeAheadFindBar::textChanged(const QString &str) {
    QTextCursor cursor = d->te->textCursor();
    if (str.isEmpty()) {
        d->but_next->setEnabled(false);
        d->but_prev->setEnabled(false);
        d->le_find->setStyleSheet("");
        cursor.clearSelection();
        d->te->setTextCursor(cursor);
    }
    else {
        d->but_next->setEnabled(true);
        d->but_prev->setEnabled(true);
        cursor.setPosition(cursor.selectionStart());
        d->te->setTextCursor(cursor);
        d->text = str;
        d->doFind();
    }
}

void TypeAheadFindBar::findNext() {
    d->doFind();
}

void TypeAheadFindBar::findPrevious() {
    d->doFind(true);
}

void TypeAheadFindBar::caseToggled() {
    d->caseSensitive = d->cb_case->checkState();
}
