/*
 * rippercc.cpp
 *
 * Copyright (C) 2016
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
#include "rippercc.h"

#include "qjsonwrapper.h"

#include <psiaccountcontrollinghost.h>
#include <optionaccessinghost.h>
#include <iconfactoryaccessinghost.h>
#include <activetabaccessinghost.h>
#include <accountinfoaccessinghost.h>
#include <stanzasendinghost.h>

#include <QDomElement>
#include <QDomDocument>
#include <QDomText>
#include <QNetworkProxy>
#include <QNetworkReply>

#define TIMER_INTERVAL (30 * 60 * 1000) /* 30 minutes */
#define RIPPER_DB_URL "https://ripper.cc/api/v1/plugin/jabber?format=json"
#define RIPPER_PREFIX "Ripper! "
#define RIPPER_GROUP "Rippers"

#define ATTENTION_MESSAGE                                                                          \
	"<b>ATTENTION! WARNING!</b> This man real ripper, read more here in his profile.<br/>"                \
	"<b>ВНИМАНИЕ! ОСТОРОЖНО!</b> Этот человек реальный обманщик, подробнее прочитать в его профиле.<br/>" \
	"<a href=\"https://ripper.cc%1\">https://ripper.cc%1</a>"

#ifndef HAVE_QT5
Q_EXPORT_PLUGIN2(RipperCC, RipperCC);
#endif

RipperCC::RipperCC()
	: _enabled(false)
	, _accountHost(0)
	, _optionHost(0)
	, _stanzaSending(0)
	, _accountInfo(0)
	, _appInfo(0)
	, _nam(0)
	, _timer(new QTimer(this))
	, _optionsForm(0)
{
	_timer->setInterval(TIMER_INTERVAL);
	_timer->setSingleShot(true);
	connect(_timer, SIGNAL(timeout()), SLOT(updateRipperDb()));
}


RipperCC::~RipperCC()
{
}

QWidget *RipperCC::options()
{
	if (!_enabled) {
		return 0;
	}

	_optionsForm = new RipperCCOptions();
	_optionsForm->setOptionAccessingHost(_optionHost);
	_optionsForm->loadSettings();
	return qobject_cast<QWidget*>(_optionsForm);
}

bool RipperCC::enable()
{
	_enabled = true;

	Proxy psiProxy = _appInfo->getProxyFor(name());
	QNetworkProxy::ProxyType type;
	if(psiProxy.type == "socks") {
		type = QNetworkProxy::Socks5Proxy;
	} else {
		type = QNetworkProxy::HttpProxy;
	}

	QNetworkProxy proxy(type, psiProxy.host, psiProxy.port, psiProxy.user, psiProxy.pass);

	_nam = new QNetworkAccessManager(this);
	if (!proxy.hostName().isEmpty())
		_nam->setProxy(proxy);

	updateRipperDb();

	return _enabled;
}

bool RipperCC::disable()
{
	_timer->stop();
	_enabled = false;
	_nam->deleteLater();
	_nam = 0;
	return true;
}

void RipperCC::applyOptions()
{
	_optionsForm->saveSettings();
}

void RipperCC::restoreOptions()
{
}

QPixmap RipperCC::icon() const
{
	return QPixmap(":/icons/rippercc.png");
}

QString RipperCC::pluginInfo()
{
	return QString();
}

bool RipperCC::incomingStanza(int account, const QDomElement& stanza)
{
	if (!_enabled) {
		return false;
	}

	handleStanza(account, stanza, true);

	return false;
}

bool RipperCC::outgoingStanza(int account, QDomElement &stanza)
{
	if (!_enabled) {
		return false;
	}

	handleStanza(account, stanza, false);

	return false;

}

void RipperCC::handleStanza(int account, const QDomElement &stanza, bool incoming)
{
	if (stanza.tagName() != QLatin1String("message") || stanza.attribute(QLatin1String("type")) != QLatin1String("chat"))
		return;

	QString from = incoming ? stanza.attribute(QLatin1String("from")) : stanza.attribute(QLatin1String("to"));
	QString jid = from.split(QLatin1Char('/')).first();

	int ripperIndex = -1;
	for (int i = 0; i < _rippers.size(); ++i) {
		if (_rippers.at(i).jid == jid) {
			ripperIndex = i;
			break;
		}
	}

	if (ripperIndex < 0)
		return;

	int attentionInterval = _optionHost->getPluginOption("attention-interval", 1).toInt() * 60;

	if (_rippers.at(ripperIndex).lastAttentionTime.isValid() && _rippers.at(ripperIndex).lastAttentionTime.secsTo(QDateTime::currentDateTime()) < attentionInterval)
		return;

	_rippers[ripperIndex].lastAttentionTime = QDateTime::currentDateTime();

	_accountHost->appendSysMsg(account, from, QString::fromUtf8(ATTENTION_MESSAGE).arg(_rippers.at(ripperIndex).url));

	QString ripperNick = _contactInfo->name(account, jid);
	if (!ripperNick.startsWith(QLatin1String(RIPPER_PREFIX))) {

		// <iq id="ab1da" type="set">
		//   <query xmlns="jabber:iq:roster">
		//     <item name="ripper" jid="juliet@example.com">
		//       <group="rippers"/>
		//     </item>
		//   </query>
		// </iq>

		QDomDocument doc;
		QDomElement iqElement = doc.createElement(QLatin1String("iq"));
		iqElement.setAttribute(QLatin1String("type"), QLatin1String("set"));
		iqElement.setAttribute(QLatin1String("id"), _stanzaSending->uniqueId(account));

		QDomElement queryElement = doc.createElement(QLatin1String("query"));
		queryElement.setAttribute(QLatin1String("xmlns"), QLatin1String("jabber:iq:roster"));

		QDomElement itemElement = doc.createElement(QLatin1String("item"));
		itemElement.setAttribute(QLatin1String("name"), QLatin1String(RIPPER_PREFIX) + ripperNick);
		itemElement.setAttribute(QLatin1String("jid"), jid);

		QDomElement groupElement = doc.createElement(QLatin1String("group"));

		QDomText textNode = doc.createTextNode(QLatin1String(RIPPER_GROUP));

		groupElement.appendChild(textNode);
		itemElement.appendChild(groupElement);
		queryElement.appendChild(itemElement);
		iqElement.appendChild(queryElement);

		_stanzaSending->sendStanza(account, iqElement);
	}
}

void RipperCC::updateRipperDb()
{
	QNetworkRequest request(QString(RIPPER_DB_URL));
	request.setRawHeader("User-Agent", "RipperCC Plugin (Psi+)");
	QNetworkReply *reply = _nam->get(request);
	connect(reply, SIGNAL(finished()), SLOT(parseRipperDb()));
}

void RipperCC::parseRipperDb()
{
	_timer->start();

	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

	// Occurs error
	if(reply->error() != QNetworkReply::NoError) {
		qDebug() << "RippperCC Plugin:" << reply->errorString();
		reply->close();
		return;
	}

	// No errors
	QByteArray ba = reply->readAll();

	QVariantMap ripperMap = QJsonWrapper::parseJson(ba).toMap();
	if (!ripperMap.contains(QLatin1String("rippers")))
		return;

	QVariantList ripperList = ripperMap.value(QLatin1String("rippers")).toList();
	if (ripperList.isEmpty())
		return;

	_rippers.clear();
	foreach (const QVariant &item, ripperList) {
		Ripper ripper;
		ripper.jid = item.toMap().value(QLatin1String("jabber")).toString();
		ripper.url = item.toMap().value(QLatin1String("link")).toString();
		_rippers << ripper;
	}
}
