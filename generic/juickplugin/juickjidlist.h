/*
 * juickjidlist.h - plugin
 * Copyright (C) 2010 Khryukin Evgeny
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef JUICKJIDLIST_H
#define JUICKJIDLIST_H

#include <QDialog>

namespace Ui {
	class JuickJidDialog;
}

class JuickJidList : public QDialog
{
	Q_OBJECT
public:
	JuickJidList(const QStringList& jids, QWidget *p = 0);
	~JuickJidList();

signals:
	void listUpdated(const QStringList&);

private slots:
	void addPressed();
	void delPressed();
	void okPressed();
	void enableButtons();

private:
	Ui::JuickJidDialog *ui_;
	QStringList jidList_;
};

#endif // JUICKJIDLIST_H
