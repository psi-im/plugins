/*
 * watcheditem.cpp - plugin
 * Copyright (C) 2010  Evgeny Khryukin
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

#include "watcheditem.h"

WatchedItem::WatchedItem(QListWidget *parent)
    : QListWidgetItem(parent)
    , jid_("")
    , text_("")
    , sFile_("")
    , aUse_(false)
    , groupChat_(false)
{
}

WatchedItem::WatchedItem(const QString &jid, const QString &text, const QString &sFile, bool aUse, QListWidget *parent)
    : QListWidgetItem(parent)
    , jid_(jid)
    , text_(text)
    , sFile_(sFile)
    , aUse_(aUse)
    , groupChat_(false)
{
}

QString WatchedItem::settingsString() const
{
    QStringList l = QStringList() << jid_ << text_ << sFile_;
    l << (aUse_ ? "1" : "0");
    l << (groupChat_ ? "1" : "0");
    return l.join(splitStr);
}

void WatchedItem::setSettings(const QString &settings)
{
    QStringList l = settings.split(splitStr);
    if(!l.isEmpty())
        jid_ = l.takeFirst();
    if(!l.isEmpty())
        text_ = l.takeFirst();
    if(!l.isEmpty())
        sFile_ = l.takeFirst();
    if(!l.isEmpty())
        aUse_ = l.takeFirst().toInt();
    if(!l.isEmpty())
        groupChat_ = l.takeFirst().toInt();
}

WatchedItem* WatchedItem::copy()
{
    WatchedItem *wi = new WatchedItem();
    wi->setWatchedText(text_);
    wi->setJid(jid_);
    wi->setUse(aUse_);
    wi->setSFile(sFile_);
    wi->setText(text());
    wi->setGroupChat(groupChat_);
    return wi;
}
