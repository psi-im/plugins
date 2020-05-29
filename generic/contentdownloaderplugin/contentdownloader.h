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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CONTENTDOWLOADER_H
#define CONTENTDOWLOADER_H

#include "applicationinfoaccessinghost.h"
#include "applicationinfoaccessor.h"
#include "cditemmodel.h"
#include "optionaccessinghost.h"
#include "optionaccessor.h"
#include "plugininfoprovider.h"
#include "psiplugin.h"
#include <QWidget>

class Form;

class ContentDownloader : public QObject,
                          public PsiPlugin,
                          public OptionAccessor,
                          public ApplicationInfoAccessor,
                          public PluginInfoProvider {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.psi-plus.ContentDownloader" FILE "psiplugin.json")
    Q_INTERFACES(PsiPlugin OptionAccessor ApplicationInfoAccessor PluginInfoProvider)
public:
    ContentDownloader();
    ~ContentDownloader();

    // from PsiPlugin
    QString  name() const;
    QWidget *options();
    bool     enable();
    bool     disable();
    void     applyOptions();
    void     restoreOptions();

    // from OptionAccessor
    void setOptionAccessingHost(OptionAccessingHost *host);
    void optionChanged(const QString &option);

    // from ApplicationInfoAccessor
    void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host);

    // from PluginInfoProvider
    QString pluginInfo();

private:
    bool                          enabled;
    OptionAccessingHost *         psiOptions;
    ApplicationInfoAccessingHost *appInfoHost;
    QString                       texto;
    Form *                        form_;
};

#endif // CONTENTDOWLOADER_H
