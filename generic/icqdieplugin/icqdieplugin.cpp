/*
 * icqdieplugin.cpp - plugin
 * Copyright (C) 2009  Ivan Borzenkov <ivan1986@list.ru>
 *
 * THE BEER-WARE LICENSE (Revision 42):
 * I wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy a beer in return.
 *
 * Alternatively, this file may be used under the terms of the GNU
 * General Public License version 2 or (at your option) any later version
 * or GNU Lesser General Public License version 2 or (at your option) any 
 * later version as published by the Free Software Foundation and 
 * appearing in the file copying.txt included in the packaging of this file.
 * Please review the following information to ensure the GNU General 
 * Public License version 2.0 requirements will be met: 
 * <http://www.gnu.org/licenses/>.
 */

#include <QtGui>
#include <QtCore>
#include <QMap>

#include "psiplugin.h"
#include "optionaccessor.h"
#include "optionaccessinghost.h"
#include "stanzafilter.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"
#include "activetabaccessor.h"
#include "activetabaccessinghost.h"
#include "accountinfoaccessor.h"
#include "accountinfoaccessinghost.h"
#include "plugininfoprovider.h"
#include "ui_icqdieoptions.h"

#define cVer "0.1.5"
#define constMessageRecv "msgr"
#define constMessageNoRecv "msgnr"
#define constCustom "custom"
#define constActiveTab "actvtb"
#define constPauseTime "whttm"
#define constMessageCount "msgcnt"
#define constTransports "transp"

class IcqDie: public QObject, public PsiPlugin, public OptionAccessor, public StanzaSender,  public StanzaFilter, public ActiveTabAccessor,
		public AccountInfoAccessor, public PluginInfoProvider
{
	Q_OBJECT
		Q_INTERFACES(PsiPlugin OptionAccessor StanzaSender StanzaFilter ActiveTabAccessor
			     AccountInfoAccessor PluginInfoProvider)

public:
	IcqDie();
	virtual QString name() const;
	virtual QString shortName() const;
	virtual QString version() const;
	virtual QWidget* options();
	virtual bool enable();
	virtual bool disable();
	virtual void applyOptions();
	virtual void restoreOptions();
	virtual void setOptionAccessingHost(OptionAccessingHost* host);
	virtual void optionChanged(const QString& option);
	virtual void setStanzaSendingHost(StanzaSendingHost *host);
	virtual bool incomingStanza(int account, const QDomElement& stanza);
	virtual bool outgoingStanza(int account, QDomElement& stanza);
	virtual void setActiveTabAccessingHost(ActiveTabAccessingHost* host);
	virtual void setAccountInfoAccessingHost(AccountInfoAccessingHost* host);
	virtual QString pluginInfo();

private:
	bool enabled;
	AccountInfoAccessingHost* AccInfoHost;
	ActiveTabAccessingHost* ActiveTabHost;
	OptionAccessingHost* psiOptions;
	StanzaSendingHost* StanzaHost;
	QString MessageRecv;
	QString MessageNoRecv;
	typedef QPair<QDateTime,int> CounterInfo;
	QMap<QString, CounterInfo> Counter;
	enum stat {
		ignore = '-', send = '+', block = '!',
	};
	typedef QMap<QString, stat> CustomList;
	CustomList Custom;
	QVector<QString> Transports;
	CustomList ParseCustomText(QString sCustom);
	int PauseTime;
	int MessageCount;
	bool ActiveTabIsEnable;
	Ui::options ui;

};

Q_EXPORT_PLUGIN(IcqDie);

IcqDie::IcqDie()
{
	ActiveTabIsEnable = true;

	Custom.clear();
	Custom["other"] = send;
	Custom["nil"] = ignore;

	Counter.clear();

	Transports.clear();
	Transports << "icq" << "jit";

	PauseTime = 120;
	MessageCount = 0;

	enabled = false;
	MessageRecv = trUtf8("Я Вам как Linux скажу, только Вы не обижайтесь. "
			  "Этот человек, конечно, получит Ваше сообщение, но лучше бы Вам общаться с ним через Jabber. "
			  "А то не ровен час - аська сдохнет, старушка своё отжила. Его JID: %1.\n\n"
			  "Искренне Ваш, Debian Sid.");
	MessageNoRecv = trUtf8("Я Вам как Linux скажу, только Вы не обижайтесь. "
			  "Этот человек имел в виду всех пользователей аськи, поэтому если Вы до сих пор сидите в этой сети, "
			  "то он не получит Ваше сообщение, поэтому Вам придётся общаться с ним через Jabber. Его JID: %1.\n\n"
			  "Если Вы не знаете что такое Jabber, то есть Google - он всё знает и поможет любому, кто к нему обратится.\n"
			  "Искренне Ваш, Debian Sid.");

	ActiveTabHost = 0;
	AccInfoHost = 0;
	psiOptions = 0;
	StanzaHost = 0;
}

