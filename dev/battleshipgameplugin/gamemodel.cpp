/*
 * gamemodel.cpp - Battleship game plugin
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

#include <QCryptographicHash>

#include "gamemodel.h"

GameModel::GameModel(QObject *parent)
	: QObject(parent)
	, status_(StatusNone)
	, lastShot_(-1)
	, draw_(false)
	, oppDraw_(false)
	, myAccept_(false)
	, oppResign_(false)
	, myResign_(false)
	, destroyed_(false)
{
	connect(&myBoard_, SIGNAL(shipDestroyed(int)), this, SLOT(myShipDestroyed()), Qt::DirectConnection);
}

void GameModel::init()
{
	setStatus(StatusBoardInit);
	opBoard_.init(GameBoard::CellUnknown, false);
	myBoard_.init(GameBoard::CellFree, true);
	myBoard_.makeShipRandomPosition();
}

void GameModel::setError()
{
	setStatus(StatusError);
}

bool GameModel::initOpponentBoard(const QStringList &data)
{
	bool err = false;
	int cnt = data.count();
	for (int i = 0; i < cnt; ++i)
	{
		const QString str = data.at(i);
		const QString t = str.section(';', 0, 0);
		int n           = str.section(';', 1, 1).toInt();
		const QString s = str.section(';', 2);
		if (t == "cell")
		{
			if (!opBoard_.updateCellDigest(n, s))
			{
				err = true;
				break;
			}
		}
		else if (t == "ship")
		{
			if (!opBoard_.updateShipDigest(n, s))
			{
				err = true;
				break;
			}
		}
	}
	return !err;
}

bool GameModel::uncoverOpponentBoard(const QStringList &data)
{
	bool err = false;
	int cnt = data.count();
	for (int i = 0; i < cnt; ++i)
	{
		const QString str = data.at(i);
		int pos = str.section(';', 0, 0).toInt();
		GameBoard::CellStatus cs = (str.section(';', 1, 1) == "1") ? GameBoard::CellOccupied : GameBoard::CellFree;
		const QString seed = str.section(';', 2);
		if (!opBoard_.updateCell(pos, cs, seed))
		{
			err = true;
			break;
		}
	}
	emit oppBoardUpdated(0, 0, 10, 10);
	return !err;
}

void GameModel::setOpponentDraw(bool draw)
{
	oppDraw_ = draw;
}

void GameModel::setOpponentAcceptedDraw(bool accept)
{
	if (draw_)
	{
		if (accept)
			setStatus(StatusDraw);
		else
			draw_ = false;
	}
}

void GameModel::opponentResign()
{
	oppResign_ = true;
	setStatus(StatusWin);
}

void GameModel::sendCoveredBoard()
{
	myBoard_.calculateCellsHash();
	emit gameEvent("covered-board\n" + myBoard_.toStringList(true).join("\n"));
}

void GameModel::localTurn(int pos)
{
	if (status_ == StatusMyTurn)
	{
		lastShot_ = pos;
		QString data = QString("turn\npos;%1").arg(pos);
		if (draw_)
			data.append("\ndraw");
		setStatus(StatusWaitingTurnAccept);
		emit gameEvent(data);
	}
}

void GameModel::opponentTurn(int pos)
{
	if (status_ == StatusWaitingOpponent)
	{
		lastShot_  = pos;
		destroyed_ = false;
		draw_      = false;
		if (pos != -1)
		{
			myBoard_.shot(pos);
			int row = pos / 10;
			int col = pos % 10;
			emit myBoardUpdated(col, row, 1, 1);
			if (lastShotResult() == "miss")
				setStatus(StatusMyTurn);
			else if (myBoard_.isAllDestroyed())
				setStatus(StatusLose);
			else if (oppDraw_)
				setStatus(StatusMyTurn);
			else
				setStatus(StatusWaitingOpponent);
		}
	}
}

bool GameModel::handleResult()
{
	if (myAccept_)
	{
		setStatus(StatusDraw);
		return true;
	}
	if (myResign_)
	{
		setStatus(StatusLose);
		return true;
	}
	return false;
}

bool GameModel::handleTurnResult(const QString &res, const QString &seed)
{
	GameBoard::CellStatus cs = GameBoard::CellUnknown;
	if (res == "miss")
		cs = GameBoard::CellMiss;
	else if (res == "hit" || res == "destroy")
		cs = GameBoard::CellHit;
	if (cs != GameBoard::CellUnknown && opBoard_.updateCell(lastShot_, cs, seed))
	{
		int snum = -1;
		if (res != "destroy" || (snum = opBoard_.findAndInitShip(lastShot_)) != -1)
		{
			QPoint p(lastShot_ / 10, lastShot_ % 10);
			QRect r;
			r.setTopLeft(p);
			r.setSize(QSize(1, 1));
			if (snum != -1)
			{
				opBoard_.setShipDestroy(snum, true);
				r = opBoard_.shipRect(snum, true);
			}
			if (cs == GameBoard::CellMiss)
				setStatus(StatusWaitingOpponent);
			else if (snum != -1 && opBoard_.isAllDestroyed())
				setStatus(StatusWin);
			else if (draw_)
				setStatus(StatusWaitingOpponent);
			else
				setStatus(StatusMyTurn);
			emit oppBoardUpdated(r.left(), r.top(), r.width(), r.height());
			return true;
		}
	}
	setStatus(StatusError);
	return false;
}

QString GameModel::lastShotResult() const
{
	QString res;
	if (lastShot_ != -1)
	{
		const GameBoard::GameCell &cell = myBoard_.cell(lastShot_);
		const GameBoard::CellStatus cs = cell.status;
		if (cs == GameBoard::CellHit)
		{
			if (destroyed_)
				res = "destroy";
			else
				res = "hit";
		}
		else
			res = "miss";
	}
	return res;
}

QString GameModel::lastShotSeed() const
{
	QString res;
	if (lastShot_ != -1)
		res = myBoard_.cell(lastShot_).seed;
	return res;
}

QStringList GameModel::getUncoveredBoard() const
{
	return myBoard_.toStringList(false);
}

void GameModel::setStatus(GameStatus s)
{
	status_ = s;
	emit statusChanged();
}

void GameModel::myShipDestroyed()
{
	destroyed_ = true;
}

void GameModel::setLocalDraw(bool draw)
{
	draw_ = draw;
}

void GameModel::localAccept()
{
	if (status_ == StatusMyTurn && oppDraw_)
	{
		myAccept_ = true;
		setStatus(StatusDraw);
		emit gameEvent("turn\naccept");
	}
}

void GameModel::localResign()
{
	if (status_ == StatusMyTurn)
	{
		myResign_ = true;
		setStatus(StatusLose);
		emit gameEvent("turn\nresign");
	}
}

//------------- GameBoard ----------------

GameBoard::GameBoard(QObject *parent)
	: QObject(parent)
{
}

void GameBoard::init(CellStatus s, bool genseed)
{
	cells_.clear();
	qDeleteAll(ships_);
	ships_.clear();

	for (int i = 0; i < 100; ++i)
	{
		cells_.append(GameCell(s));
		if (genseed)
			cells_[i].seed = genSeed(32);
	}
	ships_.append(new GameShip(5, QString(), this));
	ships_.append(new GameShip(4, QString(), this));
	ships_.append(new GameShip(3, QString(), this));
	ships_.append(new GameShip(2, QString(), this));
	ships_.append(new GameShip(2, QString(), this));
	ships_.append(new GameShip(1, QString(), this));
	ships_.append(new GameShip(1, QString(), this));
}

void GameBoard::makeShipRandomPosition()
{
	int scnt = ships_.count();
	for (int snum = 0; snum < scnt; ++snum)
	{
		GameShip *ship = ships_.at(snum);
		int slen = ship->length();
		GameShip::ShipType dir;
		for ( ; ; )
		{
			dir = GameShip::ShipHorizontal;
			int div;
			if (slen > 1 && (qrand() & 1))
			{
				dir = GameShip::ShipVertical;
				div = 100 - 10 * (slen - 1);
			}
			else
				div = 100 - (slen - 1);
			ship->setDirection(dir);
			ship->setPosition(qrand() % div);
			if (isShipPositionLegal(snum))
				break;
		}
		int offset = 1;
		if (dir == GameShip::ShipVertical)
			offset = 10;
		int pos = ship->position();
		QCryptographicHash sha1(QCryptographicHash::Sha1);
		for ( ; slen != 0; --slen)
		{
			cells_[pos].ship   = snum;
			cells_[pos].status = CellOccupied;
			sha1.addData(cells_.at(pos).seed.toUtf8());
			pos += offset;
		}
		ship->setDigest(sha1.result().toHex());
	}
}

void GameBoard::calculateCellsHash()
{
	int cnt = cells_.count();
	QCryptographicHash sha1(QCryptographicHash::Sha1);
	for (int i = 0; i < cnt; ++i)
	{
		sha1.reset();
		sha1.addData(cells_.at(i).seed.toUtf8());
		sha1.addData((cells_.at(i).ship == -1) ? "0" : "1");
		cells_[i].digest = QString(sha1.result().toHex());
	}
}

bool GameBoard::updateCellDigest(int pos, const QString &digest)
{
	if (pos >= 0 && pos < cells_.count() && digest.length() == 40)
	{
		cells_[pos].digest = digest;
		return true;
	}
	return false;
}

bool GameBoard::updateCell(int pos, CellStatus cs, const QString &seed)
{
	if (pos >= 0 && pos < cells_.count())
	{
		if (!cells_.at(pos).seed.isEmpty()) // again shot at this position
			return true;

		QString seed_   = seed + ((cs == CellHit || cs == CellOccupied) ? "1" : "0");
		QString digest_ =  QCryptographicHash::hash(seed_.toUtf8(), QCryptographicHash::Sha1).toHex();
		if (digest_ == cells_.at(pos).digest)
		{
			cells_[pos].seed   = seed;
			if (cells_.at(pos).status == CellUnknown)
				cells_[pos].status = cs;
			return true;
		}
	}
	return false;
}

bool GameBoard::updateShipDigest(int length, const QString &digest)
{
	GameShip *ship = findShip(length, QString());
	if (ship)
	{
		ship->setDigest(digest);
		return true;
	}
	return false;
}

void GameBoard::shot(int pos)
{
	CellStatus cs = cells_.at(pos).status;
	if (cs == CellFree)
		cells_[pos].status = CellMiss;
	else if (cs == CellOccupied)
	{
		cells_[pos].status = CellHit;
		int snum = cells_.at(pos).ship;
		GameShip *ship = ships_.at(snum);
		bool destr = true;
		int p = -1;
		while ((p = ship->nextPosition(p)) != -1)
			if (cells_.at(p).status != CellHit)
			{
				destr = false;
				break;
			}
		if (destr)
		{
			ship->setDestroyed(true);
			emit shipDestroyed(snum);
		}
	}
}

GameShip::ShipType GameBoard::shipDirection(int pos)
{
	GameShip::ShipType dir = GameShip::ShipDirUnknown;
	if ((pos >= 10 && (cells_.at(pos - 10).status == CellHit || cells_.at(pos - 10).status == CellOccupied))
			|| (pos <= 89 && (cells_.at(pos + 10).status == CellHit || cells_.at(pos + 10).status == CellOccupied)))
		dir = GameShip::ShipVertical;
	else
	{
		int col = pos % 10;
		if ((col > 0 && (cells_.at(pos - 1).status == CellHit || cells_.at(pos - 1).status == CellOccupied))
			|| (col < 9 && (cells_.at(pos + 1).status == CellHit || cells_.at(pos + 1).status == CellOccupied)))
			dir = GameShip::ShipHorizontal;
	}
	return dir;
}

int GameBoard::findAndInitShip(int pos)
{
	GameShip::ShipType dir = shipDirection(pos);
	if (dir == GameShip::ShipDirUnknown)
		dir = GameShip::ShipHorizontal; // For one-cell ship
	// Find starting coordinate
	int spos = pos;
	if (dir == GameShip::ShipHorizontal)
		while (spos % 10 != 0 && (cells_.at(spos - 1).status == CellHit || cells_.at(spos - 1).status == CellOccupied))
			--spos;
	else
		while (spos >= 10 && (cells_.at(spos - 10).status == CellHit || cells_.at(spos - 10).status == CellOccupied))
			spos -= 10;
	// There do calculating the ship's hash and size
	int ssz = 0;
	int epos = spos;
	QCryptographicHash sha1(QCryptographicHash::Sha1);
	for ( ; ; )
	{
		++ssz;
		sha1.addData(cells_.at(epos).seed.toUtf8());
		if (dir == GameShip::ShipHorizontal)
		{
			if (epos % 10 == 9 || (cells_.at(epos + 1).status != CellHit && cells_.at(epos + 1).status != CellOccupied))
				break;
			++epos;
		}
		else
		{
			if (epos >= 90 || (cells_.at(epos + 10).status != CellHit && cells_.at(epos + 10).status != CellOccupied))
				break;
			epos += 10;
		}
	}
	// Find ship by hash and size
	QString digest = QString(sha1.result().toHex());
	int snum = -1;
	for (int i = 0; i < ships_.count(); ++i)
	{
		GameShip *ship = ships_.at(i);
		if (ship->length() == ssz && ship->digest() == digest)
		{
			ship->setDirection(dir);
			ship->setPosition(spos);
			snum = i;
			break;
		}
	}
	if (snum != -1)
	{
		// Update cells
		for (int i = 0; i < ssz; ++i)
		{
			cells_[spos].ship = snum;
			++spos;
			if (dir == GameShip::ShipVertical)
				spos += 9;
		}
	}
	return snum;
}

QRect GameBoard::shipRect(int snum, bool margin) const
{
	QRect r;
	GameShip *ship = ships_.at(snum);
	int pos = ship->position();
	r.setTopLeft(QPoint(pos % 10, pos / 10));
	if (ship->direction() == GameShip::ShipHorizontal)
	{
		r.setWidth(ship->length());
		r.setHeight(1);
	}
	else
	{
		r.setWidth(1);
		r.setHeight(ship->length());
	}
	if (margin)
	{
		r.adjust(-1, -1, 1, 1);
#ifdef HAVE_QT5
		r = r.intersected(QRect(0, 0, 10, 10));
#else
		r = r.intersect(QRect(0, 0, 10, 10));
#endif
	}
	return r;
}

void GameBoard::setShipDestroy(int n, bool margin)
{
	GameShip *ship = ships_.at(n);
	if (!ship->isDestroyed())
	{
		ship->setDestroyed(true);
		if (margin)
			fillShipMargin(n);
		emit shipDestroyed(n);
	}
}

const GameBoard::GameCell &GameBoard::cell(int pos) const
{
	return cells_.at(pos);
}

QStringList GameBoard::toStringList(bool covered) const
{
	QStringList res;
	int cnt = cells_.count();
	for (int i = 0; i < cnt; ++i)
	{
		const GameCell &c = cells_.at(i);
		QString s;
		if (covered)
			s = QString("cell;%1;%2").arg(i).arg(c.digest);
		else
			s = QString("%1;%2;%3")
				.arg(i)
				.arg((c.ship == -1) ? "0" : "1")
				.arg(c.seed);
		res.append(s);
	}
	if (covered)
	{
		cnt = ships_.count();
		for (int i = 0; i < cnt; ++i)
		{
			const GameShip *ship = ships_.at(i);
			res.append(QString("ship;%1;%2").arg(ship->length()).arg(ship->digest()));
		}
	}
	return res;
}

bool GameBoard::isAllDestroyed() const
{
	foreach (const GameShip *ship, ships_)
		if (!ship->isDestroyed())
			return false;
	return true;
}

QString GameBoard::genSeed(int len)
{
	static QString chars("1234567890qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM-=[]/!@#$%^&*()");
	int ccnt = chars.length();
	int rnd = 0;
	QString res;
	for (int i = 0; i < len; ++i)
	{
		if (rnd < ccnt)
			rnd = qrand();
		res.append(chars.at(rnd % ccnt));
		rnd /= ccnt;
	}
	return res;
}

GameShip *GameBoard::findShip(int length, const QString &digest)
{
	foreach (GameShip *ship, ships_)
		if (ship->length() == length && ship->digest() == digest)
			return ship;
	return NULL;
}

bool GameBoard::isShipPositionLegal(int shipNum)
{
	GameShip *ship = ships_.at(shipNum);
	GameShip::ShipType dir = ship->direction();
	int pos = ship->position();
	int len = ship->length();
	int off = (dir == GameShip::ShipHorizontal) ? 1 : 10;
	int row = pos / 10;
	int col = pos % 10;
	int epos = pos + (len - 1) * off;
	if ((dir == GameShip::ShipHorizontal && (int)(epos) / 10 != row)
			|| (dir == GameShip::ShipVertical && epos >= 100))
		return false;

	int m = 1;
	if (dir == GameShip::ShipHorizontal)
	{
		if (row > 0)
		{
			++m;
			pos -= 10;
		}
		if (col > 0)
		{
			--pos;
			++len;
		}
		if (row < 9)
			++m;
		if (epos % 10 < 9)
			++len;
	}
	else
	{
		if (col > 0)
		{
			++m;
			pos -= 1;
		}
		if (row > 0)
		{
			pos -= 10;
			++len;
		}
		if (col < 9)
			++m;
		if (epos < 90)
			++len;
	}
	for ( ; m != 0; --m)
	{
		int p = pos;
		for (int l = len; l != 0; --l)
		{
			CellStatus cs = cells_.at(p).status;
			if ((cs == CellOccupied || cs == CellHit) && cells_.at(p).ship != shipNum)
				return false;
			p += off;
		}
		pos += (dir == GameShip::ShipHorizontal) ? 10 : 1;
	}
	return true;
}

void GameBoard::fillShipMargin(int n)
{
	GameShip *ship = ships_.at(n);
	int pos = ship->position();
	int len = ship->length();
	GameShip::ShipType dir = ship->direction();
	struct
	{
		int offset;
		int weight;
	} m[8];
	m[7].offset = -11; m[0].offset = -10; m[1].offset =  -9;
	m[6].offset =  -1;                    m[2].offset =   1;
	m[5].offset =   9; m[4].offset =  10; m[3].offset =  11;
	for (int i = 1; i <= len; ++i)
	{
		int row = pos / 10;
		int col = pos % 10;
		for (int k = 0; k < 8; ++k)
			m[k].weight = 0;
		if (row > 0)
		{
			++m[7].weight;
			++m[0].weight;
			++m[1].weight;
		}
		if (row < 9)
		{
			++m[5].weight;
			++m[4].weight;
			++m[3].weight;
		}
		if (col > 0)
		{
			++m[7].weight;
			++m[6].weight;
			++m[5].weight;
		}
		if (col < 9)
		{
			++m[1].weight;
			++m[2].weight;
			++m[3].weight;
		}
		int nextOffset;
		if (dir == GameShip::ShipHorizontal)
		{
			nextOffset = 1;
			++m[0].weight;
			++m[4].weight;
			if (i == 1)
			{
				++m[7].weight;
				++m[6].weight;
				++m[5].weight;
			}
			if (i == len)
			{
				++m[1].weight;
				++m[2].weight;
				++m[3].weight;
			}
		}
		else
		{
			nextOffset = 10;
			++m[6].weight;
			++m[2].weight;
			if (i == 1)
			{
				++m[7].weight;
				++m[0].weight;
				++m[1].weight;
			}
			if (i == len)
			{
				++m[5].weight;
				++m[4].weight;
				++m[3].weight;
			}
		}
		for (int k = 0; k < 8; ++k)
			if (m[k].weight == 3 || ((k & 1) == 0 && m[k].weight == 2))
			{
				if (cells_.at(pos + m[k].offset).status == CellUnknown)
					cells_[pos + m[k].offset].status = CellMargin;
			}
		pos += nextOffset;
	}
}

//------------- GameShip ----------------

GameShip::GameShip(int len, const QString &digest, QObject *parent)
	: QObject(parent)
	, length_(len)
	, direction_(ShipDirUnknown)
	, firstPos_(-1)
	, destroyed_(false)
	, digest_(digest)
{
}

void GameShip::setDirection(ShipType dir)
{
	direction_ = dir;
}

void GameShip::setPosition(int pos)
{
	firstPos_ = pos;
}

void GameShip::setDigest(const QString &digest)
{
	digest_ = digest;
}

void GameShip::setDestroyed(bool destr)
{
	destroyed_ = destr;
}

int GameShip::nextPosition(int prev)
{
	if (prev == -1)
		return firstPos_;
	int offs = (direction_ == ShipHorizontal) ? 1 : 10;
	if ((prev - firstPos_) >= (length_ - 1) * offs)
		return -1;
	return prev + offs;
}
