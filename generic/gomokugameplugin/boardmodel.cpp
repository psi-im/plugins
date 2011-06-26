/*
 * boardmodel.cpp - Gomoku Game plugin
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

#include "boardmodel.h"
#include "common.h"

BoardModel::BoardModel(QObject *parent) :
    QAbstractTableModel(parent),
    selectX(-1),
    selectY(-1),
    myElement_(GameElement::TypeNone),
    boardSizeX(15),
    boardSizeY(15),
    columnCount_(0),
    rowCount_(0),
    lastX(-1),
    lastY(-1),
    turnsCount(0),
    blackCount(0),
    whiteCount(0),
    loadGameFlag(false)
{
	gameElements.clear();
}

BoardModel::~BoardModel()
{
	while (!gameElements.isEmpty()) {
		delete gameElements.takeFirst();
	}
}

void BoardModel::init(GameElement::ElementType myElement)
{
	setHeaders();
	reset();
	myElement_ = myElement;
	if (myElement_ == GameElement::TypeBlack) {
		setGameStatus(StatusThinking);
	} else {
		setGameStatus(StatusWaitingOpponent);
	}
}

void BoardModel::reset()
{
	while (!gameElements.isEmpty()) {
		delete gameElements.takeFirst();
	}
	lastX = -1;
	lastY = -1;
	turnsCount = 0;
	blackCount = 0;
	whiteCount = 0;
	selectX = -1;
	selectY = -1;
	loadGameFlag = false;
	gameStatus = StatusNone;
	myElement_ = GameElement::TypeNone;
	QAbstractTableModel::reset();
}

void BoardModel::setHeaders()
{
	rowCount_ = boardSizeY + 4;
	columnCount_ = boardSizeX + 4;
}

void BoardModel::setGameStatus(GameStatus status)
{
	if (gameStatus != status) {
		gameStatus = status;
		emit changeGameStatus(status);
	}
}

Qt::ItemFlags BoardModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = Qt::NoItemFlags | Qt::ItemIsEnabled;
	int row = index.row();
	if (row < 2 || row > rowCount_ - 2) {
		return flags;
	}
	int col = index.column();
	if (col < 2 || col > columnCount_ - 2) {
		return flags;
	}
	return (flags |  Qt::ItemIsSelectable);
}

QVariant BoardModel::headerData(int /*section*/, Qt::Orientation /*orientation*/, int /*role*/) const
{
	return QVariant();
}

QVariant BoardModel::data(const QModelIndex & /*index*/, int /*role*/) const
{
	return QVariant();
}

bool BoardModel::setData(const QModelIndex &index, const QVariant &/*value*/, int role)
{
	if (index.isValid() && role == Qt::DisplayRole) {
		emit dataChanged(index, index);
		return true;
	}
	return false;
}

int BoardModel::rowCount(const QModelIndex & /*parent*/) const
{
	return rowCount_;
}

int BoardModel::columnCount(const QModelIndex & /*parent*/) const
{
	return columnCount_;
}

GameElement *BoardModel::getGameElement(int x, int y)
{
	const int idx = getGameElementIndex(x, y);
	if (idx != -1)
		return gameElements.at(idx);
	return NULL;
}

int BoardModel::getGameElementIndex(int x, int y) const
{
	int cnt = gameElements.size();
	for (int i = 0; i < cnt; i++) {
		GameElement *el = gameElements.at(i);
		if (el->x() == x && el->y() == y)
			return i;
	}
	return -1;
}

bool BoardModel::setGameElement(GameElement* el)
{
	if (!el)
		return false;
	int x = el->x();
	if (x < 0 || x >= columnCount())
		return false;
	int y = el->y();
	if (y < 0 || y >= rowCount())
		return false;
	const int idx = getGameElementIndex(x, y);
	if (idx != -1) {
		GameElement *old_el = gameElements.at(idx);
		gameElements[idx] = el;
		delete old_el;
	} else {
		gameElements.push_back(el);
	}
	if (el->type() == GameElement::TypeBlack) {
		++blackCount;
	} else {
		++whiteCount;
	}
	return true;
}

/**
 * Был щелчек мышью по доске
 */
bool BoardModel::clickToBoard(QModelIndex index)
{
	if (gameStatus == StatusThinking) {
		if (index.isValid()) {
			int x = index.column() - 2;
			int y = index.row() - 2;
			if (setElementToBoard(x, y, true)) {
				setGameStatus(StatusWaitingAccept);
				emit setupElement(x, y);
				return true;
			}
		}
	}
	return false;
}

