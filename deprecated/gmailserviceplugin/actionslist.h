/*
 * actionslist.h - plugin
 * Copyright (C) 2010-2011 Evgeny Khryukin
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

#ifndef ACTIONSLIST_H
#define ACTIONSLIST_H

#include <QAction>
#include <QPointer>

class ActionsList : public QObject
{
    Q_OBJECT
public:
    ActionsList(QObject* p);
    ~ActionsList();
    QAction* newAction(QObject* p, int account, const QString& contact, QIcon ico);
    void updateActionsVisibility(int account, bool isVisible);
    void updateAction(int account, const QString& jid, bool isChecked);

signals:
    void changeNoSaveState(int account, QString jid, bool val);

private slots:
    void actionActivated(bool val);

private:
    typedef QList< QPointer<QAction> > AList;
    QHash<int, AList > list_;
};

#endif // ACTIONSLIST_H
