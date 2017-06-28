/*
 * mainwindow.cpp - plugin
 * Copyright (C) 2010  Evgeny Khryukin
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

#include "mainwindow.h"

#include <QFileDialog>
#include <QTextCodec>
#include <QTextStream>

SelectFigure::SelectFigure(const QString& player, QWidget *parent)
	: QWidget(parent)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowModality(Qt::WindowModal);
	setFixedSize(62,62);
	setStyleSheet("QPushButton { background-color: #e9edff;}"
		      "QPushButton:hover { background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #e9edff, stop: 1 black)}");
	QGridLayout *layout = new QGridLayout(this);
	tb_queen = new QPushButton(this);
	tb_castle = new QPushButton(this);
	tb_knight = new QPushButton(this);
	tb_bishop = new QPushButton(this);
	tb_queen->setFixedSize(25,25);
	tb_queen->setObjectName("queen");
	tb_castle->setFixedSize(25,25);
	tb_castle->setObjectName("rook");
	tb_knight->setFixedSize(25,25);
	tb_knight->setObjectName("knight");
	tb_bishop->setFixedSize(25,25);
	tb_bishop->setObjectName("bishop");
	if(player == "white") {
                tb_queen->setIcon(QIcon(QPixmap(":/chessplugin/figures/white_queen.png").scaled(22,22, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
                tb_castle->setIcon(QIcon(QPixmap(":/chessplugin/figures/white_castle.png").scaled(22,22, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
                tb_knight->setIcon(QIcon(QPixmap(":/chessplugin/figures/white_knight.png").scaled(22,22, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
                tb_bishop->setIcon(QIcon(QPixmap(":/chessplugin/figures/white_bishop.png").scaled(22,22, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
	}
	else {
                tb_queen->setIcon(QIcon(QPixmap(":/chessplugin/figures/black_queen.png").scaled(22,22, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
                tb_castle->setIcon(QIcon(QPixmap(":/chessplugin/figures/black_castle.png").scaled(22,22, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
                tb_knight->setIcon(QIcon(QPixmap(":/chessplugin/figures/black_knight.png").scaled(22,22, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
                tb_bishop->setIcon(QIcon(QPixmap(":/chessplugin/figures/black_bishop.png").scaled(22,22, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
	}

	layout->addWidget(tb_queen,0,0);
	layout->addWidget(tb_castle,1,0);
	layout->addWidget(tb_knight,0,1);
	layout->addWidget(tb_bishop,1,1);

	connect(tb_queen, SIGNAL(clicked()), this, SLOT(figureSelected()));
	connect(tb_castle, SIGNAL(clicked()), this, SLOT(figureSelected()));
	connect(tb_knight, SIGNAL(clicked()), this, SLOT(figureSelected()));
	connect(tb_bishop, SIGNAL(clicked()), this, SLOT(figureSelected()));
}

void SelectFigure::figureSelected()
{
	QString objectName = sender()->objectName();
	newFigure(objectName);
	close();
}


//-------------------------
//----ChessWindow----------
//-------------------------
ChessWindow::ChessWindow(Figure::GameType type, bool enableSound_ , QWidget *parent)
	: QMainWindow(parent)
	, enabledSound(enableSound_)
	, movesCount(0)
{
	ui_.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	setFixedSize(610,555);
	setWindowIcon(QIcon(QPixmap(":/chessplugin/figures/Chess.png")));
	setStyleSheet("QMainWindow *{background-color: #ffffe7; color: black; }"
		      "QMenu  {background-color: #ffa231;}"
		      "QMenu::item { background-color: #ffa231; padding: 1px; padding-right: 5px; padding-left: 18px; }"
		      "QMenu::item:selected:!disabled {background-color: #ffeeaf; border: 1px solid #74440e; }"
		      "QMenu::item:disabled {color: #646464; }"
		      "QMenu::separator { height: 2px; background: yellow;}"
		      "QMenu::item:checked { background-color: #ffeeaf;}"
		      "QPushButton { background-color: #e9edff;}"
		      "QPushButton:hover { background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #e9edff, stop: 1 black)}");

	model_ = new BoardModel(type, this);
	model_->reset();
	ui_.tv_board->setModel(model_);
	if(type == Figure::WhitePlayer)
		ui_.tv_board->setCurrentIndex(model_->kingIndex());
	else
		ui_.tv_board->setCurrentIndex(model_->invert(model_->kingIndex()));

	ui_.te_moves->setText(tr("    White    Black\n"));

	connect(model_, SIGNAL(move(int,int,int,int, QString)), this, SIGNAL(move(int,int,int,int, QString)));
	connect(model_, SIGNAL(move(int,int,int,int, QString)), this, SLOT(addMove(int,int,int,int)));
	connect(model_, SIGNAL(figureKilled(Figure*)), this, SLOT(figureKilled(Figure*)));
	connect(model_, SIGNAL(needNewFigure(QModelIndex, QString)), this, SLOT(needNewFigure(QModelIndex, QString)));

	createMenu();
}

void ChessWindow::closeEvent(QCloseEvent *e)
{
	e->ignore();
	emit closeBoard();
}

void ChessWindow::moveRequest(int oldX, int oldY, int newX, int newY, const QString& figure) {
	bool b = model_->moveRequested(oldX, oldY, newX, newY);
	ui_.tv_board->viewport()->update();
	if(!b)
		emit error();
	else {
		emit moveAccepted();
		addMove(oldX, oldY, newX, newY);
	}
	if(!figure.isEmpty())
		model_->updateFigure(model_->index(7-newY,newX), figure);//7- - for compatibility with tkabber

	int state = model_->checkGameState();
	if(state == 2)
		emit lose();
	else if(state == 1)
		emit draw();
}

void ChessWindow::addMove(int oldX, int oldY, int newX, int newY) {
	Figure *f = 0;
	if(model_->gameType_ == Figure::WhitePlayer) { //for compatibility with tkabber
		oldY = 7-oldY;
		newY = 7-newY;
		f = model_->findFigure(model_->index(newY, newX));
	}
	else if(model_->gameType_ == Figure::BlackPlayer) {
		f = model_->findFigure(model_->index(7-newY, newX));
		oldX = 7-oldX;
		newX = 7-newX;
	}

	QString type = " ";
	if(f)
		type = f->typeString();

	QString text = ui_.te_moves->toPlainText();
	int moveNumber = movesCount+2;
	if(moveNumber&1) {
		text += "   "+type[0]+model_->headerData(oldX, Qt::Horizontal).toString().toLower()+model_->headerData(oldY, Qt::Vertical).toString()+"-"
			+model_->headerData(newX, Qt::Horizontal).toString().toLower()+model_->headerData(newY, Qt::Vertical).toString()+"\n";
	}
	else {
		text += QString::number(moveNumber/2)+". "+type[0]+model_->headerData(oldX, Qt::Horizontal).toString().toLower()
			+model_->headerData(oldY, Qt::Vertical).toString()+"-"+model_->headerData(newX, Qt::Horizontal).toString().toLower()
			+model_->headerData(newY, Qt::Vertical).toString();
	}

	ui_.te_moves->setText(text);
	QTextCursor cur = ui_.te_moves->textCursor();
	cur.setPosition(text.length());
	ui_.te_moves->setTextCursor(cur);
	movesCount++;
}

void ChessWindow::createMenu() {
	QMenuBar *menuBar = ui_.menubar;
	menuBar->setStyleSheet("QMenuBar::item {background-color: #ffffe7; border-radius: 1px; border: 1px solid #74440e; color: black;"
			       "spacing: 10px; padding: 1px 4px; background: transparent; }"
			       "QMenuBar::item:selected { background-color: #ffeeaf; color: black;  }"
			       "QMenuBar::item:pressed { background: #ffeeaf; color: black;  }");

	QAction *loadAction = new QAction(tr("Load game"), menuBar);
	QAction *saveAction = new QAction(tr("Save game"), menuBar);
	QAction *quitAction = new QAction(tr("Quit"), menuBar);
	loseAction = new QAction(tr("Resign"), menuBar);
	QAction *soundAction = new QAction(tr("Enable sound"),menuBar);
	soundAction->setCheckable(true);
	soundAction->setChecked(enabledSound);

	QMenu *fileMenu = menuBar->addMenu(tr("File"));
	QMenu *gameMenu = menuBar->addMenu(tr("Game"));

	fileMenu->addAction(loadAction);
	fileMenu->addAction(saveAction);
	fileMenu->addSeparator();
	fileMenu->addAction(quitAction);
	gameMenu->addAction(loseAction);
	gameMenu->addSeparator();
	gameMenu->addAction(soundAction);

	connect(loadAction, SIGNAL(triggered()), this, SLOT(load()));
	connect(saveAction, SIGNAL(triggered()), this, SLOT(save()));
	connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));
	connect(loseAction, SIGNAL(triggered()), this, SIGNAL(lose()));
	connect(soundAction, SIGNAL(triggered(bool)), this, SIGNAL(toggleEnableSound(bool)));
}

void ChessWindow::load() {
	QString fileName = QFileDialog::getOpenFileName(0,tr("Load game"), "", tr("*.chs"));
	if(fileName.isEmpty()) return;

	QFile file(fileName);
	if(file.open(QIODevice::ReadOnly)) {
		QTextStream in(&file);
		in.setCodec("UTF-8");
		QString settings = in.readAll();
		model_->loadSettings(settings);
		if(model_->gameType_ == Figure::WhitePlayer)
			ui_.tv_board->setCurrentIndex(model_->kingIndex());
		else
			ui_.tv_board->setCurrentIndex(model_->invert(model_->kingIndex()));

		emit load(settings);
		ui_.te_moves->setText(tr("  White     Black\n"));
		movesCount = 0;
	}
}

void ChessWindow::loadRequest(const QString& settings) {
	model_->loadSettings(settings, false);
	if(model_->gameType_ == Figure::WhitePlayer)
		ui_.tv_board->setCurrentIndex(model_->kingIndex());
	else
		ui_.tv_board->setCurrentIndex(model_->invert(model_->kingIndex()));

	ui_.te_moves->setText(tr("  White     Black\n"));
	movesCount = 0;
}

void ChessWindow::save() {
	QString fileName = QFileDialog::getSaveFileName(0,tr("Save game"), "", tr("*.chs"));
	if(fileName.isEmpty())
		return;
	if(fileName.right(4) != ".chs")
		fileName.append(".chs");

	QFile file(fileName);
	if(file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		QTextStream out(&file);
		out.setCodec("UTF-8");
		out.setGenerateByteOrderMark(false);
		out << model_->saveString();
	}
}

void ChessWindow::figureKilled(Figure *figure) {
	QPixmap pix = figure->getPixmap().scaled(24,24, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	QLabel *label = new QLabel;
	label->setFixedSize(24,24);
	label->setPixmap(pix);
	if(figure->gameType() == Figure::WhitePlayer) {
		ui_.white_layout->addWidget(label);
		if(!model_->myMove)
			ui_.tv_board->setCurrentIndex(model_->kingIndex());
	}
	else {
		ui_.black_layout->addWidget(label);
		if(!model_->myMove)
			ui_.tv_board->setCurrentIndex(model_->invert(model_->kingIndex()));
	}
}

void ChessWindow::needNewFigure(QModelIndex index, const  QString& player) {
	tmpIndex_ = index;
	if(model_->gameType_ == Figure::BlackPlayer)
		index = model_->invert(index);

	SelectFigure *sf = new SelectFigure(player, this);
	QPoint pos = ui_.tv_board->pos();
	pos.setX(pos.x() + index.column()*50 + 4);
	pos.setY(pos.y() + index.row()*50 + 25);
	sf->move(pos);
	connect(sf, SIGNAL(newFigure(QString)), this, SLOT(newFigure(QString)));
	sf->show();
}

void ChessWindow::newFigure(QString figure) {
	model_->updateFigure(tmpIndex_, figure);
}

void ChessWindow::youWin()
{
	model_->gameState_ = 2;
	model_->updateView();
	loseAction->setEnabled(false);
}

void ChessWindow::youLose()
{
	model_->gameState_ = 3;
	model_->updateView();
}

void ChessWindow::youDraw()
{
	model_->gameState_ = 1;
	model_->updateView();
}
