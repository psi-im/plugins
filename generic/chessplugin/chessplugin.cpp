/*
 * chessplugin.cpp - plugin
 * Copyright (C) 2010-2011  Evgeny Khryukin
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

#include <QMessageBox>
#include <QFileDialog>
#include <QDomElement>

#include "psiplugin.h"
#include "accountinfoaccessor.h"
#include "accountinfoaccessinghost.h"
#include "optionaccessor.h"
#include "optionaccessinghost.h"
#include "activetabaccessinghost.h"
#include "activetabaccessor.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"
#include "iconfactoryaccessor.h"
#include "iconfactoryaccessinghost.h"
#include "toolbariconaccessor.h"
#include "menuaccessor.h"
#include "applicationinfoaccessor.h"
#include "applicationinfoaccessinghost.h"
#include "stanzafilter.h"
#include "plugininfoprovider.h"
#include "eventcreatinghost.h"
#include "eventcreator.h"
#include "contactinfoaccessinghost.h"
#include "contactinfoaccessor.h"
#include "popupaccessinghost.h"
#include "popupaccessor.h"
#include "soundaccessinghost.h"
#include "soundaccessor.h"

#include "mainwindow.h"
#include "figure.h"
#include "ui_options.h"
#include "invitedialog.h"
#include "request.h"

#define cVer "0.2.8"

#define soundStartConst "soundstart"
#define soundFinishConst "soundfinish"
#define soundMoveConst "soundmove"
#define soundErrorConst "sounderror"
#define constDndDisable "dnddsbl"
#define constDefSoundSettings "defsndstngs"

using namespace Chess;

class ChessPlugin: public QObject, public PsiPlugin, public OptionAccessor, public ActiveTabAccessor, public MenuAccessor, public ApplicationInfoAccessor,
				   public ToolbarIconAccessor, public IconFactoryAccessor, public StanzaSender, public AccountInfoAccessor, public StanzaFilter,
				   public PluginInfoProvider, public EventCreator, public ContactInfoAccessor, public PopupAccessor, public SoundAccessor
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "com.psi-plus.ChessPlugin")
	Q_INTERFACES(PsiPlugin AccountInfoAccessor OptionAccessor ActiveTabAccessor MenuAccessor StanzaFilter ContactInfoAccessor SoundAccessor
				 ToolbarIconAccessor IconFactoryAccessor StanzaSender ApplicationInfoAccessor PluginInfoProvider EventCreator PopupAccessor)

public:
	ChessPlugin();
	virtual QString name() const;
	virtual QString shortName() const;
	virtual QString version() const;
	virtual QWidget* options();
	virtual bool enable();
	virtual bool disable();
	virtual void applyOptions();
	virtual void restoreOptions();
	virtual void optionChanged(const QString& /*option*/){};
	virtual void setAccountInfoAccessingHost(AccountInfoAccessingHost* host);
	virtual void setOptionAccessingHost(OptionAccessingHost* host);
	virtual void setActiveTabAccessingHost(ActiveTabAccessingHost* host);
	virtual void setIconFactoryAccessingHost(IconFactoryAccessingHost* host);
	virtual void setStanzaSendingHost(StanzaSendingHost *host);
	virtual QList < QVariantHash > getButtonParam();
	virtual QAction* getAction(QObject* , int , const QString& ) { return 0; };
	virtual QList < QVariantHash > getAccountMenuParam();
	virtual QList < QVariantHash > getContactMenuParam();
	virtual QAction* getContactAction(QObject* , int , const QString& ) { return 0; };
	virtual QAction* getAccountAction(QObject* , int ) { return 0; };
	virtual void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host);
	virtual bool incomingStanza(int account, const QDomElement& xml);
	virtual bool outgoingStanza(int account, QDomElement& xml);
	virtual void setEventCreatingHost(EventCreatingHost *host);
	virtual void setContactInfoAccessingHost(ContactInfoAccessingHost* host);
	virtual QString pluginInfo();
	virtual void setPopupAccessingHost(PopupAccessingHost* host);
	virtual void setSoundAccessingHost(SoundAccessingHost* host);
	virtual QPixmap icon() const;

