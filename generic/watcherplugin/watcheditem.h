/*
 * watcheditem.h - plugin
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef WATCHEDITEM_H
#define WATCHEDITEM_H

#include <QListWidgetItem>

const QString splitStr = "&split&";

class WatchedItem : public QListWidgetItem {
public:
    WatchedItem(QListWidget *parent = nullptr);
    WatchedItem(const QString &jid, const QString &text = QString(), const QString &sFile = QString(),
                bool aUse = false, QListWidget *parent = nullptr);
    QString settingsString() const;
    void    setSettings(const QString &settings);
    void    setJid(const QString &jid) { jid_ = jid; };
    void    setWatchedText(const QString &text) { text_ = text; };
    void    setSFile(const QString &sFile) { sFile_ = sFile; };
    void    setUse(bool use) { aUse_ = use; };
    void    setGroupChat(bool gc) { groupChat_ = gc; };
    QString jid() const { return jid_; };
    QString watchedText() const { return text_; };
    QString sFile() const { return sFile_; };
    bool    alwaysUse() const { return aUse_; };
    bool    groupChat() const { return groupChat_; };

    WatchedItem *copy();

private:
    QString jid_, text_, sFile_;
    bool    aUse_, groupChat_;
};

#endif // WATCHEDITEM_H
