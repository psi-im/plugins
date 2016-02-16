/*
 * Copyright (C) 2016 Khryukin Evgeny
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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <QDomElement>
#include <QWidget>
#include <QVariant>
#include <QFile>
#include <QDataStream>
//#include <QTextStream>

#include "enummessagesplugin.h"

#include "optionaccessinghost.h"
#include "activetabaccessinghost.h"
#include "applicationinfoaccessinghost.h"
#include "psiaccountcontrollinghost.h"

#include "defines.h"


static const char* propAcc = "em_account";
static const char* propJid  = "em_jid";
static const QString emIdName = "psi_em_id";
static const QString htmlimNS = "http://www.w3.org/1999/xhtml";
static const QString xhtmlProtoNS = "http://jabber.org/protocol/xhtml-im";



EnumMessagesPlugin::EnumMessagesPlugin()
	: enabled(false)
	, _psiOptions(0)
	, _activeTab(0)
	, _applicationInfo(0)
	, _accContrller(0)

{
}

QString EnumMessagesPlugin::name() const
{
	return constPluginName;
}

QString EnumMessagesPlugin::shortName() const
{
	return "enummessages";
}

QString EnumMessagesPlugin::version() const
{
	return constVersion;
}

QWidget* EnumMessagesPlugin::options()
{
	return 0;
}

bool EnumMessagesPlugin::enable()
{
	enabled = true;
	QFile f(_applicationInfo->appCurrentProfileDir(ApplicationInfoAccessingHost::DataLocation) + QString(constEnumsFileName));
	if(f.exists() && f.open(QFile::ReadOnly)) {
		QDataStream s(&f);
		s >>_enumsIncomming;
	}

	return true;
}

bool EnumMessagesPlugin::disable()
{
	enabled = false;
	QFile f(_applicationInfo->appCurrentProfileDir(ApplicationInfoAccessingHost::DataLocation) + QString(constEnumsFileName));
	if(f.open(QFile::WriteOnly | QFile::Truncate)) {
		QDataStream s(&f);
		s << _enumsIncomming;
	}
	return true;
}

void EnumMessagesPlugin::applyOptions()
{
}

void EnumMessagesPlugin::restoreOptions()
{
}

QPixmap EnumMessagesPlugin::icon() const
{
	return QPixmap(":/icons/em.png");
}

void EnumMessagesPlugin::setOptionAccessingHost(OptionAccessingHost* host)
{
	_psiOptions = host;
}

void EnumMessagesPlugin::setActiveTabAccessingHost(ActiveTabAccessingHost* host)
{
	_activeTab = host;
}

void EnumMessagesPlugin::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host)
{
	_applicationInfo = host;
}

void EnumMessagesPlugin::setPsiAccountControllingHost(PsiAccountControllingHost *host)
{
	_accContrller = host;
}

bool EnumMessagesPlugin::incomingStanza(int account, const QDomElement& stanza)
{
	if(!enabled)
		return false;

	if (stanza.tagName() == "message" ) {
		QString type = stanza.attribute("type");

		if(type != "chat")
			return false;

		if(stanza.firstChildElement("body").isNull())
			return false;

		if (!stanza.hasAttribute(emIdName))
			return false;

		const QString jid(stanza.attribute("from").split('/').first());

		quint16 num = stanza.attribute(emIdName,"1").toUShort();

		quint16 myNum = 0;
		JidEnums jids;
		if (_enumsIncomming.contains(account)) {
			jids = _enumsIncomming.value(account);

			if(jids.contains(jid)) {
				myNum = jids.value(jid);
			}
		}

		if (num > myNum+1) {
			QString missed;
			while (num > myNum+1) {
				missed += QString("%1 ").arg(numToFormatedStr(myNum+1));
				++myNum;
			}
			_accContrller->appendSysMsg(account, jid, tr("Missed messages: %1").arg(missed));
		}


		jids.insert(jid, num);
		_enumsIncomming.insert(account, jids);

		QDomDocument doc = stanza.ownerDocument();
		QDomElement& nonConst = const_cast<QDomElement&>(stanza);
		addMessageNum(&doc, &nonConst, num, QColor(Qt::red));
	}

	return false;
}

bool EnumMessagesPlugin::outgoingStanza(int account, QDomElement &stanza)
{
	if(!enabled)
		return false;

	if (stanza.tagName() == "message" ) {
		QString type = stanza.attribute("type");

		if(type != "chat")
			return false;

		if(stanza.firstChildElement("body").isNull())
			return false;

		const QString jid(stanza.attribute("to").split('/').first());

		quint16 num = 1;

		JidEnums jids;
		if (_enumsOutgoing.contains(account)) {
			jids = _enumsOutgoing.value(account);

			if(jids.contains(jid)) {
				num = jids.value(jid);
				++num;
			}
		}

		jids.insert(jid, num);
		_enumsOutgoing.insert(account, jids);

		stanza.setAttribute(emIdName, num);
	}
	return false;
}

void EnumMessagesPlugin::setupChatTab(QWidget* tab, int account, const QString &contact)
{
	tab->setProperty(propAcc, account);
	tab->setProperty(propJid, contact);
	connect(tab, SIGNAL(destroyed()), SLOT(removeWidget()));
}

bool EnumMessagesPlugin::appendingChatMessage(int account, const QString &contact, QString &body, QDomElement &html, bool local)
{
	if(!enabled || !local)
		return false;

	if(body.isEmpty())
		return false;

	const QString jid(contact.split('/').first());

	quint16 num = 0;

	JidEnums jids;
	if (_enumsOutgoing.contains(account)) {
		jids = _enumsOutgoing.value(account);

		if(jids.contains(jid)) {
			num = jids.value(jid);
		}
	}

	if (num == 0)
		return false;

	QDomNode bodyNode;
	QDomDocument doc = html.ownerDocument();

//	QString s;
//	QTextStream str(&s, QIODevice::WriteOnly);
//	html.save(str, 2);
//	qDebug() << s;

	if(html.isNull()) {
		html = doc.createElement("body");
		html.setAttribute("xmlns", htmlimNS);
		doc.appendChild(html);
	}
	else {
		bodyNode = html.firstChild();

	}
	if(bodyNode.isNull()) {
		nl2br(&html, &doc, body);
	}

	QDomElement msgNum = doc.createElement("span");
	msgNum.setAttribute("style", "color: " + QColor(Qt::green).name());
	msgNum.appendChild(doc.createTextNode(QString("%1 ").arg(numToFormatedStr(num))));

	QDomNode n = html.firstChild();
	html.insertBefore(msgNum,n);

	return false;
}

void EnumMessagesPlugin::removeWidget()
{
	QWidget* w = static_cast<QWidget*>(sender());
	int account = w->property(propAcc).toInt();
	QString jid = w->property(propJid).toString();

	if(_enumsOutgoing.contains(account)) {
		JidEnums jids = _enumsOutgoing.value(account);
		if(jids.contains(jid.split('/').first())) {
			jids.remove(jid);
			_enumsOutgoing[account] = jids;
		}
	}

}

void EnumMessagesPlugin::addMessageNum(QDomDocument *doc, QDomElement *stanza, quint16 num, const QColor& color)
{
	bool appendBody = false;

	QDomElement body;
	QDomElement element = stanza->firstChildElement("html");
	if(element.isNull()) {
		element = doc->createElement("html");
		element.setAttribute("xmlns", xhtmlProtoNS);
	}
	else {
		body = element.firstChildElement("body");

	}
	if(body.isNull()) {
		body = doc->createElement("body");
		body.setAttribute("xmlns", htmlimNS);
		appendBody = true;
	}

	QDomElement msgNum = doc->createElement("span");
	msgNum.setAttribute("style", "color: " + color.name());
	msgNum.appendChild(doc->createTextNode(QString("%1 ").arg(numToFormatedStr(num))));

	if(appendBody) {
		body.appendChild(msgNum);
		nl2br(&body, doc, stanza->firstChildElement("body").text());
	}
	else {
		QDomNode n = body.firstChild();
		body.insertBefore(msgNum,n);
	}

	element.appendChild(body);
	stanza->appendChild(element);
}

QString EnumMessagesPlugin::numToFormatedStr(int number)
{
	return QString("%1").arg(number, 5, 10, QChar('0'));
}

void EnumMessagesPlugin::nl2br(QDomElement *body, QDomDocument *doc, const QString &msg)
{
	foreach (const QString& str, msg.split("\n")) {
		body->appendChild(doc->createTextNode(str));
		body->appendChild(doc->createElement("br"));
	}
	body->removeChild(body->lastChild());
}

QString EnumMessagesPlugin::pluginInfo()
{
	return tr("Authors: ") + "Dealer_WeARE\n\n";
}


#ifndef HAVE_QT5
	Q_EXPORT_PLUGIN(EnumMessagesPlugin)
#endif
