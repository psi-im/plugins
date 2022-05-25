/*
 * deferredstanzasender.cpp - plugin
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

#ifndef DEFERREDSTANZASENDER_H
#define DEFERREDSTANZASENDER_H

#include "stanzasendinghost.h"
#include <QDomElement>
#include <QPair>
#include <QTimer>

class DefferedStanzaSender : public QObject {
    Q_OBJECT
public:
    DefferedStanzaSender(StanzaSendingHost *host, QObject *p = nullptr);

    void sendStanza(int account, const QDomElement &xml);
    void sendStanza(int account, const QString &xml);
    void sendMessage(int account, const QString &to, const QString &body, const QString &subject, const QString &type);
    QString uniqueId(int account) const;

private slots:
    void timeout();

private:
    StanzaSendingHost              *stanzaSender_;
    QTimer                         *timer_;
    typedef QPair<int, QDomElement> XmlStanzaItem;
    typedef QPair<int, QString>     StringStanzaItem;
    struct MessageItem {
        MessageItem(int acc, const QString &_to, const QString &_body, const QString &_subject, const QString &_type) :
            account(acc), to(_to), body(_body), subject(_subject), type(_type)
        {
        }
        MessageItem() { }

        int     account;
        QString to;
        QString body;
        QString subject;
        QString type;
    };
    enum ItemType { Xml, String, Message };
    struct Item {
        Item(ItemType t, XmlStanzaItem x) : type(t), xmlItem(x) { }
        Item(ItemType t, StringStanzaItem s) : type(t), stringItem(s) { }
        Item(ItemType t, const MessageItem &m) : type(t), messageItem(m) { }

        ItemType type;

        XmlStanzaItem    xmlItem;
        StringStanzaItem stringItem;
        MessageItem      messageItem;
    };

    QList<Item> items_;
};

#endif // DEFERREDSTANZASENDER_H
