/*
 * autoreplyplugin.cpp - plugin
 * Copyright (C) 2009-2010  Khryukin Evgeny
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

#include <QSpinBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QDomElement>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

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

#define cVer "0.3.2"
#define constMessage "mssg"
#define constDisableFor "dsblfr"
#define constEnableFor "enblfr"
#define constTimes "tms"
#define constActiveTab "actvtb"
#define constResetTime "rsttm"
#define constEnableDisable "enbldsbl"
#define constDisableForAcc "dsblfracc"
#define constSOnline "online"
#define constSAway "away"
#define constSChat "chat"
#define constSDnd "dnd"
#define constSInvis "invis"
#define constSXa "xa"
#define constNotInRoster "ntnrstr"

class AutoReply: public QObject, public PsiPlugin, public OptionAccessor, public StanzaSender,	public StanzaFilter,
				 public ActiveTabAccessor, public AccountInfoAccessor, public PluginInfoProvider
{
	Q_OBJECT
#ifdef HAVE_QT5
	Q_PLUGIN_METADATA(IID "com.psi-plus.AutoReplyPlugin")
#endif
	Q_INTERFACES(PsiPlugin OptionAccessor StanzaSender StanzaFilter ActiveTabAccessor AccountInfoAccessor PluginInfoProvider)

	public:
	AutoReply();
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
	virtual QPixmap icon() const;

private:
	bool enabled;
	AccountInfoAccessingHost* AccInfoHost;
	ActiveTabAccessingHost* ActiveTabHost;
	OptionAccessingHost* psiOptions;
	StanzaSendingHost* StanzaHost;
	QTextEdit *messageWidget;
	QTextEdit *disableforWidget;
	QString Message;
	QString DisableFor;
	QSpinBox *spinWidget;
	QSpinBox *resetWidget;
	QCheckBox *activetabWidget;
	QComboBox *enabledisableWidget;
	QTextEdit *DisableForAccWidget;
	QCheckBox *sonlineWidget;
	QCheckBox *sawayWidget;
	QCheckBox *sdndWidget;
	QCheckBox *sxaWidget;
	QCheckBox *schatWidget;
	QCheckBox *sinvisWidget;
	QCheckBox *NotInRosterWidget;
	bool NotInRoster;
	int EnableDisable;
	struct Base {
		int Account;
		QString Jid;
		int count;
		QDateTime LastMes;
	};
	QVector<Base> Counter;
	int Times;
	int ResetTime;
	bool ActiveTabIsEnable;
	bool SOnline;
	bool SAway;
	bool SDnd;
	bool SXa;
	bool SChat;
	bool SInvis;
	QString DisableForAcc;
	bool FindAcc(int account, QString jid, int &i);

private slots:
	void setEnableDisableText(int Arg);
};

#ifndef HAVE_QT5
Q_EXPORT_PLUGIN(AutoReply);
#endif

AutoReply::AutoReply() {
	ActiveTabIsEnable = true;
	NotInRosterWidget = 0;
	NotInRoster = true;
	EnableDisable = 1;
	Counter.clear();
	Times = 2;
	ResetTime = 5;
	spinWidget = 0;
	activetabWidget = 0;
	DisableForAcc = "";
	DisableForAccWidget = 0;
	resetWidget = 0;
	DisableFor = "juick@juick.com\npsi-dev@conference.jabber.ru\njubo@nologin.ru\njabrss@cmeerw.net\nrss2jabber.com\nbot.talk.google.com\nbot.rambler.ru\nnotify@planary.ru\nwebtoim@gmail.com\nwebtoim1@gmail.com\narx-bot-11@onblabla.ru\nen2ru@jtalk.ru\nru2en@jtalk.ru\ngluxi@inhex.net\nisida@xmpp.ru\ntwitter.tweet.im\nrss@isida-bot.com\nhuti.ua@gmail.com";
	enabled = false;
	Message = "I'll write you later...";
	messageWidget = 0;
	ActiveTabHost = 0;
	AccInfoHost = 0;
	disableforWidget = 0;
	psiOptions = 0;
	StanzaHost = 0;
	sonlineWidget = 0;
	sawayWidget = 0;
	sdndWidget = 0;
	sxaWidget = 0;
	schatWidget = 0;
	sinvisWidget = 0;
	SOnline = 0;
	SAway = 1;
	SDnd = 1;
	SXa = 1;
	SChat = 0;
	SInvis = 0;
}

QString AutoReply::name() const {
	return "Auto Reply Plugin";
}

QString AutoReply::shortName() const {
	return "replyer";
}

QString AutoReply::version() const {
	return cVer;
}

bool AutoReply::enable() {
	if (psiOptions) {
		enabled = true;
		QVariant vMessage(Message);
		vMessage = psiOptions->getPluginOption(constMessage);
		if (!vMessage.isNull()) {
			Message = vMessage.toString();
		}
		QVariant vEnableDisable(EnableDisable);
		vEnableDisable = psiOptions->getPluginOption(constEnableDisable);
		if(!vEnableDisable.isNull()) {
			EnableDisable = vEnableDisable.toInt();
		}
		if(EnableDisable) {
			QVariant vDisableFor(DisableFor);
			vDisableFor = psiOptions->getPluginOption(constDisableFor);
			if(!vDisableFor.isNull()) {
				DisableFor = vDisableFor.toString();
			}
		} else {
			QVariant vDisableFor(DisableFor);
			vDisableFor = psiOptions->getPluginOption(constEnableFor);
			if(!vDisableFor.isNull()) {
				DisableFor = vDisableFor.toString();
			} else {
				DisableFor = "";
			}
		}
		QVariant vTimes(Times);
		vTimes = psiOptions->getPluginOption(constTimes);
		if (!vTimes.isNull()) {
			Times = vTimes.toInt();
		}
		QVariant vActiveTabIsEnable(ActiveTabIsEnable);
		vActiveTabIsEnable = psiOptions->getPluginOption(constActiveTab);
		if(!vActiveTabIsEnable.isNull()) {
			ActiveTabIsEnable = vActiveTabIsEnable.toBool();
		}
		QVariant vResetTime(ResetTime);
		vResetTime = psiOptions->getPluginOption(constResetTime);
		if(!vResetTime.isNull()) {
			ResetTime = vResetTime.toInt();
		}
		QVariant vDisableForAcc(DisableForAcc);
		vDisableForAcc = psiOptions->getPluginOption(constDisableForAcc);
		if (!vDisableForAcc.isNull()) {
			DisableForAcc = vDisableForAcc.toString();
		}
		QVariant vSOnline(SOnline);
		vSOnline = psiOptions->getPluginOption(constSOnline);
		if(!vSOnline.isNull()) {
			SOnline = vSOnline.toBool();
		}
		QVariant vSAway(SAway);
		vSAway = psiOptions->getPluginOption(constSAway);
		if(!vSAway.isNull()) {
			SAway = vSAway.toBool();
		}
		QVariant vSDnd(SDnd);
		vSDnd = psiOptions->getPluginOption(constSDnd);
		if(!vSDnd.isNull()) {
			SDnd = vSDnd.toBool();
		}
		QVariant vSXa(SXa);
		vSXa = psiOptions->getPluginOption(constSXa);
		if(!vSXa.isNull()) {
			SXa = vSXa.toBool();
		}
		QVariant vSChat(SChat);
		vSChat = psiOptions->getPluginOption(constSChat);
		if(!vSChat.isNull()) {
			SChat = vSChat.toBool();
		}
		QVariant vSInvis(SInvis);
		vSInvis = psiOptions->getPluginOption(constSInvis);
		if(!vSInvis.isNull()) {
			SInvis = vSInvis.toBool();
		}
		QVariant vNotInRoster(NotInRoster);
		vNotInRoster = psiOptions->getPluginOption(constNotInRoster);
		if(!vNotInRoster.isNull()) {
			NotInRoster = vNotInRoster.toBool();
		}
	}
	return enabled;
}

bool AutoReply::disable() {
	enabled = false;
	return true;
}

void AutoReply::applyOptions() {
	if (messageWidget == 0 || disableforWidget == 0 || spinWidget == 0 || activetabWidget == 0 || resetWidget == 0 || enabledisableWidget ==0) {
		return;
	}
	QVariant vMessage(messageWidget->toPlainText());
	psiOptions->setPluginOption(constMessage, vMessage);
	Message = vMessage.toString();
	QVariant vEnableDisable(enabledisableWidget->currentIndex());
	psiOptions->setPluginOption(constEnableDisable, vEnableDisable);
	EnableDisable = vEnableDisable.toInt();
	if(EnableDisable) {
		QVariant vDisableFor(disableforWidget->toPlainText());
		psiOptions->setPluginOption(constDisableFor, vDisableFor);
		DisableFor = vDisableFor.toString();
	} else {
		QVariant vDisableFor(disableforWidget->toPlainText());
		psiOptions->setPluginOption(constEnableFor, vDisableFor);
		DisableFor = vDisableFor.toString();
	}
	QVariant vTimes(spinWidget->value());
	psiOptions->setPluginOption(constTimes, vTimes);
	Times = vTimes.toInt();
	QVariant vActiveTabIsEnable(activetabWidget->isChecked());
	psiOptions->setPluginOption(constActiveTab, vActiveTabIsEnable);
	ActiveTabIsEnable = vActiveTabIsEnable.toBool();
	QVariant vResetTime(resetWidget->value());
	psiOptions->setPluginOption(constResetTime, vResetTime);
	ResetTime = vResetTime.toInt();
	QVariant vDisableForAcc(DisableForAccWidget->toPlainText());
	psiOptions->setPluginOption(constDisableForAcc, vDisableForAcc);
	DisableForAcc = vDisableForAcc.toString();
	QVariant vSOnline(sonlineWidget->isChecked());
	psiOptions->setPluginOption(constSOnline, vSOnline);
	SOnline = vSOnline.toBool();
	QVariant vSAway(sawayWidget->isChecked());
	psiOptions->setPluginOption(constSAway, vSAway);
	SAway = vSAway.toBool();
	QVariant vSDnd(sdndWidget->isChecked());
	psiOptions->setPluginOption(constSDnd, vSDnd);
	SDnd = vSDnd.toBool();
	QVariant vSXa(sxaWidget->isChecked());
	psiOptions->setPluginOption(constSXa, vSXa);
	SXa = vSXa.toBool();
	QVariant vSChat(schatWidget->isChecked());
	psiOptions->setPluginOption(constSChat, vSChat);
	SChat = vSChat.toBool();
	QVariant vSInvis(sinvisWidget->isChecked());
	psiOptions->setPluginOption(constSInvis, vSInvis);
	SInvis = vSInvis.toBool();
	QVariant vNotInRoster(NotInRosterWidget->isChecked());
	psiOptions->setPluginOption(constNotInRoster, vNotInRoster);
	NotInRoster = vNotInRoster.toBool();
}

void AutoReply::restoreOptions() {
	if (messageWidget == 0 || disableforWidget == 0 || spinWidget == 0 || activetabWidget == 0 || resetWidget ==0 || enabledisableWidget == 0) {
		return;
	}
	QVariant vMessage(Message);
	vMessage = psiOptions->getPluginOption(constMessage);
	if (!vMessage.isNull()) {
		messageWidget->setText(vMessage.toString());
	}
	else {
		messageWidget->setText(Message);
	}
	QVariant vEnableDisable(EnableDisable);
	vEnableDisable = psiOptions->getPluginOption(constEnableDisable);
	if(!vEnableDisable.isNull()) {
		enabledisableWidget->setCurrentIndex(vEnableDisable.toInt());
	}
	else {
		enabledisableWidget->setCurrentIndex(EnableDisable);
	}
	if(EnableDisable) {
		QVariant vDisableFor(DisableFor);
		vDisableFor = psiOptions->getPluginOption(constDisableFor);
		if(!vDisableFor.isNull()) {
			disableforWidget->setText(vDisableFor.toString());
		}
		else {
			disableforWidget->setText(DisableFor);
		}
	} else {
		QVariant vDisableFor(DisableFor);
		vDisableFor = psiOptions->getPluginOption(constEnableFor);
		if(!vDisableFor.isNull()) {
			disableforWidget->setText(vDisableFor.toString());
		}
		else {
			disableforWidget->setText("");
		}
	}
	QVariant vTimes(Times);
	vTimes = psiOptions->getPluginOption(constTimes);
	if (!vTimes.isNull()) {
		spinWidget->setValue(vTimes.toInt());
	}
	else {
		spinWidget->setValue(Times);
	}
	QVariant vActiveTabIsEnable(ActiveTabIsEnable);
	vActiveTabIsEnable = psiOptions->getPluginOption(constActiveTab);
	if(!vActiveTabIsEnable.isNull()) {
		activetabWidget->setChecked(vActiveTabIsEnable.toBool());
	}
	else {
		activetabWidget->setChecked(ActiveTabIsEnable);
	}
	QVariant vResetTime(ResetTime);
	vResetTime = psiOptions->getPluginOption(constResetTime);
	if (!vResetTime.isNull()) {
		resetWidget->setValue(vResetTime.toInt());
	}
	else {
		resetWidget->setValue(ResetTime);
	}
	QVariant vDisableForAcc(DisableForAcc);
	vDisableForAcc = psiOptions->getPluginOption(constDisableForAcc);
	if (!vDisableForAcc.isNull()) {
		DisableForAccWidget->setText(vDisableForAcc.toString());
	}
	else {
		DisableForAccWidget->setText(DisableForAcc);
	}

	QVariant vSOnline(SOnline);
	vSOnline = psiOptions->getPluginOption(constSOnline);
	if(!vSOnline.isNull()) {
		sonlineWidget->setChecked(vSOnline.toBool());
	}
	else {
		sonlineWidget->setChecked(SOnline);
	}
	QVariant vSAway(SAway);
	vSAway = psiOptions->getPluginOption(constSAway);
	if(!vSAway.isNull()) {
		sawayWidget->setChecked(vSAway.toBool());
	}
	else {
		sawayWidget->setChecked(SAway);
	}
	QVariant vSDnd(SDnd);
	vSDnd = psiOptions->getPluginOption(constSDnd);
	if(!vSDnd.isNull()) {
		sdndWidget->setChecked(vSDnd.toBool());
	}
	else {
		sdndWidget->setChecked(SDnd);
	}
	QVariant vSXa(SXa);
	vSXa = psiOptions->getPluginOption(constSXa);
	if(!vSXa.isNull()) {
		sxaWidget->setChecked(vSXa.toBool());
	}
	else {
		sxaWidget->setChecked(SXa);
	}
	QVariant vSChat(SChat);
	vSChat = psiOptions->getPluginOption(constSChat);
	if(!vSChat.isNull()) {
		schatWidget->setChecked(vSChat.toBool());
	}
	else {
		schatWidget->setChecked(SChat);
	}
	QVariant vSInvis(SInvis);
	vSInvis = psiOptions->getPluginOption(constSInvis);
	if(!vSInvis.isNull()) {
		sinvisWidget->setChecked(vSInvis.toBool());
	}
	else {
		sinvisWidget->setChecked(SInvis);
	}
	QVariant vNotInRoster(NotInRoster);
	vNotInRoster = psiOptions->getPluginOption(constNotInRoster);
	if(!vNotInRoster.isNull()) {
		NotInRosterWidget->setChecked(vNotInRoster.toBool());
	}
	else {
		NotInRosterWidget->setChecked(NotInRoster);
	}
}

QWidget* AutoReply::options() {
	if (!enabled) {
		return 0;
	}
	QWidget *optionsWid = new QWidget();
	messageWidget = new QTextEdit();
	messageWidget->setMaximumHeight(60);
	messageWidget->setText(Message);
	disableforWidget = new QTextEdit();
	disableforWidget->setText(DisableFor);
	enabledisableWidget = new QComboBox();
	enabledisableWidget->addItem(tr("Enable"));
	enabledisableWidget->addItem(tr("Disable"));
	enabledisableWidget->setCurrentIndex(EnableDisable);
	DisableForAccWidget = new QTextEdit();
	DisableForAccWidget->setText(DisableForAcc);
	spinWidget = new QSpinBox();
	spinWidget->setMinimum(-1);
	spinWidget->setValue(Times);
	resetWidget = new QSpinBox();
	resetWidget->setMaximum(2000);
	resetWidget->setMinimum(1);
	resetWidget->setValue(ResetTime);
	activetabWidget = new QCheckBox(tr("Disable if chat window is active"));
	activetabWidget->setChecked(ActiveTabIsEnable);
	NotInRosterWidget = new QCheckBox(tr("Disable if contact isn't from your roster"));
	NotInRosterWidget->setChecked(NotInRoster);

	sonlineWidget = new QCheckBox(tr("Online"));
	sonlineWidget->setChecked(SOnline);
	sawayWidget = new QCheckBox(tr("Away"));
	sawayWidget->setChecked(SAway);
	sdndWidget = new QCheckBox(tr("Dnd"));
	sdndWidget->setChecked(SDnd);
	sxaWidget = new QCheckBox(tr("XA"));
	sxaWidget->setChecked(SXa);
	schatWidget = new QCheckBox(tr("Chat"));
	schatWidget->setChecked(SChat);
	sinvisWidget = new QCheckBox(tr("Invisible"));
	sinvisWidget->setChecked(SInvis);

	QGroupBox *groupBox = new QGroupBox(tr("Enable if status is:"));
	QHBoxLayout *statusLayout = new QHBoxLayout;
	statusLayout->addWidget(sonlineWidget);
	if(psiOptions->getGlobalOption("options.ui.menu.status.chat").toBool()) {
		statusLayout->addWidget(schatWidget); }
	statusLayout->addWidget(sawayWidget);
	statusLayout->addWidget(sdndWidget);
	if(psiOptions->getGlobalOption("options.ui.menu.status.xa").toBool()) {
		statusLayout->addWidget(sxaWidget); }
	if(psiOptions->getGlobalOption("options.ui.menu.status.invisible").toBool()) {
		statusLayout->addWidget(sinvisWidget); }
	statusLayout->addStretch();
	groupBox->setLayout(statusLayout);

	QVBoxLayout *Layout = new QVBoxLayout;
	Layout->addWidget(new QLabel(tr("Auto Reply Message:")));
	Layout->addWidget(messageWidget);
	QVBoxLayout *disableLayout = new QVBoxLayout;
	QHBoxLayout *EnDis = new QHBoxLayout;
	EnDis->addWidget(enabledisableWidget);
	EnDis->addWidget(new QLabel(tr("for JIDs and conferences:")));
	QLabel *Label = new QLabel(tr("You can also specify a part of JID\n(without any additional symbols)"));
	QFont font;
	font.setPointSize(8);
	Label->setFont(font);
	disableLayout->addLayout(EnDis);
	disableLayout->addWidget(disableforWidget);
	disableLayout->addWidget(Label);
	QVBoxLayout *AccLayout = new QVBoxLayout;
	AccLayout->addWidget(new QLabel(tr("Disable for your accounts (specify your JIDs):")));
	AccLayout->addWidget(DisableForAccWidget);
	QHBoxLayout *resetLayout = new QHBoxLayout;
	resetLayout->addWidget(new QLabel(tr("Timeout to reset counter:")));
	resetLayout->addWidget(resetWidget);
	resetLayout->addWidget(new QLabel(tr("min.")));
	resetLayout->addStretch();
	QHBoxLayout *timesLayout = new QHBoxLayout;
	timesLayout->addWidget(new QLabel(tr("Send maximum")));
	timesLayout->addWidget(spinWidget);
	timesLayout->addWidget(new QLabel(tr("times (-1=infinite)")));
	timesLayout->addStretch();
	QVBoxLayout *flags = new QVBoxLayout;
	flags->addLayout(AccLayout);
	flags->addStretch();
	flags->addLayout(timesLayout);
	flags->addLayout(resetLayout);
	flags->addWidget(activetabWidget);
	flags->addWidget(NotInRosterWidget);
	QHBoxLayout *hLayout = new QHBoxLayout;
	hLayout->addLayout(disableLayout);
	QFrame *frame = new QFrame();
	frame->setMinimumWidth(8);
	hLayout->addWidget(frame);
	hLayout->addLayout(flags);
	QLabel *wikiLink = new QLabel(tr("<a href=\"http://psi-plus.com/wiki/plugins#autoreply_plugin\">Wiki (Online)</a>"));
	wikiLink->setOpenExternalLinks(true);
	QVBoxLayout *tab1Layout = new QVBoxLayout(optionsWid);
	tab1Layout->addLayout(Layout);
	tab1Layout->addStretch();
	tab1Layout->addLayout(hLayout);
	tab1Layout->addWidget(groupBox);
	tab1Layout->addWidget(wikiLink);

	connect(enabledisableWidget, SIGNAL(currentIndexChanged(int)), SLOT(setEnableDisableText(int)));

	return optionsWid;
}

void AutoReply::setOptionAccessingHost(OptionAccessingHost* host) {
	psiOptions = host;
}

void AutoReply::optionChanged(const QString& option) {
	Q_UNUSED(option);
}

void AutoReply::setStanzaSendingHost(StanzaSendingHost *host) {
	StanzaHost = host;
}

bool AutoReply::incomingStanza(int account, const QDomElement& stanza) {
	if (enabled) {
		if (stanza.tagName() == "message") {
			QString Status = AccInfoHost->getStatus(account);
			bool state = false;
			if(Status == "online" && SOnline) { state = true;
			} else {
				if(Status == "away" && SAway) { state = true;
				} else {
					if(Status == "chat" && SChat) { state = true;
					} else {
						if(Status == "xa" && SXa) { state = true;
						} else {
							if(Status == "dnd" && SDnd) { state = true;
							} else {
								if(Status == "invisible" && SInvis) { state = true;
								}
							}
						}
					}
				}
			}
			if(!state)	return false;

			QStringList Disable = DisableForAcc.split(QRegExp("\\s+"), QString::SkipEmptyParts);
			QString AccJid = AccInfoHost->getJid(account);
			while(!Disable.isEmpty()) {
				if(AccJid == Disable.takeFirst()) return false;
			}

			QString type = "";
			type = stanza.attribute("type");
			if(type == "groupchat" || type == "error" || type == "normal")	return false;

			QDomElement Body = stanza.firstChildElement("body");
			if(Body.isNull())  return false;

			if(Body.text() == Message)	return false;

			QDomElement rec =  stanza.firstChildElement("received");
			if(!rec.isNull())  return false;

			QDomElement subj = stanza.firstChildElement("subject");
			if (subj.text() == "AutoReply" || subj.text() == "StopSpam" || subj.text() == "StopSpam Question") return false;

			QString from = stanza.attribute("from");
			QString to = stanza.attribute("to");
			QString valF = from.split("/").takeFirst();
			QString valT = to.split("/").takeFirst();
			if(valF.toLower() == valT.toLower())  return false;

			if(!from.contains("@")) return false;

			Disable = DisableFor.split(QRegExp("\\s+"), QString::SkipEmptyParts);
			if(EnableDisable) {
				while(!Disable.isEmpty()) {
					QString J =	 Disable.takeFirst();
					if(J.toLower() == valF.toLower() || from.contains(J, Qt::CaseInsensitive)) {
						return false;
					}
				}
			}
			else {
				bool b = false;
				while(!Disable.isEmpty()) {
					QString J =	 Disable.takeFirst();
					if(J.toLower() == valF.toLower() || from.contains(J, Qt::CaseInsensitive)) {
						b = true;
					}
				}
				if(!b) { return false;
				}
			}

			if(ActiveTabIsEnable) {
				QString getJid = ActiveTabHost->getJid();
				if(getJid.toLower() == from.toLower()) return false;
			}

			if(NotInRoster) {
				QStringList Roster = AccInfoHost->getRoster(account);
				if(!Roster.contains(valF, Qt::CaseInsensitive)) return false;
			}

			if(Times == 0) return false;
			if(Times != -1) {
				int i = Counter.size();
				if(FindAcc(account, from, i)) {
					Base &B = Counter[i];
					if(B.count >= Times) {
						if(QDateTime::currentDateTime().secsTo(B.LastMes) >= -ResetTime*60) {
							return false;
						}
						else {
							B.count = 1;
							B.LastMes = QDateTime::currentDateTime();
						}
					}
					else {
						B.count++;
						B.LastMes = QDateTime::currentDateTime();
					}
				}
				else {
					Base B = {account, from, 1, QDateTime::currentDateTime() };
					Counter << B;
				}
			}

			QString mes = "<message to='" + StanzaHost->escape(from) + "'";
			if(type != "") {
				mes += " type='" + StanzaHost->escape(type) + "'";
			}
			else {
				mes += "><subject>AutoReply</subject";
			}
			mes += "><body>" + StanzaHost->escape(Message) + "</body></message>";
			StanzaHost->sendStanza(account, mes);
		}
	}
	return false;
}

bool AutoReply::outgoingStanza(int /*account*/, QDomElement& /*stanza*/)
{
	return false;
}

