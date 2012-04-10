/*
 * birthdayreminderplugin.cpp - plugin
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

#include <QFileDialog>
#include <QDomElement>

#include "psiplugin.h"
#include "stanzafilter.h"
#include "accountinfoaccessor.h"
#include "accountinfoaccessinghost.h"
#include "applicationinfoaccessor.h"
#include "applicationinfoaccessinghost.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"
#include "optionaccessor.h"
#include "optionaccessinghost.h"
#include "popupaccessor.h"
#include "popupaccessinghost.h"
#include "iconfactoryaccessinghost.h"
#include "iconfactoryaccessor.h"
#include "plugininfoprovider.h"
#include "soundaccessinghost.h"
#include "soundaccessor.h"

#include "ui_options.h"

#define cVer "0.3.4"
#define constLastCheck "lstchck"
#define constDays "days"
#define constInterval "intrvl"
#define constTimeout "timeout"
#define constStartCheck "strtchck"
#define constCheckFromRoster "chckfrmrstr"
#define constLastUpdate "lstupdate"
#define constUpdateInterval "updtintvl"
#define constSoundFile "sndfl"

#define POPUP_OPTION_NAME  "Birthday Reminder Plugin"

static const QString id = "111aaa222bbb";

class Reminder : public QObject, public PsiPlugin, public StanzaFilter, public AccountInfoAccessor, public ApplicationInfoAccessor,
		public StanzaSender, public OptionAccessor, public PopupAccessor, public IconFactoryAccessor,
		public PluginInfoProvider, public SoundAccessor
{
	Q_OBJECT
	Q_INTERFACES(PsiPlugin StanzaFilter AccountInfoAccessor ApplicationInfoAccessor StanzaSender OptionAccessor
		     PopupAccessor IconFactoryAccessor PluginInfoProvider SoundAccessor)

public:
	Reminder();
	virtual QString name() const;
	virtual QString shortName() const;
	virtual QString version() const;
        virtual QWidget* options();
	virtual bool enable();
        virtual bool disable();
        virtual void applyOptions();
        virtual void restoreOptions();
        virtual bool incomingStanza(int account, const QDomElement& xml);
	virtual bool outgoingStanza(int account, QDomElement& xml);
        virtual void setAccountInfoAccessingHost(AccountInfoAccessingHost* host);
        virtual void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host);
        virtual void setStanzaSendingHost(StanzaSendingHost *host);
        virtual void setOptionAccessingHost(OptionAccessingHost* host);
	virtual void optionChanged(const QString& ){};
        virtual void setPopupAccessingHost(PopupAccessingHost* host);
        virtual void setIconFactoryAccessingHost(IconFactoryAccessingHost* host);
	virtual void setSoundAccessingHost(SoundAccessingHost* host);
	virtual QString pluginInfo();

private:
        bool enabled;
        OptionAccessingHost *psiOptions;
	AccountInfoAccessingHost *accInfoHost;
	ApplicationInfoAccessingHost *appInfoHost;
	StanzaSendingHost *stanzaHost;
        PopupAccessingHost* popup;
	IconFactoryAccessingHost* icoHost;
	SoundAccessingHost* sound_;
        QString Dir;        
	QString lastCheck;
	int days_;
	int interval;
	//int timeout;
	bool startCheck;
	bool checkFromRoster;
	QString lastUpdate;
	int updateInterval;
	QString soundFile;
	bool updateInProgress;
	int popupId;

	QPointer<QWidget> options_;
	Ui::Options ui_;

	QString CheckBirthdays();

private slots:
	void UpdateVCard();
	bool Check();
	void clearCache();
	void getSound();
	void checkSound();
	void playSound(const QString&);
	void timeoutStopUpdate();
};

Q_EXPORT_PLUGIN(Reminder);

Reminder::Reminder()
	: enabled(false)
	, psiOptions(0)
	, accInfoHost(0)
	, appInfoHost(0)
	, stanzaHost(0)
	, popup(0)
	, icoHost(0)
	, Dir("")
	, lastCheck("1901010101")
	, days_(5)
	, interval(24)
	//, timeout(15)
	, startCheck(true)
	, checkFromRoster(true)
	, lastUpdate("19010101")
	, updateInterval(30)
	, soundFile("sound/reminder.wav")
	, updateInProgress(false)
	, popupId(0)
{
}

QString Reminder::name() const {
        return "Birthday Reminder Plugin";
}

QString Reminder::shortName() const {
        return "reminder";
}

QString Reminder::version() const {
        return cVer;
}

bool Reminder::enable() {
	if(!psiOptions)
		return enabled;

	QFile file(":/reminder/birthday.png");
	if ( file.open(QIODevice::ReadOnly) ) {
		QByteArray image = file.readAll();
		icoHost->addIcon("reminder/birthdayicon",image);
		file.close();
	} else {
		return enabled;
	}

	enabled = true;

	lastCheck = psiOptions->getPluginOption(constLastCheck, lastCheck).toString();
	days_ = psiOptions->getPluginOption(constDays, days_).toInt();
	interval = psiOptions->getPluginOption(constInterval, interval).toInt();
	startCheck = psiOptions->getPluginOption(constStartCheck, startCheck).toBool();
	checkFromRoster = psiOptions->getPluginOption(constCheckFromRoster, checkFromRoster).toBool();
	updateInterval = psiOptions->getPluginOption(constUpdateInterval, updateInterval).toInt();
	lastUpdate = psiOptions->getPluginOption(constLastUpdate, lastUpdate).toString();
	soundFile = psiOptions->getPluginOption(constSoundFile, QVariant(soundFile)).toString();

	int timeout = psiOptions->getPluginOption(constTimeout, QVariant(15000)).toInt()/1000;
	popupId = popup->registerOption(POPUP_OPTION_NAME, timeout, "plugins.options."+shortName()+"."+constTimeout);

	Dir = appInfoHost->appVCardDir() + QDir::separator() + "Birthdays";
	QDir BirthDay(Dir);
	if(!BirthDay.exists(Dir)) {
		BirthDay.mkdir(Dir);
		return enabled;
	}
	if(startCheck) {
		lastCheck = QDateTime::currentDateTime().toString("yyyyMMddhh");
		psiOptions->setPluginOption(constLastCheck, QVariant(lastCheck));
		QTimer::singleShot(4000, this, SLOT(Check())); //необходимо для инициализации приложения
	}

	return enabled;
}

bool Reminder::disable() {
	enabled = false;
	popup->unregisterOption(POPUP_OPTION_NAME);
	return true;
}

QWidget* Reminder::options() {
	if(!enabled)
		return 0;

	options_ = new QWidget();
	ui_.setupUi(options_);

	ui_.tb_get->setIcon(icoHost->getIcon("psi/browse"));
	ui_.tb_check->setIcon(icoHost->getIcon("psi/play"));

	restoreOptions();

	connect(ui_.pb_update, SIGNAL(clicked()), SLOT(UpdateVCard()));
	connect(ui_.pb_check, SIGNAL(clicked()), SLOT(Check()));
	connect(ui_.pb_clear_cache, SIGNAL(clicked()), SLOT(clearCache()));
	connect(ui_.tb_check, SIGNAL(clicked()), SLOT(checkSound()));
	connect(ui_.tb_get, SIGNAL(clicked()), SLOT(getSound()));

	restoreOptions();

	return options_;
}

bool Reminder::incomingStanza(int /*account*/, const QDomElement& stanza) {
	if (enabled) {
		if(stanza.tagName() == "iq") {
			if(stanza.attribute("id") == id) {
				QDomNode VCard = stanza.firstChild();
				QDomElement BDay = VCard.firstChildElement("BDAY");
				if(!BDay.isNull()) {
					QString Jid = stanza.attribute("from");
					QString Nick = VCard.firstChildElement("NICKNAME").text();
					QString Date = BDay.text();
					if(Date != "") {
						Jid.replace("@", "_at_");
						QFile file(Dir + QDir::separator() + Jid);
						if(file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
							QTextStream out(&file);
							out.setCodec("UTF-8");
							out.setGenerateByteOrderMark(false);
							out << Date << "__" << Nick << endl;
						}
					}
				}
				return true;
			}
		}

		if(stanza.tagName() == "presence") {
			if((lastCheck.toLong() + interval) <= QDateTime::currentDateTime().toString("yyyyMMddhh").toLong()) {
				lastCheck = QDateTime::currentDateTime().toString("yyyyMMddhh");
				psiOptions->setPluginOption(constLastCheck, QVariant(lastCheck));
				Check();
			}
			if(updateInterval) {
				if((lastUpdate.toLong() + updateInterval) <= QDateTime::currentDateTime().toString("yyyyMMdd").toLong()) {
					lastUpdate = QDateTime::currentDateTime().toString("yyyyMMdd");
					psiOptions->setPluginOption(constLastUpdate, QVariant(lastUpdate));
					UpdateVCard();
				}
			}
		}
	}
	return false;
}

