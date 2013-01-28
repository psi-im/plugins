/*
 * gnupg.cpp - plugin main class
 *
 * Copyright (C) 2013  Ivan Romanov <drizt@land.ru>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QRegExp>
#include <QDomElement>
#include <QMessageBox>
#include "options.h"
#include "gnupg.h"

Q_EXPORT_PLUGIN(GnuPG);

GnuPG::GnuPG()
	: _enabled(false)
	, _optionsForm(0)
{
}


GnuPG::~GnuPG()
{
}

QWidget *GnuPG::options()
{
	if (!_enabled) {
		return 0;
	}

	_optionsForm = new Options();
	return qobject_cast<QWidget*>(_optionsForm);
}

bool GnuPG::enable()
{
    _enabled = true;
	return _enabled;
}

bool GnuPG::disable()
{
	_enabled = false;
	return true;
}

void GnuPG::applyOptions()
{
}

void GnuPG::restoreOptions()
{
}
