/*
 * cleanerplugin.h - plugin
 * Copyright (C) 2009-2010  Khryukin Evgeny
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

#ifndef CLEANERPLUGIN_H
#define CLEANERPLUGIN_H

#include <QWidget>
#include "psiplugin.h"
#include "applicationinfoaccessor.h"
#include "applicationinfoaccessinghost.h"
#include "iconfactoryaccessor.h"
#include "iconfactoryaccessinghost.h"
#include "optionaccessor.h"
#include "optionaccessinghost.h"
#include "plugininfoprovider.h"

class CleanerMainWindow;

class CleanerPlugin : public QObject, public PsiPlugin, public ApplicationInfoAccessor,
		    public IconFactoryAccessor, public OptionAccessor, public PluginInfoProvider
{
        Q_OBJECT
	Q_INTERFACES(PsiPlugin ApplicationInfoAccessor IconFactoryAccessor OptionAccessor PluginInfoProvider)
public:
        CleanerPlugin();
        virtual QString name() const;
        virtual QString shortName() const;
        virtual QString version() const;
        virtual QWidget* options();
        virtual bool enable();
        virtual bool disable();

        virtual void applyOptions() {};
        virtual void restoreOptions() {};
        virtual void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host);
        virtual void setIconFactoryAccessingHost(IconFactoryAccessingHost* host);
        virtual void setOptionAccessingHost(OptionAccessingHost* host);
        virtual void optionChanged(const QString& ) {};
	virtual QString pluginInfo();

private:
        ApplicationInfoAccessingHost *appInfo;
        IconFactoryAccessingHost* iconHost;
        OptionAccessingHost* psiOptions;
        bool enabled;
	QPointer<CleanerMainWindow> cln;
        friend class CleanerMainWindow;
        int height, width;

private slots:
        void start();
        void deleteCln();

    };

#endif // CLEANERPLUGIN_H
