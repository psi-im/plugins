/*
 * storagenotesplugin.cpp - plugin
 * Copyright (C) 2010  Evgeny Khryukin
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

#include "storagenotesplugin.h"
#include "notescontroller.h"

#include <QDomElement>
#include <QIcon>
#include <QLabel>
#include <QVBoxLayout>

StorageNotesPlugin::StorageNotesPlugin() :
    stanzaSender(nullptr), iconHost(nullptr), accInfo(nullptr), popup(nullptr), enabled(false), controller_(nullptr)
{
}

QString StorageNotesPlugin::name() const { return "Storage Notes Plugin"; }

QString StorageNotesPlugin::shortName() const { return "storagenotes"; }

QString StorageNotesPlugin::version() const { return constVersion; }

bool StorageNotesPlugin::enable()
{
    enabled = true;

    QFile file(":/storagenotes/storagenotes.png");
    file.open(QIODevice::ReadOnly);
    QByteArray image = file.readAll();
    iconHost->addIcon("storagenotes/storagenotes", image);
    file.close();
    controller_ = new NotesController(this);

    return enabled;
}

bool StorageNotesPlugin::disable()
{
    delete controller_;
    controller_ = nullptr;
    enabled     = false;

    return true;
}

QWidget *StorageNotesPlugin::options()
{
    if (!enabled) {
        return nullptr;
    }
    QWidget *    optionsWid = new QWidget();
    QVBoxLayout *vbox       = new QVBoxLayout(optionsWid);

    QLabel *wikiLink = new QLabel(
        tr("<a href=\"https://psi-plus.com/wiki/en:plugins#storage_notes_plugin\">Wiki (Online)</a>"), optionsWid);
    wikiLink->setOpenExternalLinks(true);

    vbox->addWidget(wikiLink);
    vbox->addStretch();

    return optionsWid;
}

bool StorageNotesPlugin::incomingStanza(int account, const QDomElement &xml)
{
    if (!enabled)
        return false;

    if (xml.tagName() == "iq" && xml.attribute("id") == NOTES_ID) {
        if (xml.attribute("type") == "error") {
            controller_->error(account);
        } else if (xml.attribute("type") == "result") {
            QList<QDomElement> notes;
            QDomNodeList       noteList = xml.elementsByTagName("note");
            for (int i = 0; i < noteList.size(); i++)
                notes.append(noteList.at(i).toElement());

            if (!notes.isEmpty()) {
                controller_->incomingNotes(account, notes);
            } else {
                controller_->saved(account);
            }
        }
        return true;
    }
    return false;
}

bool StorageNotesPlugin::outgoingStanza(int /* account*/, QDomElement & /*xml*/) { return false; }

void StorageNotesPlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost *host) { accInfo = host; }

void StorageNotesPlugin::setIconFactoryAccessingHost(IconFactoryAccessingHost *host) { iconHost = host; }

void StorageNotesPlugin::setStanzaSendingHost(StanzaSendingHost *host) { stanzaSender = host; }

void StorageNotesPlugin::setPopupAccessingHost(PopupAccessingHost *host) { popup = host; }

void StorageNotesPlugin::start()
{
    if (!enabled)
        return;

    int acc = sender()->property("account").toInt();
    controller_->start(acc);
}

QList<QVariantHash> StorageNotesPlugin::getAccountMenuParam()
{
    QVariantHash hash;
    hash["icon"]    = QVariant(QString("storagenotes/storagenotes"));
    hash["name"]    = QVariant(tr("Storage Notes"));
    hash["reciver"] = QVariant::fromValue(qobject_cast<QObject *>(this));
    hash["slot"]    = QVariant(SLOT(start()));
    QList<QVariantHash> l;
    l.push_back(hash);
    return l;
}

QList<QVariantHash> StorageNotesPlugin::getContactMenuParam() { return QList<QVariantHash>(); }

QString StorageNotesPlugin::pluginInfo()
{
    return name() + "\n\n" + tr("Author: ") + "Dealer_WeARE\n" + tr("Email: ") + "wadealer@gmail.com\n\n"
        + tr("This plugin is an implementation of XEP-0049: Private XML Storage.\n"
             "The plugin is fully compatible with notes saved using Miranda IM.\n"
             "The plugin is designed to keep notes on the jabber server with the ability to access them from anywhere "
             "using Psi or Miranda IM.");
}

QPixmap StorageNotesPlugin::icon() const { return QPixmap(":/storagenotes/storagenotes.png"); }
