/*
 * jd_mainwin.h - plugin
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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef JD_MAINWIN_H
#define JD_MAINWIN_H

class JDModel;

#include "jd_commands.h"
#include "ui_jd_mainwin.h"

class JDMainWin : public QDialog
{
    Q_OBJECT
public:
    JDMainWin(const QString& name,const QString& jid, int acc, QWidget* p = 0);
    ~JDMainWin();

private slots:
    void incomingMessage(const QString& message, JDCommands::Command command);
    void refresh();
    void doSend();
    void outgoingMessage(const QString& message);
    void indexChanged(const QModelIndex& index);
    void indexContextMenu(const QModelIndex& index);
    void moveItem(const QString& oldPath, const QString& newPath);
    void clearLog();

private:
    void parse(QString message);
    void appendMessage(const QString& message, bool outgoing = true);
    void recursiveFind(const QString& dir);

protected:

private:
    Ui::JDMainWin ui_;
    JDModel* model_;
    JDCommands* commands_;
    QString currentDir_;
    bool refreshInProgres_;
    QString yourJid_;
};

#endif // JD_MAINWIN_H
