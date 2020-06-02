/*
 * jabberdiskplugin.cpp - plugin
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

#include <QAction>
#include <QDomElement>

#include "jabberdiskcontroller.h"
#include "jabberdiskplugin.h"

static const QString constJids_ = "jids";

JabberDiskPlugin::JabberDiskPlugin() :
    enabled(false), psiOptions(nullptr), jids_({ "disk.jabbim.cz" }), iconHost(nullptr)
{
}

QString JabberDiskPlugin::name() const { return "Jabber Disk Plugin"; }

bool JabberDiskPlugin::enable()
{
    enabled = true;
    jids_   = psiOptions->getPluginOption(constJids_, jids_).toStringList();

    return enabled;
}

bool JabberDiskPlugin::disable()
{
    JabberDiskController::instance()->reset();
    enabled = false;
    return true;
}

void JabberDiskPlugin::applyOptions()
{
    if (!options_)
        return;

    jids_.clear();
    for (int i = 0; i < ui_.lw_jids->count(); i++) {
        jids_.append(ui_.lw_jids->item(i)->text());
    }
    psiOptions->setPluginOption(constJids_, jids_);
}

void JabberDiskPlugin::restoreOptions()
{
    if (!options_)
        return;

    ui_.lw_jids->addItems(jids_);
}

QWidget *JabberDiskPlugin::options()
{
    if (!enabled) {
        return nullptr;
    }
    options_ = new QWidget();
    ui_.setupUi(options_);
    ui_.cb_hack->setVisible(false);

    restoreOptions();

    connect(ui_.pb_add, &QPushButton::clicked, this, &JabberDiskPlugin::addJid);
    connect(ui_.pb_delete, &QPushButton::clicked, this, &JabberDiskPlugin::removeJid);

    return options_;
}

//этот слот нужен для активации кнопки "Применить"
void JabberDiskPlugin::hack() { ui_.cb_hack->toggle(); }

bool JabberDiskPlugin::incomingStanza(int account, const QDomElement &xml)
{
    if (!enabled)
        return false;

    if (xml.tagName() == "message" && !xml.firstChildElement("body").isNull()) {
        const QString from = xml.attribute("from");
        bool          find = false;
        for (const QString &jid : jids_) {
            if (from.contains(jid, Qt::CaseInsensitive)) {
                find = true;
                break;
            }
        }
        if (find) {
            return JabberDiskController::instance()->incomingStanza(account, xml);
        }
    }

    return false;
}

void JabberDiskPlugin::addJid()
{
    if (!options_)
        return;

    const QString &txt = ui_.le_addJid->text();
    if (!txt.isEmpty()) {
        ui_.lw_jids->addItem(txt);
        hack();
    }
}

void JabberDiskPlugin::removeJid()
{
    if (!options_)
        return;

    QListWidgetItem *i = ui_.lw_jids->currentItem();
    ui_.lw_jids->removeItemWidget(i);
    delete i;

    hack();
}

bool JabberDiskPlugin::outgoingStanza(int /* account*/, QDomElement & /* xml*/) { return false; }

void JabberDiskPlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost *host)
{
    JabberDiskController::instance()->setAccountInfoAccessingHost(host);
}

void JabberDiskPlugin::setIconFactoryAccessingHost(IconFactoryAccessingHost *host)
{
    iconHost = host;
    // JabberDiskController::instance()->setIconFactoryAccessingHost(host);
}

void JabberDiskPlugin::setStanzaSendingHost(StanzaSendingHost *host)
{
    JabberDiskController::instance()->setStanzaSendingHost(host);
}

void JabberDiskPlugin::setOptionAccessingHost(OptionAccessingHost *host) { psiOptions = host; }

// void JabberDiskPlugin::setPopupAccessingHost(PopupAccessingHost* host)
//{
//    popup = host;
//}

QList<QVariantHash> JabberDiskPlugin::getAccountMenuParam() { return QList<QVariantHash>(); }

QList<QVariantHash> JabberDiskPlugin::getContactMenuParam() { return QList<QVariantHash>(); }

QAction *JabberDiskPlugin::getContactAction(QObject *p, int acc, const QString &jid)
{
    for (const QString &j : jids_) {
        if (jid.contains(j)) {
            QAction *act = new QAction(iconHost->getIcon("psi/save"), tr("Jabber Disk"), p);
            act->setProperty("account", acc);
            act->setProperty("jid", jid.toLower().split("/").at(0));
            connect(act, &QAction::triggered, JabberDiskController::instance(), &JabberDiskController::initSession);
            return act;
        }
    }

    return nullptr;
}

QString JabberDiskPlugin::pluginInfo()
{
    return tr("Treat some jids as services implementing Jabber Disk protocol and handle your files with them.");
}
