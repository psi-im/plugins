/*
 * jd_view.h - plugin
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

#ifndef JD_VIEW_H
#define JD_VIEW_H

#include <QTreeView>

class JDView : public QTreeView
{
    Q_OBJECT
public:
    JDView(QWidget *p = 0);
    ~JDView();

signals:
    void contextMenu(const QModelIndex&);
    void newIndex(const QModelIndex&);

protected:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);
    void mousePressEvent(QMouseEvent *e);
    //void dragEnterEvent(QDragEnterEvent *event);
    //void dropEvent(QDropEvent *event);
};

#endif // JD_VIEW_H
