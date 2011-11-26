/*
 * videostatuspluginwin.cpp - plugin
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
#include "w32api.h"
#include "windows.h"

#include "psiplugin.h"
#include "plugininfoprovider.h"
#include "optionaccessinghost.h"
#include "optionaccessor.h"
#include "accountinfoaccessinghost.h"
#include "accountinfoaccessor.h"
#include "psiaccountcontroller.h"
#include "psiaccountcontrollinghost.h"

#include "ui_winoptions.h"

#define constVersion "0.1.2"

#define constStatus "status"
#define constStatusMessage "statusmessage"
#define constSetOnline "setonline"
#define constRestoreDelay "restoredelay"
#define constSetDelay "setdelay"
#define constFullScreen "fullscreen"

static const int StatusPlaying = 0;

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

private:
	bool enabled;
	OptionAccessingHost* psiOptions;
	AccountInfoAccessingHost* accInfo;
	PsiAccountControllingHost* accControl;
	QString status, statusMessage;
	Ui::OptionsWidget ui_;
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
	bool isFullscreenWindow();
	void setPsiGlobalStatus(const bool set);
	void startCheckTimer();
	void setStatusTimer(const int delay, const bool isStart);

private slots:
	void delayTimeout();
	void fullSTTimeout();

};

Q_EXPORT_PLUGIN(VideoStatusChanger);

VideoStatusChanger::VideoStatusChanger() {
	enabled = false;
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
		statuses_.clear();
		status = psiOptions->getPluginOption(constStatus, QVariant(status)).toString();
		statusMessage = psiOptions->getPluginOption(constStatusMessage, QVariant(statusMessage)).toString();
		setOnline = psiOptions->getPluginOption(constSetOnline, QVariant(setOnline)).toBool();
		restoreDelay = psiOptions->getPluginOption(constRestoreDelay, QVariant(restoreDelay)).toInt();
		setDelay = psiOptions->getPluginOption(constSetDelay, QVariant(setDelay)).toInt();
		fullScreen = psiOptions->getPluginOption(constFullScreen, fullScreen).toBool();
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
	return true;
}

void VideoStatusChanger::applyOptions() {
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

void VideoStatusChanger::restoreOptions() {
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

void VideoStatusChanger::setStatusTimer(const int delay, const bool isStart)
{
	//запуск таймера установки / восстановления статуса
	if ((isStart | setOnline) != 0) {
		QTimer::singleShot(delay*1000, this, SLOT(delayTimeout()));
		isStatusSet = isStart;
	}
}

bool VideoStatusChanger::isFullscreenWindow()
{
	HWND topWindow = GetForegroundWindow();
	HWND desktop = GetDesktopWindow();
	RECT windowRect;
	RECT desktopRect;
	if (GetWindowRect(topWindow, &windowRect) && GetWindowRect(desktop, &desktopRect)) {
		if ((windowRect.right - windowRect.left) == (desktopRect.right - desktopRect.left)
		    && (windowRect.bottom - windowRect.top) == (desktopRect.bottom - desktopRect.top)) {
			return true;
		}
		else {
			return false;
		}
	}
	return false;
}

void VideoStatusChanger::fullSTTimeout()
{
	bool full = isFullscreenWindow();
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
#include "videostatuspluginwin.moc"