public slots:
	void closeBoardEvent();
	void move(int oldX, int oldY, int newX, int newY, const QString& figure);
	void moveAccepted();
	void error();
	void load(const QString& settings);

private slots:
	void toolButtonPressed();
	void menuActivated();
	void invite(Request& r);
	void sendInvite(const Request&, const QString& resource, const QString& color);
	void accept();
	void reject();
	void youWin();
	void youLose();
	void draw();
	void getSound();
	void testSound();
	void toggleEnableSound(bool enable);
	void doInviteDialog(const QString& jid);

private:
	const QString newId();
	int checkId(const QString& id);
	void acceptGame();
	void rejectGame();
	void stopGame();
	void playSound(const QString& filename);
	void boardClosed();
	int findRequest(const QString& jid);
	void doPopup(const QString& text);

private:
	bool enabled;
	OptionAccessingHost* psiOptions;
	AccountInfoAccessingHost *accInfoHost;
	ActiveTabAccessingHost* activeTab;
	IconFactoryAccessingHost *icoHost;
	ApplicationInfoAccessingHost* appInfo;
	StanzaSendingHost *stanzaSender;
	EventCreatingHost *psiEvent;
	ContactInfoAccessingHost* contactInfo;
	PopupAccessingHost* popup;
	SoundAccessingHost* sound_;

	ChessWindow *board;
	bool game_, theEnd_;
	bool waitFor;
	int id;
	QString tmpId;

	QString soundStart, soundFinish, soundMove, soundError;
	bool DndDisable, DefSoundSettings, enableSound;

	Ui::options ui_;

	QList<Request> requests;
	QList<Request> invites;
	Request currentGame_;
};

ChessPlugin::ChessPlugin()
	: enabled(false)
	, psiOptions(0)
	, accInfoHost(0)
	, activeTab(0)
	, icoHost(0)
	, appInfo(0)
	, stanzaSender(0)
	, psiEvent(0)
	, contactInfo(0)
	, popup(0)
	, sound_(0)
	, board(0)
	, game_(false)
	, theEnd_(false)
	, waitFor(false)
	, id(111)
	, soundStart("sound/chess_start.wav")
	, soundFinish("sound/chess_finish.wav")
	, soundMove("sound/chess_move.wav")
	, soundError("sound/chess_error.wav")
	, DndDisable(true)
	, DefSoundSettings(false)
	, enableSound(true)
{
}

QString ChessPlugin::name() const {
	return "Chess Plugin";
}

QString ChessPlugin::shortName() const {
	return "chessplugin";
}

QString ChessPlugin::version() const {
	return cVer;
}

bool ChessPlugin::enable() {
	if(!psiOptions) {
		return false;
	}

	game_ = false;
	theEnd_ = false;
	waitFor = false;
	id = 111;
	requests.clear();
	invites.clear();

	enabled = true;
	QFile file(":/chessplugin/figures/Black queen 2d.png");
	if(file.open(QIODevice::ReadOnly)) {
		QByteArray ico = file.readAll();
		icoHost->addIcon("chessplugin/chess", ico);
		file.close();
	}
	soundStart = psiOptions->getPluginOption(soundStartConst, QVariant(soundStart)).toString();
	soundFinish = psiOptions->getPluginOption(soundFinishConst, QVariant(soundFinish)).toString();
	soundMove = psiOptions->getPluginOption(soundMoveConst, QVariant(soundMove)).toString();
	soundError = psiOptions->getPluginOption(soundErrorConst, QVariant(soundError)).toString();
	DndDisable = psiOptions->getPluginOption(constDndDisable, QVariant(DndDisable)).toBool();
	DefSoundSettings = psiOptions->getPluginOption(constDefSoundSettings, QVariant(DefSoundSettings)).toBool();
	return enabled;
}

bool ChessPlugin::disable() {
	if(board) {
		delete (board);
		board = 0;
		game_ = false;
	}

	requests.clear();
	invites.clear();
	enabled = false;
	return true;
}