QString IcqDie::name() const { return "Icq Must Die Plugin"; }

QString IcqDie::shortName() const { return "icqdie"; }

QString IcqDie::version() const { return cVer; }

IcqDie::CustomList IcqDie::ParseCustomText(QString sCustom)
{
	IcqDie::CustomList Custom;
	Custom.clear();
	QStringList Clist;
	Clist = sCustom.split(QRegExp("\n"), QString::SkipEmptyParts);
	while(!Clist.isEmpty())
	{
		//удаляем пробелы и комментарии
		QString C = Clist.takeFirst().remove(QRegExp("\\s+")).remove(QRegExp("\\#.*$"));
		stat s;
		QString id = C;
		id.remove(0,1);
		if (C[0] == '-')
			s = ignore;
		else if (C[0] == '!')
			s = block;
		else if (C[0] == '+')
			s = send;
		else
		{
			s = send;
			//прилепляем назад
			id = C[0] + id;
		}
		Custom[id] = s;
	}
	//если удалили дефолтовые, то восстанавливаем
	if (Custom.find("nil") == Custom.end())
		Custom["nil"] = ignore;
	if (Custom.find("other") == Custom.end())
		Custom["other"] = send;
	//qDebug() << "ParseCustomText" << Custom;
	return Custom;
}

bool IcqDie::enable() {
	if (!psiOptions)
		return enabled;

	enabled = true;

	MessageRecv = psiOptions->getPluginOption(constMessageRecv, QVariant(MessageRecv)).toString();
	MessageNoRecv = psiOptions->getPluginOption(constMessageNoRecv, QVariant(MessageNoRecv)).toString();

	PauseTime = psiOptions->getPluginOption(constPauseTime, QVariant(PauseTime)).toInt();
	MessageCount = psiOptions->getPluginOption(constMessageCount, QVariant(MessageCount)).toInt();
	ActiveTabIsEnable = psiOptions->getPluginOption(constActiveTab, QVariant(ActiveTabIsEnable)).toBool();

	QVariant vCustom;
	vCustom = psiOptions->getPluginOption(constCustom);
	if (!vCustom.isNull())
		Custom = ParseCustomText(vCustom.toString());

	QVariant vTransports;
	vTransports = psiOptions->getPluginOption(constTransports);
	if (!vTransports.isNull())
	{
		QString sTransports = vTransports.toString();
		Transports.clear();
		QStringList Tlist;
		Tlist = sTransports.split(QRegExp("\n"), QString::SkipEmptyParts);
		while(!Tlist.isEmpty())
			Transports << Tlist.takeFirst().remove(QRegExp("\\s+"));
	}

	return enabled;
}

bool IcqDie::disable()
{
	enabled = false;
	return true;
}

void IcqDie::applyOptions()
{
	psiOptions->setPluginOption(constMessageRecv, QVariant(MessageRecv = ui.messageRecv->toPlainText()));
	psiOptions->setPluginOption(constMessageNoRecv, QVariant(MessageNoRecv = ui.messageNoRecv->toPlainText()));

	QString sCustom;
	psiOptions->setPluginOption(constCustom, QVariant(sCustom = ui.custom->toPlainText()));
	Custom = ParseCustomText(sCustom);

	psiOptions->setPluginOption(constActiveTab, QVariant(ActiveTabIsEnable = ui.activetabWidget->isChecked()));
	psiOptions->setPluginOption(constMessageCount, QVariant(MessageCount = ui.messageCount->value()));
	psiOptions->setPluginOption(constPauseTime, QVariant(PauseTime = ui.pauseWidget->value()));

	//сохранили и разобрали парсеры
	QString sTransports;
	psiOptions->setPluginOption(constTransports, QVariant(sTransports = ui.transportsWidget->toPlainText()));

	Transports.clear();
	QStringList Tlist;
	Tlist = sTransports.split(QRegExp("\n"), QString::SkipEmptyParts);
	while(!Tlist.isEmpty())
		Transports << Tlist.takeFirst().remove(QRegExp("\\s+"));
}

void IcqDie::restoreOptions()
{

	ui.messageRecv->setText(MessageRecv);
	ui.messageNoRecv->setText(MessageNoRecv);
	ui.messageCount->setValue(MessageCount);
	ui.pauseWidget->setValue(PauseTime);
	ui.activetabWidget->setChecked(ActiveTabIsEnable);

	ui.custom->setText(psiOptions->getPluginOption(constCustom, QVariant("+other\n-nil")).toString());

	QString text;
	foreach(QString t,Transports)
	{
		if (!text.isEmpty())
			text += "\n";
		text += t;
	}
	ui.transportsWidget->setText(text);
}

QWidget* IcqDie::options()
{
	if (!enabled)
		return 0;

	QWidget *options = new QWidget;
	ui.setupUi(options);
	ui.wiki->setText(tr("<a href=\"http://psi-plus.com/wiki/plugins#icq_must_die_plugin\">Wiki (Online)</a>"));
	ui.wiki->setOpenExternalLinks(true);

	restoreOptions();

	return options;
}