bool BoardModel::opponentTurn(int x, int y)
{
	if (gameStatus == StatusWaitingOpponent) {
		if (setElementToBoard(x, y, false)) {
			if (checkGameForLose()) {
				QMetaObject::invokeMethod(this, "setLose", Qt::QueuedConnection);
			} else if (checkGameForDraw()) {
				QMetaObject::invokeMethod(this, "setMyDraw", Qt::QueuedConnection);
			} else {
				setGameStatus(StatusThinking);
			}
			return true;
		}
		setGameStatus(StatusError);
	}
	return false;
}

void BoardModel::setAccept()
{
	if (gameStatus == StatusWaitingAccept) {
		if (!loadGameFlag) {
			setGameStatus(StatusWaitingOpponent);
		} else {
			// Получили подтверждение загрузки игры
			int i = blackCount - whiteCount;
			if ((myElement_ == GameElement::TypeBlack && i == 0) || (myElement_ == GameElement::TypeWhite && i == 1)) {
				setGameStatus(StatusThinking);
			} else {
				setGameStatus(StatusWaitingOpponent);
			}
		}
	}
}

void BoardModel::setError()
{
	setGameStatus(StatusError);
}

void BoardModel::setClose()
{
	setGameStatus(StatusEndGame);
}

void BoardModel::setWin()
{
	setGameStatus(StatusWin);
}

void BoardModel::opponentDraw()
{
	setGameStatus(StatusDraw);
}

void BoardModel::setResign()
{
	setLose();
}

/**
 * Мы проиграли. Сообщаем оппоненту и самому себе.
 */
void BoardModel::setLose()
{
	emit lose();
	setGameStatus(StatusLose);
}

/**
 * Ничья. Сообщаем оппоненту и самому себе.
 */
void BoardModel::setMyDraw()
{
	emit draw();
	setGameStatus(StatusDraw);
}

/**
 * Добавляет новый элемент (в данном случае камень) в массив игровых элементов.
 * Возвращает true в случае удачи.
 */
bool BoardModel::setElementToBoard(int x, int y, bool my_element)
{
	if (x < 0 || y < 0 || x >= boardSizeX || y >= boardSizeY)
		return false;
	if (turnsCount == 0 && (x != 7 || y != 7)) {
		emit doPopup(tr("The first turn can be only H8."));
		return false;
	}
	if (getGameElement(x, y) != NULL)
		return false;
	GameElement *el;
	if (my_element) {
		el = new GameElement(myElement_, x, y);
	} else {
		el = new GameElement((myElement_ == GameElement::TypeBlack) ? GameElement::TypeWhite : GameElement::TypeBlack, x, y);
	}
	if (setGameElement(el)) {
		lastX = x;
		lastY = y;
		turnsCount++;
		QModelIndex model = index(y + 2, x + 2); // Offset for margin
		emit dataChanged(model, model);
		return true;
	} else {
		delete el;
	}
	return false;
}

/**
 * Проверяем проиграли ли мы после последнего хода оппонента
 */