QWidget* ChessPlugin::options() {
	if(!enabled)
		return 0;

	QWidget *options = new QWidget;
	ui_.setupUi(options);
	ui_.wiki->setText(tr("<a href=\"https://psi-plus.com/wiki/plugins#chess_plugin\">Wiki (Online)</a>"));
	ui_.wiki->setOpenExternalLinks(true);

	ui_.play_error->setIcon(icoHost->getIcon("psi/play"));
	ui_.play_finish->setIcon(icoHost->getIcon("psi/play"));
	ui_.play_move->setIcon(icoHost->getIcon("psi/play"));;
	ui_.play_start->setIcon(icoHost->getIcon("psi/play"));

	ui_.select_error->setIcon(icoHost->getIcon("psi/browse"));
	ui_.select_finish->setIcon(icoHost->getIcon("psi/browse"));
	ui_.select_move->setIcon(icoHost->getIcon("psi/browse"));
	ui_.select_start->setIcon(icoHost->getIcon("psi/browse"));

	restoreOptions();

	connect(ui_.play_error, SIGNAL(pressed()), this, SLOT(testSound()));
	connect(ui_.play_finish, SIGNAL(pressed()), this, SLOT(testSound()));
	connect(ui_.play_move, SIGNAL(pressed()), this, SLOT(testSound()));
	connect(ui_.play_start, SIGNAL(pressed()), this, SLOT(testSound()));

	connect(ui_.select_error, SIGNAL(pressed()), this, SLOT(getSound()));
	connect(ui_.select_finish, SIGNAL(pressed()), this, SLOT(getSound()));
	connect(ui_.select_start, SIGNAL(pressed()), this, SLOT(getSound()));
	connect(ui_.select_move, SIGNAL(pressed()), this, SLOT(getSound()));
	return options;
}

void ChessPlugin::applyOptions(){
	soundError = ui_.le_error->text();
	psiOptions->setPluginOption(soundErrorConst, QVariant(soundError));
	soundFinish = ui_.le_finish->text();
	psiOptions->setPluginOption(soundFinishConst, QVariant(soundFinish));
	soundMove = ui_.le_move->text();
	psiOptions->setPluginOption(soundMoveConst, QVariant(soundMove));
	soundStart = ui_.le_start->text();
	psiOptions->setPluginOption(soundStartConst, QVariant(soundStart));
	DndDisable = ui_.cb_disable_dnd->isChecked();
	psiOptions->setPluginOption(constDndDisable, QVariant(DndDisable));
	DefSoundSettings = ui_.cb_sound_override->isChecked();
	psiOptions->setPluginOption(constDefSoundSettings, QVariant(DefSoundSettings));
}

void ChessPlugin::restoreOptions(){
	ui_.le_error->setText(soundError);
	ui_.le_finish->setText(soundFinish);
	ui_.le_move->setText(soundMove);
	ui_.le_start->setText(soundStart);
	ui_.cb_disable_dnd->setChecked(DndDisable);
	ui_.cb_sound_override->setChecked(DefSoundSettings);
}

void ChessPlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost* host) {
	accInfoHost = host;
}

void ChessPlugin::setOptionAccessingHost(OptionAccessingHost* host) {
	psiOptions = host;
}

void ChessPlugin::setActiveTabAccessingHost(ActiveTabAccessingHost* host) {
	activeTab = host;
}

void ChessPlugin::setIconFactoryAccessingHost(IconFactoryAccessingHost *host) {
	icoHost = host;
}

void ChessPlugin::setStanzaSendingHost(StanzaSendingHost *host) {
	stanzaSender = host;
}

void ChessPlugin::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host) {
	appInfo = host;
}

void ChessPlugin::setContactInfoAccessingHost(ContactInfoAccessingHost *host) {
	contactInfo = host;
}

void ChessPlugin::setEventCreatingHost(EventCreatingHost *host) {
	psiEvent = host;
}

