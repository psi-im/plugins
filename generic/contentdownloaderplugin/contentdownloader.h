/*
 * contentdownloader.h - plugin interface
 * Copyright (C) 2010  Ivan Romanov <drizt@land.ru>
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


#ifndef CONTENTDOWLOADER_H
#define CONTENTDOWLOADER_H

#include <QtCore>
#include <QtGui>
#include <QWidget>
#include "psiplugin.h"
#include "applicationinfoaccessor.h"
#include "applicationinfoaccessinghost.h"
#include "optionaccessor.h"
#include "optionaccessinghost.h"
#include "plugininfoprovider.h"
#include "cditemmodel.h"

class Form;

class ContentDownloader : public QObject, public PsiPlugin, public OptionAccessor, public ApplicationInfoAccessor , public PluginInfoProvider {
	Q_OBJECT
	Q_INTERFACES(PsiPlugin OptionAccessor ApplicationInfoAccessor PluginInfoProvider)
public:
	ContentDownloader();
	~ContentDownloader();

	// from PsiPlugin
	QString name() const;
	QString shortName() const;
	QString version() const;
	QWidget *options();
	bool enable();
	bool disable();
	void applyOptions();
	void restoreOptions();

	// from OptionAccessor
	void setOptionAccessingHost(OptionAccessingHost* host);
	void optionChanged(const QString& option);

	// from ApplicationInfoAccessor
	void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host);

	// from PluginInfoProvider
	QString pluginInfo();

private:
	bool enabled;
	OptionAccessingHost *psiOptions;
	ApplicationInfoAccessingHost *appInfoHost;
	QString texto;
	Form *form_;
};

#endif // CONTENTDOWLOADER_H