bool AutoReply::FindAcc(int account, QString jid, int &i) {
	for(; i > 0;) {
		Base B = Counter[--i];
		if(B.Account == account && B.Jid == jid) {
			return true;
		}
	}
	return false;
}

void AutoReply::setActiveTabAccessingHost(ActiveTabAccessingHost* host) {
	ActiveTabHost = host;
}

void AutoReply::setAccountInfoAccessingHost(AccountInfoAccessingHost* host) {
	AccInfoHost = host;
}

void AutoReply::setEnableDisableText(int Arg) {
	if(Arg) {
		QVariant vDisableFor(DisableFor);
		vDisableFor = psiOptions->getPluginOption(constDisableFor);
		if(!vDisableFor.isNull()) {
			disableforWidget->setText(vDisableFor.toString());
		}
		else {
			disableforWidget->setText(DisableFor);
		}
	} else {
		QVariant vDisableFor(DisableFor);
		vDisableFor = psiOptions->getPluginOption(constEnableFor);
		if(!vDisableFor.isNull()) {
			disableforWidget->setText(vDisableFor.toString());
		}
		else {
			disableforWidget->setText("");
		}
	}
}

QString AutoReply::pluginInfo() {
	return tr("Author: ") +	 "Dealer_WeARE\n"
	     + tr("Email: ") + "wadealer@gmail.com\n\n"
	     + trUtf8("This plugin acts as an auto-answering machine. It has a number of simple configuration options, which you can use to:\n"
				  "* set a text message for auto-answer\n"
				  "* exclude specified jids, including conferences, from the objects for auto-answer (if a jid conference is set, the exception will include all private messages)\n"
				  "* disable the auto-responder for some of your accounts\n"
				  "* set the number of sent auto messages\n"
				  "* set the time interval after which the number of auto messages counter will be reset\n"
				  "* disable the auto-responder for the active tab\n"
				  "* disable the auto-responder for contacts that are not in your roster\n"
				  "The list of exceptions for jids has two operating modes:\n"
				  "* auto-responder is switched off for the list of exceptions, for the others is switched on (Disable mode)\n"
				  "* auto-responder is switched on for the list of exceptions, for the others is switched off (Enable mode) ");
}

QPixmap AutoReply::icon() const
{
	return QPixmap(":/icons/autoreply.png");
}

#include "autoreplyplugin.moc"