void ChessPlugin::setPopupAccessingHost(PopupAccessingHost *host) {
	popup = host;
}

void ChessPlugin::setSoundAccessingHost(SoundAccessingHost *host) {
	sound_ = host;
}

void ChessPlugin::doPopup(const QString &text) {
	popup->initPopup(text, tr("Chess Plugin"), "chessplugin/chess");
}

QList < QVariantHash > ChessPlugin::getButtonParam() {
	QList< QVariantHash > l;
	QVariantHash hash;
	hash["tooltip"] = QVariant(tr("Chess!"));
	hash["icon"] = QVariant(QString("chessplugin/chess"));
	hash["reciver"] = qVariantFromValue(qobject_cast<QObject *>(this));
	hash["slot"] = QVariant(SLOT(toolButtonPressed()));
	l.push_back(hash);
	return l;
}

QList < QVariantHash > ChessPlugin::getAccountMenuParam() {
	return QList < QVariantHash >();
}

QList < QVariantHash > ChessPlugin::getContactMenuParam() {
	QList< QVariantHash > l;
	QVariantHash hash;
	hash["name"] = QVariant(tr("Chess!"));
	hash["icon"] = QVariant(QString("chessplugin/chess"));
	hash["reciver"] = qVariantFromValue(qobject_cast<QObject *>(this));
	hash["slot"] = QVariant(SLOT(menuActivated()));
	l.push_back(hash);
	return l;
}

void ChessPlugin::toolButtonPressed() {
	if(!enabled) return;
	if(game_) {
		if((DefSoundSettings || psiOptions->getGlobalOption("options.ui.notifications.sounds.enable").toBool()) && enableSound)
			playSound(soundError);

		doPopup(tr("You are already playing!"));
		return;
	}
	QString yourJid_ = activeTab->getYourJid();;
	QString tmpJid("");
	int account_ = 0;
	while (yourJid_ != (tmpJid = accInfoHost->getJid(account_))){
		++account_;
		if (tmpJid == "-1") return;
	}

	if(accInfoHost->getStatus(account_) == "offline") return;

	Request r;
	r.yourJid = yourJid_;
	r.jid = activeTab->getJid();
	r.account = account_;

	invite(r);
}

void ChessPlugin::menuActivated() {
	if(!enabled) {
		return;
	}

	if(game_) {
		if((DefSoundSettings || psiOptions->getGlobalOption("options.ui.notifications.sounds.enable").toBool()) && enableSound)
			playSound(soundError);
		doPopup(tr("You are already playing!"));
		return;
	}

	int account_ = sender()->property("account").toInt();
	if(accInfoHost->getStatus(account_) == "offline") {
		return;
	}

	Request r;
	r.jid = sender()->property("jid").toString();
	r.yourJid = accInfoHost->getJid(account_);
	r.account = account_;

	invite(r);
}

// В этом методе создается диалог, с помощью которого Вы выбираете цвет фигур,
// и на какой ресурс отправить приглашение
void ChessPlugin::invite(Request &r) {
	QStringList resList;
	QStringList tmp = r.jid.split("/");
	if(contactInfo->isPrivate(r.account, r.jid) && r.jid.contains("/")) {
		r.jid = tmp.takeFirst();
		resList.append(tmp.join("/"));
	}
	else {
		r.jid = tmp.first();
		resList = contactInfo->resources(r.account, r.jid);
	}
	InviteDialog *id = new InviteDialog(r, resList);
	connect(id, SIGNAL(play(const Request&, const QString&, const QString&)), this, SLOT(sendInvite(const Request&, const QString&, const QString&)));
	id->show();
}

//В этом методе отправляется приглашение
void ChessPlugin::sendInvite(const Request& req, const QString& resource, const QString& color) {
	Request r = req;
	r.chessId = "ch_111";
	r.jid += "/" + stanzaSender->escape(resource);
	r.requestId = newId();
	stanzaSender->sendStanza(r.account,
							 QString("<iq type=\"set\" to=\"%1\" id=\"%2\"><create xmlns=\"games:board\" id=\"%4\" type=\"chess\" color=\"%3\"></create></iq>")
							 .arg(r.jid)
							 .arg(r.requestId)
							 .arg(color)
							 .arg(r.chessId));
	if(color == "white")
		r.type = Figure::WhitePlayer;
	else
		r.type = Figure::BlackPlayer;

	waitFor = true;
	invites.push_back(r);
}

