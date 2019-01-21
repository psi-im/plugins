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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <QTimer>
#include <QEventLoop>
#include <QStringList>
#include <QDomElement>

#include "jd_commands.h"
#include "jabberdiskcontroller.h"

#define TIMER_INTERVAL 5000

JDCommands::JDCommands(int account, const QString &jid, QObject *p)
    : QObject(p)
    , account_(account)
    , jid_(jid)
    , jdc(JabberDiskController::instance())
    , timer_(new QTimer(this))
    , eventLoop_(new QEventLoop(this))
    , lastCommand_(CommandNoCommand)
{
    timer_->setInterval(TIMER_INTERVAL);
    connect(jdc, SIGNAL(stanza(int,QDomElement)), this, SLOT(incomingStanza(int,QDomElement)));
    connect(timer_, SIGNAL(timeout()), this, SLOT(timeOut()));
}

JDCommands::~JDCommands()
{
    timeOut();
}

void JDCommands::incomingStanza(int account, const QDomElement &xml)
{
    if(account != account_ || xml.attribute("from").split("/").at(0).toLower() != jid_)
        return;

    emit incomingMessage(xml.firstChildElement("body").text(), lastCommand_);
    lastCommand_ = CommandNoCommand;

    timeOut();
}

void JDCommands::get(const QString& file)
{
    sendStanza("get "+file, CommandGet);
}

void JDCommands::cd(const QString& dir)
{
    sendStanza("cd "+dir, CommandCd);
}

void JDCommands::help()
{
    sendStanza("help", CommandHelp);
}

void JDCommands::intro()
{
    sendStanza("intro", CommandIntro);
}

void JDCommands::hash(const QString& file)
{
    sendStanza("hash "+file, CommandHash);
}

void JDCommands::rm(const QString& path)
{
    sendStanza("rm "+path, CommandRm);
}

void JDCommands::du()
{
    sendStanza("du", CommandDu);
}

void JDCommands::mkDir(const QString& dir)
{
    sendStanza("mkdir "+dir, CommandMkDir);
}

void JDCommands::lang(const QString& l)
{
    sendStanza("lang "+l, CommandLang);
}

void JDCommands::pwd()
{
    sendStanza("pwd", CommandPwd);
}

void JDCommands::ls(const QString& dir)
{
    QString txt = "ls";
    if(!dir.isEmpty())
        txt += " "+dir;
    sendStanza(txt, CommandLs);
}

void JDCommands::send(const QString &toJid, const QString &file)
{
    sendStanza("send "+toJid+" "+file, CommandSend);
}

void JDCommands::mv(const QString& oldFile, const QString& newFile)
{
    sendStanza("mv "+oldFile+" "+newFile, CommandMv);
}

void JDCommands::link(const QString &file)
{
    sendStanza("link "+file, CommandLink);
}

void JDCommands::sendStanzaDirect(const QString &text)
{
    emit outgoingMessage(text);
    QString id;
    jdc->sendStanza(account_, jid_, text, &id);
}

void JDCommands::sendStanza(const QString &text, Command c)
{
    emit outgoingMessage(text);
    lastCommand_ = c;
    QString id;
    jdc->sendStanza(account_, jid_, text, &id);

    timer_->start();
    eventLoop_->exec();
}

void JDCommands::timeOut()
{
    if(timer_->isActive())
        timer_->stop();

    if(eventLoop_->isRunning())
        eventLoop_->quit();
}

bool JDCommands::isReady() const
{
    return !eventLoop_->isRunning();
}
