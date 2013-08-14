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
#include "psiplugin.h"
#include "plugininfoprovider.h"
#include "optionaccessinghost.h"
#include "optionaccessor.h"
#include "accountinfoaccessinghost.h"
#include "accountinfoaccessor.h"
#include "psiaccountcontroller.h"
#include "psiaccountcontrollinghost.h"

#include "ui_options.h"

#ifdef Q_WS_WIN
#include "windows.h"
#elif defined (Q_WS_X11)
#include <QCheckBox>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QDBusInterface>
#include <QDBusConnectionInterface>
#include <QX11Info>
#include <X11/Xlib.h>

static const QString MPRIS_PREFIX = "org.mpris";
static const QString MPRIS2_PREFIX = "org.mpris.MediaPlayer2";
static const QString GMP_PREFIX = "com.gnome";

static const int StatusPlaying = 0;
static const int gmpStatusPlaying = 3;

typedef QList<Window> WindowList;
typedef QPair<QString, QString> StringMap;

//имена сервисов. Для добавления нового плеера дописываем имя сервиса
static const QList<StringMap> players = QList<StringMap>() << StringMap("vlc", "VLC")
							   << StringMap("Totem", "Totem (>=2.30.2)")
							   << StringMap("kaffeine", "Kaffeine (>=1.0)")
							   << StringMap("mplayer", "GNOME MPlayer");
struct PlayerStatus
{
	int playStatus;
	int playOrder;
	int playRepeat;
	int stopOnce;
};

Q_DECLARE_METATYPE(PlayerStatus);

static const QDBusArgument & operator<<(QDBusArgument &arg, const PlayerStatus &ps)
{
	arg.beginStructure();
	arg << ps.playStatus
	    << ps.playOrder
	    << ps.playRepeat
	    << ps.stopOnce;
	arg.endStructure();
	return arg;
}

static const QDBusArgument & operator>>(const QDBusArgument &arg, PlayerStatus &ps)
{
	arg.beginStructure();
	arg >> ps.playStatus
	    >> ps.playOrder
	    >> ps.playRepeat
	    >> ps.stopOnce;
	arg.endStructure();
	return arg;
}
#endif

#define constVersion "0.2.4"

#define constStatus "status"
#define constStatusMessage "statusmessage"
#define constSetOnline "setonline"
#define constRestoreDelay "restoredelay"
#define constSetDelay "setdelay"
#define constFullScreen "fullscreen"

static const int timeout = 10000;

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
	virtual QPixmap icon() const;

private:
	bool enabled;
	OptionAccessingHost* psiOptions;
	AccountInfoAccessingHost* accInfo;
	PsiAccountControllingHost* accControl;
	QString status, statusMessage;
	Ui::OptionsWidget ui_;
#ifdef Q_WS_X11
	bool playerGMPlayer_; //только для не MPRIS плеера GMPlayer
	QHash<QString, bool> playerDictList;
	QPointer<QTimer> checkTimer; //Таймер Gnome Mplayer
	QStringList validPlayers_; //список включенных плееров
	QStringList services_; //очередь плееров которые слушает плагин
	void connectToBus(const QString &service_);
	void disconnectFromBus(const QString &service_);
	void startCheckTimer();
	bool isPlayerValid(const QString &service);
#endif
	QTimer fullST;
	bool isStatusSet; // здесь храним информацию, установлен ли уже статус (чтобы не устанавливать повторно при каждом срабатывании таймера)
	bool setOnline;
	int restoreDelay; //задержка восстановления статуса
	int setDelay; //задержка установки статуса
	bool fullScreen;

	struct StatusString {
		QString status;
		QString message;
	};
	QHash<int, StatusString> statuses_;
#ifdef Q_WS_WIN
	void getDesktopSize();
	bool isFullscreenWindow();

	int desktopWidth;
	int desktopHeight;
#endif
	void setPsiGlobalStatus(const bool set);
	void setStatusTimer(const int delay, const bool isStart);