//Этот метод вызывается, когда вы принимаете приглашение
void ChessPlugin::accept() {
	stanzaSender->sendStanza(currentGame_.account,
							 QString("<iq type=\"result\" to=\"%1\" id=\"%2\"><create xmlns=\"games:board\" type=\"chess\" id=\"%3\"/></iq>")
							 .arg(currentGame_.jid)
							 .arg(currentGame_.requestId)
							 .arg(currentGame_.chessId));
	acceptGame();
}

//Этот метод вызывается, когда вы отказываетесь от игры
void ChessPlugin::reject() {
	stanzaSender->sendStanza(currentGame_.account,
							 QString("<iq type=\"error\" to=\"%1\" id=\"%2\"></iq>")
							 .arg(currentGame_.jid)
							 .arg(currentGame_.requestId));
	rejectGame();
}

// Этот метод вызывается, когда вы закрыли доску.
void ChessPlugin::closeBoardEvent() {
	stanzaSender->sendStanza(currentGame_.account,
							 QString("<iq type=\"set\" to=\"%1\" id=\"%2\"><close xmlns=\"games:board\" id=\"%3\" type=\"chess\"></close></iq>")
							 .arg(currentGame_.jid)
							 .arg(newId())
							 .arg(currentGame_.chessId));

	if((DefSoundSettings || psiOptions->getGlobalOption("options.ui.notifications.sounds.enable").toBool()) && enableSound)
		playSound(soundFinish);
	stopGame();
}

// Этот метод вызывается, когда ваш опонент закрыл доску.
void ChessPlugin::boardClosed() {
	if(theEnd_) return;
	QMessageBox::warning(board, tr("Chess Plugin"),
						 tr("Your opponent has closed the board!\n You can still save the game."), QMessageBox::Ok);
}

// Окончание игры
void ChessPlugin::stopGame() {
	delete (board);
	board = 0;
	game_ = false;
	theEnd_ = false;
}

// Начало игры
void ChessPlugin::acceptGame() {
	if(game_) return;
	game_ = true;
	waitFor = false;
	theEnd_ = false;
	board = new ChessWindow(currentGame_.type, enableSound);
	connect(board, SIGNAL(closeBoard()), this, SLOT(closeBoardEvent()), Qt::QueuedConnection);
	connect(board, SIGNAL(move(int,int,int,int, QString)), this, SLOT(move(int,int,int,int, QString)));
	connect(board, SIGNAL(moveAccepted()), this, SLOT(moveAccepted()));
	connect(board, SIGNAL(error()), this, SLOT(error()));
	connect(board, SIGNAL(load(QString)), this, SLOT(load(QString)));
	connect(board, SIGNAL(draw()), this, SLOT(draw()));
	connect(board, SIGNAL(lose()), this, SLOT(youLose()));
	connect(board, SIGNAL(toggleEnableSound(bool)), this, SLOT(toggleEnableSound(bool)));
	board->show();
	if((DefSoundSettings || psiOptions->getGlobalOption("options.ui.notifications.sounds.enable").toBool()) && enableSound)
		playSound(soundStart);
}

// Отказ от игры
void ChessPlugin::rejectGame() {
	game_ = false;
	waitFor = false;
	theEnd_ = false;
	if((DefSoundSettings || psiOptions->getGlobalOption("options.ui.notifications.sounds.enable").toBool()) && enableSound)
		playSound(soundFinish);
	doPopup(tr("The game was rejected"));
}

