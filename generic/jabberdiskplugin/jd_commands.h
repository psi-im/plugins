/*
 * jd_commands.h - plugin
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

#ifndef JD_COMMANDS_H
#define JD_COMMANDS_H

#include <QObject>
class QEventLoop;
class QTimer;
class QDomElement;

class JabberDiskController;

class JDCommands : public QObject
{
    Q_OBJECT
public:
    JDCommands(int account = -1, const QString& jid = QString(), QObject* p = nullptr);
    virtual ~JDCommands();

    enum Command {
        CommandNoCommand,
        CommandGet,
        CommandCd,
        CommandHelp,
        CommandIntro,
        CommandHash,
        CommandRm,
        CommandDu,
        CommandMkDir,
        CommandLang,
        CommandPwd,
        CommandLs,
        CommandSend,
        CommandMv,
        CommandLink
    };

    void get(const QString& file);
    void cd(const QString& dir);
    void help();
    void intro();
    void hash(const QString& file);
    void rm(const QString& path);
    void du();
    void mkDir(const QString& dir);
    void lang(const QString& l);
    void pwd();
    void ls(const QString& dir = QString());
    void send(const QString& toJid, const QString& file);
    void mv(const QString& oldFile, const QString& newFile);
    void link(const QString& file);

    void sendStanzaDirect(const QString& text);
    bool isReady() const;

signals:
    void incomingMessage(const QString&, JDCommands::Command);
    void outgoingMessage(const QString&);

private slots:
    void incomingStanza(int account, const QDomElement& xml);
    void timeOut();

private:
    void sendStanza(const QString& text, Command c);

private:
    int account_;
    QString jid_;
    JabberDiskController* jdc;
    QTimer* timer_;
    QEventLoop* eventLoop_;
    Command lastCommand_;
};

#endif // JD_COMMANDS_H
