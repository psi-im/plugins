/*
 * captchaformsplugin.cpp - plugin
 * Copyright (C) 2010  Khryukin Evgeny
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

#include "captchadialog.h"
#include "loader.h"
#include "ui_options.h"

#include "psiplugin.h"
#include "optionaccessor.h"
#include "optionaccessinghost.h"
#include "plugininfoprovider.h"
#include "eventcreatinghost.h"
#include "eventcreator.h"
#include "stanzafilter.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"
#include "accountinfoaccessinghost.h"
#include "accountinfoaccessor.h"
#include "applicationinfoaccessinghost.h"
#include "applicationinfoaccessor.h"


#define constVersion "0.1.1"
#define constProxyHost "host"
#define constProxyPort "port"
#define constProxyUser "user"
#define constProxyPass "pass"
#define constAutopopup "autopopup"
#define constUseProxy "useproxy"

static const QStringList methods = QStringList() << "qa" << "ocr" << "picture_q" << "picture_recog";

class CaptchaFormsPlugin : public QObject, public PsiPlugin, public OptionAccessor, public PluginInfoProvider, public EventCreator,
			   public StanzaFilter, public StanzaSender, public AccountInfoAccessor, public ApplicationInfoAccessor
{
        Q_OBJECT
	Q_INTERFACES(PsiPlugin OptionAccessor PluginInfoProvider EventCreator StanzaFilter StanzaSender AccountInfoAccessor ApplicationInfoAccessor)
public:
	CaptchaFormsPlugin();
        virtual QString name() const;
        virtual QString shortName() const;
        virtual QString version() const;
        virtual QWidget* options();
        virtual bool enable();
        virtual bool disable();
	virtual void optionChanged(const QString&){};
        virtual void applyOptions();
        virtual void restoreOptions();
	virtual void setOptionAccessingHost(OptionAccessingHost* host);
	virtual void setEventCreatingHost(EventCreatingHost* host);
	virtual bool incomingStanza(int account, const QDomElement& xml);
	virtual bool outgoingStanza(int account, QDomElement& xml);
	virtual void setStanzaSendingHost(StanzaSendingHost *host);
	virtual void setAccountInfoAccessingHost(AccountInfoAccessingHost* host);
	virtual void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host);
	virtual QString pluginInfo();
	virtual QIcon icon() const;

private:
        OptionAccessingHost *psiOptions;
	EventCreatingHost *psiEvent;
	StanzaSendingHost *stanzaSender;
	AccountInfoAccessingHost* accInfo;
	ApplicationInfoAccessingHost* appInfo;
	bool enabled;
	int id;
	bool autopopup, useProxy;
	QList< QHash<QString, QString> > challenges_;
	QHash< QString, QPointer<CaptchaDialog> > dialogs_;
	Ui::Options ui_;


	bool isValidChallenge(const QDomElement& xml, QHash<QString, QString>& dataFields) const;
	int findChalleng(const QString& field, const QString& value);

private slots:
	void eventActivated(const QString&);
	void submitChallenge(const QString&, const QString&);
	void cancelChallenge(const QString&);
	void loaderData(const QString& id, const QByteArray& data);
	void loaderError(const QString& id);
};

Q_EXPORT_PLUGIN(CaptchaFormsPlugin);


CaptchaFormsPlugin::CaptchaFormsPlugin()
	: psiOptions(0)
	, psiEvent(0)
	, stanzaSender(0)
	, accInfo(0)
	, appInfo(0)
	, enabled(false)
	, id(111)
	, autopopup(true)
	, useProxy(false)

{
}

QString CaptchaFormsPlugin::name() const {
	return "Captcha Forms Plugin";
}

QString CaptchaFormsPlugin::shortName() const {
	return "captcha";
}

QString CaptchaFormsPlugin::version() const {
        return constVersion;
}

bool CaptchaFormsPlugin::enable()
{
    if(psiOptions) {
	enabled = true;
	id = 111;
	useProxy = psiOptions->getPluginOption(constUseProxy, QVariant(useProxy)).toBool();
	autopopup = psiOptions->getPluginOption(constAutopopup, QVariant(autopopup)).toBool();

	if(!useProxy)
		appInfo->getProxyFor(name()); //register proxy
    }
    return enabled;
}

bool CaptchaFormsPlugin::disable()
{
	enabled = false;
        return true;
}

QWidget* CaptchaFormsPlugin::options()
{
        if (!enabled) {
		return 0;
	}
        QWidget *options = new QWidget();
	ui_.setupUi(options);

	restoreOptions();
	return options;
}

void CaptchaFormsPlugin::applyOptions()
{
	useProxy = ui_.cb_use_proxy->isChecked();
	psiOptions->setPluginOption(constUseProxy, QVariant(useProxy));
	autopopup = ui_.cb_autopopup->isChecked();
	psiOptions->setPluginOption(constAutopopup, QVariant(autopopup));

	if(!useProxy)
		appInfo->getProxyFor(name()); //register proxy
}

void CaptchaFormsPlugin::restoreOptions()
{
	ui_.cb_autopopup->setChecked(autopopup);
	ui_.cb_use_proxy->setChecked(useProxy);
}

void CaptchaFormsPlugin::setOptionAccessingHost(OptionAccessingHost *host)
{
    psiOptions = host;
}

void CaptchaFormsPlugin::setEventCreatingHost(EventCreatingHost *host)
{
	psiEvent = host;
}

void CaptchaFormsPlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost *host)
{
	accInfo = host;
}

void CaptchaFormsPlugin::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host)
{
	appInfo = host;
}

void CaptchaFormsPlugin::setStanzaSendingHost(StanzaSendingHost *host)
{
	stanzaSender = host;
}

bool CaptchaFormsPlugin::incomingStanza(int account, const QDomElement &xml)
{
	if(enabled) {
		QHash<QString, QString> dataFields;
		if(!isValidChallenge(xml, dataFields) )
			return false;
		else {
			dataFields["id"] = QString::number(id++);
			dataFields["account"] = QString::number(account);
			challenges_.push_back(dataFields);
			if(autopopup)
				eventActivated(dataFields.value("sender"));
			else
				psiEvent->createNewEvent(account, dataFields.value("sender"), tr("Captcha Plugin: CAPTCHA from %1")
						 .arg(dataFields.value("sender")), this, SLOT(eventActivated(QString)));

			return true;
		}
	}
	return false;
}

bool CaptchaFormsPlugin::outgoingStanza(int /*account*/, QDomElement& /*xml*/)
{
	return false;
}