// Вы отправляете вашему сопернику сохраненную партию
void ChessPlugin::load(const QString& settings) {
	stanzaSender->sendStanza(currentGame_.account,
							 QString("<iq type=\"set\" to=\"%1\" id=\"%2\"><load xmlns=\"games:board\" id=\"%3\" type=\"chess\">%4</load></iq>")
							 .arg(currentGame_.jid)
							 .arg(newId())
							 .arg(currentGame_.chessId)
							 .arg(settings));
}

// Вы походили
void ChessPlugin::move(int oldX, int oldY, int newX, int newY, const QString& figure) {
	QString stanza = QString("<iq type=\"set\" to=\"%1\" id=\"%2\"><turn xmlns=\"games:board\" type=\"chess\" id=\"%7\"><move pos=\"%3,%4;%5,%6\">")
							 .arg(currentGame_.jid)
							 .arg(newId())
							 .arg(QString::number(oldX))
							 .arg(QString::number(oldY))
							 .arg(QString::number(newX))
							 .arg(QString::number(newY))
							 .arg(currentGame_.chessId);
	if(!figure.isEmpty())
		stanza += QString("<promotion>%1</promotion>").arg(figure);
	stanza += "</move></turn></iq>";
	stanzaSender->sendStanza(currentGame_.account, stanza);
	if((DefSoundSettings || psiOptions->getGlobalOption("options.ui.notifications.sounds.enable").toBool()) && enableSound)
		playSound(soundMove);
}

// Вы согласились с ходом
void ChessPlugin::moveAccepted() {
	stanzaSender->sendStanza(currentGame_.account,
							 QString("<iq type=\"result\" to=\"%1\" id=\"%2\"><turn type=\"chess\" id=\"%3\" xmlns=\"games:board\"/></iq>")
							 .arg(currentGame_.jid)
							 .arg(tmpId)
							 .arg(currentGame_.chessId));
}

void ChessPlugin::youLose() {
	if(theEnd_) return;
	stanzaSender->sendStanza(currentGame_.account,
							 QString("<iq type=\"set\" to=\"%1\" id=\"%2\"><turn xmlns=\"games:board\" type=\"chess\" id=\"%3\"><resign/></turn></iq>")
							 .arg(currentGame_.jid)
							 .arg(newId())
							 .arg(currentGame_.chessId));
	board->youLose();
	theEnd_ = true;
	QMessageBox::information(board, tr("Chess Plugin"),
							 tr("You Lose."), QMessageBox::Ok);
}

void ChessPlugin::youWin() {
	if(theEnd_) return;
	if((DefSoundSettings || psiOptions->getGlobalOption("options.ui.notifications.sounds.enable").toBool()) && enableSound)
		playSound(soundStart);
	board->youWin();
	theEnd_ = true;
	QMessageBox::information(board, tr("Chess Plugin"),
							 tr("You Win!"), QMessageBox::Ok);
}

void ChessPlugin::draw() {
	if(!theEnd_) {
		stanzaSender->sendStanza(currentGame_.account,
								 QString("<iq type=\"set\" to=\"%1\" id=\"%2\"><turn xmlns=\"games:board\" type=\"chess\" id=\"%3\"><draw/></turn></iq>")
								 .arg(currentGame_.jid)
								 .arg(newId())
								 .arg(currentGame_.chessId));
		if((DefSoundSettings || psiOptions->getGlobalOption("options.ui.notifications.sounds.enable").toBool()) && enableSound)
			playSound(soundStart);
		board->youDraw();
		theEnd_ = true;
		QMessageBox::information(board, tr("Chess Plugin"),
								 tr("Draw!"), QMessageBox::Ok);
	}
}

void ChessPlugin::error() {
	if((DefSoundSettings || psiOptions->getGlobalOption("options.ui.notifications.sounds.enable").toBool()) && enableSound)
		playSound(soundError);
	QMessageBox::warning(board, tr("Chess Plugin"),
						 tr("Unknown error!"), QMessageBox::Ok);
	board->close();
}

