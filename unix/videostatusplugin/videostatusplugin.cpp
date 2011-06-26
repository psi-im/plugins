/*
 * videostatusplugin.cpp - plugin
 * Copyright (C) 2010  KukuRuzo, Khryukin Evgeny
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

#include <QtDBus>

#include "psiplugin.h"
#include "plugininfoprovider.h"
#include "optionaccessinghost.h"
#include "optionaccessor.h"
#include "accountinfoaccessinghost.h"
#include "accountinfoaccessor.h"
#include "psiaccountcontroller.h"
#include "psiaccountcontrollinghost.h"

#include "ui_options.h"

#define constVersion "0.0.7"

#define constPlayerVLC "playervlc"
#define constPlayerTotem "playertotem"
#define constPlayerGMPlayer "playergmplayer"
#define constPlayerKaffeine "playerkaffeine"
#define constStatus "status"
#define constStatusMessage "statusmessage"
#define constSetOnline "setonline"
#define constRestoreDelay "restoredelay"
#define constSetDelay "setdelay"

struct Status {
	int int1;
	int int2;
	int int3;
	int int4;
};

const QDBusArgument & operator<<(QDBusArgument &arg, const Status &change) {
	arg.beginStructure();
	arg << change.int1 << change.int2 << change.int3 << change.int4;
	arg.endStructure();
	return arg;
}

const QDBusArgument & operator>>(const QDBusArgument &arg, Status &change) {
	arg.beginStructure();
	arg >> change.int1 >> change.int2 >> change.int3 >> change.int4;
	arg.endStructure();
	return arg;
}

Q_DECLARE_METATYPE(Status);


const int timeout = 10000; //Интервал опроса проигрывателей, милисекунды
const QString totemService = "org.mpris.Totem";
const QString vlcService = "org.mpris.vlc";
const QString GMPlayerService = "com.gnome.mplayer";
const QString kaffeineService = "org.mpris.kaffeine";

class VideoStatusChanger : public QObject, public PsiPlugin, public PluginInfoProvider, public OptionAccessor
			, public PsiAccountController, public AccountInfoAccessor
{
        Q_OBJECT
	Q_INTERFACES(PsiPlugin PluginInfoProvider OptionAccessor PsiAccountController AccountInfoAccessor)
public:
	VideoStatusChanger();
        virtual QString name() const;
        virtual QString shortName() const;
        virtual QString version() const;
        virtual QWidget* options();
        virtual bool enable();
        virtual bool disable();
	virtual void applyOptions();
	virtual void restoreOptions();
	virtual void optionChanged(const QString&) {};
	virtual void setOptionAccessingHost(OptionAccessingHost* host);
	virtual void setAccountInfoAccessingHost(AccountInfoAccessingHost* host);
	virtual void setPsiAccountControllingHost(PsiAccountControllingHost* host);
	virtual QString pluginInfo();

private:
	bool enabled;
	OptionAccessingHost* psiOptions;
	AccountInfoAccessingHost* accInfo;
	PsiAccountControllingHost* accControl;
        bool playerVLC, playerTotem, playerGMPlayer, playerKaffeine; //настройки пользователя, следим за проигрывателем или нет
	QString status, statusMessage;
	Ui::OptionsWidget ui_;
	QPointer<QTimer> timer;
	bool isTotem, isVLC;
	bool isStatusSet; // здесь храним информацию, установлен ли уже статус (чтобы не устанавливать повторно при каждом срабатывании таймера)
	bool setOnline;
	int restoreDelay;
	int setDelay;
	QHash<QString, bool> runningPlayers;

	struct StatusString {
		QString status;
		QString message;
	};
	QHash<int, StatusString> statuses_;

	bool sendDBusCall(QString service, QString path, QString interface, QString command);
	void setPsiGlobalStatus(bool set);

private slots:
	void timeOut(); //здесь проверяем проигрыватели
	void delayTimeout();

};

Q_EXPORT_PLUGIN(VideoStatusChanger);

VideoStatusChanger::VideoStatusChanger() {
	enabled = false;
	playerVLC = false;
	playerTotem = false;
	playerGMPlayer = false;
	playerKaffeine = false;
	status = "dnd";
	statusMessage = "";
	psiOptions = 0;
	accInfo = 0;
	accControl = 0;
	isStatusSet = false;
	setOnline = true;
	restoreDelay = 20;
	setDelay = 10;
}

QString VideoStatusChanger::name() const {
	return "Video Status Changer Plugin";
}

QString VideoStatusChanger::shortName() const {
	return "videostatus";
}

QString VideoStatusChanger::version() const {
        return constVersion;
}

void VideoStatusChanger::setOptionAccessingHost(OptionAccessingHost* host) {
	psiOptions = host;
}

void VideoStatusChanger::setAccountInfoAccessingHost(AccountInfoAccessingHost* host) {
	accInfo = host;
}

void VideoStatusChanger::setPsiAccountControllingHost(PsiAccountControllingHost* host) {
	accControl = host;
}

bool VideoStatusChanger::enable() {
	if(psiOptions) {
		enabled = true;
		statuses_.clear();
		runningPlayers.clear();
		playerVLC = psiOptions->getPluginOption(constPlayerVLC, QVariant(playerVLC)).toBool();
		playerTotem = psiOptions->getPluginOption(constPlayerTotem, QVariant(playerTotem)).toBool();
                playerGMPlayer = psiOptions->getPluginOption(constPlayerGMPlayer, QVariant(playerGMPlayer)).toBool();
                playerKaffeine = psiOptions->getPluginOption(constPlayerKaffeine, QVariant(playerKaffeine)).toBool();
		status = psiOptions->getPluginOption(constStatus, QVariant(status)).toString();
		statusMessage = psiOptions->getPluginOption(constStatusMessage, QVariant(statusMessage)).toString();
		setOnline = psiOptions->getPluginOption(constSetOnline, QVariant(setOnline)).toBool();
		restoreDelay = psiOptions->getPluginOption(constRestoreDelay, QVariant(restoreDelay)).toInt();
		setDelay = psiOptions->getPluginOption(constSetDelay, QVariant(setDelay)).toInt();
		if(!timer){
			timer = new QTimer();
			timer->setInterval(timeout);
			connect(timer, SIGNAL(timeout()), this, SLOT(timeOut()));
		}
		timer->start();

		if(playerVLC)
			runningPlayers.insert(vlcService, false);
		if(playerTotem)
			runningPlayers.insert(totemService, false);
                if(playerGMPlayer)
                        runningPlayers.insert(GMPlayerService, false);
                if(playerKaffeine)
                        runningPlayers.insert(kaffeineService, false);
	}
	return enabled;
}

bool VideoStatusChanger::disable(){
	enabled = false;
	if(timer) {
		timer->stop();
		disconnect(timer, SIGNAL(timeout()), this, SLOT(timeOut()));
		delete(timer);
	}
        return true;
}

void VideoStatusChanger::applyOptions() {
	playerVLC = ui_.cb_vlc->isChecked();
	psiOptions->setPluginOption(constPlayerVLC, QVariant(playerVLC));

	playerTotem = ui_.cb_totem->isChecked();
	psiOptions->setPluginOption(constPlayerTotem, QVariant(playerTotem));

        playerGMPlayer = ui_.cb_gmp->isChecked();
        psiOptions->setPluginOption(constPlayerGMPlayer, QVariant(playerGMPlayer));

        playerKaffeine = ui_.cb_kaffeine->isChecked();
        psiOptions->setPluginOption(constPlayerKaffeine, QVariant(playerKaffeine));

	status = ui_.cb_status->currentText();
	psiOptions->setPluginOption(constStatus, QVariant(status));
	statusMessage = ui_.le_message->text();
	psiOptions->setPluginOption(constStatusMessage, QVariant(statusMessage));
	setOnline = ui_.cb_online->isChecked();
	psiOptions->setPluginOption(constSetOnline, QVariant(setOnline));
	restoreDelay = ui_.sb_restoreDelay->value();
	psiOptions->setPluginOption(constRestoreDelay, QVariant(restoreDelay));
	setDelay = ui_.sb_setDelay->value();
	psiOptions->setPluginOption(constSetDelay, QVariant(setDelay));

	runningPlayers.clear();
	if(playerVLC)
		runningPlayers.insert(vlcService, false);
	if(playerTotem)
		runningPlayers.insert(totemService, false);
        if(playerGMPlayer)
                runningPlayers.insert(GMPlayerService, false);
        if(playerKaffeine)
                runningPlayers.insert(kaffeineService, false);
    }

void VideoStatusChanger::restoreOptions() {
	ui_.cb_vlc->setChecked(playerVLC);
	ui_.cb_totem->setChecked(playerTotem);
        ui_.cb_gmp->setChecked(playerGMPlayer);
        ui_.cb_kaffeine->setChecked(playerKaffeine);
	QStringList list;
	list << "away" << "xa" << "dnd";
	ui_.cb_status->addItems(list);
	ui_.cb_status->setCurrentIndex(ui_.cb_status->findText(status));
	ui_.le_message->setText(statusMessage);
	ui_.cb_online->setChecked(setOnline);
	ui_.sb_restoreDelay->setValue(restoreDelay);
	ui_.sb_setDelay->setValue(setDelay);
}

QWidget* VideoStatusChanger::options(){
        if (!enabled) {
		return 0;
	}
        QWidget *optionsWid = new QWidget();
	ui_.setupUi(optionsWid);
	restoreOptions();

        return optionsWid;
}

QString VideoStatusChanger::pluginInfo() {
	return tr("Authors: ") +  "Dealer_WeARE, KukuRuzo\n\n"
			+ trUtf8("This plugin is designed to set the custom status when you see the video in selected video player. \n"
				 "Note: This plugin is designed to work in Linux family operating systems ONLY. \n\n"
				 "To work with Totem player you need to enable appropriate plugin in this player (Edit\\Plugins\\D-Bus);\n\n"
				 "To work with VLC player you need to enable the option \"Control Interface D-Bus\" in the Advanced Settings tab on \"Interface\\Control Interface\" section of the player settings; \n\n"
				 "To work with Kaffeine player you must have player version (>= 1.0), additional configuration is not needed; \n\n"
				 "To work with GNOME MPlayer additional configuration is not needed.");
}

void VideoStatusChanger::timeOut() {
	if(playerTotem) {
		runningPlayers.insert(totemService, sendDBusCall(totemService,"/Player","org.freedesktop.MediaPlayer","GetStatus") );
	}

	if(playerVLC) {
		runningPlayers.insert(vlcService, sendDBusCall(vlcService,"/Player","org.freedesktop.MediaPlayer","GetStatus") );
	}

        if(playerGMPlayer){
                runningPlayers.insert(GMPlayerService, sendDBusCall(GMPlayerService,"/",GMPlayerService,"GetPlayState") );
        }

        if(playerKaffeine){
                runningPlayers.insert(kaffeineService, sendDBusCall(kaffeineService,"/Player","org.freedesktop.MediaPlayer","GetStatus") );
        }

	if(runningPlayers.values().contains(true)) {
		if(!isStatusSet) {
			//setPsiGlobalStatus(isStatusSet);
			QTimer::singleShot(setDelay*1000, this, SLOT(delayTimeout()));
			isStatusSet = true;
		}
	}
	else if(isStatusSet) {
		if(setOnline)
			QTimer::singleShot(restoreDelay*1000, this, SLOT(delayTimeout()));
		isStatusSet = false;
	}
}

void VideoStatusChanger::delayTimeout() {
	setPsiGlobalStatus(!isStatusSet);
}

bool VideoStatusChanger::sendDBusCall(QString service, QString path, QString interface, QString command){
	//qDebug("Checking service %s", qPrintable(service));
	QDBusInterface player(service, path, interface);
	if(player.isValid()) {
            //Check if player Gnome-Mplayer then sends another call to service
            if(!service.contains(GMPlayerService)){
		qDBusRegisterMetaType<Status>();
		//qDebug("MetaType registered");
		QDBusReply<Status> reply = player.call(command);
		//qDebug("Command %s sent", qPrintable(command));
		if(reply.isValid()) {
			//qDebug("Command %s is valid", qPrintable(command));
			int st = reply.value().int1;
			//qDebug("Player status %d", st);
			return !st;
		}
		else {
			return false;
		}
            }else{
                //Get PlayState reply from com.gnome.mplayer (2-paused, 3-playing,stopped, 6-just_oppened(playing))
                QDBusReply<int> gmstatereply = player.call(command);                                
                if(gmstatereply.isValid()){
                    //Get Track position reply in precents. Needed to catch stopped state
                    QDBusReply<double> gmposreply = player.call("GetPercent");
                    if(gmposreply.isValid()){
                        //if state is playing or just_oppened(playing) and if not stopped returns true
                        if(((gmstatereply.value()==3)||(gmstatereply.value()==6)) && gmposreply.value()>0){
                            return true;
                        }else{
                            return false;
                        }
                    }
                }else{
                    return false;
                }
            }
	}
	//qDebug("Error creating DBus interface %s", qPrintable(service));
	return false;
}

void VideoStatusChanger::setPsiGlobalStatus(bool set) {
	if (!enabled) return;
	int account = 0;
	StatusString s;
	while (accInfo->getJid(account) != "-1") {
		QString accStatus = accInfo->getStatus(account);
		if(accStatus != "offline" && accStatus != "invisible") {
			if(set) {
				if(statuses_.contains(account)) {
					s = statuses_.value(account);
					accControl->setStatus(account, s.status, s.message);
				}
				else
					accControl->setStatus(account, "online", "");
			}
			else {
				s.status = accStatus;
				s.message = accInfo->getStatusMessage(account);
				if(s.status != status || s.message != statusMessage)
					statuses_.insert(account, s);
				accControl->setStatus(account, status, statusMessage);
			}
		}
		++account;
	}
}
#include "videostatusplugin.moc"

