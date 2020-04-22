/*
 * invitedialog.h - plugin
 * Copyright (C) 2010-2011  Evgeny Khryukin
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

#ifndef INVITEDIALOG_H
#define INVITEDIALOG_H

#include "request.h"
#include "ui_invitationdialog.h"
#include "ui_invitedialog.h"
#include <QCloseEvent>

namespace Chess {

class InviteDialog : public QDialog {
    Q_OBJECT
public:
    InviteDialog(const Request &r, const QStringList &resources, QWidget *parent = nullptr);

private:
    Ui::InviteDialog ui_;
    QStringList      resources_;
    Request          r;

private slots:
    void buttonPressed();

signals:
    void play(const Request &r, const QString &, const QString &);
};

class InvitationDialog : public QDialog {
    Q_OBJECT
public:
    InvitationDialog(const QString &jid, QString color, QWidget *parent = nullptr);

private:
    Ui::InvitationDialog ui_;
    bool                 accepted;

private slots:
    void buttonPressed();

signals:
    void accept();
    void reject();

protected:
    void closeEvent(QCloseEvent *e);
};
}

#endif // INVITEDIALOG_H
