/*
 * redirectplugin.cpp - plugin
 * Copyright (C) 2013  Sergey Ilinykh
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

#include <QDomElement>
#include <QDateTime>
#include <QTextDocument>

#include "redirectplugin.h"

#include "optionaccessinghost.h"
#include "stanzasendinghost.h"
#include "accountinfoaccessinghost.h"
#include "applicationinfoaccessinghost.h"
#include "contactinfoaccessinghost.h"


bool Redirector::enable() {
    if (psiOptions) {
        enabled = true;
        targetJid = psiOptions->getPluginOption("jid").toString();
    }
    return enabled;
}

bool Redirector::disable() {
    enabled = false;
    return true;
}

void Redirector::applyOptions() {
    if (!options_)
        return;

    targetJid = ui_.le_jid->text();
    psiOptions->setPluginOption("jid", targetJid);
}

void Redirector::restoreOptions() {
    if (!options_)
        return;

    targetJid = psiOptions->getPluginOption("jid").toString();
    ui_.le_jid->setText(targetJid);
}

QWidget* Redirector::options() {
    if (!enabled) {
        return 0;
    }
    options_ = new QWidget();
    ui_.setupUi(options_);

    restoreOptions();

    return options_;
}

bool Redirector::incomingStanza(int account, const QDomElement& stanza) {
    Q_UNUSED(account)

    if (!enabled || stanza.tagName() != "message") {
        return false;
    }
    int targetAccount = accInfoHost->findOnlineAccountForContact(targetJid);
    QDomNodeList bodies = stanza.elementsByTagName("body");
    if (targetAccount == -1 || bodies.count() == 0) {
        return false;
    }

    int contactId;
    QString from = stanza.attribute("from");

    QDomDocument doc;
    QDomElement e = doc.createElement("message");
    e.setAttribute("to", ui_.le_jid->text());
    e.setAttribute("type", "chat");
    // TODO id?
    contactId = contactIdMap.value(from);
    if (!contactId) {
        contactIdMap.insert(from, nextContactId);
        contactId = nextContactId++;
    }
    QDomElement body = doc.createElement("body");
    e.appendChild(body);
    body.appendChild(doc.createTextNode(QString("#%1 %2").arg(contactId).arg(bodies.at(0).toElement().text().toHtmlEscaped())));
    QDomElement forward = e.appendChild(doc.createElementNS("urn:xmpp:forward:0", "forwarded")).toElement();
    forward.appendChild(doc.createElementNS("urn:xmpp:delay", "delay")).toElement()
            .setAttribute("stamp", QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddThh:mm:ssZ"));

    forward.appendChild(doc.importNode(stanza, true));

    stanzaHost->sendStanza(targetAccount, e);

    return true;
}

bool Redirector::outgoingStanza(int /*account*/, QDomElement& /*xml*/) {
    return false;
}

QString Redirector::pluginInfo() {
    return tr("Author: ") +  "rion\n"
            + tr("Email: ") + "rion4ik@gmail.com\n\n"
            + trUtf8("Redirects all incoming messages to some jid and allows one to redirect messages back.");
}
