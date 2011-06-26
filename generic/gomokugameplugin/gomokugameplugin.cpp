/*
 * gomokugameplugin.cpp - Gomoku Game plugin
 * Copyright (C) 2011  Aleksey Andreev
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * You can also redistribute and/or modify this program under the
 * terms of the Psi License, specified in the accompanied COPYING
 * file, as published by the Psi Project; either dated January 1st,
 * 2005, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "gomokugameplugin.h"
#include "gamesessions.h"
#include "common.h"

#define constVersion            "0.0.8"
#define constShortPluginName    "gomokugameplugin"
#define constDndDisable         "dnddsbl"
#define constConfDisable        "confdsbl"
#define constDefSoundSettings   "defsndstngs"
#define constSaveWndPosition    "savewndpos"
#define constSaveWndWidthHeight "savewndwh"
#define constWindowTop          "wndtop"
#define constWindowLeft         "wndleft"
#define constWindowWidth        "wndwidth"
#define constWindowHeight       "wndheight"


Q_EXPORT_PLUGIN(GomokuGamePlugin);

GomokuGamePlugin::GomokuGamePlugin(QObject *parent) :
		QObject(parent),
		enabled_(false),
		psiOptions(NULL),
		psiTab(NULL),
		psiIcon(NULL),
		psiAccInfo(NULL),
		psiContactInfo(NULL),
		psiSender(NULL),
		psiEvent(NULL),
		psiSound(NULL),
		psiPopup(NULL),
		soundStart("sound/chess_start.wav"),
		soundFinish("sound/chess_finish.wav"),
		soundMove("sound/chess_move.wav"),
		soundError("sound/chess_error.wav"),
		dndDisable(true),
		confDisable(true),
		defSoundSettings(false)
{
}

QString GomokuGamePlugin::name() const
{
	return constPluginName;
}

QString GomokuGamePlugin::shortName() const
{
	return constShortPluginName;
}

QString GomokuGamePlugin::version() const
{
	return constVersion;
}

QWidget *GomokuGamePlugin::options()
{
	QWidget *options = new QWidget;
	ui_.setupUi(options);
	ui_.play_error->setIcon(psiIcon->getIcon("psi/play"));
	ui_.play_finish->setIcon(psiIcon->getIcon("psi/play"));
	ui_.play_move->setIcon(psiIcon->getIcon("psi/play"));;
	ui_.play_start->setIcon(psiIcon->getIcon("psi/play"));
	ui_.select_error->setIcon(psiIcon->getIcon("psi/browse"));
	ui_.select_finish->setIcon(psiIcon->getIcon("psi/browse"));
	ui_.select_move->setIcon(psiIcon->getIcon("psi/browse"));
	ui_.select_start->setIcon(psiIcon->getIcon("psi/browse"));
	restoreOptions();
	connect(ui_.play_error, SIGNAL(clicked()), this, SLOT(testSound()));
	connect(ui_.play_finish, SIGNAL(clicked()), this, SLOT(testSound()));
	connect(ui_.play_move, SIGNAL(clicked()), this, SLOT(testSound()));
	connect(ui_.play_start, SIGNAL(clicked()), this, SLOT(testSound()));
	connect(ui_.select_error, SIGNAL(clicked()), this, SLOT(getSound()));
	connect(ui_.select_finish, SIGNAL(clicked()), this, SLOT(getSound()));
	connect(ui_.select_start, SIGNAL(clicked()), this, SLOT(getSound()));
	connect(ui_.select_move, SIGNAL(clicked()), this, SLOT(getSound()));
	return options;
}

bool GomokuGamePlugin::enable()
{
	if (enabled_)
		return true;
	// Грузим иконку плагина
	QFile file(":/gomokugameplugin/gomoku");
	if(file.open(QIODevice::ReadOnly)) {
		QByteArray ico = file.readAll();
		psiIcon->addIcon("gomokugameplugin/gomoku", ico);
		file.close();
	}
	// Грузим настройки плагина
	soundStart = psiOptions->getPluginOption(constSoundStart, QVariant(soundStart)).toString();
	soundFinish = psiOptions->getPluginOption(constSoundFinish, QVariant(soundFinish)).toString();
	soundMove = psiOptions->getPluginOption(constSoundMove, QVariant(soundMove)).toString();
	soundError = psiOptions->getPluginOption(constSoundError, QVariant(soundError)).toString();
	dndDisable = psiOptions->getPluginOption(constDndDisable, QVariant(dndDisable)).toBool();
	confDisable = psiOptions->getPluginOption(constConfDisable, QVariant(confDisable)).toBool();
	defSoundSettings = psiOptions->getPluginOption(constDefSoundSettings, QVariant(defSoundSettings)).toBool();
	GameSessions::saveWndPosition = psiOptions->getPluginOption(constSaveWndPosition, QVariant(GameSessions::saveWndPosition)).toBool();
	GameSessions::saveWndWidthHeight = psiOptions->getPluginOption(constSaveWndWidthHeight, QVariant(GameSessions::saveWndWidthHeight)).toBool();
	GameSessions::windowTop = psiOptions->getPluginOption(constWindowTop, QVariant(GameSessions::windowTop)).toInt();
	GameSessions::windowLeft = psiOptions->getPluginOption(constWindowLeft, QVariant(GameSessions::windowLeft)).toInt();
	GameSessions::windowWidth = psiOptions->getPluginOption(constWindowWidth, QVariant(GameSessions::windowWidth)).toInt();
	GameSessions::windowHeight = psiOptions->getPluginOption(constWindowHeight, QVariant(GameSessions::windowHeight)).toInt();
	// Создаем соединения с менеджером игровых сессий
	connect(GameSessions::instance(), SIGNAL(sendStanza(int, QString)), this, SLOT(sendGameStanza(int, QString)), Qt::QueuedConnection);
	connect(GameSessions::instance(), SIGNAL(doPopup(const QString)), this, SLOT(doPopup(const QString)), Qt::QueuedConnection);
	connect(GameSessions::instance(), SIGNAL(playSound(const QString)), this, SLOT(playSound(const QString)), Qt::QueuedConnection);
	connect(GameSessions::instance(), SIGNAL(closeWindow()), this, SLOT(onCloseWindow()), Qt::QueuedConnection);
	// Выставляем флаг и уходим
	enabled_ = true;
	return true;
}

bool GomokuGamePlugin::disable()
{
	enabled_ = false;
	GameSessions::reset();
	return true;
}

void GomokuGamePlugin::applyOptions()
{
	soundError = ui_.le_error->text();
	psiOptions->setPluginOption(constSoundError, QVariant(soundError));
	soundFinish = ui_.le_finish->text();
	psiOptions->setPluginOption(constSoundFinish, QVariant(soundFinish));
	soundMove = ui_.le_move->text();
	psiOptions->setPluginOption(constSoundMove, QVariant(soundMove));
	soundStart = ui_.le_start->text();
	psiOptions->setPluginOption(constSoundStart, QVariant(soundStart));
	dndDisable = ui_.cb_disable_dnd->isChecked();
	psiOptions->setPluginOption(constDndDisable, QVariant(dndDisable));
	confDisable = ui_.cb_disable_conf->isChecked();
	psiOptions->setPluginOption(constConfDisable, QVariant(confDisable));
	defSoundSettings = ui_.cb_sound_override->isChecked();
	psiOptions->setPluginOption(constDefSoundSettings, QVariant(defSoundSettings));
	GameSessions::saveWndPosition = ui_.cb_save_pos->isChecked();
	psiOptions->setPluginOption(constSaveWndPosition, QVariant(GameSessions::saveWndPosition));
	GameSessions::saveWndWidthHeight = ui_.cb_save_w_h->isChecked();
	psiOptions->setPluginOption(constSaveWndWidthHeight, QVariant(GameSessions::saveWndWidthHeight));
}

void GomokuGamePlugin::restoreOptions()
{
	ui_.le_error->setText(soundError);
	ui_.le_finish->setText(soundFinish);
	ui_.le_move->setText(soundMove);
	ui_.le_start->setText(soundStart);
	ui_.cb_disable_dnd->setChecked(dndDisable);
	ui_.cb_disable_conf->setChecked(confDisable);
	ui_.cb_sound_override->setChecked(defSoundSettings);
	ui_.cb_save_pos->setChecked(GameSessions::saveWndPosition);
	ui_.cb_save_w_h->setChecked(GameSessions::saveWndWidthHeight);
}

/**
 * Получение списка ресурсов и вызов формы для отправки приглашения
 */