bool Reminder::outgoingStanza(int /*account*/, QDomElement& /*xml*/)
{
	return false;
}

void Reminder::setAccountInfoAccessingHost(AccountInfoAccessingHost* host) {
	accInfoHost = host;
}

void Reminder::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host) {
	appInfoHost = host;
}

void Reminder::applyOptions() {
	if(!options_)
		return;

	days_ = ui_.sb_start->value();
	psiOptions->setPluginOption(constDays, QVariant(days_));

	interval = ui_.sb_check_interval->value();
	psiOptions->setPluginOption(constInterval, QVariant(interval));

	startCheck = ui_.cb_startupcheck->isChecked();
	psiOptions->setPluginOption(constStartCheck, QVariant(startCheck));

	checkFromRoster = ui_.cb_active_accounts->isChecked();
	psiOptions->setPluginOption(constCheckFromRoster, QVariant(checkFromRoster));

	updateInterval = ui_.sb_update_interval->value();
	psiOptions->setPluginOption(constUpdateInterval, QVariant(updateInterval));

	soundFile = ui_.le_sound->text();
	psiOptions->setPluginOption(constSoundFile, QVariant(soundFile));
}

void Reminder::restoreOptions() {
	if(!options_)
		return;

	ui_.sb_start->setValue(days_);
	ui_.sb_check_interval->setValue(interval);
	ui_.cb_startupcheck->setChecked(startCheck);
	ui_.cb_active_accounts->setChecked(checkFromRoster);
	ui_.sb_update_interval->setValue(updateInterval);
	ui_.le_sound->setText(soundFile);
}

