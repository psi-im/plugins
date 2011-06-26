/*
 * iconset.h - plugin
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

#ifndef ICONSET_H
#define ICONSET_H

#include <QIcon>

//for Psi plugin only
class IconFactoryAccessingHost;

class Iconset : public QObject
{
	Q_OBJECT
public:
	static Iconset* instance();
	static void reset();
	~Iconset();

	QIcon getIcon(const QString& name);

	//for Psi plugin only
	void setIconHost(IconFactoryAccessingHost* _icoHost);

private:
	Iconset();
	static Iconset* instance_;

	//for Psi plugin only
	IconFactoryAccessingHost* icoHost;
};

#endif // ICONSET_H
