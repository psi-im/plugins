/*
 * gnupg.h - plugin main class
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

#ifndef ALERTWEIRDJID_H
#define ALERTWEIRDJID_H

#include <QtCore>
#include "psiplugin.h"
#include "applicationinfoaccessinghost.h"
#include "plugininfoprovider.h"

class Options;

class GnuPG : public QObject, public PsiPlugin, public PluginInfoProvider
{
	Q_OBJECT
	Q_INTERFACES(PsiPlugin PluginInfoProvider)

public:
	GnuPG();
	~GnuPG();

	// from PsiPlugin
	QString name() const { return "GnuPG Key Manager"; }
	QString shortName() const { return "gnupg"; }
	QString version() const { return "0.2.1"; }

	QWidget *options();
	bool enable();
	bool disable();
	void applyOptions();
	void restoreOptions();

	// from PluginInfoProvider
	QString pluginInfo();

private:
	bool _enabled;
	Options *_optionsForm;
};

#endif // ALERTWEIRDJID_H
