/*
 * optionswidget.h - plugin
 * Copyright (C) 2011  Khryukin Evgeny
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
#ifndef OPTIONSWIDGET_H
#define OPTIONSWIDGET_H

#include "ui_optionswidget.h"

class OptionsWidget : public QWidget
{
	Q_OBJECT
public:
	OptionsWidget(QWidget* p = 0);

	void applyOptions();
	void restoreOptions();

private slots:
	void addServer();
	void delServer();
	void editServer();
	void addNewServer(const QString&);
	void applyButtonActivate();
	void requstNewShortcut();
	void onNewShortcut(const QKeySequence&);

private:
	QString shortCut;
	QString format;
	QString fileName;
	QStringList servers;
	int defaultAction;
	Ui::OptionsWidget ui_;
};

#endif // OPTIONSWIDGET_H
