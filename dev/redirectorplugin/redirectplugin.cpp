/*
 * redirectplugin.cpp - plugin
 * Copyright (C) 2013  Il'inykh Sergey
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

#include <QDomElement>
#include <QDateTime>

#include "psiplugin.h"
#include "optionaccessor.h"
#include "optionaccessinghost.h"
#include "stanzafilter.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"
#include "accountinfoaccessor.h"
#include "accountinfoaccessinghost.h"
#include "applicationinfoaccessor.h"
#include "applicationinfoaccessinghost.h"
#include "popupaccessor.h"
#include "popupaccessinghost.h"
#include "iconfactoryaccessor.h"
#include "iconfactoryaccessinghost.h"
#include "plugininfoprovider.h"
#include "contactinfoaccessinghost.h"
#include "contactinfoaccessor.h"

#include "ui_options.h"

#define cVer "0.0.1"

class Redirector: public QObject, public PsiPlugin, public OptionAccessor, public StanzaSender,  public StanzaFilter,
public AccountInfoAccessor, public ApplicationInfoAccessor,
public PluginInfoProvider, public ContactInfoAccessor
{
	Q_OBJECT
	Q_INTERFACES(PsiPlugin OptionAccessor StanzaSender StanzaFilter AccountInfoAccessor ApplicationInfoAccessor
				 PluginInfoProvider ContactInfoAccessor)

public:
	inline Redirector() : enabled(false)
	  , psiOptions(0)
	  , stanzaHost(0)
	  , accInfoHost(0)
	  , appInfoHost(0)
	  , contactInfo(0) {}
	QString name() const { return "Redirect Plugin"; }
	QString shortName() const { return "redirect"; }
	QString version() const { return cVer; }
	//PsiPlugin::Priority priority() {return PriorityNormal;}
	QWidget* options();
	bool enable();
	bool disable();
	void applyOptions();
	void restoreOptions();
	void setOptionAccessingHost(OptionAccessingHost* host) { psiOptions = host; }
	void optionChanged(const QString& ) {}
	void setStanzaSendingHost(StanzaSendingHost *host) { stanzaHost = host; }
	bool incomingStanza(int account, const QDomElement& xml);
	bool outgoingStanza(int account, QDomElement& xml);
	void setAccountInfoAccessingHost(AccountInfoAccessingHost* host) { accInfoHost = host; }
	void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host) { appInfoHost = host; }
	void setContactInfoAccessingHost(ContactInfoAccessingHost* host) { contactInfo = host; }
	QString pluginInfo();

private slots:

private:
	QString targetJid;
	QHash<QString, int> contactIdMap;
	int nextContactId;
	int targetAccount;
	QWidget *options_;

	bool enabled;
	OptionAccessingHost* psiOptions;
	StanzaSendingHost* stanzaHost;
	AccountInfoAccessingHost *accInfoHost;
	ApplicationInfoAccessingHost *appInfoHost;
	ContactInfoAccessingHost* contactInfo;

	Ui::Options ui_;
};

Q_EXPORT_PLUGIN(Redirector);


bool Redirector::enable() {
	if (psiOptions) {
		enabled = true;
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
	targetAccount = contactInfo->findOnlineAccountFor(targetJid);
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
	if (!enabled || stanza.tagName() != "message") {
		return false;
	}

	// redirect only messages atm

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
	e.appendChild(doc.createElement("body").appendChild(
					  doc.createTextNode(QString("#%1").arg(contactId))));
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
			+ trUtf8("Redirects all incoming messages to some jid and allows to redirect messages back.");
}

#include "redirectplugin.moc"
