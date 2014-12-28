/*
 * pluginwindow.cpp - Battleship Game plugin
 * Copyright (C) 2014  Aleksey Andreev
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

#include "pluginwindow.h"
#include "boarddelegate.h"

PluginWindow::PluginWindow(const QString &jid, QWidget *parent)
	: QMainWindow(parent)
	, gm_(NULL)
{
	setAttribute(Qt::WA_DeleteOnClose);
	ui.setupUi(this);
	ui.lbOpponent->setText(jid);
}

void PluginWindow::initBoard()
{
	if (!gm_)
	{
		gm_ = new GameModel(this);
		connect(gm_, SIGNAL(gameEvent(QString)), this, SIGNAL(gameEvent(QString)));
		connect(gm_, SIGNAL(statusChanged()), this, SLOT(updateStatus()));
		connect(ui.actionNewGame, SIGNAL(triggered()), this, SLOT(newGame()));
		connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
		connect(ui.btnFreeze, SIGNAL(clicked()), this, SLOT(freezeShips()));
		connect(ui.btnDraw, SIGNAL(toggled(bool)), gm_, SLOT(setLocalDraw(bool)));
		connect(ui.btnAccept, SIGNAL(clicked()), gm_, SLOT(localAccept()));
		connect(ui.btnResign, SIGNAL(clicked()), gm_, SLOT(localResign()));
		connect(ui.actionResign, SIGNAL(triggered()), gm_, SLOT(localResign()));
		BoardModel *bmodel = new BoardModel(this);
		bmodel->init(gm_);
		BoardDelegate *bd_ = new BoardDelegate(bmodel, this);
		ui.tvBoard->setItemDelegate(bd_);
		ui.tvBoard->setModel(bmodel);
	}
	gm_->init();
	ui.tvBoard->reset();
}

void PluginWindow::setError()
{
	gm_->setError();
}

QStringList PluginWindow::dataExchange(const QStringList &data)
{
	const QString cmd = data.at(0);
	int cnt = data.count();
	if (cmd == "init-opp-board")
	{
		QStringList b = data;
		b.removeAt(0);
		if (gm_->initOpponentBoard(b))
			return QStringList("ok");
	}
	else if (cmd == "check-opp-board")
	{
		QStringList b = data;
		b.removeAt(0);
		if (gm_->uncoverOpponentBoard(b))
			return QStringList("ok");
	}
	else if (cmd == "start")
	{
		GameModel::GameStatus st = GameModel::StatusWaitingOpponent;
		if (cnt == 2 && data.at(1) == "first")
				st = GameModel::StatusMyTurn;
		gm_->setStatus(st);
	}
	else if (cmd == "turn")
	{
		int pos     = -1;
		bool draw   = false;
		bool accept = false;
		bool resign = false;
		for (int i = 1; i < cnt; ++i)
		{
			const QString str = data.at(i);
			const QString t = str.section(';', 0, 0);
			if (t == "shot")
				pos = str.section(';', 1, 1).toInt();
			else if (t == "draw")
				draw = true;
			else if (t == "accept")
				accept = true;
			else if (t == "resign")
				resign = true;
		}
		QStringList res("ok");
		gm_->setOpponentDraw(draw);
		gm_->setOpponentAcceptedDraw(accept);
		gm_->opponentTurn(pos);
		if (pos != -1)
			res.append(QString("result;%1;%2").arg(gm_->lastShotResult()).arg(gm_->lastShotSeed()));
		if (resign)
			gm_->opponentResign();
		res.append(QString("status;%1").arg(stringStatus(true)));
		return res;
	}
	else if (cmd == "turn-result")
	{
		bool err = true;
		if (cnt == 2)
		{
			QString	str = data.at(1);
			if (str.section(';', 0, 0) == "shot-result")
			{
				const QString res = str.section(';', 1, 1);
				const QString seed = str.section(';', 2);
				if (gm_->handleTurnResult(res, seed))
					err = false;
			}
		}
		else if (gm_->handleResult())
			err = false;
		if (!err)
		{
			QStringList res("ok");
			res.append(QString("status;%1").arg(stringStatus(true)));
			return res;
		}
	}
	else if (cmd == "get-uncovered-board")
	{
		QStringList res = gm_->getUncoveredBoard();
		return res;
	}
	return QStringList("error");
}

QString PluginWindow::stringStatus(bool short_) const
{
	switch (gm_->status())
	{
	case GameModel::StatusBoardInit:
		return short_ ? QString("init") : tr("Setting ships position");
		break;
	case GameModel::StatusMyTurn:
		return short_ ? QString("turn") : tr("Your turn");
	case GameModel::StatusWaitingTurnAccept:
		return short_ ? QString("waiting") : tr("Waiting for accept");
	case GameModel::StatusWaitingOpponent:
		return short_ ? QString("waiting") : tr("Waiting for opponent");
	case GameModel::StatusWin:
		return short_ ? QString("end") : tr("You Win!");
	case GameModel::StatusLose:
		return short_ ? QString("end") : tr("You Lose.");
	case GameModel::StatusDraw:
		return short_ ? QString("end") : tr("Draw");
	case GameModel::StatusError:
		return short_ ? QString("err") : tr("Error");
	default:
		break;
	}
	return QString();
}

void PluginWindow::updateStatus()
{
	updateWidgets();
	ui.lbStatus->setText(stringStatus(false));
}

void PluginWindow::freezeShips()
{
	ui.btnFreeze->setEnabled(false);
	gm_->sendCoveredBoard();
}

void PluginWindow::updateWidgets()
{
	GameModel::GameStatus st = gm_->status();
	ui.btnFreeze->setEnabled((st == GameModel::StatusBoardInit));
	bool f = (st == GameModel::StatusMyTurn);
	ui.btnDraw->setEnabled(f && !gm_->isOpponentDraw());
	if (f)
		ui.btnDraw->setChecked(false);
	ui.btnAccept->setEnabled((f && gm_->isOpponentDraw()));
	ui.btnResign->setEnabled(f);
	ui.actionResign->setEnabled(f);
	ui.actionNewGame->setEnabled((st == GameModel::StatusWin
				|| st == GameModel::StatusLose
				|| st == GameModel::StatusDraw
				|| st == GameModel::StatusError));
}

void PluginWindow::newGame()
{
	emit gameEvent("new-game");
}