bool BoardModel::checkGameForLose()
{
	int max_x = columnCount() - 1;
	int max_y = rowCount() - 1;
	if (lastX < 0 || lastX >= max_x || lastY < 0 || lastY >= max_y)
		return false;
	// Проверяем вверх
	int vert = 1;
	if (lastY > 0) {
		int tmp_y = lastY - 1;
		for (; tmp_y >= 0; tmp_y--) {
			GameElement *el = getGameElement(lastX, tmp_y);
			if (!el || el->type() == myElement_)
				break;
		}
		vert = lastY - tmp_y;
		if (vert > 5)
			return false;
	}
	// Проверяем вниз
	if (lastY < max_y) {
		int tmp_y = lastY + 1;
		for (; tmp_y <= max_y; tmp_y++) {
			GameElement *el = getGameElement(lastX, tmp_y);
			if (!el || el->type() == myElement_)
				break;
		}
		vert += tmp_y - lastY - 1;
		if (vert > 5)
			return false;
	}
	// Слева
	int horiz = 1;
	if (lastX > 0) {
		int tmp_x = lastX - 1;
		for (; tmp_x >= 0; tmp_x--) {
			GameElement *el = getGameElement(tmp_x, lastY);
			if (!el || el->type() == myElement_)
				break;
		}
		horiz = lastX - tmp_x;
		if (horiz > 5)
			return false;
	}
	// Справа
	if (lastX < max_x) {
		int tmp_x = lastX + 1;
		for (; tmp_x <= max_x; tmp_x++) {
			GameElement *el = getGameElement(tmp_x, lastY);
			if (!el || el->type() == myElement_)
				break;
		}
		horiz += tmp_x - lastX - 1;
		if (horiz > 5)
			return false;
	}
	// Лево вверх
	int diag1 = 1;
	if (lastX > 0 && lastY > 0) {
		int tmp_y = lastY - 1;
		for (int tmp_x = lastX - 1; tmp_x >= 0; tmp_x--) {
			GameElement *el = getGameElement(tmp_x, tmp_y);
			if (!el || el->type() == myElement_)
				break;
			tmp_y--;
			if (tmp_y < 0)
				break;
		}
		diag1 = lastY - tmp_y;
		if (diag1 > 5)
			return false;
	}
	// Право вниз
	if (lastX < max_x && lastY < max_y) {
		int tmp_y = lastY + 1;
		for (int tmp_x = lastX + 1; tmp_x <= max_x; tmp_x++) {
			GameElement *el = getGameElement(tmp_x, tmp_y);
			if (!el || el->type() == myElement_)
				break;
			tmp_y++;
			if (tmp_y > max_y)
				break;
		}
		diag1 += tmp_y - lastY - 1;
		if (diag1 > 5)
			return false;
	}
	// Право верх
	int diag2 = 1;
	if (lastX < max_x && lastY > 0) {
		int tmp_y = lastY - 1;
		for (int tmp_x = lastX + 1; tmp_x <= max_x; tmp_x++) {
			GameElement *el = getGameElement(tmp_x, tmp_y);
			if (!el || el->type() == myElement_)
				break;
			tmp_y--;
			if (tmp_y < 0)
				break;
		}
		diag2 = lastY - tmp_y;
		if (diag2 > 5)
			return false;
	}
	// Лево низ
	if (lastX > 0 && lastY < max_y) {
		int tmp_y = lastY + 1;
		for (int tmp_x = lastX - 1; tmp_x >= 0; tmp_x--) {
			GameElement *el = getGameElement(tmp_x, tmp_y);
			if (!el || el->type() == myElement_)
				break;
			tmp_y++;
			if (tmp_y > max_y)
				break;
		}
		diag2 += tmp_y - lastY - 1;
		if (diag2 > 5)
			return false;
	}
	if (vert == 5 || horiz == 5 || diag1 == 5 || diag2 == 5)
		return true;
	return false;
}

/**
 * Проверяем наличие ничьей
 */
bool BoardModel::checkGameForDraw()
{
	// Если доска заполнена
	return (turnsCount == (boardSizeX * boardSizeY));
}

/**
 * Возвращает номер хода
 */
int BoardModel::turnNum()
{
	return (turnsCount + 1);
}

/**
 * Текущий цвет игрока
 */
GameElement::ElementType BoardModel::elementType()
{
	return myElement_;
}

/**
 * Переключает цвет игрока (предусмотрено международными правилами)
 */
bool BoardModel::doSwitchColor(bool local_init)
{
	if (local_init) {
		if (gameStatus != StatusThinking)
			return false;
	} else {
		if (gameStatus != StatusWaitingOpponent)
			return false;
	}
	if (turnsCount != 3)
		return false;
	turnsCount++;
	if (local_init) {
		myElement_ = GameElement::TypeBlack;
		setGameStatus(StatusWaitingAccept);
		emit switchColor();
	} else {
		myElement_ = GameElement::TypeWhite;
		setGameStatus(StatusThinking);
	}
	return true;
}

/**
 * Сохраняет состояние игры в строку
 */
QString BoardModel::saveToString() const
{
	QString res_str = "gomokugameplugin.save.1;\n";
	foreach (GameElement *el, gameElements) {
		int x = el->x();
		int y = el->y();
		if ((gameStatus == StatusWaitingAccept || gameStatus == StatusError) && x == lastX && y == lastY) // Не сохраняем не подтвержденное
			continue;
		res_str.append(QString("Element:%1,%2,%3;\n")
				.arg(x)
				.arg(y)
				.arg((el->type() == GameElement::TypeBlack) ? "black" : "white"));
	}
	res_str.append(QString("Color:%1;\n").arg((myElement_ == GameElement::TypeBlack) ? "black" : "white"));
	QString stat_str;
	if (gameStatus == StatusError) {
		stat_str = "error";
	} else if (gameStatus == StatusWin) {
		stat_str = "win";
	} else if (gameStatus == StatusLose) {
		stat_str = "lose";
	} else {
		stat_str = "play";
	}
	res_str.append(QString("Status:%1;\n").arg(stat_str));
	return res_str;
}