void CaptchaFormsPlugin::eventActivated(const QString& from)
{
	int index = findChalleng("sender", from);
	if(index == -1)
		return;

	QHash<QString, QString> dataFields = challenges_.at(index);
	QString id = dataFields.value("id");
	QPointer<CaptchaDialog> cd = new CaptchaDialog(id);
	cd->setBody(dataFields.value("body"));
	cd->setQuestion(dataFields.value("label"));
	connect(cd, SIGNAL(ok(QString, QString)), this, SLOT(submitChallenge(QString, QString)));
	connect(cd, SIGNAL(cancel(QString)), this, SLOT(cancelChallenge(QString)));
	dialogs_[id] = cd;


	if(dataFields.contains("data")) {
		QByteArray ba;
		ba.append(dataFields.value("data"));
		QPixmap pix = QPixmap::fromImage(QImage::fromData(QByteArray::fromBase64(ba)));
		cd->setPixmap(pix);
	}
	else {
		Loader *ld = new Loader(id, this);
		if(useProxy) {
			int acc = dataFields.value("account").toInt();
			QString host_ = accInfo->proxyHost(acc);
			if(!host_.isEmpty()) {
				ld->setProxy(host_, accInfo->proxyPort(acc), accInfo->proxyUser(acc), accInfo->proxyPassword(acc));
			}
		}
		else {
			Proxy p = appInfo->getProxyFor(name());
			ld->setProxy(p.host, p.port, p.user, p.pass);
		}
		QString url = dataFields.value("uri");
		if(url.isEmpty()) {
			QString str = dataFields.value("body");
			QRegExp re(".*(http://[^\"]+).*");
			if(re.indexIn(str) != -1)
				url = re.cap(1) + "/image";
		}
		ld->start(url);
		connect(ld, SIGNAL(data(QString,QByteArray)), SLOT(loaderData(QString, QByteArray)));
		connect(ld, SIGNAL(error(QString)), SLOT(loaderError(QString)));
	}
	cd->show();
}

void CaptchaFormsPlugin::loaderError(const QString& id)
{
	if(!dialogs_.contains(id))
		return;

	QPointer<CaptchaDialog> cd = dialogs_.value(id);
	if(cd) {
		cd->setText(tr("Error! Image can not be loaded."));
	}
	else
		dialogs_.remove(id);
}

void CaptchaFormsPlugin::loaderData(const QString& id, const QByteArray& data)
{
	if(!dialogs_.contains(id))
		return;

	QPointer<CaptchaDialog> cd = dialogs_.value(id);
	if(cd) {
		cd->setPixmap(QPixmap::fromImage(QImage::fromData(data)));
	}
	else
		dialogs_.remove(id);
}

int CaptchaFormsPlugin::findChalleng(const QString& field, const QString& value)
{
	int index = -1;
	for(int i = 0; i < challenges_.size(); i++) {
		QHash<QString, QString> dataFields = challenges_.at(i);
		if(dataFields.contains(field) && dataFields.value(field) == value) {
			index = i;
			break;
		}
	}
	return index;
}