void GomokuGamePlugin::invite(int account, QString full_jid)
{
	QStringList jid_parse = full_jid.split("/");
	QString jid = jid_parse.takeFirst();
	if (jid.isEmpty())
		return;
	QStringList res_list;
	if (psiContactInfo->isPrivate(account, full_jid)) {
		// Это конференция
		if (jid_parse.size() == 0)
			return;
		res_list.push_back(jid_parse.join("/"));
	} else {
		// Получаем список ресурсов оппонента
		res_list = psiContactInfo->resources(account, jid);
	}
	// Отправляем приглашение
	GameSessions::instance()->invite(account, jid, res_list);
}

// ------------------------------------------ Slots ------------------------------------------

/**
 * Кто то кликнул по кнопке тулбара в окне чата
 */
void GomokuGamePlugin::toolButtonPressed()
{
	if (!enabled_)
		return;
	// Получаем наш account id
	QString jid = psiTab->getYourJid();
	int account = -1;
	for (int i = 0; ; i++) {
		QString str1 = psiAccInfo->getJid(i);
		if (str1 == jid) {
			account = i;
			break;
		}
		if (str1 == "-1")
			return;
	}
	// Проверяем статус аккаунта
	if (account == -1 || psiAccInfo->getStatus(account) == "offline")
		return;
	// --
	invite(account, psiTab->getJid());
}

