/*
 * view.h - plugin
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

#ifndef VIEW_H
#define VIEW_H

#include <QTableView>
#include <QKeyEvent>

class IconFactoryAccessingHost;

class Viewer : public QTableView {
    Q_OBJECT

public:
    Viewer(QWidget *parent = nullptr) : QTableView(parent) {};
    void init(IconFactoryAccessingHost *iconHost);
    void deleteSelected();

private:
    IconFactoryAccessingHost *iconHost_ = nullptr;

protected:
    void keyPressEvent(QKeyEvent *e);
    void contextMenuEvent( QContextMenuEvent * e );

private slots:
    void itemClicked(const QModelIndex& index);

signals:
    void getSound(const QModelIndex&);
    void checkSound(const QModelIndex&);

};

#endif // VIEW_H
