/*
 * screenshotplugin.cpp - plugin
 * Copyright (C) 2009-2011  Evgeny Khryukin
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

class QAction;

#include "applicationinfoaccessinghost.h"
#include "applicationinfoaccessor.h"
#include "iconfactoryaccessinghost.h"
#include "iconfactoryaccessor.h"
#include "menuaccessor.h"
#include "optionaccessinghost.h"
#include "optionaccessor.h"
#include "plugininfoprovider.h"
#include "psiplugin.h"
#include "shortcutaccessinghost.h"
#include "shortcutaccessor.h"

#include "controller.h"
#include "defines.h"
#include "options.h"
#include "optionswidget.h"
#include "screenshoticonset.h"

class ScreenshotPlugin : public QObject,
                         public PsiPlugin,
                         public OptionAccessor,
                         public ShortcutAccessor,
                         public PluginInfoProvider,
                         public IconFactoryAccessor,
                         public MenuAccessor,
                         public ApplicationInfoAccessor {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.psi-plus.ScreenshotPlugin" FILE "psiplugin.json")
    Q_INTERFACES(PsiPlugin OptionAccessor ShortcutAccessor PluginInfoProvider IconFactoryAccessor MenuAccessor
                     ApplicationInfoAccessor)

public:
    ScreenshotPlugin();

    virtual QString             name() const;
    virtual QWidget *           options();
    virtual bool                enable();
    virtual bool                disable();
    virtual void                setOptionAccessingHost(OptionAccessingHost *host);
    virtual void                optionChanged(const QString & /*option*/) {};
    virtual void                setShortcutAccessingHost(ShortcutAccessingHost *host);
    virtual void                setIconFactoryAccessingHost(IconFactoryAccessingHost *host);
    virtual void                setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host);
    virtual QList<QVariantHash> getAccountMenuParam();
    virtual QList<QVariantHash> getContactMenuParam();
    virtual QAction *           getContactAction(QObject *, int, const QString &) { return nullptr; };
    virtual QAction *           getAccountAction(QObject *, int) { return nullptr; };
    virtual void                setShortcuts();

    virtual void applyOptions();
    virtual void restoreOptions();

    virtual QString pluginInfo();
    virtual QPixmap icon() const;

private slots:
    void openImage();

private:
    void disconnectShortcut();

private:
    bool                          enabled_;
    OptionAccessingHost *         psiOptions;
    ShortcutAccessingHost *       psiShortcuts;
    IconFactoryAccessingHost *    icoHost;
    ApplicationInfoAccessingHost *appInfo;

    QPointer<OptionsWidget> optionsWid;
    Controller *            controller_;
};

ScreenshotPlugin::ScreenshotPlugin() :
    enabled_(false), psiOptions(nullptr), psiShortcuts(nullptr), icoHost(nullptr), appInfo(nullptr),
    controller_(nullptr)
{
}

QString ScreenshotPlugin::name() const { return constName; }

QWidget *ScreenshotPlugin::options()
{
    if (!enabled_) {
        return nullptr;
    }
    optionsWid = new OptionsWidget();

    restoreOptions();
    return optionsWid;
}

bool ScreenshotPlugin::enable()
{
    QFile file(":/screenshotplugin/screenshot");
    file.open(QIODevice::ReadOnly);
    QByteArray image = file.readAll();
    icoHost->addIcon("screenshotplugin/screenshot", image);
    file.close();

    Options::instance()->setPsiOptions(psiOptions);
    ScreenshotIconset::instance()->setIconHost(icoHost);

    controller_ = new Controller(appInfo);
    appInfo->getProxyFor(constName); // init proxy settings

    enabled_ = true;
    return enabled_;
}

bool ScreenshotPlugin::disable()
{
    disconnectShortcut();

    delete controller_;
    controller_ = nullptr;
    enabled_    = false;

    return true;
}

void ScreenshotPlugin::setOptionAccessingHost(OptionAccessingHost *host) { psiOptions = host; }

void ScreenshotPlugin::setShortcutAccessingHost(ShortcutAccessingHost *host) { psiShortcuts = host; }

void ScreenshotPlugin::setIconFactoryAccessingHost(IconFactoryAccessingHost *host) { icoHost = host; }

void ScreenshotPlugin::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host) { appInfo = host; }

void ScreenshotPlugin::setShortcuts()
{
    const QString shortCut = psiOptions->getPluginOption(constShortCut).toString();
    psiShortcuts->connectShortcut(QKeySequence::fromString(shortCut, QKeySequence::NativeText), controller_,
                                  SLOT(onShortCutActivated()));
}

void ScreenshotPlugin::disconnectShortcut()
{
    const QString shortCut = psiOptions->getPluginOption(constShortCut).toString();
    psiShortcuts->disconnectShortcut(QKeySequence::fromString(shortCut, QKeySequence::NativeText), controller_,
                                     SLOT(onShortCutActivated()));
}

QList<QVariantHash> ScreenshotPlugin::getAccountMenuParam()
{
    QVariantHash hash;
    hash["icon"]    = QVariant(QString("screenshotplugin/screenshot"));
    hash["name"]    = QVariant(tr("Upload Image"));
    hash["reciver"] = QVariant::fromValue(qobject_cast<QObject *>(this));
    hash["slot"]    = QVariant(SLOT(openImage()));
    QList<QVariantHash> l;
    l.push_back(hash);
    return l;
}

QList<QVariantHash> ScreenshotPlugin::getContactMenuParam() { return QList<QVariantHash>(); }

void ScreenshotPlugin::openImage()
{
    if (!enabled_)
        return;

    controller_->openImage();
}

void ScreenshotPlugin::applyOptions()
{
    optionsWid->applyOptions();

    disconnectShortcut();
    setShortcuts();
}

void ScreenshotPlugin::restoreOptions() { optionsWid->restoreOptions(); }

QString ScreenshotPlugin::pluginInfo()
{
    return tr("This plugin allows you to make screenshots and save them to your hard drive or upload them to an FTP "
              "or "
              "HTTP server.\n"
              "The plugin has the following settings:\n"
              "* Shortcut -- hotkey to make the screenshot (by default, Ctrl+Alt+P)\n"
              "* Format -- the file format in which the screenshot will be stored (default: .jpg)\n"
              "* File Name -- format of the filename (default: pic-yyyyMMdd-hhmmss, where yyyyMMdd=YYYYMMDD, and "
              "hhmmss "
              "are current date in the format yearmonthday-hourminutesecond)\n"
              "The address of FTP server is specified as ftp://ftp.domain.tld/path1/path2")
        + tr("\n\nSettings for authorization on some hostings can be found here: "
             "http://code.google.com/p/qscreenshot/wiki/Authorization");
}

QPixmap ScreenshotPlugin::icon() const { return QPixmap(":/screenshotplugin/screenshot"); }

#include "screenshotplugin.moc"
