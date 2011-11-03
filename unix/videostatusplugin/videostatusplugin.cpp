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

#include <QDBusArgument>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QDBusInterface>
#include <QDBusConnectionInterface>
#include <QDesktopWidget>

#include "psiplugin.h"
#include "plugininfoprovider.h"
#include "optionaccessinghost.h"
#include "optionaccessor.h"
#include "accountinfoaccessinghost.h"
#include "accountinfoaccessor.h"
#include "psiaccountcontroller.h"
#include "psiaccountcontrollinghost.h"

#include "ui_options.h"

#include <QX11Info>
#include <X11/Xlib.h>

#define constVersion "0.1.0"

#define constPlayerVLC "vlcplayer"
#define constPlayerTotem "totemplayer"
#define constPlayerGMPlayer "gmplayer"
#define constPlayerKaffeine "kaffeineplayer"
#define constStatus "status"
#define constStatusMessage "statusmessage"
#define constSetOnline "setonline"
#define constRestoreDelay "restoredelay"
#define constSetDelay "setdelay"
#define constFullScreen "fullscreen"
#define vlcService "org.mpris.vlc"
#define totemService "org.mpris.Totem"
#define gmplayerService "com.gnome.mplayer"
#define kaffeineService "org.mpris.kaffeine"

typedef QList<Window> WindowList;

struct PlayerStatus {
	int playStatus;
	int playOrder;
	int playRepeat;
	int stopOnce;
};

Q_DECLARE_METATYPE(PlayerStatus);

static const QDBusArgument & operator<<(QDBusArgument &arg, const PlayerStatus &ps) {
	arg.beginStructure();
	arg << ps.playStatus
	    << ps.playOrder
	    << ps.playRepeat
	    << ps.stopOnce;
	arg.endStructure();
	return arg;
}

static const QDBusArgument & operator>>(const QDBusArgument &arg, PlayerStatus &ps) {
	arg.beginStructure();
	arg >> ps.playStatus
	    >> ps.playOrder
	    >> ps.playRepeat
	    >> ps.stopOnce;
	arg.endStructure();
	return arg;
}

static const int StatusPlaying = 0;

static const int timeout = 10000;
static const QString busName = "Session";

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
	QPointer<QTimer> checkTimer; //Таймер Gnome Mplayer
	QTimer fullST;
	bool isStatusSet; // здесь храним информацию, установлен ли уже статус (чтобы не устанавливать повторно при каждом срабатывании таймера)
	bool setOnline;
	int restoreDelay; //задержка восстановления статуса
	int setDelay; //задержка установки статуса
	QStringList validPlayers_; //список включенных плееров
	QStringList players_; //очередь плееров которые слушает плагин
	bool fullScreen;

	struct StatusString {
		QString status;
		QString message;
	};
	QHash<int, StatusString> statuses_;

	bool sendDBusCall(const QString &service, const QString &path, const QString &interface, const QString &command);
	void connectToBus(const QString &service_);
	void disconnectFromBus(const QString &service_);
	void setPsiGlobalStatus(const bool set);
	void startCheckTimer();
	void setStatusTimer(const int delay, const bool isStart);
	void setValidPlayers();
	bool isPlayerValid(const QString &service);

private slots:
	void checkMprisService(const QString &name, const QString &oldOwner, const QString &newOwner);
	void timeOut(); //здесь проверяем проигрыватели
	void delayTimeout();
	void onPlayerStatusChange(const PlayerStatus &ps);
	void fullSTTimeout();

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
	fullScreen = false;
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
		playerVLC = psiOptions->getPluginOption(constPlayerVLC, QVariant(playerVLC)).toBool();
		playerTotem = psiOptions->getPluginOption(constPlayerTotem, QVariant(playerTotem)).toBool();
		playerGMPlayer = psiOptions->getPluginOption(constPlayerGMPlayer, QVariant(playerGMPlayer)).toBool();
		playerKaffeine = psiOptions->getPluginOption(constPlayerKaffeine, QVariant(playerKaffeine)).toBool();
		setValidPlayers();
		statuses_.clear();
		status = psiOptions->getPluginOption(constStatus, QVariant(status)).toString();
		statusMessage = psiOptions->getPluginOption(constStatusMessage, QVariant(statusMessage)).toString();
		setOnline = psiOptions->getPluginOption(constSetOnline, QVariant(setOnline)).toBool();
		restoreDelay = psiOptions->getPluginOption(constRestoreDelay, QVariant(restoreDelay)).toInt();
		setDelay = psiOptions->getPluginOption(constSetDelay, QVariant(setDelay)).toInt();
		fullScreen = psiOptions->getPluginOption(constFullScreen, fullScreen).toBool();
		qDBusRegisterMetaType<PlayerStatus>();
		//подключаемся к сессионной шине с соединением по имени busName
		QDBusConnection::connectToBus(QDBusConnection::SessionBus, busName);
		players_ = QDBusConnection(busName).interface()->registeredServiceNames().value();
		//проверка на наличие уже запущенных плееров
		foreach(QString player, validPlayers_) {
			if (players_.contains(player)) {
				connectToBus(player);
			}
		}
		//цепляем сигнал появления новых плееров
		QDBusConnection(busName).connect(QLatin1String("org.freedesktop.DBus"),
			    QLatin1String("/org/freedesktop/DBus"),
			    QLatin1String("org.freedesktop.DBus"),
			    QLatin1String("NameOwnerChanged"),
			    this,
			    SLOT(checkMprisService(QString, QString, QString)));

		fullST.setInterval(timeout);
		connect(&fullST, SIGNAL(timeout()), SLOT(fullSTTimeout()));
		if(fullScreen)
			fullST.start();
	}
	return enabled;
}