bool ChessPlugin::incomingStanza(int account, const QDomElement& xml)
{
	if(!enabled) {
		return false;
	}

	if(xml.tagName() == "iq") {
		const QString xmlType = xml.attribute("type");
		if(xmlType == "set") {
			QDomElement createElem = xml.firstChildElement("create");
			if(!createElem.isNull() && createElem.attribute("xmlns") == "games:board"
			   && createElem.attribute("type") == "chess") {
				const QString xmlId = stanzaSender->escape(xml.attribute("id"));
				const QString xmlFrom = stanzaSender->escape(xml.attribute("from"));
				if((DndDisable && accInfoHost->getStatus(account) == "dnd") || game_) {
					stanzaSender->sendStanza(account,
											 QString("<iq type=\"error\" to=\"%1\" id=\"%2\"></iq>")
											 .arg(xmlFrom)
											 .arg(xmlId));
					return true;
				}
				const QString color = stanzaSender->escape(createElem.attribute("color"));

				Request r;
				r.requestId = xmlId;
				r.chessId = stanzaSender->escape(createElem.attribute("id"));
				r.account = account;
				r.jid = xmlFrom;
				r.yourJid = accInfoHost->getJid(account);
				r.type = Figure::WhitePlayer;
				if(color == "white")
					r.type = Figure::BlackPlayer;
				requests.append(r);

				psiEvent->createNewEvent(account, r.jid, tr("Chess Plugin: Invitation from %1").arg(r.jid), this, SLOT(doInviteDialog(QString)));
				return true;
			}
			QDomElement turn = xml.firstChildElement("turn");
			if(!turn.isNull() && turn.attribute("xmlns") == "games:board" && turn.attribute("type") == "chess" && game_) {
				const QString xmlId = stanzaSender->escape(xml.attribute("id"));
				const QString xmlFrom = stanzaSender->escape(xml.attribute("from"));
				if(xmlFrom.toLower() != currentGame_.jid.toLower()) {
					return true;  // Игнорируем станзы от "левых" джидов
				}
				tmpId = xmlId;
				QDomNode node = turn.firstChild();
				while(!node.isNull()) {
					QDomElement childElem = node.toElement();
					if(childElem.tagName() == "move") {
						QStringList tmpMove = childElem.attribute("pos").split(";");
						int oldX = tmpMove.first().split(",").first().toInt();
						int oldY = tmpMove.first().split(",").last().toInt();
						int newX = tmpMove.last().split(",").first().toInt();
						int newY = tmpMove.last().split(",").last().toInt();
						QDomElement promElem = childElem.firstChildElement("promotion");
						QString figure = "";
						if(!promElem.isNull())
							figure = promElem.text();
						board->moveRequest(oldX,oldY,newX,newY,figure);
						if((DefSoundSettings || psiOptions->getGlobalOption("options.ui.notifications.sounds.enable").toBool()) && enableSound)
							playSound(soundMove);
					}
					else if(childElem.tagName() == "draw") {
						draw();
						return true;
					}
					else if(childElem.tagName() == "resign") {
						youWin();
						return true;
					}
					node = node.nextSibling();
				}
				return true;
			}
			QDomElement closeElem = xml.firstChildElement("close");
			if(!closeElem.isNull() && closeElem.attribute("xmlns") == "games:board" && closeElem.attribute("type") == "chess" && game_) {
				const QString xmlFrom = stanzaSender->escape(xml.attribute("from"));
				if(xmlFrom.toLower() != currentGame_.jid.toLower()) {
					return true;  // Игнорируем станзы от "левых" джидов
				}
				boardClosed();
				return true;
			}
			QDomElement loadElem = xml.firstChildElement("load");
			if(!loadElem.isNull() && loadElem.attribute("xmlns") == "games:board" && loadElem.attribute("type") == "chess" && game_) {
				const QString xmlFrom = stanzaSender->escape(xml.attribute("from"));
				if(xmlFrom.toLower() != currentGame_.jid.toLower()) {
					return true;  // Игнорируем станзы от "левых" джидов
				}
				board->loadRequest(loadElem.text());
				return true;
			}
		}
		else if(xmlType == "result") {
			const QString xmlId = stanzaSender->escape(xml.attribute("id"));
			if(waitFor) {
				int ind = checkId(xmlId);
				if(ind != -1) {
					currentGame_ = invites.at(ind);
					invites.clear();
					acceptGame();
					return true;
				}
			}
		}
		else if(xmlType == "error") {
			const QString xmlId = stanzaSender->escape(xml.attribute("id"));
			if(waitFor) {
				int ind = checkId(xmlId);
				if(ind != -1) {
					invites.removeAt(ind);
					rejectGame();
					return true;
				}
			}
		}
	}
	return false;
}