/**
 * Кто то выбрал плагин в меню ростера
 */
void GomokuGamePlugin::menuActivated()
{
	if(!enabled_)
		return;
	int account = sender()->property("account").toInt();
	if (psiAccInfo->getStatus(account) == "offline")
		return;
	QString jid = sender()->property("jid").toString();
	invite(account, jid);
}

/**
 * Отсылка станзы по запросу игры
 */
void GomokuGamePlugin::sendGameStanza(int account, QString stanza)
{
	if (!enabled_ || psiAccInfo->getStatus(account) == "offline")
		return;
	psiSender->sendStanza(account, stanza);
}

/**
 * Пришло приглашение от другого игрока
 */
void GomokuGamePlugin::showInvitation(QString from)
{
	GameSessions::instance()->showInvitation(-1, from);
}

void GomokuGamePlugin::testSound() {
	QObject *sender_ = sender();
	if (sender_ == (ui_.play_error)) {
		psiSound->playSound(ui_.le_error->text());
	} else if (sender_ == ui_.play_finish) {
		psiSound->playSound(ui_.le_finish->text());
	} else if (sender_ == ui_.play_move) {
		psiSound->playSound(ui_.le_move->text());
	} else if (sender_ == ui_.play_start) {
		psiSound->playSound(ui_.le_start->text());
	}
}

void GomokuGamePlugin::getSound() {
	QObject *sender_ = sender();
	QLineEdit *le = 0;
	if (sender_ == ui_.select_error) {
		le = ui_.le_error;
	} else if (sender_ == ui_.select_finish) {
		le = ui_.le_finish;
	} else if (sender_ == ui_.select_move) {
		le = ui_.le_move;
	} else if (sender_ == ui_.select_start) {
		le = ui_.le_start;
	}
	if (!le)
		return;
	QString file_name = QFileDialog::getOpenFileName(0, tr("Choose a sound file"), "", tr("Sound (*.wav)"));
	if (file_name.isEmpty())
		return;
	le->setText(file_name);
}

void GomokuGamePlugin::doPopup(const QString text) {
	psiPopup->initPopup(text, tr(constPluginName), "gomokugameplugin/gomoku");
}

