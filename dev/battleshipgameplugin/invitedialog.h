/*
* invitedialog.h - Battleship Game plugin
* Copyright (C) 2014  Aleksey Andreev
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

#ifndef INVITEDIALOG_H
#define INVITEDIALOG_H

#include <QDialog>
#include <QCloseEvent>

#include "ui_invitedialog.h"
#include "ui_invitationdialog.h"

namespace Ui {
    class InvateDialog;
}

class InviteDialog : public QDialog {
	Q_OBJECT
public:
	InviteDialog(const QString &jid, const QStringList &resources, QWidget *parent = 0);
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

class InvitationDialog : public QDialog
{
	Q_OBJECT
public:
	InvitationDialog(const QString &jid, bool first, QWidget *parent = 0);

private:
	Ui::InvitationDialog ui_;

private slots:
	void okPressed();

};

#endif // INVITEDIALOG_H
