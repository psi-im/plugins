/*
 * pluginwindow.cpp - Gomoku Game plugin
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

#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

#include "pluginwindow.h"
#include "ui_pluginwindow.h"
#include "common.h"

//-------------------------- HintElementWidget -------------------------

HintElementWidget::HintElementWidget(QWidget *parent) :
	QFrame(parent),
	hintElement(NULL)
{
}

HintElementWidget::~HintElementWidget()
{
	if (hintElement)
		delete hintElement;
}

void HintElementWidget::setElementType(GameElement::ElementType type)
{
	if (hintElement)
		delete hintElement;
	hintElement = new GameElement(type, 0, 0);
	QFrame::update();
}

void HintElementWidget::paintEvent(QPaintEvent *event)
{
	QFrame::paintEvent(event);
	if (!hintElement)
		return;
	QRect rect = this->rect();
	QPainter painter(this);
	hintElement->paint(&painter, rect);
}

//------------------------ PluginWindow --------------------------

PluginWindow::PluginWindow(QString full_jid, QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::PluginWindow),
	bmodel(NULL),
	delegate(NULL),
	element_(""),
	gameActive(false)
{
	ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	ui->lbOpponent->setText(full_jid);
}

PluginWindow::~PluginWindow()
{
	delete ui;
}

void PluginWindow::init(QString element)
{
	element_ = element;
	GameElement::ElementType elemType;
	if (element_ == "white") {
		elemType = GameElement::TypeWhite;
	} else {
		elemType = GameElement::TypeBlack;
	}
	// Инициируем модель доски
	if (bmodel == NULL) {
		bmodel = new BoardModel(this);
		connect(bmodel, SIGNAL(changeGameStatus(BoardModel::GameStatus)), this, SLOT(changeGameStatus(BoardModel::GameStatus)));
		connect(bmodel, SIGNAL(setupElement(int, int)), this, SLOT(setupElement(int, int)));
		connect(bmodel, SIGNAL(lose()), this, SLOT(setLose()));
		connect(bmodel, SIGNAL(draw()), this, SIGNAL(draw()));
		connect(bmodel, SIGNAL(switchColor()), this, SIGNAL(switchColor()));
		connect(bmodel, SIGNAL(doPopup(const QString)), this, SIGNAL(doPopup(const QString)));
	}
	bmodel->init(elemType);
	ui->board->setModel(bmodel);
	// Создаем делегат
	if (delegate == NULL) {
		delegate = new BoardDelegate(bmodel, ui->board);
	}
	// Инициируем BoardView
	ui->board->setItemDelegate(delegate);
	ui->board->reset();
	// Объекты GUI
	ui->hintElement->setElementType(elemType);
	ui->actionNewGame->setEnabled(false);
	ui->actionResign->setEnabled(true);
	ui->actionSwitchColor->setEnabled(false);
	ui->lsTurnsList->clear();
	//--
	emit playSound(constSoundStart);
	gameActive = true;
}

void PluginWindow::changeGameStatus(BoardModel::GameStatus status)
{
	int step = bmodel->turnNum();
	if (step == 4) {
		if (status == BoardModel::StatusThinking && element_ == "white") {
			ui->actionSwitchColor->setEnabled(true);
		}
	} else if (step == 5) {
		ui->actionSwitchColor->setEnabled(false);
	}
	QString stat_str = "n/a";
	if (status == BoardModel::StatusWaitingOpponent) {
		stat_str = tr("Waiting for opponent");
		ui->actionResign->setEnabled(true);
		emit changeGameSession("wait-opponent-command");
	} else if (status == BoardModel::StatusWaitingAccept) {
		stat_str = tr("Waiting for accept");
		emit changeGameSession("wait-opponent-accept");
	} else if (status == BoardModel::StatusThinking) {
		stat_str = tr("Your turn");
		emit changeGameSession("wait-game-window");
		ui->actionResign->setEnabled(true);
		emit playSound(constSoundMove);
	} else if (status == BoardModel::StatusEndGame) {
		stat_str = tr("End of game");
		endGame();
	} else if (status == BoardModel::StatusError) {
		stat_str = tr("Error");
		endGame();
	} else if (status == BoardModel::StatusWin) {
		stat_str = tr("Win!");
		endGame();
	} else if (status == BoardModel::StatusLose) {
		stat_str = tr("Lose.");
		endGame();
	} else if (status == BoardModel::StatusDraw) {
		stat_str = tr("Draw.");
		endGame();
		QMessageBox *msgBox = new QMessageBox(this);
		msgBox->setIcon(QMessageBox::Information);
		msgBox->setWindowTitle(tr("Gomoku Plugin"));
		msgBox->setText(tr("Draw."));
		msgBox->setStandardButtons(QMessageBox::Ok);
		msgBox->setWindowModality(Qt::WindowModal);
		msgBox->exec();
		delete msgBox;
	}
	ui->lbStatus->setText(stat_str);
}

void PluginWindow::endGame()
{
	gameActive = false;
	ui->actionResign->setEnabled(false);
	ui->actionNewGame->setEnabled(true);
	emit changeGameSession("none");
	emit playSound(constSoundFinish);
}

/**
 * В списке ходов сменился активный элемент
 */