private slots:
#ifdef Q_WS_X11
	void checkMprisService(const QString &name, const QString &oldOwner, const QString &newOwner);
	void onPlayerStatusChange(const PlayerStatus &ps);
	void onPropertyChange(const QDBusMessage &msg);
	void timeOut(); //здесь проверяем проигрыватель Gnome Mplayer
	void asyncCallFinished(QDBusPendingCallWatcher *watcher);
#endif

	void delayTimeout();
	void fullSTTimeout();

};

Q_EXPORT_PLUGIN(VideoStatusChanger);

VideoStatusChanger::VideoStatusChanger()
{
	enabled = false;
#ifdef Q_WS_X11
	playerGMPlayer_ = false;
	foreach (StringMap item, players) {
		playerDictList.insert(item.first, false);
	}
#endif
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

QString VideoStatusChanger::name() const
{
	return "Video Status Changer Plugin";
}

QString VideoStatusChanger::shortName() const
{
	return "videostatus";
}

QString VideoStatusChanger::version() const
{
	return constVersion;
}

void VideoStatusChanger::setOptionAccessingHost(OptionAccessingHost* host)
{
	psiOptions = host;
}

void VideoStatusChanger::setAccountInfoAccessingHost(AccountInfoAccessingHost* host)
{
	accInfo = host;
}

void VideoStatusChanger::setPsiAccountControllingHost(PsiAccountControllingHost* host)
{
	accControl = host;
}

bool VideoStatusChanger::enable()
{
	if(psiOptions) {
		enabled = true;
#ifdef Q_WS_X11
		qDBusRegisterMetaType<PlayerStatus>();
		services_ = QDBusConnection::sessionBus().interface()->registeredServiceNames().value();
		//проверка на наличие уже запущенных плееров
		foreach (const QString& item, playerDictList.keys()) {
			bool option = psiOptions->getPluginOption(item, QVariant(playerDictList.value(item))).toBool();
			playerDictList[item] = option;
			if (item.contains("mplayer")) {
				playerGMPlayer_ = option;
			}
			foreach (const QString& service, services_){
				if (service.contains(item, Qt::CaseInsensitive)) {
					connectToBus(service);
				}
			}
		}
#endif
		statuses_.clear();
		status = psiOptions->getPluginOption(constStatus, QVariant(status)).toString();
		statusMessage = psiOptions->getPluginOption(constStatusMessage, QVariant(statusMessage)).toString();
		setOnline = psiOptions->getPluginOption(constSetOnline, QVariant(setOnline)).toBool();
		restoreDelay = psiOptions->getPluginOption(constRestoreDelay, QVariant(restoreDelay)).toInt();
		setDelay = psiOptions->getPluginOption(constSetDelay, QVariant(setDelay)).toInt();
		fullScreen = psiOptions->getPluginOption(constFullScreen, fullScreen).toBool();
#ifdef Q_WS_X11
		//цепляем сигнал появления новых плееров
		QDBusConnection::sessionBus().connect(QLatin1String("org.freedesktop.DBus"),
						      QLatin1String("/org/freedesktop/DBus"),
						      QLatin1String("org.freedesktop.DBus"),
						      QLatin1String("NameOwnerChanged"),
						      this,
						      SLOT(checkMprisService(QString, QString, QString)));
#endif
		fullST.setInterval(timeout);
		connect(&fullST, SIGNAL(timeout()), SLOT(fullSTTimeout()));
		if(fullScreen)
			fullST.start();
	}
	return enabled;
}

bool VideoStatusChanger::disable()
{
	enabled = false;
	fullST.stop();
#ifdef Q_WS_X11
	//отключаем прослушку активных плееров
	foreach(const QString& player, services_) {
		disconnectFromBus(player);
	}
	//отключаеся от шины
	QDBusConnection::sessionBus().disconnect(QLatin1String("org.freedesktop.DBus"),
						 QLatin1String("/org/freedesktop/DBus"),
						 QLatin1String("org.freedesktop.DBus"),
						 QLatin1String("NameOwnerChanged"),
						 this,
						 SLOT(checkMprisService(QString, QString, QString)));
	//убиваем таймер если он есть
	if(checkTimer) {
		checkTimer->stop();
		disconnect(checkTimer, SIGNAL(timeout()), this, SLOT(timeOut()));
		delete(checkTimer);
	}
#endif
	return true;
}

void VideoStatusChanger::applyOptions()
{
#ifdef Q_WS_X11
	//читаем состояние плееров
	if (playerDictList.size() > 0) {
		foreach (const QString& item, playerDictList.keys()) {
			QCheckBox *cb = ui_.groupBox->findChild<QCheckBox *>(item);
			if (cb) {
				playerDictList[item] = cb->isChecked();
				if (item.contains("mplayer")) {
					playerGMPlayer_ = cb->isChecked();
				}
				psiOptions->setPluginOption(item, QVariant(cb->isChecked()));
			}
		}
	}
#endif
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

	if(fullScreen) {
		fullST.start();
	}
	else if (fullST.isActive()) {
		fullST.stop();
	}

}

void VideoStatusChanger::restoreOptions()
{
#ifdef Q_WS_X11
	//читаем состояние плееров
	if (playerDictList.size() > 0) {
		foreach (const QString& item, playerDictList.keys()) {
			bool option = psiOptions->getPluginOption(item, QVariant(playerDictList.value(item))).toBool();
			QCheckBox *cb = ui_.groupBox->findChild<QCheckBox *>(item);
			if (cb) {
				cb->setChecked(option);
			}
		}
	}
#elif defined (Q_WS_WIN)
	ui_.groupBox->hide();
#endif
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

QWidget* VideoStatusChanger::options()
{
	if (!enabled) {
		return 0;
	}
	QWidget *optionsWid = new QWidget();
	ui_.setupUi(optionsWid);
#ifdef Q_WS_X11
	//добавляем чекбоксы плееров
	int i = 0;
	int columns = (players.length() < 5) ? 2 : 3;
	int rows = (players.length() + columns - 1) / columns;
	foreach (StringMap item, players) {
		i = players.indexOf(item);
		if (i != -1) {
			QCheckBox *cb = new QCheckBox(item.second);
			cb->setObjectName(item.first);
			cb->setChecked(false);
			ui_.gridLayout->addWidget(cb,i/rows,i%columns);
		}
	}
#endif
	restoreOptions();

	return optionsWid;
}

QString VideoStatusChanger::pluginInfo()
{
	return tr("Authors: ") +  "Dealer_WeARE, KukuRuzo\n\n"
			+ trUtf8("This plugin is designed to set the custom status when you watching the video in selected video players. \n"
				 "Note: This plugin is designed to work in Linux family operating systems and in Windows OS. \n\n"
				 "In Linux plugin uses DBUS to work with video players and X11 functions to detect fullscreen applications. \n"
				 "In Windows plugin uses WinAPI functions to detect fullscreen applications. \n\n"
				 "To work with Totem player you need to enable appropriate plugin in this player (Edit\\Plugins\\D-Bus);\n\n"
				 "To work with VLC player you need to enable the option \"Control Interface D-Bus\" in the Advanced Settings tab on \"Interface\\Control Interface\" section of the player settings; \n\n"
				 "To work with Kaffeine player you must have player version (>= 1.0), additional configuration is not needed; \n\n"
				 "To work with GNOME MPlayer additional configuration is not needed.");
}

QPixmap VideoStatusChanger::icon() const
{
	return QPixmap(":/icons/videostatus.png");
}

#ifdef Q_WS_X11
bool VideoStatusChanger::isPlayerValid(const QString &service) //проверка является ли плеер разрешенным
{
	foreach (const QString& item, playerDictList.keys()) {
		if (service.contains(item, Qt::CaseInsensitive) && playerDictList.value(item)) {
			return true;
		}
	}
	return false;
}

void VideoStatusChanger::checkMprisService(const QString &name, const QString &oldOwner, const QString &newOwner)
{
	//слот вызывается при изменении имён сервисов в шине
	Q_UNUSED(oldOwner);
	if ((name.startsWith(MPRIS_PREFIX) || name.startsWith(GMP_PREFIX)) && isPlayerValid(name)) {
		int playerIndex = services_.indexOf(name);
		if (playerIndex == -1) {
			if (!newOwner.isEmpty()) {
				//если сервис только появился добавляем его в очередь и подключаемся к нему
				services_.append(name);
				connectToBus(name);
			}
		}
		else if (newOwner.isEmpty()) {
			//если сервис был то отключаемся от него и удаляем из очереди
			disconnectFromBus(name);
			services_.removeAt(playerIndex);
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

void VideoStatusChanger::connectToBus(const QString &service_)
{
	if (service_.contains(MPRIS_PREFIX) && !service_.contains(MPRIS2_PREFIX)) {
		QDBusConnection::sessionBus().connect(service_,
						      QLatin1String("/Player"),
						      QLatin1String("org.freedesktop.MediaPlayer"),
						      QLatin1String("StatusChange"),
						      QLatin1String("(iiii)"),
						      this,
						      SLOT(onPlayerStatusChange(PlayerStatus)));
	}
	else if (service_.contains(MPRIS2_PREFIX)) {
		QDBusConnection::sessionBus().connect(service_,
						      QLatin1String("/org/mpris/MediaPlayer2"),
						      QLatin1String("org.freedesktop.DBus.Properties"),
						      QLatin1String("PropertiesChanged"),
						      this,
						      SLOT(onPropertyChange(QDBusMessage)));
	}
	else if (service_.contains(GMP_PREFIX)) {
		startCheckTimer();
	}
}

void VideoStatusChanger::disconnectFromBus(const QString &service_)
{
	if (service_.contains(MPRIS_PREFIX) && !service_.contains(MPRIS2_PREFIX)) {
		QDBusConnection::sessionBus().disconnect(MPRIS_PREFIX + "." + service_,
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
	else if (service_.contains(MPRIS2_PREFIX)) {
		QDBusConnection::sessionBus().disconnect(MPRIS2_PREFIX + "." + service_.toLower(),
							 QLatin1String("/org/mpris/MediaPlayer2"),
							 QLatin1String("org.freedesktop.DBus.Properties"),
							 QLatin1String("PropertiesChanged"),
							 this,
							 SLOT(onPropertyChange(QDBusMessage)));
	}
	else if (service_.contains("mplayer")) {
		startCheckTimer();
	}
	if (!fullST.isActive() && fullScreen) {
		fullST.start();
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

void VideoStatusChanger::onPropertyChange(const QDBusMessage &msg)
{
	QDBusArgument arg = msg.arguments().at(1).value<QDBusArgument>();
	QVariantMap map = qdbus_cast<QVariantMap>(arg);
	QVariant v = map.value(QLatin1String("PlaybackStatus"));
	if (v.isValid()) {
		if (v.toString() == QLatin1String("Playing")) {
			fullST.stop();
			setStatusTimer(setDelay, true);
		}
		else if (v.toString() == QLatin1String("Paused") || v.toString() == QLatin1String("Stopped")) {
			setStatusTimer(restoreDelay, false);
			fullST.start();
		}
	}

}

void VideoStatusChanger::timeOut()
{
	if(playerGMPlayer_) {
		QString gmplayerService = GMP_PREFIX + ".mplayer";
		QDBusMessage msg = QDBusMessage::createMethodCall(gmplayerService, "/", gmplayerService, "GetPlayState");
		QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(msg);
		QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
		connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
				 this, SLOT(asyncCallFinished(QDBusPendingCallWatcher*)));
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

void VideoStatusChanger::asyncCallFinished(QDBusPendingCallWatcher *watcher)
{
	watcher->deleteLater();
	QDBusMessage msg = watcher->reply();
	if (msg.type() == QDBusMessage::InvalidMessage || msg.arguments().isEmpty()) {
		return;
	}
	QVariant reply = msg.arguments().first();
	if (reply.type() != QVariant::Int) {
		return;
	}
	else {
		int stat = reply.toInt();
		if (stat == gmpStatusPlaying && (!isStatusSet)) {
			fullST.stop();
			setStatusTimer(setDelay, true);
		}
		if(stat != gmpStatusPlaying && isStatusSet) {
			setStatusTimer(restoreDelay, false);
			fullST.start();
		}
	}
}

#endif

void VideoStatusChanger::fullSTTimeout()
{
#ifdef Q_WS_X11
	Window w = activeWindow();
	Display  *display = QX11Info::display();
	bool full = false;
	static Atom state = XInternAtom(display, "_NET_WM_STATE", False);
	static Atom  fullScreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
	Atom actual_type;
	int actual_format;
	unsigned long nitems;
	unsigned long bytes;
	unsigned char *data = 0;

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
#elif defined (Q_WS_WIN)
	bool full = isFullscreenWindow();
#endif
	if(full) {
		if(!isStatusSet) {
			setStatusTimer(setDelay, true);
		}
	}
	else if(isStatusSet) {
		setStatusTimer(restoreDelay, false);
	}
}

#ifdef Q_WS_WIN

void VideoStatusChanger::getDesktopSize()
{
	RECT dSize;
	if (GetWindowRect(GetDesktopWindow(), &dSize)) {
		desktopWidth = abs(dSize.right - dSize.left);
		desktopHeight = abs(dSize.bottom - dSize.top);
	}
	else{
		desktopWidth = GetSystemMetrics(SM_CXSCREEN);
		desktopHeight = GetSystemMetrics(SM_CYSCREEN);
	}
}

bool VideoStatusChanger::isFullscreenWindow()
{
	HWND topWindow = GetForegroundWindow();
	if (topWindow != NULL) {
		//check for Progman window by GetWindow method
		bool isDesktop = (GetWindow(GetWindow(GetDesktopWindow(), GW_CHILD), GW_HWNDLAST) == topWindow);
		if (!isDesktop) {
			//check for Progman and WorkerW by the name of the windows class
			QByteArray ba(4 * sizeof(wchar_t), 0);
			if (GetClassNameW(topWindow, (wchar_t*)ba.data(), ba.size()) > 0) {
				QString className = QString::fromWCharArray((wchar_t*)ba.data(), ba.size());
				if (className.contains("Progman", Qt::CaseInsensitive)
				    || className.contains("WorkerW", Qt::CaseInsensitive)) {
					isDesktop = true;
				}
			}
		}
		getDesktopSize();
		WINDOWINFO info;
		memset(&info, 0, sizeof(WINDOWINFO));
		if (GetWindowInfo(topWindow, &info)) {
			int x = info.rcWindow.left;
			int y = info.rcWindow.top;
			int wWidth = info.rcClient.right;
			int wHeight = info.rcClient.bottom;
			if (x == 0
			    && y == 0
			    && wWidth >= desktopWidth
			    && wHeight >= desktopHeight
			    && !isDesktop) {
				return true;
			}
		}
	}
	return false;
}
#endif

void VideoStatusChanger::setStatusTimer(const int delay, const bool isStart)
{
	//запуск таймера установки / восстановления статуса
	if ((isStart | setOnline) != 0) {
		QTimer::singleShot(delay*1000, this, SLOT(delayTimeout()));
		isStatusSet = isStart;
	}
}

void VideoStatusChanger::delayTimeout()
{
	setPsiGlobalStatus(!isStatusSet);
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