/**
 * Загружает игру из строки
 */
bool BoardModel::loadFromString(QString settings, bool my_load)
{
	QStringList settingsList = settings.split(";");
	if (settingsList.isEmpty() || settingsList.takeFirst() != "gomokugameplugin.save.1")
		return false;
	bool res = true;
	int black_cnt = 0;
	int white_cnt = 0;
	GameStatus status = StatusNone;
	GameElement::ElementType my = GameElement::TypeNone;
	QList<GameElement *> tmpElements;
	while (!settingsList.isEmpty()) {
		QString str1 = settingsList.takeFirst().trimmed();
		if (str1.isEmpty())
			continue;
		QStringList setStrList = str1.split(":", QString::SkipEmptyParts);
		if (setStrList.size() != 2) {
			res = false;
			break;
		}
		QString parName = setStrList.at(0).trimmed();
		if (parName == "Element") {
			QStringList elemPar = setStrList.at(1).trimmed().split(",");
			if (elemPar.size() != 3) {
				res = false;
				break;
			}
			bool fOk;
			int x = elemPar.at(0).toInt(&fOk);
			if (!fOk || x < 0 || x >= columnCount()) {
				res = false;
				break;
			}
			int y = elemPar.at(1).toInt(&fOk);
			if (!fOk || y < 0 || y >= rowCount()) {
				res = false;
				break;
			}
			GameElement::ElementType type;
			if (elemPar.at(2) == "black") {
				type = GameElement::TypeBlack;
				++black_cnt;
			} else if (elemPar.at(2) == "white") {
				type = GameElement::TypeWhite;
				++white_cnt;
			} else {
				res = false;
				break;
			}
			GameElement *el = new GameElement(type, x, y);
			if (!el) {
				res = false;
				break;
			}
			tmpElements.push_back(el);
		} else if (parName == "Color") {
			if (setStrList.at(1) == "black") {
				my = GameElement::TypeBlack;
			} else if (setStrList.at(1) == "white") {
				my = GameElement::TypeWhite;
			}
		} else if (parName == "Status") {
			if (setStrList.at(1) == "error") {
				status = StatusError;
			} else if (setStrList.at(1) == "win") {
				status = StatusWin;
			} else if (setStrList.at(1) == "lose") {
				status = StatusLose;
			}
		}
	}
	int delta = black_cnt - white_cnt;
	if (!res || delta < 0 || delta > 1 || my == GameElement::TypeNone) {
		// Неудачная загрузка, удаляем созданные объекты
		while (!tmpElements.isEmpty())
			delete tmpElements.takeFirst();
		return false;
	}
	// Сбрасываем модель
	reset();
	// Указываем наш цвет
	if (my_load) {
		myElement_ = my;
	} else {
		if (my == GameElement::TypeBlack) {
			myElement_ = GameElement::TypeWhite;
		} else {
			myElement_ = GameElement::TypeBlack;
		}
	}
	// Загружаем элементы на доску
	while (!tmpElements.isEmpty()) {
		gameElements.push_back(tmpElements.takeFirst());
	}
	// Устанавливаем кол-во ходов
	turnsCount = black_cnt + white_cnt;
	// Количество элементов
	blackCount = black_cnt;
	whiteCount = white_cnt;
	// Выставляем статус игры
	if (status != StatusWin && status != StatusLose && status != StatusError) {
		status = StatusWaitingOpponent;
		if (!my_load) {
			if ((myElement_ == GameElement::TypeBlack && delta == 0) || (myElement_ == GameElement::TypeWhite && delta == 1)) {
				status = StatusThinking;
			}
		} else {
			status = StatusWaitingAccept;
			loadGameFlag = true; // Для специальной обработки после получения акцепта
		}
	}
	setGameStatus(status);
	// --
	emit layoutChanged();
	return true;
}

/**
 * Устанавливает новые координаты подсвечиваемого объекта и дает команды на перисовку
 */
void BoardModel::setSelect(int x, int y)
{
	int old_x = selectX;
	int old_y = selectY;
	selectX = x + 2;
	selectY = y + 2;
	if (old_x != selectX || old_y != selectY) {
		if (old_x != -1 && old_y != -1) {
			QModelIndex idx = index(old_y, old_x);
			emit dataChanged(idx, idx);
		}
		QModelIndex idx = index(selectY, selectX);
		emit dataChanged(idx, idx);
	}
}