void CaptchaFormsPlugin::submitChallenge(const QString& id, const QString& text)
{
	int index = findChalleng("id", id);
	if(index == -1)
		return;

	QHash<QString, QString> dataFields = challenges_.at(index);
	QString mes = QString("<iq type=\"set\" from=\"%1\" to=\"%2\" xml:lang=\"en\" id=\"%3\">"
				"<captcha xmlns=\"urn:xmpp:captcha\"><x xmlns=\"jabber:x:data\" type=\"submit\">"
				"<field var=\"FORM_TYPE\"><value>urn:xmpp:captcha</value></field>"
				"<field var=\"from\"><value>%4</value></field>"
				"<field var=\"challenge\"><value>%5</value></field>"
				"<field var=\"%6\"><value>%7</value></field>"
				"</x></captcha></iq>")
			.arg(stanzaSender->escape(dataFields.value("to")), stanzaSender->escape(dataFields.value("sender")), stanzaSender->escape(dataFields.value("id")))
			.arg(stanzaSender->escape(dataFields.value("from")), stanzaSender->escape(dataFields.value("challenge")))
			.arg(stanzaSender->escape(dataFields.value("method")), text);

	stanzaSender->sendStanza(dataFields.value("account").toInt(), mes);

	challenges_.removeAt(index);
}

void CaptchaFormsPlugin::cancelChallenge(const QString& id)
{
	int index = findChalleng("id", id);
	if(index == -1)
		return;

	QHash<QString, QString> dataFields = challenges_.at(index);

	QString mes = QString("<message type=\"error\" to=\"%1\" xml:lang=\"en\" id=\"%2\">"
			"<error xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\" type=\"modify\">"
			"<not-acceptable xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/></error></message>")
			//.arg(dataFields.value("to"))
			.arg(stanzaSender->escape(dataFields.value("sender")))
			.arg(stanzaSender->escape(dataFields.value("challenge")));
	stanzaSender->sendStanza(dataFields.value("account").toInt(), mes);

	challenges_.removeAt(index);
}

bool CaptchaFormsPlugin::isValidChallenge(const QDomElement& stanza, QHash<QString, QString>& dataFields) const
{
	if(stanza.tagName() != "message")
		return false;

	QDomElement captcha = stanza.firstChildElement("captcha");
	if(captcha.attribute("xmlns") != "urn:xmpp:captcha")
		return false;

	bool find = false;
	QDomElement formElem = captcha.firstChildElement("x");
	while (!formElem.isNull() ) {
		if(formElem.attribute("xmlns") == "jabber:x:data") {
			find = true;
			break;
		}
		formElem = formElem.nextSiblingElement("x");
	}

	if(!find)
		return false;

	dataFields["sender"] = stanza.attribute("from");
	dataFields["body"] = stanza.firstChildElement("body").text();
	dataFields["to"] = stanza.attribute("to");

	find = false;
	QString id = stanza.attribute("id");

	QDomNode field = formElem.firstChild();
	while(!field.isNull()) {
		QDomElement tmp = field.toElement();
		if(tmp.tagName() != "field")
			field = field.nextSibling();

		QString var = tmp.attribute("var");

		if(var == "FORM_TYPE") {
			if(tmp.firstChildElement("value").text() == "urn:xmpp:captcha") {
				find = true;
			}
		}
		else if(var == "challenge") {
			if(tmp.firstChildElement("value").text() == id) {
				dataFields["challenge"] = id;  // id запроса
			}
		}
		else if(var == "from") {
			QString from = tmp.firstChildElement("value").text();
			if(!from.isEmpty()) {
				dataFields["from"] = from;
			}
		}
		else if(tmp.attribute("type") != "hidden") {
			if(!var.isEmpty() && methods.contains(var)) {
				dataFields["label"] = tmp.attribute("label");  //вопрос
				dataFields["method"] = var; // метод
				QDomNodeList uriList = tmp.elementsByTagName("uri");
				for(int i = 0; i < uriList.size(); i++) {
					QDomElement uriElem = uriList.at(i).toElement();
					QString link = uriElem.text();
					if(uriElem.attribute("type").left(5) == "image"
					   && link.left(4) == "http") {
						dataFields["uri"] = link;  // ссылка на контент в интернете
						dataFields["uri_type"] = uriElem.attribute("type"); // тип изображения
						break;
					}
				}
			}
		}
		field = field.nextSibling();
	}

	QDomElement data = stanza.firstChildElement("data");
	if(!data.isNull()) {
		dataFields["data"] = data.text();
	}

	QStringList f = dataFields.keys();
	if( !find || !f.contains("challenge") || !f.contains("from") )
		return false;

	return true;
}

QString CaptchaFormsPlugin::pluginInfo()
{
	return tr("Author: ") +  "Dealer_WeARE\n"
			+ tr("Email: ") + "wadealer@gmail.com\n\n"
			+ trUtf8("This plugin is designed to pass of captcha directly from the Psi+.");
}

QIcon CaptchaFormsPlugin::icon() const
{
	return QIcon(":/icons/captcha.png");
}

#include "captchaformsplugin.moc"
