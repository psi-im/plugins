/*
 * invitedialog.h - plugin
 * Copyright (C) 2014  Aleksey Andreev
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * You can also redistribute and/or modify this program under the
 * terms of the Psi License, specified in the accompanied COPYING
 * file, as published by the Psi Project; either dated January 1st,
 * 2005, or (at your option) any later version.
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

#ifndef INVITEDIALOG_H
#define INVITEDIALOG_H

#include "ui_invitationdialog.h"
#include "ui_invitedialog.h"

#include <QCloseEvent>
#include <QDialog>

namespace Ui {
    class InviteDialog;
}

class InviteDialog : public QDialog {
    Q_OBJECT
public:
    InviteDialog(const QString &jid, const QStringList &resources, QWidget *parent = nullptr);
    ~InviteDialog();

private:
    Ui::InviteDialog *ui;
    bool accepted_;
    QString jid_;

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void acceptFirst();
    void acceptSecond();

signals:
    void acceptGame(QString jid, bool first);

};

class InvitationDialog : public QDialog {
    Q_OBJECT
public:
    InvitationDialog(const QString &jid, bool first, QWidget *parent = nullptr);

private:
    Ui::InvitationDialog ui_;

private slots:
    void okPressed();

};

#endif // INVITEDIALOG_H