void PluginWindow::turnSelected()
{
	QListWidgetItem *item = ui->lsTurnsList->currentItem();
	if (item) {
		bmodel->setSelect(item->data(Qt::UserRole).toInt(), item->data(Qt::UserRole + 1).toInt());
	}
}

/**
 * Пришел сигнал с модели доски об установки игроком нового элемента
 */
void PluginWindow::setupElement(int x, int y)
{
	appendStep(x, y, true);
	emit setElement(x, y);
}

/**
 * Добавление хода в список ходов
 */
void PluginWindow::appendStep(int x, int y, bool my_turn)
{
	QString str1;
	if (my_turn) {
		str1 = tr("You: ");
	} else {
		str1 = tr("Opp: ");
	}
	int turnNum = bmodel->turnNum() - 1;
	if (x == -1 && y == -1) {
		str1 += tr("%1- swch", "Switch color").arg(turnNum);
	} else {
		str1 += QString("%1- %2%3").arg(turnNum).arg(horHeaderString.at(x))
			.arg(QString::number(y + 1));
	}
	QListWidgetItem *item = new QListWidgetItem(str1, ui->lsTurnsList);
	item->setData(Qt::UserRole, x);
	item->setData(Qt::UserRole + 1, y);
	ui->lsTurnsList->addItem(item);
	ui->lsTurnsList->setCurrentItem(item);
}

/**
 * Подтверждение последнего хода в списке
 */
void PluginWindow::acceptStep()
{
	//
}

/**
 * Пришло подтверждение хода от противника
 */
void PluginWindow::setAccept()
{
	bmodel->setAccept();
	acceptStep();
}

/**
 * Пришел ход от противника
 */
void PluginWindow::setTurn(int x, int y)
{
	if (bmodel) {
		if (bmodel->opponentTurn(x, y)) {
			appendStep(x, y, false);
			emit accepted();
			if (bmodel->turnNum() == 4) { // Ходы сквозные, значит 4й всегда белые
				ui->actionSwitchColor->setEnabled(true);
				doSwitchColor();
			}
			return;
		}
	}
	emit error();
}

/**
 * Оппонент поменял цвет
 */
void PluginWindow::setSwitchColor()
{
	if (bmodel->doSwitchColor(false)) {
		ui->hintElement->setElementType(GameElement::TypeWhite);
		appendStep(-1, -1, false);
		emit accepted();
	} else {
		emit error();
	}
}

/**
 * Пришла ошибка от противника
 */
void PluginWindow::setError()
{
	bmodel->setError();
	QMessageBox *msgBox = new QMessageBox(this);
	msgBox->setIcon(QMessageBox::Warning);
	msgBox->setWindowTitle(tr("Gomoku Plugin"));
	msgBox->setText(tr("Game Error!"));
	msgBox->setStandardButtons(QMessageBox::Ok);
	msgBox->setWindowModality(Qt::WindowModal);
	msgBox->exec();
	delete msgBox;
}

/**
 * Оппонент закрыл игровую доску.
 */
void PluginWindow::setClose()
{
	bmodel->setClose();
	QMessageBox *msgBox = new QMessageBox(this);
	msgBox->setIcon(QMessageBox::Warning);
	msgBox->setWindowTitle(tr("Gomoku Plugin"));
	msgBox->setText(tr("Your opponent has closed the board!\n You can still save the game."));
	msgBox->setStandardButtons(QMessageBox::Ok);
	msgBox->setWindowModality(Qt::WindowModal);
	msgBox->exec();
	delete msgBox;
}

/**
 * Реакция на закрытие нашей доски
 */
void PluginWindow::closeEvent (QCloseEvent *event)
{
	emit closeBoard(gameActive, y(), x(), width(), height()); // Отправляем сообщение оппоненту только если игра не завершена
	gameActive = false;
	event->accept();
}

/**
 * Предлагаем сменить цвет
 */