void IcqDie::setOptionAccessingHost(OptionAccessingHost* host) { psiOptions = host; }

void IcqDie::optionChanged(const QString& option) { Q_UNUSED(option); }

void IcqDie::setStanzaSendingHost(StanzaSendingHost *host) { StanzaHost = host; }

bool IcqDie::incomingStanza(int account, const QDomElement& stanza)
{
	if (!enabled)
		return false;
	if (stanza.tagName() != "message")
		return false;

	//реагируем только на чат и на сообщение
	QString type = stanza.attribute("type");
	if(type != "chat" && type != "")
		return false;

	QDomElement Body = stanza.firstChildElement("body");
	//если пустое сообщение, то ничего не делаем
	if(Body.isNull())
		return false;
	QDomElement rec =  stanza.firstChildElement("received");
	if(!rec.isNull())
		return false;

	QString from = stanza.attribute("from"); QStringList f = from.split("/");
	QString valF = f.takeFirst(); QStringList fid = valF.split("@");
	if (fid.count() < 2)
		return false;
	QString idF = fid.takeFirst();
	QString server = fid.takeFirst();
	QString to = stanza.attribute("to"); QStringList t = to.split("/");
	QString valT = t.takeFirst();

	//игнорируем сообщения от всех, кромя транспортов
	bool fromTransport = false;
	foreach(QString Transport, Transports)
		if (server.indexOf(Transport, Qt::CaseInsensitive) == 0)
			fromTransport = true;
	if(!fromTransport)
		return false;

	//разбираемся от кого оно
	stat todo = Custom["nil"];
	if (Custom.find(idF) != Custom.end()) //нашли в списке - делаем что указано
		todo = Custom[idF];
	else //проверяем, есть ли он в ростере
	{
		QStringList Roster = AccInfoHost->getRoster(account);
		while(!Roster.isEmpty())
		{
			QString jid = Roster.takeFirst();
			if(valF.toLower() == jid.toLower())
				todo = Custom["other"];
		}
	}

	//если игнорировать - пропускаем и ничего
	if (todo == ignore)
		return false;

	//если блокировать - отправляем сообщение и давим его
	if (todo == block)
	{
		if(!Counter.contains(from))
			Counter[from].second = 0;
		Counter[from].second++;
		//не посылать больше N раз
		if (MessageCount > 0 && Counter[from].second > MessageCount)
			return true;

		QString mes = "<message to='" + from + "'";
		if(type != "")
			mes += " type='" + type + "'";
		else
			mes += "><subject>IcqDie</subject";
		mes += "><body>" + MessageNoRecv.arg(valT) + "</body></message>";
		StanzaHost->sendStanza(account, mes);
		return true;
	}

	//если уже посылали, то таймаут
	if(!Counter.contains(from))
		Counter[from].first = QDateTime::currentDateTime();
	else
	{
		QDateTime old = Counter[from].first;
		Counter[from].first = QDateTime::currentDateTime();
		if(QDateTime::currentDateTime().secsTo(old) >= -PauseTime*60)
			return false;
	}

	//если пришло сообщение в активный чат, то не посылать
	if(ActiveTabIsEnable)
	{
		QString getJid = ActiveTabHost->getJid();
		if(getJid.toLower() == from.toLower())
			return false;
	}

	if(!Counter.contains(from))
		Counter[from].second = 0;
	Counter[from].second++;
	//не посылать больше N раз
	if (MessageCount > 0 && Counter[from].second > MessageCount)
		return false;

	//отправляем сообщение
	QString mes = "<message to='" + from + "'";
	if(type != "")
		mes += " type='" + type + "'";
	else
		mes += "><subject>IcqDie</subject";
	mes += "><body>" + MessageRecv.arg(valT) + "</body></message>";
	StanzaHost->sendStanza(account, mes);

	return false;
}

bool IcqDie::outgoingStanza(int account, QDomElement& stanza)
{
	return false;
}

void IcqDie::setActiveTabAccessingHost(ActiveTabAccessingHost* host) {
	ActiveTabHost = host;
}

void IcqDie::setAccountInfoAccessingHost(AccountInfoAccessingHost* host) {
	AccInfoHost = host;
}

QString IcqDie::pluginInfo() {
	return tr("Author: ") +  "ivan1986\n\n"
			+ trUtf8("This plugin is designed to help you transfer as many contacts as possible from ICQ to Jabber.\n"
			 "The plugin has a number of simple settings that can help you:\n"
			 "* set a special message text\n"
			 "* exclude specific ICQ numbers\n"
			 "* set the time interval after which the message will be repeated\n"
			 "* set the max count of messages by contact\n"
			 "* disable the message for the active window/tab\n"
			 "* disable messages for contacts that are not in your roster");
}

#include "icqdieplugin.moc"