void GomokuGamePlugin::playSound(const QString sound_id) {
	if (defSoundSettings || psiOptions->getGlobalOption("options.ui.notifications.sounds.enable").toBool()) {
		if (sound_id == constSoundMove) {
			psiSound->playSound(soundMove);
		} else if (sound_id == constSoundStart) {
			psiSound->playSound(soundStart);
		} else if (sound_id == constSoundFinish) {
			psiSound->playSound(soundFinish);
		} else if (sound_id == constSoundError) {
			psiSound->playSound(soundError);
		}
	}
}

void GomokuGamePlugin::onCloseWindow()
{
	if (GameSessions::saveWndPosition) {
		psiOptions->setPluginOption(constWindowTop, QVariant(GameSessions::windowTop));
		psiOptions->setPluginOption(constWindowLeft, QVariant(GameSessions::windowLeft));
	}
	if (GameSessions::saveWndWidthHeight) {
		psiOptions->setPluginOption(constWindowWidth, QVariant(GameSessions::windowWidth));
		psiOptions->setPluginOption(constWindowHeight, QVariant(GameSessions::windowHeight));
	}
}

// --------------------- Plugin info provider ---------------------------

QString GomokuGamePlugin::pluginInfo()
{
	return tr("Author: ") +  "Liuch\n"
		+ tr("Email: ") + "liuch@mail.ru\n\n"
		+ trUtf8("This plugin allows you to play gomoku with your friends.\n"
			 "For sending commands, normal messages are used, so this plugin will always work wherever you are able to log in."
			 "To invite a friend for a game, you can use contact menu item or the button on the toolbar in a chat window.");
}

// --------------------- Option accessor ---------------------------

void GomokuGamePlugin::setOptionAccessingHost(OptionAccessingHost *host)
{
	psiOptions = host;
}

void GomokuGamePlugin::optionChanged(const QString &/*option*/)
{
}

// --------------------- Iconfactory accessor ---------------------------
void GomokuGamePlugin::setIconFactoryAccessingHost(IconFactoryAccessingHost *host)
{
	psiIcon = host;
}

// --------------------- Toolbar icon accessor ---------------------------
QList<QVariantHash> GomokuGamePlugin::getButtonParam()
{
	QList<QVariantHash> list;
	QVariantHash hash;
	hash["tooltip"] = QVariant(tr("Gomoku game"));
	hash["icon"] = QVariant(QString("gomokugameplugin/gomoku"));
	hash["reciver"] = qVariantFromValue(qobject_cast<QObject *>(this));
	hash["slot"] = QVariant(SLOT(toolButtonPressed()));
	list.push_back(hash);
	return list;
}

QAction* GomokuGamePlugin::getAction(QObject* /*parent*/, int /*account*/, const QString& /*contact*/)
{
	return NULL;
}

// --------------------- Activetab accessor ---------------------------

void GomokuGamePlugin::setActiveTabAccessingHost(ActiveTabAccessingHost *host)
{
	psiTab = host;
}

// --------------------- Account info accessor ---------------------------

void GomokuGamePlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost * host)
{
	psiAccInfo = host;
}

// --------------------- Contact info accessor ---------------------------

void GomokuGamePlugin::setContactInfoAccessingHost(ContactInfoAccessingHost * host)
{
	psiContactInfo = host;
}

// --------------------- Stanza sender ---------------------------

void GomokuGamePlugin::setStanzaSendingHost(StanzaSendingHost *host)
{
	psiSender = host;
}

// --------------------- Stanza filter ---------------------------

