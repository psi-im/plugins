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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef PLUGINWINDOW_H
#define PLUGINWINDOW_H

#include "ui_pluginwindow.h"
#include "gamemodel.h"

class PluginWindow : public QMainWindow
{
	Q_OBJECT

public:
	PluginWindow(const QString &jid, QWidget *parent = 0);
	void initBoard();
	void setError();
	QStringList dataExchange(const QStringList &data);

private:
	QString stringStatus(bool short_) const;

private:
	Ui::PluginWindow ui;
	GameModel *gm_;

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