void Reminder::setStanzaSendingHost(StanzaSendingHost *host) {
	stanzaHost = host;
}

void Reminder::UpdateVCard() {
	if(enabled && !updateInProgress) {
		updateInProgress = true;
		QString path = appInfoHost->appVCardDir();
		QDir dir(path);
		foreach (QString filename, dir.entryList(QDir::Files)) {
			QFile file(path + QDir::separator() + filename);
			if(file.open(QIODevice::ReadOnly)) {
				QTextStream in(&file);
				in.setCodec("UTF-8");
				QDomDocument doc;
				doc.setContent(in.readAll());
				QDomElement vCard = doc.documentElement();
				QDomElement BDay = vCard.firstChildElement("BDAY");
				if(!BDay.isNull()) {
					QString Nick = vCard.firstChildElement("NICKNAME").text();
					QString Date = BDay.text();
					if(Date != "") {
						filename.replace("%5f", "_");
						filename.replace("%2d", "-");
						filename.replace("%25", "%");
						filename.remove(".xml");
						QFile file(Dir + QDir::separator() + filename);
						if(file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
							QTextStream out(&file);
							out.setCodec("UTF-8");
							out.setGenerateByteOrderMark(false);
							out << Date << "__" << Nick << endl;
						}
					}
				}
			}
		}

		int accs = -1;
		bool b = true;
		while(b) {
			QStringList Jids = accInfoHost->getRoster(++accs);
			if(!Jids.isEmpty()) {
				if(Jids.first() == "-1") {
					b = false;
				}
				else {
					if(accInfoHost->getStatus(accs) != "offline") {
						foreach(QString Jid, Jids) {
							QString text = "<iq type=\"get\" to=\"" + Jid + "\" id=\"" + id + "\"><vCard xmlns=\"vcard-temp\" version=\"2.0\" prodid=\"-//HandGen//NONSGML vGen v1.0//EN\" /></iq>";
							stanzaHost->sendStanza(accs, text);
						}
					}
				}
			}
		}
		QTimer::singleShot(30000, this, SLOT(timeoutStopUpdate())); //30 секунд дольжно хватить, чтобы получить все vCard'ы
	}
}

void Reminder::timeoutStopUpdate() {
	updateInProgress = false;
}