bool VideoStatusChanger::disable(){
	enabled = false;
	fullST.stop();
	//отключаем прослушку активных плееров
	foreach(const QString &player, players_) {
		disconnectFromBus(player);
	}
	//отключаеся от шины
	QDBusConnection(busName).disconnect(QLatin1String("org.freedesktop.DBus"),
		       QLatin1String("/org/freedesktop/DBus"),
		       QLatin1String("org.freedesktop.DBus"),
		       QLatin1String("NameOwnerChanged"),
		       this,
		       SLOT(checkMprisService(QString, QString, QString)));
	QDBusConnection::disconnectFromBus(busName);
	//убиваем таймер если он есть
	if(checkTimer) {
		checkTimer->stop();
		disconnect(checkTimer, SIGNAL(timeout()), this, SLOT(timeOut()));
		delete(checkTimer);
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

	fullScreen = ui_.cb_fullScreen->isChecked();
	psiOptions->setPluginOption(constFullScreen, fullScreen);

	setValidPlayers();

	if(fullScreen) {
		fullST.start();
	}
	else if (fullST.isActive()) {
		fullST.stop();
	}

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
	ui_.cb_fullScreen->setChecked(fullScreen);
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

void VideoStatusChanger::setValidPlayers()
{
	//функция работы со списком разрешенных плееров - ?
	int index;
	if (playerVLC && !isPlayerValid(vlcService)) {
		validPlayers_ << vlcService;
	}
	else if (!playerVLC && isPlayerValid(vlcService)) {
		index = validPlayers_.indexOf(vlcService);
		validPlayers_.removeAt(index);
	}
	if (playerTotem && !isPlayerValid(totemService)) {
		validPlayers_ << totemService;
	}
	else if (!playerTotem && isPlayerValid(totemService)) {
		index = validPlayers_.indexOf(totemService);
		validPlayers_.removeAt(index);
	}
	if (playerKaffeine && !isPlayerValid(kaffeineService)) {
		validPlayers_ << kaffeineService;
	}
	else if (!playerKaffeine && isPlayerValid(kaffeineService)) {
		index = validPlayers_.indexOf(kaffeineService);
		validPlayers_.removeAt(index);
	}
	if (playerGMPlayer && !isPlayerValid(gmplayerService)) {
		validPlayers_ << gmplayerService;
	}
	else if (!playerGMPlayer && isPlayerValid(gmplayerService)) {
		index = validPlayers_.indexOf(gmplayerService);
		validPlayers_.removeAt(index);
	}
}

bool VideoStatusChanger::isPlayerValid(const QString &service) //проверка является ли плеер разрешенным
{
	return validPlayers_.contains(service);
}

void VideoStatusChanger::checkMprisService(const QString &name, const QString &oldOwner, const QString &newOwner)
{
	//слот вызывается при изменении имён сервисов в шине
	Q_UNUSED(oldOwner);
	if ((name.startsWith("org.mpris") || name.startsWith(gmplayerService)) && isPlayerValid(name)) {
		int playerIndex = players_.indexOf(name);
		if (playerIndex == -1) {
			if (!newOwner.isEmpty()) {
				//если сервис только появился добавляем его в очередь и подключаемся к нему
				players_.append(name);
				connectToBus(name);
			}
		}
		else if (newOwner.isEmpty()) {
			//если сервис был то отключаемся от него и удаляем из очереди
			disconnectFromBus(name);
			players_.removeAt(playerIndex);
		}
	}
}


void VideoStatusChanger::startCheckTimer()
{
	//работа с таймером для плеера Gnome MPlayer
	if(!checkTimer) {
		checkTimer = new QTimer();
		checkTimer->setInterval(timeout);
		connect(checkTimer, SIGNAL(timeout()), this, SLOT(timeOut()));
		checkTimer->setInterval(timeout);
		checkTimer->start();
	}
	else {
		checkTimer->stop();
		disconnect(checkTimer);
		delete(checkTimer);
		setStatusTimer(restoreDelay, false);
	}
}

void VideoStatusChanger::setStatusTimer(const int delay, const bool isStart)
{
	//запуск таймера установки / восстановления статуса
	if ((isStart | setOnline) != 0) {
		QTimer::singleShot(delay*1000, this, SLOT(delayTimeout()));
		isStatusSet = isStart;
	}
}

void VideoStatusChanger::connectToBus(const QString &service_)
{
	if (service_.contains("org.mpris")) {
		QDBusConnection(busName).connect(service_,
			    QLatin1String("/Player"),
			    QLatin1String("org.freedesktop.MediaPlayer"),
			    QLatin1String("StatusChange"),
			    QLatin1String("(iiii)"),
			    this,
			    SLOT(onPlayerStatusChange(PlayerStatus)));
	}
	else if (service_.contains(gmplayerService)) {
		startCheckTimer();
	}
}

void VideoStatusChanger::disconnectFromBus(const QString &service_)
{
	if (service_.contains("org.mpris")) {
		QDBusConnection(busName).disconnect(service_,
			       QLatin1String("/Player"),
			       QLatin1String("org.freedesktop.MediaPlayer"),
			       QLatin1String("StatusChange"),
			       QLatin1String("(iiii)"),
			       this,
			       SLOT(onPlayerStatusChange(PlayerStatus)));
		if (isStatusSet) {
			setStatusTimer(restoreDelay, false);
		}
	}
	else if (service_.contains(gmplayerService)) {
		startCheckTimer();
	}
}

void VideoStatusChanger::onPlayerStatusChange(const PlayerStatus &st)
{
	if (st.playStatus == StatusPlaying) {
		fullST.stop();
		setStatusTimer(setDelay, true);
	}
	else {
		setStatusTimer(restoreDelay, false);
		fullST.start();
	}
}

void VideoStatusChanger::timeOut() {

	if(playerGMPlayer) {
		bool reply = sendDBusCall(gmplayerService,"/", gmplayerService,"GetPlayState");
		if(reply) {
			if(!isStatusSet) {
				fullST.stop();
				setStatusTimer(setDelay, true);
			}
		}
		else if(isStatusSet) {
			setStatusTimer(restoreDelay, false);
			fullST.start();
		}
	}
}

static WindowList getWindows(Atom prop)
{
	WindowList res;
	Atom type = 0;
	int format = 0;
	uchar* data = 0;
	ulong count, after;
	Display* display = QX11Info::display();
	Window window = QX11Info::appRootWindow();
	if (XGetWindowProperty(display, window, prop, 0, 1024 * sizeof(Window) / 4, False, AnyPropertyType,
			       &type, &format, &count, &after, &data) == Success)
	{
		Window* list = reinterpret_cast<Window*>(data);
		for (uint i = 0; i < count; ++i)
			res += list[i];
		if (data)
			XFree(data);
	}
	return res;
}

static Window activeWindow()
{
	static Atom net_active = 0;
	if (!net_active)
		net_active = XInternAtom(QX11Info::display(), "_NET_ACTIVE_WINDOW", True);

	return getWindows(net_active).value(0);
}

void VideoStatusChanger::fullSTTimeout()
{
	Window w = activeWindow();
	Display  *display = QX11Info::display();
	bool full = false;
	static Atom state = XInternAtom(display, "_NET_WM_STATE", False);
	static Atom  fullScreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
	Atom actual_type;
	int actual_format;
	unsigned long nitems;
	unsigned long bytes;
	unsigned char *data;

	if (XGetWindowProperty(display, w, state, 0, (~0L), False, AnyPropertyType,
			       &actual_type, &actual_format, &nitems, &bytes, &data) == Success) {
		if(nitems != 0) {
			Atom *atom = reinterpret_cast<Atom*>(data);
			for (ulong i = 0; i < nitems; i++) {
				if(atom[i] == fullScreen) {
					full = true;
					break;
				}
			}
		}
	}
	if(data)
		XFree(data);

	if(full) {
		if(!isStatusSet) {
			setStatusTimer(setDelay, true);
		}
	}
	else if(isStatusSet) {
		setStatusTimer(restoreDelay, false);
	}
}

void VideoStatusChanger::delayTimeout() {
	setPsiGlobalStatus(!isStatusSet);
}

bool VideoStatusChanger::sendDBusCall(const QString &service, const QString &path, const QString &interface, const QString &command){
	//qDebug("Checking service %s", qPrintable(service));
	QDBusInterface player(service, path, interface);
	if(player.isValid()) {
		//Get PlayState reply from com.gnome.mplayer (2-paused, 3-playing,stopped, 6-just_oppened(playing))
		QDBusReply<int> gmstatereply = player.call(command);
		if(gmstatereply.isValid()) {
			//Get Track position reply in precents. Needed to catch stopped state
			QDBusReply<double> gmposreply = player.call("GetPercent");
			if(gmposreply.isValid()){
				//if state is playing or just_oppened(playing) and if not stopped returns true
				if(((gmstatereply.value()==3)||(gmstatereply.value()==6)) && gmposreply.value()>0){
					return true;
				}
				else {
					return false;
				}
			}
		}
		else {
			return false;
		}
	}
	return false;
}

void VideoStatusChanger::setPsiGlobalStatus(const bool set) {
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
