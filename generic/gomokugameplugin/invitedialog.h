/*
 * invitedialog.h - plugin
 * Copyright (C) 2010  Evgeny Khryukin, Aleksey Andreev
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

#include "ui_invitationdialog.h"
#include "ui_invitedialog.h"

#include <QCloseEvent>
#include <QDialog>

namespace Ui {
    class InviteDialog;
}

namespace GomokuGame {
class InviteDialog : public QDialog {
    Q_OBJECT
public:
    InviteDialog(const int account,
                 const QString &jid,
                 const QStringList &resources,
                 QWidget *parent = nullptr);
    ~InviteDialog();

private:
    Ui::InviteDialog *ui;
    bool accepted;
    int myAcc;
    QString jid_;

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void acceptBlack();
    void acceptWhite();

signals:
    void acceptGame(int my_acc, QString jid, QString element);
    void rejectGame(int my_acc, QString jid);

};

class InvitationDialog : public QDialog {
    Q_OBJECT
public:
    InvitationDialog(const int account,
                     const QString &jid,
                     QString color,
                     const QString &id,
                     QWidget *parent = nullptr);

private:
    Ui::InvitationDialog ui_;
    bool accepted_;
    int account_;
    QString id_;

private slots:
    void buttonPressed();

signals:
    void accepted(int, QString);
    void rejected(int, QString);

protected:
    void closeEvent(QCloseEvent *e);
};
} // namespace GomokuGame

#endif // INVITEDIALOG_H
