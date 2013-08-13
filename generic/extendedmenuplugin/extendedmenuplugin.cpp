/*
 * extendedmenuplugin.cpp - plugin
 * Copyright (C) 2010-2011  Khryukin Evgeny
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

#include <QMenu>
#include <QClipboard>
#include <QApplication>
#include <QDomElement>

#include "psiplugin.h"
#include "accountinfoaccessor.h"
#include "accountinfoaccessinghost.h"
#include "optionaccessor.h"
#include "optionaccessinghost.h"
#include "iconfactoryaccessor.h"
#include "iconfactoryaccessinghost.h"
#include "popupaccessor.h"
#include "popupaccessinghost.h"
#include "menuaccessor.h"
#include "plugininfoprovider.h"
#include "contactinfoaccessinghost.h"
#include "contactinfoaccessor.h"
#include "stanzafilter.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"
#include "toolbariconaccessor.h"

#include "ui_options.h"

#define cVer "0.1.2"

#define constInterval "intrvl"
#define constAction "action"
#define constMenu "menu"

#define POPUP_OPTION_NAME "Extended Menu Plugin"

static const QString pingString = "<iq from='%1' to='%2' type='get' id='%3'><ping xmlns='urn:xmpp:ping'/></iq>";
static const QString lastSeenString = "<iq from='%1' to='%2' type='get' id='%3'><query xmlns='jabber:iq:last'/></iq>";
static const QString timeString = "<iq from='%1' to='%2' type='get' id='%3'><time xmlns='urn:xmpp:time'/></iq>";

enum ActionType { NoAction = 0, CopyJid, CopyNick, CopyStatusMessage, RequestPing, RequestLastSeen, RequestTime };

class ExtendedMenuPlugin: public QObject, public PsiPlugin, public OptionAccessor,  public AccountInfoAccessor,
		public IconFactoryAccessor, public PopupAccessor, public MenuAccessor, public PluginInfoProvider,
		public ContactInfoAccessor, public StanzaSender, public StanzaFilter, public ToolbarIconAccessor
{
	Q_OBJECT
	Q_INTERFACES(PsiPlugin  AccountInfoAccessor OptionAccessor IconFactoryAccessor PopupAccessor  MenuAccessor
		     ContactInfoAccessor PluginInfoProvider StanzaFilter StanzaSender ToolbarIconAccessor)

public:
	ExtendedMenuPlugin();
	virtual QString name() const;
	virtual QString shortName() const;
	virtual QString version() const;
        virtual QWidget* options();
	virtual Priority priority();
	virtual bool enable();
        virtual bool disable();
        virtual void applyOptions();
        virtual void restoreOptions();
	virtual void setAccountInfoAccessingHost(AccountInfoAccessingHost* host);
	virtual void setOptionAccessingHost(OptionAccessingHost* host);
	virtual void optionChanged(const QString& ) {}
	virtual void setIconFactoryAccessingHost(IconFactoryAccessingHost* host);
	virtual void setPopupAccessingHost(PopupAccessingHost* host);
	virtual QList < QVariantHash > getAccountMenuParam();
	virtual QList < QVariantHash > getContactMenuParam();
	virtual QAction* getContactAction(QObject* , int , const QString& );
	virtual QAction* getAccountAction(QObject* , int ) { return 0; }
	virtual QList < QVariantHash > getButtonParam();
	virtual QAction* getAction(QObject* parent, int account, const QString& contact);
	virtual void setContactInfoAccessingHost(ContactInfoAccessingHost* host);
	virtual void setStanzaSendingHost(StanzaSendingHost *host);
	virtual bool incomingStanza(int account, const QDomElement& xml);
	virtual bool outgoingStanza(int , QDomElement &) { return false; }
	virtual QString pluginInfo();
	virtual QIcon icon() const;

private slots:
	void menuActivated();
	void toolbarActionActivated();

private:
        bool enabled;
	OptionAccessingHost* psiOptions;
	AccountInfoAccessingHost *accInfo;
	IconFactoryAccessingHost *icoHost;
	PopupAccessingHost* popup;
	ContactInfoAccessingHost* contactInfo;
	StanzaSendingHost* stanzaSender;
	bool enableMenu, enableAction;
	int popupId;

	Ui::Options ui_;

	struct Request {

		Request(const QString& id_, ActionType type_, const QTime& t = QTime::currentTime())
			: id(id_), time(t), type(type_)
		{
		}

		bool operator==(const Request& other)
		{
			return id == other.id;
		}

		QString id;
		QTime time;
		ActionType type;
	};

	typedef QList<Request> Requests;
	QHash<int, Requests> requestList_;

	void showPopup(const QString& text, const QString& title);
	void fillMenu(QMenu* m, int account, const QString& jid);
	void addRequest(int account, const Request& r);
	void doCommand(int account, const QString &jid, const QString& command, ActionType type_);
};

Q_EXPORT_PLUGIN(ExtendedMenuPlugin)

ExtendedMenuPlugin::ExtendedMenuPlugin()
	: enabled(false)
	, psiOptions(0)
	, accInfo(0)
	, icoHost(0)
	, popup(0)
	, contactInfo(0)
	, stanzaSender(0)
	, enableMenu(true)
	, enableAction(false)
	, popupId(0)
{
}

QString ExtendedMenuPlugin::name() const
{
	return "Extended Menu Plugin";
}

QString ExtendedMenuPlugin::shortName() const
{
	return "extmenu";
}

QString ExtendedMenuPlugin::version() const
{
        return cVer;
}

PsiPlugin::Priority ExtendedMenuPlugin::priority()
{
	return PsiPlugin::PriorityLow;
}

bool ExtendedMenuPlugin::enable()
{
	enabled = true;
	requestList_.clear();
	enableMenu = psiOptions->getPluginOption(constMenu, enableMenu).toBool();
	enableAction = psiOptions->getPluginOption(constAction, enableAction).toBool();
	int interval = psiOptions->getPluginOption(constInterval, QVariant(5000)).toInt()/1000;
	popupId = popup->registerOption(POPUP_OPTION_NAME, interval, "plugins.options."+shortName()+"."+constInterval);

	QFile f(":/icons/icons/ping.png");
	f.open(QIODevice::ReadOnly);
	icoHost->addIcon("menu/ping", f.readAll());
	f.close();

	f.setFileName(":/icons/icons/copyjid.png");
	f.open(QIODevice::ReadOnly);
	icoHost->addIcon("menu/copyjid", f.readAll());
	f.close();

	f.setFileName(":/icons/icons/copynick.png");
	f.open(QIODevice::ReadOnly);
	icoHost->addIcon("menu/copynick", f.readAll());
	f.close();

	f.setFileName(":/icons/icons/copystatusmsg.png");
	f.open(QIODevice::ReadOnly);
	icoHost->addIcon("menu/copystatusmsg", f.readAll());
	f.close();

	f.setFileName(":/icons/extendedmenu.png");
	f.open(QIODevice::ReadOnly);
	icoHost->addIcon("menu/extendedmenu", f.readAll());
	f.close();

	return enabled;
}

bool ExtendedMenuPlugin::disable()
{
        enabled = false;
	requestList_.clear();

	popup->unregisterOption(POPUP_OPTION_NAME);

	return true;
}

QWidget* ExtendedMenuPlugin::options()
{
	if(enabled) {
		QWidget *w = new QWidget();
		ui_.setupUi(w);
		restoreOptions();
		return w;
	}

	return 0;
}

void ExtendedMenuPlugin::applyOptions()
{
	enableMenu = ui_.cb_menu->isChecked();
	psiOptions->setPluginOption(constMenu, enableMenu);
	enableAction = ui_.cb_action->isChecked();
	psiOptions->setPluginOption(constAction, enableAction);
}

void ExtendedMenuPlugin::restoreOptions()
{
	ui_.cb_action->setChecked(enableAction);
	ui_.cb_menu->setChecked(enableMenu);
}

void ExtendedMenuPlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost* host)
{
	accInfo = host;
}

void ExtendedMenuPlugin::setOptionAccessingHost(OptionAccessingHost* host)
{
	psiOptions = host;
}

void ExtendedMenuPlugin::setIconFactoryAccessingHost(IconFactoryAccessingHost *host)
{
	icoHost = host;
}

void ExtendedMenuPlugin::setPopupAccessingHost(PopupAccessingHost* host)
{
	popup = host;
}

void ExtendedMenuPlugin::setContactInfoAccessingHost(ContactInfoAccessingHost *host)
{
	contactInfo = host;
}

void ExtendedMenuPlugin::setStanzaSendingHost(StanzaSendingHost *host)
{
	stanzaSender = host;
}

static const QString secondsToString(ulong seconds)
{
	QString res;
	int sec = seconds%60;
	int min = ((seconds-sec)/60)%60;
	int h = ((((seconds-sec)/60)-min)/60)%24;
	int day = (((((seconds-sec)/60)-min)/60)-h)/24;
	if(day) {
		res += QObject::tr("%n day(s) ", "", day);
	}
	if(h) {
		res += QObject::tr("%n hour(s) ", "", h);
	}
	if(min) {
		res += QObject::tr("%n minute(s) ", "", min);
	}
	if(sec) {
		res += QObject::tr("%n second(s) ", "", sec);
	}
	return res;
}

static int stringToInt(const QString& str)
{
	int sign = (str.at(0) == '-') ? -1 : 1;
	int result = str.mid(1,2).toInt()*sign;
	return result;
}

static QDomElement getFirstChildElement(const QDomElement &elem)
{
	QDomElement newElem;
	QDomNode node = elem.firstChild();
	while(!node.isNull()) {
		if(!node.isElement()) {
			node = node.nextSibling();
			continue;
		}

		newElem = node.toElement();
		break;
	}

	return newElem;
}

bool ExtendedMenuPlugin::incomingStanza(int account, const QDomElement &xml)
{
	if(enabled) {
		if(xml.tagName() == "iq" && xml.hasAttribute("id")) {
			if(requestList_.contains(account)) {
				Requests rl = requestList_.value(account);
				foreach(const Request& r, rl) {
					if(r.id == xml.attribute("id")) {
						const QString jid = xml.attribute("from");
						QString name;
						if(contactInfo->isPrivate(account, jid)) {
							name = jid;
						}
						else {
							name = contactInfo->name(account, jid.split("/").first());
						}
						switch(r.type) {
							case(RequestPing):
							{
								const QString title = tr("Ping %1").arg(name);
								if(xml.attribute("type") == "result") {
									double msecs = ((double)r.time.msecsTo(QTime::currentTime()))/1000;
									showPopup(tr("Pong from %1 after %2 secs")
										  .arg(jid)
										  .arg(QString::number(msecs, 'f', 3)),
										  title);
								}
								else {
									showPopup(tr("Feature not implemented"),
										  title);
								}
								break;
							}
							case(RequestLastSeen):
							{
								const QString title = tr("%1 Last Activity").arg(name);
								QDomElement query = xml.firstChildElement("query");
								if(!query.isNull()
									&& query.attribute("xmlns") == "jabber:iq:last"
									&& query.hasAttribute("seconds"))
								{
									ulong secs = query.attribute("seconds").toInt();
									QString text;
									if(secs) {
										if(jid.contains("@")) {
											if(jid.contains("/")) {
												text = tr("%1 Last Activity was %2 ago").arg(jid, secondsToString(secs));
											}
											else {
												text = tr("%1 went offline %2 ago").arg(jid, secondsToString(secs));
											}
										}
										else {
											text = tr("%1 uptime is %2").arg(jid, secondsToString(secs));
										}
									}
									else {
										text = tr("%1 is online!").arg(jid);
									}
									showPopup(text, title);
								}
								else {
									QDomElement error = xml.firstChildElement("error");
									if(!error.isNull()) {
										QString text = tr("Unknown error!");
										QString tagName = getFirstChildElement(error).tagName();
										if(!tagName.isEmpty()) {
											if(tagName == "service-unavailable") {
												text = tr("Service unavailable");
											}
											else if(tagName == "feature-not-implemented") {
												text = tr("Feature not implemented");
											}
											else if(tagName == "not-allowed" || tagName == "forbidden") {
												text = tr("You are not authorized to retrieve Last Activity information");
											}
										}
										showPopup(text, title);
									}
								}
								break;
							}
							case(RequestTime):
							{
								QDomElement time = xml.firstChildElement("time");
								if(!time.isNull()) {
									const QString title = tr("%1 Time").arg(name);
									QDomElement utc = time.firstChildElement("utc");
									if(!utc.isNull()) {
										QString zone = time.firstChildElement("tzo").text();
										QDateTime dt = QDateTime::fromString(utc.text(), Qt::ISODate);
										dt = dt.addSecs(stringToInt(zone)*3600);
										showPopup(tr("%1 time is %2").arg(jid, dt.toString("yyyy-MM-dd hh:mm:ss")), title);
									}
									else if(!xml.firstChildElement("error").isNull()) {
										showPopup(tr("Feature not implemented"), title);
									}
								}
								break;
							}
							default:
								break;
						}
						rl.removeAll(r);
						if(!rl.isEmpty()) {
							requestList_.insert(account, rl);
						}
						else {
							requestList_.remove(account);
						}
						break;
					}
				}
			}
		}
	}

	return false;
}

void ExtendedMenuPlugin::showPopup(const QString &text, const QString &title)
{
	int interval = popup->popupDuration(POPUP_OPTION_NAME);
	if(interval) {
		popup->initPopup(text, title, "psi/headline", popupId);
	}
}

QList < QVariantHash > ExtendedMenuPlugin::getButtonParam()
{
	return QList < QVariantHash >();
}

QAction* ExtendedMenuPlugin::getAction(QObject *parent, int account, const QString &contact)
{
	if(!enableAction) {
		return 0;
	}
	QAction* act = new QAction(icoHost->getIcon("menu/extendedmenu"), tr("Extended Actions"), parent);
	act->setProperty("account", account);
	act->setProperty("jid", contact);
	connect(act, SIGNAL(triggered()), SLOT(toolbarActionActivated()));
	return act;
}

QList < QVariantHash > ExtendedMenuPlugin::getAccountMenuParam()
{
	return QList < QVariantHash >();
}

QList < QVariantHash > ExtendedMenuPlugin::getContactMenuParam()
{
	return QList < QVariantHash >();
}

QAction* ExtendedMenuPlugin::getContactAction(QObject* obj, int account, const QString& jid)
{
	if(!enableMenu) {
		return 0;
	}
	QMenu *parent = qobject_cast<QMenu*>(obj);
	if(parent) {
		QMenu *menu = parent->addMenu(icoHost->getIcon("menu/extendedmenu"), tr("Extended Actions"));
		fillMenu(menu, account, jid);
	}
	return 0;
}

inline void setupAction(QAction* act, int account, const QString& jid, ActionType type)
{
	act->setProperty("jid", jid);
	act->setProperty("account", account);
	act->setProperty("type", type);
}

void ExtendedMenuPlugin::fillMenu(QMenu *menu, int account, const QString& jid)
{
	bool isOnline = (accInfo->getStatus(account) != "offline");

	QAction *copyJidAction = menu->addAction(icoHost->getIcon("menu/copyjid"), tr("Copy JID"), this, SLOT(menuActivated()));
	setupAction(copyJidAction, account, jid, CopyJid);

	QAction *copyNickAction = menu->addAction(icoHost->getIcon("menu/copynick"), tr("Copy Nick"), this, SLOT(menuActivated()));
	setupAction(copyNickAction, account, jid, CopyNick);

	QAction *copyStatusMessageAction = menu->addAction(icoHost->getIcon("menu/copystatusmsg"), tr("Copy Status Message"), this, SLOT(menuActivated()));
	setupAction(copyStatusMessageAction, account, jid, CopyStatusMessage);

	QAction *pingAction = menu->addAction(icoHost->getIcon("menu/ping"), tr("Ping"), this, SLOT(menuActivated()));
	setupAction(pingAction, account, jid, RequestPing);
	pingAction->setEnabled(isOnline);

	QAction *lastSeenAction = menu->addAction(icoHost->getIcon("psi/search"), tr("Last Activity"), this, SLOT(menuActivated()));
	setupAction(lastSeenAction, account, jid, RequestLastSeen);
	lastSeenAction->setEnabled(isOnline);

	QAction *timeAction = menu->addAction(icoHost->getIcon("psi/notification_chat_time"), tr("Entity Time"), this, SLOT(menuActivated()));
	setupAction(timeAction, account, jid, RequestTime);
	timeAction->setEnabled(isOnline);
}

void ExtendedMenuPlugin::toolbarActionActivated()
{
	QAction *act = static_cast<QAction*>(sender());
	const QString jid = act->property("jid").toString();
	int account = act->property("account").toInt();
	QMenu m;
	m.setStyleSheet(((QWidget*)(act->parent()))->styleSheet());
	fillMenu(&m, account, jid);
	m.exec(QCursor::pos());
}

void ExtendedMenuPlugin::menuActivated()
{
	QAction *act = qobject_cast<QAction*>(sender());
	QString jid = act->property("jid").toString();
	int account = act->property("account").toInt();
	if(!contactInfo->isPrivate(account, jid) && jid.contains("/")) {
		jid = jid.split("/").first();
	}

	ActionType type = (ActionType)act->property("type").toInt();
	QString command;

	switch(type) {
	case CopyJid:
		QApplication::clipboard()->setText(jid);
		return;
	case CopyNick:
		QApplication::clipboard()->setText(contactInfo->name(account, jid));
		return;
	case CopyStatusMessage:
		QApplication::clipboard()->setText(contactInfo->statusMessage(account, jid));
		return;
	case RequestPing:
		command = pingString;
		break;
	case RequestTime:
		command = timeString;
		break;
	case RequestLastSeen:
		command = lastSeenString;
		break;
	default:
		return;
	}


	if(contactInfo->isPrivate(account, jid)) {
		doCommand(account, jid, command, type);
	}
	else {
		QStringList res = contactInfo->resources(account, jid);
		if(type == RequestLastSeen && res.isEmpty()) {
			doCommand(account, jid, command, type);
		}
		else {
			foreach(const QString& resource, res) {
				QString fullJid = jid;
				if(!resource.isEmpty()) {
					fullJid += QString("/") + resource;
				}
				doCommand(account, fullJid, command, type);
			}
		}
	}
}

void ExtendedMenuPlugin::doCommand(int account, const QString &jid, const QString& command, ActionType type)
{
	if(jid.isEmpty())
		return;

	const QString id = stanzaSender->uniqueId(account);
	QString str = command.arg(accInfo->getJid(account), stanzaSender->escape(jid), id);
	addRequest(account, Request(id, type));
	stanzaSender->sendStanza(account, str);
}

void ExtendedMenuPlugin::addRequest(int account, const Request &r)
{
	Requests rl = requestList_.value(account);
	rl.push_back(r);
	requestList_.insert(account, rl);
}

QString ExtendedMenuPlugin::pluginInfo()
{
	return tr("Author: ") +  "Dealer_WeARE\n"
			+ tr("Email: ") + "wadealer@gmail.com\n\n"
			+ tr("This plugin adds several additional commands into contacts context menu.");
}

QIcon ExtendedMenuPlugin::icon() const
{
	return QIcon(":/icons/extendedmenu.png");
}

#include "extendedmenuplugin.moc"