bool GomokuGamePlugin::incomingStanza(int account, const QDomElement& xml)
{
	if(xml.tagName() == "iq") {
		QString iq_type = xml.attribute("type");
		if(iq_type == "set") {
			QDomElement childElem = xml.firstChildElement("create");
			if(!childElem.isNull() && childElem.attribute("xmlns") == "games:board"
			   && childElem.attribute("type") == constProtoType) {
				QString from = xml.attribute("from");
				if ((dndDisable && psiAccInfo->getStatus(account) == "dnd")
				|| (confDisable && psiContactInfo->isPrivate(account, from))) {
					sendGameStanza(account, XML::iqErrorString(from, xml.attribute("id")));
					return true;
				}
				if (GameSessions::instance()->incomingInvitation(account, from, childElem.attribute("color"), xml.attribute("id"), childElem.attribute("id"))) {
					psiEvent->createNewEvent(account, from,
						tr("%1: Invitation from %2").arg(constPluginName).arg(from),
						this, SLOT(showInvitation(QString)));
				}
				return true;
			}
			childElem = xml.firstChildElement("turn");
			if (!childElem.isNull() && childElem.attribute("xmlns") == "games:board"
				&& childElem.attribute("type") == constProtoType) {
				QDomElement turnChildElem = childElem.firstChildElement("move");
				if (!turnChildElem.isNull()) {
					GameSessions::instance()->doTurnAction(account, xml.attribute("from"), xml.attribute("id"), turnChildElem.attribute("pos"));
					return true;
				}
				turnChildElem = childElem.firstChildElement("resign");
				if (!turnChildElem.isNull()) {
					GameSessions::instance()->youWin(account, xml.attribute("from"), xml.attribute("id"));
					return true;
				}
				turnChildElem = childElem.firstChildElement("draw");
				if (!turnChildElem.isNull()) {
					GameSessions::instance()->setDraw(account, xml.attribute("from"), xml.attribute("id"));
				}
				return true;
			}
			childElem = xml.firstChildElement("close");
			if (!childElem.isNull() && childElem.attribute("xmlns") == "games:board"
			    && childElem.attribute("type") == constProtoType) {
				GameSessions::instance()->closeRemoteGameBoard(account, xml.attribute("from"), xml.attribute("id"));
				return true;
			}
			childElem = xml.firstChildElement("load");
			if (!childElem.isNull() && childElem.attribute("xmlns") == "games:board"
			    && childElem.attribute("type") == constProtoType) {
				GameSessions::instance()->remoteLoad(account, xml.attribute("from"), xml.attribute("id"), childElem.text());
				return true;
			}
		} else if (iq_type == "result") {
			if (GameSessions::instance()->doResult(account, xml.attribute("from"), xml.attribute("id"))) {
				return true;
			}
		} else if (iq_type == "error") {
			if (GameSessions::instance()->doReject(account, xml.attribute("from"), xml.attribute("id"))) {
				return true;
			}
		}
	}
	return false;
}

bool GomokuGamePlugin::outgoingStanza(int /*account*/, QDomElement& /*xml*/)
{
	return false;
}

// --------------------- Event creator ---------------------------

void GomokuGamePlugin::setEventCreatingHost(EventCreatingHost *host)
{
	psiEvent = host;
}

// --------------------- Sound accessor ---------------------------

void GomokuGamePlugin::setSoundAccessingHost(SoundAccessingHost *host)
{
	psiSound = host;
}

// --------------------- Menu accessor ---------------------------

QList<QVariantHash> GomokuGamePlugin::getAccountMenuParam()
{
	return QList<QVariantHash>();
}

QList<QVariantHash> GomokuGamePlugin::getContactMenuParam()
{
	QList<QVariantHash> menu_list;
	QVariantHash hash;
	hash["name"] = QVariant(tr("Gomoku game!"));
	hash["icon"] = QVariant(QString("gomokugameplugin/gomoku"));
	hash["reciver"] = qVariantFromValue(qobject_cast<QObject *>(this));
	hash["slot"] = QVariant(SLOT(menuActivated()));
	menu_list.push_back(hash);
	return menu_list;
}

QAction* GomokuGamePlugin::getContactAction(QObject*, int, const QString&)
{
	return NULL;
}

QAction* GomokuGamePlugin::getAccountAction(QObject*, int)
{
	return NULL;
}

// --------------------- Popup accessor ---------------------------

void GomokuGamePlugin::setPopupAccessingHost(PopupAccessingHost *host)
{
	psiPopup = host;
}
