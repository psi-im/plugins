/*
 * gmailserviceplugin.cpp - plugin
 * Copyright (C) 2009-2011 Kravtsov Nikolai, Khryukin Evgeny
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

#include <QFileDialog>
#include <QProcess>
#include <QDomElement>
#include "gmailserviceplugin.h"
#include "common.h"

Q_EXPORT_PLUGIN(GmailNotifyPlugin);

GmailNotifyPlugin::GmailNotifyPlugin()
	: enabled(false)
	, optionsApplingInProgress_(false)
	, stanzaSender(0)
	, psiOptions(0)
	, accInfo(0)
	, popup(0)
	, accountController(0)
	, iconHost(0)
	, psiEvent(0)
	, sound_(0)
	, soundFile("sound/email.wav")
	, actions_(0)
	, popupId(0)
{
}

QString GmailNotifyPlugin::name() const
{
	return "GMail Service Plugin";
}

QString GmailNotifyPlugin::shortName() const
{
	return "gmailnotify";
}

QString GmailNotifyPlugin::version() const
{
	return PLUGIN_VERSION;
}

QWidget* GmailNotifyPlugin::options()
{
	if (!enabled)
		return 0;

	options_ = new QWidget;
	ui_.setupUi(options_);
	restoreOptions();

	ui_.tb_check->setIcon(iconHost->getIcon("psi/play"));
	ui_.tb_open->setIcon(iconHost->getIcon("psi/browse"));
	ui_.tb_open_prog->setIcon(iconHost->getIcon("psi/browse"));

	connect(ui_.tb_check, SIGNAL(clicked()), SLOT(checkSound()));
	connect(ui_.tb_open, SIGNAL(clicked()), SLOT(getSound()));
	connect(ui_.cb_accounts, SIGNAL(currentIndexChanged(int)), SLOT(updateOptions(int)));
	connect(ui_.tb_open_prog, SIGNAL(clicked()), SLOT(getProg()));

	return options_;
}

bool GmailNotifyPlugin::enable()
{
	enabled = true;
	optionsApplingInProgress_ = false;
	id_.clear();
	accounts.clear();
	mailItems_.clear();
	actions_ = new ActionsList(this);
	connect(actions_, SIGNAL(changeNoSaveState(int,QString,bool)), this, SLOT(changeNoSaveState(int,QString,bool)));

	QFile f(":/icons/gmailnotify.png");
	if(f.open(QIODevice::ReadOnly))
		iconHost->addIcon("gmailnotify/menu", f.readAll());
	f.close();

	f.setFileName(":/icons/nohistory.png");
	if(f.open(QIODevice::ReadOnly))
		iconHost->addIcon("gmailnotify/nohistory", f.readAll());
	f.close();

	soundFile = psiOptions->getPluginOption(OPTION_SOUND, soundFile).toString();
	loadLists();

	int interval = psiOptions->getPluginOption(OPTION_INTERVAL, QVariant(4000)).toInt()/1000;
	popupId = popup->registerOption(POPUP_OPTION, interval, "plugins.options."+shortName()+"."+OPTION_INTERVAL);
	program_ = psiOptions->getPluginOption(OPTION_PROG).toString();

	//Update features
	bool end = false;
	int acc = 0;
	while(!end) {
		QString jid = accInfo->getJid(acc);
		if(jid == "-1") {
			end = true;
			continue;
		}
		QStringList l = jid.split("@");
		QString domain = l.last().split("/").first();
		QString id = stanzaSender->uniqueId(acc);
		id_.append(id);
		if(accInfo->getStatus(acc) != "offline")
			stanzaSender->sendStanza(acc, QString("<iq type='get' to='%1' id='%2' ><query xmlns='http://jabber.org/protocol/disco#info'/></iq>")
					 .arg(domain)
					 .arg(id));
		acc++;
	}

	return true;
}

bool GmailNotifyPlugin::disable()
{
	qDeleteAll(accounts);
	accounts.clear();

	delete actions_;
	actions_ = 0;

	delete mailViewer_;

	popup->unregisterOption(POPUP_OPTION);
	enabled = false;
	return true;
}

void GmailNotifyPlugin::applyOptions()
{
	if (!options_)
		return;

	optionsApplingInProgress_ = true;

	soundFile = ui_.le_sound->text();
	psiOptions->setPluginOption(OPTION_SOUND, soundFile);

	program_ = ui_.le_program->text();
	psiOptions->setPluginOption(OPTION_PROG, program_);

	int index = ui_.cb_accounts->currentIndex();
	if(accounts.size() <= index || index == -1)
		return;

	AccountSettings *as = findAccountSettings(ui_.cb_accounts->currentText());
	if(!as)
		return;

	as->notifyAllUnread = !ui_.rb_new_messages->isChecked();

	as->isMailEnabled = ui_.cb_mail->isChecked();
	as->isArchivingEnabled = ui_.cb_archiving->isChecked();
	as->isSuggestionsEnabled = ui_.cb_suggestions->isChecked();
	as->isSharedStatusEnabled = ui_.cb_shared_statuses->isChecked();
	as->isNoSaveEnbaled = ui_.cb_nosave->isChecked();

	Utils::updateSettings(as, stanzaSender, accInfo);

	if(as->isMailEnabled)
		Utils::requestMail(as, stanzaSender, accInfo);

	if(as->isSharedStatusEnabled)
		Utils::requestSharedStatusesList(as, stanzaSender, accInfo);

	if(as->isNoSaveEnbaled && as->isArchivingEnabled)
		Utils::updateNoSaveState(as, stanzaSender, accInfo);

	updateActions(as);

	saveLists();

	QTimer::singleShot(2000, this, SLOT(stopOptionsApply()));
}

void GmailNotifyPlugin::stopOptionsApply()
{
	optionsApplingInProgress_ = false;
}

void GmailNotifyPlugin::saveLists()
{
	QStringList l;
	foreach(AccountSettings *as, accounts)
		l.append(as->toString());
	psiOptions->setPluginOption(OPTION_LISTS, QVariant(l));
}

void GmailNotifyPlugin::loadLists()
{
	QStringList l = psiOptions->getPluginOption(OPTION_LISTS, QVariant()).toStringList();
	foreach(QString settings, l) {
		AccountSettings *as = new AccountSettings();
		as->fromString(settings);
		accounts.append(as);
	}
}

void GmailNotifyPlugin::restoreOptions()
{
	if (!options_ || optionsApplingInProgress_)
		return;

	ui_.lb_error->hide();
	ui_.gb_settings->setEnabled(true);
	ui_.cb_mail->setVisible(true);
	ui_.cb_shared_statuses->setVisible(true);
	ui_.cb_nosave->setVisible(true);
	ui_.le_sound->setText(soundFile);
	ui_.le_program->setText(program_);

	ui_.cb_accounts->setEnabled(true);
	ui_.cb_accounts->clear();
	if(!accounts.isEmpty()) {
		foreach(AccountSettings* as, accounts) {
			if(as->account != -1)
				ui_.cb_accounts->addItem(as->jid);
		}
	}

	if(!ui_.cb_accounts->count()) {
		ui_.cb_accounts->setEnabled(false);
		ui_.gb_mail_settings->setEnabled(false);
		ui_.gb_settings->setEnabled(false);
		ui_.lb_error->setVisible(true);
	}
	else {
		ui_.cb_accounts->setCurrentIndex(0);
		updateOptions(0);
	}
}

void GmailNotifyPlugin::updateOptions(int index)
{
	if (!options_ || index >= accounts.size() || index < 0)
		return;

	AccountSettings *as = findAccountSettings(ui_.cb_accounts->currentText());
	if(!as)
		return;

	ui_.cb_mail->setChecked(as->isMailEnabled);
	ui_.cb_mail->setVisible(as->isMailSupported);
	ui_.gb_mail_settings->setEnabled(ui_.cb_mail->isChecked());
	ui_.rb_new_messages->setChecked(!as->notifyAllUnread);
	ui_.rb_all_messages->setChecked(as->notifyAllUnread);

	ui_.cb_archiving->setChecked(as->isArchivingEnabled);
	ui_.cb_suggestions->setChecked(as->isSuggestionsEnabled);	

	ui_.cb_shared_statuses->setChecked(as->isSharedStatusEnabled);
	ui_.cb_shared_statuses->setVisible(as->isSharedStatusSupported);

	ui_.cb_nosave->setChecked(as->isNoSaveEnbaled);
	ui_.cb_nosave->setVisible(as->isNoSaveSupported);
	ui_.cb_nosave->setEnabled(ui_.cb_archiving->isChecked());
}

bool GmailNotifyPlugin::incomingStanza(int account, const QDomElement& stanza)
{
	if (enabled) {
		if (stanza.tagName() == "iq") {
			QDomElement query = stanza.firstChild().toElement();
			if (!query.isNull()) {
				if(checkFeatures(account, stanza, query))
					return true;

				if(checkEmail(account, stanza, query))
					return true;

				if(checkSettings(account, stanza, query))
					return true;

				if(checkSharedStatus(account, stanza, query))
					return true;

				if(checkNoSave(account, stanza, query))
					return true;

				if(checkAttributes(account, stanza, query))
					return true;
			}
		}
		else if(stanza.tagName() == "message") {
			QDomElement x = stanza.firstChildElement("x");
			if(!x.isNull() && x.attribute("xmlns") == "google:nosave") {
				QString jid = stanza.attribute("from").split("/").first();
				bool val = (x.attribute("value") == "enabled");
				AccountSettings *as = findAccountSettings(accInfo->getJid(account));
				if(as && as->noSaveList.contains(jid)
					&& as->noSaveList.value(jid) != val)
				{
					as->noSaveList.insert(jid, val);
					showPopup(tr("No-save state for contact %1 is changed").arg(jid));
					return true;
				}
			}
		}
	}
	return false;
}

bool GmailNotifyPlugin::outgoingStanza(int account, QDomElement& stanza)
{
	if(enabled && hasAccountSettings(account)) {
		if(stanza.tagName() == "presence") {
			AccountSettings* as = findAccountSettings(accInfo->getJid(account));
			if(as && as->account == account && as->isSharedStatusEnabled && as->isSharedStatusSupported) {
				QString status = accInfo->getStatus(account);
				QString message = accInfo->getStatusMessage(account);
				if(message.length() > as->statusMax)
					message.chop(message.length() - as->statusMax);
				if(status == as->status && message == as->message) {
					return false;
				}
				as->message = message;
				as->status = status;
				qRegisterMetaType<AccountSettings*>();
				QMetaObject::invokeMethod(this, "updateSharedStatus", Qt::QueuedConnection, Q_ARG(AccountSettings*, as));
			}
		}
	}
	return false;
}

bool GmailNotifyPlugin::checkFeatures(int account, const QDomElement &stanza, const QDomElement& query)
{
	bool myReqyest = false;
	if (stanza.attribute("type") == "result"		
	     && query.tagName() == "query"
	     && query.attribute("xmlns") == "http://jabber.org/protocol/disco#info")
	{
		if(id_.contains(stanza.attribute("id"))) {
			id_.removeAll(stanza.attribute("id"));
			myReqyest = true;
		}
		bool foundGoogleExt = false;

		const QString from = stanza.attribute("from").toLower();
		if(from.contains("@")) { // нам нужен disco#info от сервера, а не от клиента.
			return false;
		}
		const QString jid = accInfo->getJid(account);
		QStringList tmpList = jid.split("@");
		const QString domain = tmpList.last().split("/").first();
		if(domain != from) { // нам нужен disco#info от нашего сервера
			return false;
		}
		QString fullJid = stanza.attribute("to");
		for (QDomNode child = query.firstChild(); !child.isNull(); child = child.nextSibling()) {
			QDomElement feature = child.toElement();
			if(feature.isNull() || feature.tagName() != "feature")
				continue;

			//If the server supports the Gmail extension
			if(feature.attribute("var") == "google:mail:notify" && feature.attribute("node").isEmpty()) {
				AccountSettings *as = create(account, jid);
				as->isMailSupported = true;
				//Utils::requestMail(as, stanzaSender, accInfo);
				foundGoogleExt = true;
			}
			else if(feature.attribute("var") == "google:setting" && feature.attribute("node").isEmpty()) {
				AccountSettings *as = create(account, jid);
				Utils::getUserSettings(as, stanzaSender, accInfo);
				foundGoogleExt = true;
			}
			else if(feature.attribute("var") == "google:shared-status" && feature.attribute("node").isEmpty()) {
				AccountSettings *as = create(account, jid);
				as->isSharedStatusSupported = true;
				as->status = accInfo->getStatus(account);
				as->message = accInfo->getStatusMessage(account);
				as->fullJid = fullJid;
				if(as->isSharedStatusEnabled)
					Utils::requestSharedStatusesList(as, stanzaSender, accInfo);
				foundGoogleExt = true;
			}
			else if(feature.attribute("var") == "google:nosave" && feature.attribute("node").isEmpty()) {
				AccountSettings *as = create(account, jid);
				as->isNoSaveSupported = true;
				updateActions(as);
				if(as->isNoSaveEnbaled)
					Utils::updateNoSaveState(as, stanzaSender, accInfo);
				foundGoogleExt = true;
			}
			else if(feature.attribute("var") == "google:roster" && feature.attribute("node").isEmpty()) {
				AccountSettings *as = create(account, jid);
				as->isAttributesSupported = true;
				if(as->isAttributesEnabled)
					Utils::requestExtendedContactAttributes(as, stanzaSender, accInfo);
				foundGoogleExt = true;
			}

		}
		if(foundGoogleExt) {
			optionsApplingInProgress_ = false;
			restoreOptions();
		}
	}
	return myReqyest;
}

bool GmailNotifyPlugin::checkEmail(int account, const QDomElement &stanza, const QDomElement& query)
{
	if (stanza.attribute("type") == "set"
		&& query.tagName() == "new-mail"
		&& query.attribute("xmlns") == "google:mail:notify")
	{
		//Server reports new mail
		//send success result
		QString from = stanza.attribute("to");
		QString to = from.split("/").at(0);
		QString iqId = stanza.attribute("id");
		QString reply = QString("<iq type='result' from='%1' to='%2' id='%3' />").arg(from,to,iqId);
		stanzaSender->sendStanza(account, reply);
		AccountSettings *as = findAccountSettings(to.toLower());
		if(!as || as->account != account) {
			return true;
		}

		//requests new mail
		Utils::requestMail(as, stanzaSender, accInfo);
		//block stanza processing
		return true;
	}
	else if(stanza.attribute("type") == "result"
		&& query.tagName() == "mailbox"
		&& query.attribute("xmlns") == "google:mail:notify")
	{
		//Email Query Response
		const QString jid = stanza.attribute("to").split("/").at(0);
		const QString server = stanza.attribute("from").toLower();
		if(!server.isEmpty() && jid.toLower() != server) {
			return false;
		}
		const QString from = stanza.attribute("to");
		AccountSettings *as = findAccountSettings(jid);
		if(!as || as->account != account) {
			return true;
		}

		as->lastMailTime = query.attribute("result-time");
		QDomElement lastmail = query.firstChildElement("mail-thread-info");
		if (!lastmail.isNull())
			as->lastMailTid = lastmail.attribute("tid");

		//save last check values
		saveLists();

		incomingMail(account, query);
		return true;
	}
	return false;
}

bool GmailNotifyPlugin::checkSettings(int account, const QDomElement &stanza, const QDomElement& query)
{
	bool foundSettings = false;
	if ( (stanza.attribute("type") == "result"
	     || stanza.attribute("type") == "set")
	    && query.tagName() == "usersetting"
	    && query.attribute("xmlns") == "google:setting")
	{
		foundSettings = true;
		//id_.removeAll(stanza.attribute("id"));
		const QString jid = stanza.attribute("to").split("/").at(0);
		const QString server = stanza.attribute("from").toLower();
		if(!server.isEmpty() && jid.toLower() != server) {
			return false;
		}
		AccountSettings *as = findAccountSettings(jid.toLower());
		if(!as || as->account != account) {
			return true;
		}
		for (QDomNode child = query.firstChild(); !child.isNull(); child = child.nextSibling()) {
			QDomElement setting = child.toElement();
			QString value = setting.attribute("value");
			if(setting.isNull() || value.isEmpty())
				continue;

			if(setting.tagName() == "autoacceptsuggestions")
				as->isSuggestionsEnabled = (value == "true");
			else if(setting.tagName() == "mailnotifications") {
				as->isMailEnabled = (value == "true");
				Utils::requestMail(as, stanzaSender, accInfo);
			}
			else if(setting.tagName() == "archivingenabled") {
				as->isArchivingEnabled = (value == "true");
				updateActions(as);
			}
		}
		restoreOptions();

		if(stanza.attribute("type") == "set") {
			showPopup(tr("Settings for an account %1 are changed").arg(jid));
			const QString str = QString("<iq to='%1' type='result' id='%2' />").arg(accInfo->getJid(account), stanza.attribute("id"));
			stanzaSender->sendStanza(account, str);
		}
	}

	return foundSettings;
}

bool GmailNotifyPlugin::checkSharedStatus(int account, const QDomElement &stanza, const QDomElement &query)
{
	bool found = false;
	if (query.tagName() == "query"
	    && query.attribute("xmlns") == "google:shared-status") {
		found = true;
		const QString jid = stanza.attribute("to").split("/").at(0);
		const QString server = stanza.attribute("from").toLower();
		if(!server.isEmpty() && jid.toLower() != server) {
			return false;
		}
		AccountSettings *as = findAccountSettings(jid);
		if(!as || as->account != account) {
			return true;
		}
		QString type = stanza.attribute("type");
		if(type == "set")
			as->sharedStatuses.clear();
		if(query.hasAttribute("status-max"))
			as->statusMax = query.attribute("status-max").toInt();
		if(query.hasAttribute("status-list-contents-max"))
			as->listContentsMax = query.attribute("status-list-contents-max").toInt();
		if(query.hasAttribute("status-list-max"))
			as->listMax = query.attribute("status-list-max").toInt();

		if(type == "result" || type == "set") {
			for (QDomNode child = query.firstChild(); !child.isNull(); child = child.nextSibling()) {
				QDomElement settings = child.toElement();
				if(settings.isNull())
					continue;
				QString tagName = settings.tagName();
				if(tagName == "status")
					as->message = settings.text();
				else if(tagName == "show")
					as->status = settings.text().replace("default", "online");
				else if(tagName == "status-list") {
					QStringList l;
					for (QDomNode child = settings.firstChild(); !child.isNull(); child = child.nextSibling()) {
						QDomElement st = child.toElement();
						if(st.isNull() || st.tagName() != "status")
							continue;

						l.append(st.text());
					}
					if(!l.isEmpty())
						as->sharedStatuses.insert(settings.attribute("show").replace("default", "online"), l);
				}
			}
		}

		if(as->sharedStatuses.isEmpty())
			as->sharedStatuses.insert(as->status, QStringList(as->message));


		if(as->isSharedStatusEnabled) {
			//timer_->start();
			accountController->setStatus(account, as->status, as->message);
			showPopup(tr("Shared Status for an account %1 is updated").arg(jid));
		}
		if(type == "set") {
			const QString str = QString("<iq to='%1' type='result' id='%2' />").arg(accInfo->getJid(account), stanza.attribute("id"));
			stanzaSender->sendStanza(account, str);
		}
	}

	return found;
}

bool GmailNotifyPlugin::checkNoSave(int account, const QDomElement &stanza, const QDomElement &query)
{
	bool found = false;
	if(query.tagName() == "query"
	   && query.attribute("xmlns") == "google:nosave")
	{
		found = true;
		const QString jid = stanza.attribute("to").split("/").at(0);
		const QString server = stanza.attribute("from").toLower();
		if(!server.isEmpty() && jid.toLower() != server) {
			return false;
		}
		AccountSettings *as = findAccountSettings(jid);
		if(!as || as->account != account) {
			return true;
		}

		const QString type = stanza.attribute("type");
		for(QDomNode child = query.firstChild(); !child.isNull(); child = child.nextSibling()) {
			QDomElement noSave = child.toElement();
			if(noSave.isNull() || noSave.tagName() != "item")
				continue;

			const QString item = noSave.attribute("jid");
			bool state = (noSave.attribute("value") == "enabled");
			bool changed;
			if(!as->noSaveList.contains(item)) {
				changed = true;
			}
			else {
				changed = as->noSaveList.value(item) != state;
			}

			if(changed) {
				as->noSaveList.insert(item, state);
				actions_->updateAction(account, item, state);

				if(type == "set") {
					showPopup(tr("No-save state for contact %1 is changed").arg(item));
				}
			}

			if(type == "set") {
				const QString str = QString("<iq to='%1' type='result' id='%2' />").arg(accInfo->getJid(account), stanza.attribute("id"));
				stanzaSender->sendStanza(account, str);
			}
		}
	}

	return found;
}

bool GmailNotifyPlugin::checkAttributes(int account, const QDomElement &stanza, const QDomElement &query)
{
	bool found = false;
	if(query.tagName() == "query"
	   && query.attribute("xmlns") == "jabber:iq:roster"
	   && query.attribute("ext") == "2")
	{
		const QString jid = stanza.attribute("to").split("/").at(0);
		const QString server = stanza.attribute("from").toLower();
		if(!server.isEmpty() && jid.toLower() != server) {
			return false;
		}
		AccountSettings *as = findAccountSettings(jid);
		if(!as || as->account != account) {
			return true;
		}

		found = true;

		const QString type = stanza.attribute("type");
		if(type == "set") {
			const QString str = QString("<iq to='%1' type='result' id='%2' />").arg(accInfo->getJid(account), stanza.attribute("id"));
			stanzaSender->sendStanza(account, str);
		}

		for(QDomNode child = query.firstChild(); !child.isNull(); child = child.nextSibling()) {
			QDomElement itemElem = child.toElement();
			if(itemElem.isNull() || itemElem.tagName() != "item")
				continue;

			const QString jid = itemElem.attribute("jid");
			const QString descr = itemElem.attribute("t");

			Attributes atr;
			if(as->attributes.contains(jid)) {
				atr = as->attributes.value(jid);
			}
			if(atr.t != descr && type == "set") {
				showPopup(tr("Attributes for contact %1 are changed").arg(jid));
			}
			atr.t = descr;
			as->attributes.insert(jid, atr);
		}
	}

	return found;
}

AccountSettings* GmailNotifyPlugin::findAccountSettings(const QString &jid)
{
	if(!jid.isEmpty()) {
		foreach(AccountSettings* as, accounts) {
			if(as->jid == jid.toLower())
				return as;
		}
	}

	return 0;
}

AccountSettings* GmailNotifyPlugin::create(int account, QString jid)
{
	jid = jid.toLower();
	if(jid.contains("/"))
		jid = jid.split("/").first();

	AccountSettings *as = findAccountSettings(jid);
	if(!as) {
		as = new AccountSettings(account, jid);
		accounts.append(as);
	}
	else
		as->account = account;

	return as;
}

void GmailNotifyPlugin::changeNoSaveState(int account, QString jid, bool val)
{
	if(!Utils::checkAccount(account, accInfo))
		return;

	QString str = QString("<iq type='set' to='%1' id='%2'>"
			    "<query xmlns='google:nosave'>"
			    "<item xmlns='google:nosave' jid='%3' value='%4'/>"
			    "</query></iq>")
			.arg(accInfo->getJid(account), stanzaSender->uniqueId(account))
			.arg(jid, (val ? "enabled" : "disabled"));

	stanzaSender->sendStanza(account, str);
//	AccountSettings *as = findAccountSettings(accInfo->getJid(account));
//	if(as) {
//		as->noSaveList.insert(jid, (val ? "enabled" : "disabled"));
//	}
}

bool GmailNotifyPlugin::hasAccountSettings(int account)
{
	bool has = false;
	foreach(AccountSettings *as, accounts) {
		if(as->account == account) {
			has = true;
			break;
		}
	}

	return has;
}

void GmailNotifyPlugin::setStanzaSendingHost(StanzaSendingHost *host)
{
	stanzaSender = host;
}

void GmailNotifyPlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost* host)
{
	accInfo = host;
}

void GmailNotifyPlugin::setOptionAccessingHost(OptionAccessingHost* host)
{
	psiOptions = host;
}

void GmailNotifyPlugin::setPopupAccessingHost(PopupAccessingHost *host)
{
	popup = host;
}

void GmailNotifyPlugin::setPsiAccountControllingHost(PsiAccountControllingHost *host)
{
	accountController = host;
}

void GmailNotifyPlugin::setIconFactoryAccessingHost(IconFactoryAccessingHost *host)
{
	iconHost = host;
}

void GmailNotifyPlugin::setEventCreatingHost(EventCreatingHost *host)
{
	psiEvent = host;
}

void GmailNotifyPlugin::setSoundAccessingHost(SoundAccessingHost *host)
{
	sound_ = host;
}

void GmailNotifyPlugin::showPopup(const QString& text)
{
	int interval = popup->popupDuration(POPUP_OPTION);
	if(!interval)
		return;

	popup->initPopup(text, name(), "gmailnotify/menu", popupId);
}

void GmailNotifyPlugin::updateSharedStatus(AccountSettings* as)
{
	if(as->sharedStatuses.contains(as->status)) {
		QStringList l = as->sharedStatuses.value(as->status);
		if(l.contains(as->message)) {
			l.removeAll(as->message);
		}
		l.push_front(as->message);
		while(l.size() > as->listContentsMax) {
			l.removeLast();
		}
		as->sharedStatuses.insert(as->status, l);
	}
	else {
		as->sharedStatuses.insert(as->status, QStringList() << as->message);
		while(as->sharedStatuses.size() > as->listMax) {
			foreach(QString key, as->sharedStatuses.keys()) {
				if(key != as->status) {
					as->sharedStatuses.remove(key);
					break;
				}
			}
		}
	}
	Utils::updateSharedStatus(as, stanzaSender, accInfo);
}

QList < QVariantHash > GmailNotifyPlugin::getButtonParam()
{
	return QList < QVariantHash >();
}

QAction* GmailNotifyPlugin::getAction(QObject* parent, int account, const QString& contact)
{
	const QString bareJid = contact.split("/").first();
	QAction *act = actions_->newAction(parent, account, bareJid, iconHost->getIcon("gmailnotify/nohistory"));
	AccountSettings* as = findAccountSettings(accInfo->getJid(account));
	if(as) {
		act->setVisible(as->isNoSaveEnbaled && as->isNoSaveSupported && as->isArchivingEnabled);
		if(as->noSaveList.contains(bareJid))
			act->setChecked(as->noSaveList.value(bareJid));
	}
	return act;
}

void GmailNotifyPlugin::updateActions(AccountSettings *as)
{
	bool val = as->isNoSaveEnbaled && as->isNoSaveSupported && as->isArchivingEnabled;
	actions_->updateActionsVisibility(as->account, val);
}

void GmailNotifyPlugin::incomingMail(int account, const QDomElement &xml)
{
	MailItemsList l;
	QDomElement mail = xml.firstChildElement("mail-thread-info");
	while(!mail.isNull()) {
		MailItem mi;
		mi.url = mail.attribute("url");
		mi.subject = mail.firstChildElement("subject").text();
		mi.text = mail.firstChildElement("snippet").text();
		mi.account = accInfo->getJid(account);
		QDomElement senders = mail.firstChildElement("senders");
		QDomElement sender = senders.firstChildElement("sender");
		QStringList fl;
		while(!sender.isNull()) {
			QString from = sender.attribute("name") + " <" + sender.attribute("address") + ">";
			fl.append(from);
			sender = sender.nextSiblingElement("sender");
		}
		fl.removeDuplicates();
		mi.from = fl.join(", ");
		l.append(mi);
		mail = mail.nextSiblingElement("mail-thread-info");
	}
	if(l.isEmpty()) {
		return;
	}

	mailItems_.append(l);
	bool soundEnabled = psiOptions->getGlobalOption("options.ui.notifications.sounds.enable").toBool();
	if(soundEnabled) {
		psiOptions->setGlobalOption("options.ui.notifications.sounds.enable", false);
		playSound(soundFile);
	}

	QString popupMessage = tr("<b>mail.google.com - incoming mail!</b>");
	if(psiOptions->getGlobalOption("options.ui.notifications.passive-popups.showMessage").toBool()) {
		popupMessage += "<br><br>";
		foreach(const MailItem& i, l)
			popupMessage += ViewMailDlg::mailItemToText(i).replace("\n", "<br>") + "<br>";
	}
	if(mailViewer_ && mailViewer_->isActiveWindow()) {
		showPopup(popupMessage);
		mailEventActivated();

	}
	else {
		if(mailItems_.count() > 1) {
			showPopup(popupMessage);
		}
		else {
			psiEvent->createNewEvent(account, accInfo->getJid(account), popupMessage, this, SLOT(mailEventActivated()));
		}

		if(mailViewer_) {
			mailViewer_->setWindowTitle("*" + mailViewer_->caption());
		}
	}
	psiOptions->setGlobalOption("options.ui.notifications.sounds.enable", soundEnabled);

	if(!program_.isEmpty()) {
		QStringList prog = program_.split(" ");
		QProcess *p = new QProcess(this);
		p->startDetached(prog.takeFirst(), prog);
		p->deleteLater();
	}
}

void GmailNotifyPlugin::mailEventActivated()
{
	if(mailItems_.isEmpty()) {
		return;
	}

	if(!mailViewer_) {
		mailViewer_ = new ViewMailDlg(mailItems_.takeFirst(), iconHost);
	}

	while(!mailItems_.isEmpty()) {
		mailViewer_->appendItems(mailItems_.takeFirst());
	}

	mailViewer_->show();
	mailViewer_->raise();
	mailViewer_->activateWindow();
}

void GmailNotifyPlugin::playSound(const QString &file)
{
	sound_->playSound(file);
}

void GmailNotifyPlugin::checkSound()
{
	playSound(ui_.le_sound->text());
}

void GmailNotifyPlugin::getSound()
{
	QString fileName = QFileDialog::getOpenFileName(0,tr("Choose a sound file"),"", tr("Sound (*.wav)"));
	if(fileName.isEmpty())
		return;
	ui_.le_sound->setText(fileName);
}

void GmailNotifyPlugin::getProg()
{
	QString fileName = QFileDialog::getOpenFileName(0,tr("Choose a program"),"","");
	if(fileName.isEmpty())
		return;
	ui_.le_program->setText(fileName);
}

QAction* GmailNotifyPlugin::getContactAction(QObject *parent, int account, const QString &contact)
{
	QAction *act = 0;
	AccountSettings* as = findAccountSettings(accInfo->getJid(account));
	if(as && as->isAttributesEnabled && as->isAttributesSupported) {
		act = new QAction(iconHost->getIcon("psi/stop"), tr("Block gmail contact"), parent);
		act->setCheckable(true);
		if(as->attributes.contains(contact) && as->attributes.value(contact).t == "B")
			act->setChecked(true);
		act->setProperty("jid", contact);
		act->setProperty("account", account);
		connect(act, SIGNAL(triggered(bool)), SLOT(blockActionTriggered(bool)));
	}

	return act;
}

void GmailNotifyPlugin::blockActionTriggered(bool block)
{
	QAction* act = static_cast<QAction*>(sender());
	const QString jid = act->property("jid").toString();
	int acc = act->property("account").toInt();
	QString str = QString("<iq type='set' id='%1'>"
				"<query xmlns='jabber:iq:roster' xmlns:gr='google:roster' gr:ext='2'>"
				"<item jid='%2' gr:t='%3'/></query></iq>")
			.arg(stanzaSender->uniqueId(acc))
			.arg(jid, block ? "B" : "");
	stanzaSender->sendStanza(acc, str);
}

QString GmailNotifyPlugin::pluginInfo()
{
	return tr("Authors: ") +  "VampiRUS\nDealer_WeARE\n\n"
			+ trUtf8("Shows notifications of new messages in your Gmailbox.\n"
			 "Note: The plugin only checks the root of your Inbox folder in your"
			 " Gmailbox for new messages. When using server side mail filtering, you may not be notified about all new messages.");
}
