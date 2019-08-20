/*
 * yandexnarod.h - plugin
 * Copyright (C) 2008-2009  Alexander Kazarin <boiler@co.ru>
 * Copyright (C) 2011  Evgeny Khryukin
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

#ifndef YANDEXNAROD_H
#define YANDEXNAROD_H

#include "applicationinfoaccessinghost.h"
#include "applicationinfoaccessor.h"
#include "iconfactoryaccessinghost.h"
#include "iconfactoryaccessor.h"
#include "menuaccessor.h"
#include "optionaccessinghost.h"
#include "optionaccessor.h"
#include "plugininfoprovider.h"
#include "popupaccessinghost.h"
#include "popupaccessor.h"
#include "psiplugin.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"

class QAction;
class uploadDialog;
class yandexnarodManage;
class yandexnarodSettings;

class yandexnarodPlugin:
        public ApplicationInfoAccessor,
        public IconFactoryAccessor,
        public MenuAccessor,
        public OptionAccessor,
        public PluginInfoProvider,
        public PopupAccessor,
        public PsiPlugin,
        public QObject,
        public StanzaSender {
    Q_OBJECT
    Q_INTERFACES(PsiPlugin OptionAccessor MenuAccessor IconFactoryAccessor StanzaSender
             PluginInfoProvider ApplicationInfoAccessor PopupAccessor)

public:
    yandexnarodPlugin();
    virtual QString name() const;
    virtual QString shortName() const;
    virtual QString version() const;
    virtual QWidget* options();
    virtual bool enable();
    virtual bool disable();
    virtual void setOptionAccessingHost(OptionAccessingHost* host);
    virtual void setIconFactoryAccessingHost(IconFactoryAccessingHost* host);
    virtual void setStanzaSendingHost(StanzaSendingHost* host);
    virtual void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host);
    virtual void setPopupAccessingHost(PopupAccessingHost* host);
    virtual void optionChanged(const QString& /*option*/) {};
    virtual void applyOptions();
    virtual void restoreOptions();
    virtual QList < QVariantHash > getAccountMenuParam();
    virtual QList < QVariantHash > getContactMenuParam();
    virtual QAction* getContactAction(QObject* /*parent*/, int /*account*/, const QString& /*contact*/) { return 0; };
    virtual QAction* getAccountAction(QObject* /*parent*/, int /*account*/) { return 0; };
    virtual QString pluginInfo();
    virtual QPixmap icon() const;

private slots:
    void manage_clicked();
    void on_btnTest_clicked();
    void actionStart();
    void onFileURL(const QString& url);

private:
    void showPopup(int account, const QString& jid, const QString& text);

private:
    OptionAccessingHost* psiOptions;
    IconFactoryAccessingHost* psiIcons;
    StanzaSendingHost* stanzaSender;
    ApplicationInfoAccessingHost* appInfo;
    PopupAccessingHost* popup;
    bool enabled;
    QString currentJid;
    int currentAccount;
    int popupId;

    QPointer<uploadDialog> uploadwidget;
    QPointer<yandexnarodSettings> settingswidget;
    QPointer<yandexnarodManage> manageDialog;
    QFileInfo fi;
};

#endif // YANDEXNAROD_H