bool ChessPlugin::outgoingStanza(int /*account*/, QDomElement& /*xml*/) {
	return false;
}

void ChessPlugin::doInviteDialog(const QString& jid) {
	if(!enabled || requests.isEmpty())
		return;

	int index = findRequest(jid);
	if(index == -1)
		return;

	Request rec = requests.takeAt(index);

	if(game_) {
		QMessageBox::information(0, tr("Chess Plugin"), tr("You are already playing!"));
		stanzaSender->sendStanza(rec.account,
								 QString("<iq type=\"error\" to=\"%1\" id=\"%2\"></iq>")
								 .arg(rec.jid)
								 .arg(rec.requestId));
		return;
	}

	currentGame_ = rec;

	QString color = "black";
	if(currentGame_.type == Figure::BlackPlayer)
		color = "white";

	InvitationDialog *id = new InvitationDialog(currentGame_.jid, color);
	connect(id, SIGNAL(accept()), this, SLOT(accept()));
	connect(id, SIGNAL(reject()), this, SLOT(reject()));
	id->show();
}

int ChessPlugin::findRequest(const QString& jid) {
	int index = -1;
	for(int i = requests.size(); i != 0;) {
		if(requests.at(--i).jid == jid) {
			index = i;
			break;
		}
	}
	return index;
}

void ChessPlugin::testSound() {
	if(ui_.play_error->isDown()) {
		playSound(ui_.le_error->text());
	} else if(ui_.play_finish->isDown()) {
		playSound(ui_.le_finish->text());
	} else if(ui_.play_move->isDown()) {
		playSound(ui_.le_move->text());
	} else if(ui_.play_start->isDown()) {
		playSound(ui_.le_start->text());
	}
}

void ChessPlugin::getSound() {
	QLineEdit *le = 0;
	if(ui_.select_error->isDown())
		le = ui_.le_error;
	if(ui_.select_finish->isDown())
		le = ui_.le_finish;
	if(ui_.select_move->isDown())
		le = ui_.le_move;
	if(ui_.select_start->isDown())
		le = ui_.le_start;

	if(!le) return;

	QString fileName = QFileDialog::getOpenFileName(0,tr("Choose a sound file"),"", tr("Sound (*.wav)"));
	if(fileName.isEmpty())
		return;
	le->setText(fileName);
}

void ChessPlugin::playSound(const QString& f) {
	sound_->playSound(f);
}

const QString ChessPlugin::newId() {
	++id;
	const QString newid = "cp_"+QString::number(id);
	return newid;
}

int ChessPlugin::checkId(const QString& checkId) {
	int index = -1;
	for(int i = invites.size(); i != 0;) {
		if(invites.at(--i).requestId == checkId) {
			index = i;
			break;
		}
	}

	return index;
}

void ChessPlugin::toggleEnableSound(bool enable) {
	enableSound = enable;
}

QString ChessPlugin::pluginInfo() {
	return tr("Author: ") +	 "Dealer_WeARE\n"
		 + tr("Email: ") + "wadealer@gmail.com\n\n"
		 + trUtf8("This plugin allows you to play chess with your friends.\n"
				  "The plugin is compatible with a similar plugin for Tkabber.\n"
				  "For sending commands, normal messages are used, so this plugin will always work wherever you are able to log in."
				  "To invite a friend for a game, you can use contact menu item or the button on the toolbar in a chat window.");
}

QPixmap ChessPlugin::icon() const
{
	return QPixmap(":/chessplugin/chess.png");
}

#include "chessplugin.moc"