void PluginWindow::doSwitchColor()
{
	QMessageBox *msgBox = new QMessageBox(this);
	msgBox->setIcon(QMessageBox::Question);
	msgBox->setWindowTitle(tr("Gomoku Plugin"));
	msgBox->setText(tr("You want to switch color?"));
	msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox->setDefaultButton(QMessageBox::No);
	msgBox->setWindowModality(Qt::WindowModal);
	int res = msgBox->exec();
	delete msgBox;
	if (res == QMessageBox::Yes) {
		element_ = "black";
		if (bmodel->doSwitchColor(true)) {
			ui->hintElement->setElementType(GameElement::TypeBlack);
			appendStep(-1, -1, true);
		}
	}
}

/**
 * Мы проиграли выставляем сигнал и показываем сообщение
 */
void PluginWindow::setLose()
{
	emit lose();
	QMessageBox *msgBox = new QMessageBox(this);
	msgBox->setIcon(QMessageBox::Information);
	msgBox->setWindowTitle(tr("Gomoku Plugin"));
	msgBox->setText(tr("You Lose."));
	msgBox->setStandardButtons(QMessageBox::Ok);
	msgBox->setWindowModality(Qt::WindowModal);
	msgBox->exec();
	delete msgBox;
}

/**
 * Мы сдались (выбран пункт меню "Resign" на доске)
 */
void PluginWindow::setResign()
{
	bmodel->setResign();
}

/**
 * Сообщение о том что оппонент проиграл, т.е. мы выиграли
 */
void PluginWindow::setWin()
{
	bmodel->setWin();
	QMessageBox *msgBox = new QMessageBox(this);
	msgBox->setIcon(QMessageBox::Information);
	msgBox->setWindowTitle(tr("Gomoku Plugin"));
	msgBox->setText(tr("You Win!"));
	msgBox->setStandardButtons(QMessageBox::Ok);
	msgBox->setWindowModality(Qt::WindowModal);
	msgBox->exec();
	delete msgBox;
}

/**
 * Обработчик сохранения игры
 */
void PluginWindow::saveGame()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save game"), "", "*.gmk");
	if (fileName.isEmpty())
		return;
	if (fileName.right(4) != ".gmk")
		fileName.append(".gmk");
	QFile file(fileName);
	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		QTextStream out(&file);
		out.setGenerateByteOrderMark(false);
		out << bmodel->saveToString();
	}
}

/**
 * Обработчик загрузки игры с локального файла
 */
void PluginWindow::loadGame()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load game"), "", "*.gmk");
	if (fileName.isEmpty())
		return;
	QFile file(fileName);
	if(file.open(QIODevice::ReadOnly)) {
		QTextStream in(&file);
		QString saved_str = in.readAll();
		if (bmodel->loadFromString(saved_str, true)) {
			ui->hintElement->setElementType(bmodel->elementType());
			ui->lsTurnsList->clear();
			emit load(saved_str.replace("\n", ""));
		}
	}
}

/**
 * Обработчик начала новой игры. Запрос у пользователя и отсылка сигнала
 */
void PluginWindow::newGame()
{
	QMessageBox *msgBox = new QMessageBox(this);
	msgBox->setIcon(QMessageBox::Question);
	msgBox->setWindowTitle(tr("Gomoku Plugin"));
	msgBox->setText(tr("You really want to begin new game?"));
	msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox->setWindowModality(Qt::WindowModal);
	int res = msgBox->exec();
	delete msgBox;
	if (res == QMessageBox::Yes) {
		emit sendNewInvite();
	}
}

/**
 * Обработчик загрузки игры, посланной оппонентом
 */
void PluginWindow::loadRemoteGame(QString load_str)
{
	if (!load_str.isEmpty()) {
		if (bmodel->loadFromString(load_str, false)) {
			ui->hintElement->setElementType(bmodel->elementType());
			ui->lsTurnsList->clear();
			emit accepted();
			return;
		}
	}
	emit error();
}

/**
 * Выбрано новое оформление (Скин)
 */
void PluginWindow::setSkin()
{
	QObject *sender_ = sender();
	if (sender_ == ui->actionSkin0) {
		ui->actionSkin0->setChecked(true);
		ui->actionSkin1->setChecked(false);
		delegate->setSkin(0);
	} else if (sender_ == ui->actionSkin1) {
		ui->actionSkin1->setChecked(true);
		ui->actionSkin0->setChecked(false);
		delegate->setSkin(1);
	}
	ui->board->repaint();
}

/**
 * Обработчик предложения оппонентом ничьей
 */
void PluginWindow::opponentDraw()
{
	bmodel->opponentDraw();
}
