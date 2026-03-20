/*
 * pluginwindow.h - Battleship Game plugin
 * Copyright (C) 2014  Aleksey Andreev
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

#ifndef PLUGINWINDOW_H
#define PLUGINWINDOW_H

#include "gamemodel.h"
#include "ui_pluginwindow.h"

class PluginWindow : public QMainWindow {
    Q_OBJECT

public:
    PluginWindow(const QString &jid, QWidget *parent = nullptr);
    void        initBoard();
    void        setError();
    QStringList dataExchange(const QStringList &data);

private:
    QString stringStatus(bool short_) const;

private:
    Ui::PluginWindow ui;
    GameModel       *gm_;

private:
    void updateWidgets();

private slots:
    void updateStatus();
    void freezeShips();
    void newGame();

signals:
    void gameEvent(QString data);
};

#endif // PLUGINWINDOW_H