QString Reminder::CheckBirthdays() {
	if(!enabled)
		return QString();

	QSet<QString> Roster_;
	if(checkFromRoster) {
		int accs = -1;
		bool b = true;
		while(b) {
			QStringList Jids = accInfoHost->getRoster(++accs);
			if(!Jids.isEmpty()) {
				if(Jids.first() == "-1") {
					b = false;
				}
				else {
					Roster_ += Jids.toSet();
				}
			}
		}
	}

	QString CheckResult;
	QDir dir(Dir);
	foreach(QString jid, dir.entryList(QDir::Files)) {
		if(jid.contains("_at_")) {
			QFile file(Dir + QDir::separator() + jid);
			if(file.open(QIODevice::ReadOnly)) {
				QTextStream in(&file);
				in.setCodec("UTF-8");
				QString line = in.readLine();
				QStringList fields = line.split("__");
				QString Date = fields.takeFirst();
				QString Nick = "";
				if(!fields.isEmpty()) {
					Nick = fields.takeFirst();
				}
				QDate Birthday = QDate::currentDate();
				if(Date.contains("-")) {
					Birthday = QDate::fromString(Date, "yyyy-MM-dd");
				} else {
					if(Date.contains(".")) {
						Birthday = QDate::fromString(Date, "d.MM.yyyy");
					} else {
						if(Date.contains("/")) {
							Birthday = QDate::fromString(Date, "d/MM/yyyy");
						}
					}
				}
				QDate current = QDate::currentDate();
				if(current != Birthday) {
					int years = current.year() - Birthday.year();
					Birthday = Birthday.addYears(years);
					int daysTo = current.daysTo(Birthday);
					QString days;
					days.setNum(daysTo);
					jid.replace("_at_", "@");
					if(!checkFromRoster || Roster_.contains(jid)) {
						if(daysTo == 0) {
							CheckResult += Nick + " (" + jid + ") " + tr("celebrates birthday today!""\n");
						} else {
							if(daysTo <= days_ && daysTo > 0) {
								CheckResult += Nick + " (" + jid + ") " + tr("celebrates birthday in %n day(s)\n", "", daysTo);
							} else {
								if(daysTo == -1) {
									CheckResult += Nick + " (" + jid + ") " + tr("celebrates birthday yesterday.\n");
								}
							}
						}
					}
				}
			}
		}
	}
	return CheckResult;
}

void Reminder::setOptionAccessingHost(OptionAccessingHost *host) {
	psiOptions = host;
}

void Reminder::setIconFactoryAccessingHost(IconFactoryAccessingHost* host) {
	icoHost = host;
}

void Reminder::setSoundAccessingHost(SoundAccessingHost *host) {
	sound_ = host;
}

bool Reminder::Check() {
	QString text = CheckBirthdays();
	if(text.isEmpty())
		return false;
	text.chop(1);

	if(psiOptions->getGlobalOption("options.ui.notifications.sounds.enable").toBool())
		playSound(soundFile);

	text = text.replace("\n", "<br>");
	popup->initPopup(text, tr("Birthday Reminder"), "reminder/birthdayicon", popupId);


	return true;
}

void Reminder::clearCache() {
	QDir dir(Dir);
	foreach(QString file, dir.entryList(QDir::Files)) {
		QFile File(Dir + QDir::separator() + file);
		if(File.open(QIODevice::ReadWrite)) {
			File.remove();
		}
	}
	lastUpdate = "19010101";
	psiOptions->setPluginOption(constLastUpdate, QVariant(lastUpdate));
}

void Reminder::setPopupAccessingHost(PopupAccessingHost* host) {
	popup = host;
}

void Reminder::playSound(const QString& f) {
	sound_->playSound(f);
}

void Reminder::getSound() {
	QString fileName = QFileDialog::getOpenFileName(0,tr("Choose a sound file"),"", tr("Sound (*.wav)"));
	if(fileName.isEmpty())
		return;
	ui_.le_sound->setText(fileName);
}

void Reminder::checkSound() {
	playSound(ui_.le_sound->text());
}

QString Reminder::pluginInfo() {
	return tr("Author: ") +  "Dealer_WeARE\n"
			+ tr("Email: ") + "wadealer@gmail.com\n\n"
			+ trUtf8("This plugin is designed to show reminders of upcoming birthdays.\n"
				 "The first time you install this plugin, you need to log on to all of your accounts, go to the plugin settings and click \"Update Birthdays\"."
				 "The plugin will then collect the information about the birthdays of all the users in your roster, but when the 'Use vCards cache' option is"
				 "selected, the users' vCards that are cached on your hard disk will be used. ");
}

#include "birthdayreminderplugin.moc"
