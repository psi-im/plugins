/*
 * gamemodel.cpp - Gomoku Game plugin
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include <QStringList>
#include <QCryptographicHash>

#include "gamemodel.h"

GameModel::GameModel(GameElement::ElementType my, int row_count, int col_count, QObject *parent) :
    QObject(parent),
    valid_(true),
    status_(StatusNone),
    accepted_(true),
    turnsCount_(0),
    blackCount_(0),
    whiteCount_(0),
    my_el(my),
    switchColor(false),
    boardSizeX_(col_count),
    boardSizeY_(row_count),
    loadedTurnsCount(0),
    chksum(ChksumNone)
{
    if (my_el == GameElement::TypeNone || col_count <= 0 || row_count <= 0)
        valid_ = false;
    selectGameStatus();
    emit statusUpdated(status_);
}

GameModel::GameModel(const QString &load_str, const bool local, QObject *parent) :
    QObject(parent),
    valid_(false),
    status_(StatusNone),
    accepted_(!local),
    turnsCount_(0),
    blackCount_(0),
    whiteCount_(0),
    my_el(GameElement::TypeNone),
    switchColor(false),
    boardSizeX_(0),
    boardSizeY_(0),
    loadedTurnsCount(0),
    chksum(ChksumNone)
{
    QStringList loadList = load_str.split(";");
    if (loadList.isEmpty() || loadList.takeFirst() != "gomokugameplugin.save.1")
        return;
    bool res = true;
    int black_cnt = 0;
    int white_cnt = 0;
    int maxCol = 0;
    int maxRow = 0;
    while (!loadList.isEmpty()) {
        QString str1 = loadList.takeFirst().trimmed();
        if (str1.isEmpty())
            continue;
        QStringList setStrList = str1.split(":", QString::SkipEmptyParts);
        if (setStrList.size() != 2) {
            res = false;
            break;
        }
        QString parName = setStrList.at(0).trimmed().toLower();
        if (parName == "element") {
            QStringList elemPar = setStrList.at(1).trimmed().split(",");
            if (elemPar.size() != 3) {
                res = false;
                break;
            }
            bool fOk;
            int x = elemPar.at(0).toInt(&fOk);
            if (!fOk || x < 0) {
                res = false;
                break;
            }
            if (x > maxCol)
                maxCol = x;
            int y = elemPar.at(1).toInt(&fOk);
            if (!fOk || y < 0) {
                res = false;
                break;
            }
            if (y > maxRow)
                maxRow = y;
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
            elementsList.push_back(el);
        } else if (parName == "color") {
            if (setStrList.at(1) == "black") {
                my_el = GameElement::TypeBlack;
            } else if (setStrList.at(1) == "white") {
                my_el = GameElement::TypeWhite;
            }
        } else if (parName == "status") {
            if (setStrList.at(1) == "error") {
                status_ = StatusError;
            } else if (setStrList.at(1) == "win") {
                status_ = (local) ? StatusWin : StatusLose;
            } else if (setStrList.at(1) == "lose") {
                status_ = (local) ? StatusLose : StatusWin;
            } else if (setStrList.at(1) == "draw") {
                status_ = StatusDraw;
            }
        } else if (parName == "switchcolor") {
            if (setStrList.at(1) == "yes")
                switchColor = true;
        }
    }
    boardSizeX_ = 15;
    boardSizeY_ = 15;
    int delta = black_cnt - white_cnt;
    if (!res || delta < 0 || delta > 1 || my_el == GameElement::TypeNone || maxRow >= boardSizeY_ || maxCol >= boardSizeX_) {
        // Неудачная загрузка, удаляем созданные объекты
        while (!elementsList.isEmpty())
            delete elementsList.takeFirst();
        return;
    }
    if (!local) {
        if (my_el == GameElement::TypeBlack)
            my_el = GameElement::TypeWhite;
        else
            my_el = GameElement::TypeBlack;
    }
    blackCount_ = black_cnt;
    whiteCount_ = white_cnt;
    turnsCount_ = black_cnt + white_cnt;
    if (switchColor)
        turnsCount_++;
    loadedTurnsCount = turnsCount_;
    const int pos = load_str.indexOf("sha1sum:", 0, Qt::CaseInsensitive);
    if (pos != -1) {
        const QString sum_str = QCryptographicHash::hash(load_str.left(pos).toLatin1().data(), QCryptographicHash::Sha1).toHex().constData();
        if (sum_str == load_str.mid(pos + 8, 40))
            chksum = ChksumCorrect;
        else
            chksum = ChksumIncorrect;
    }
    valid_ = true;
    if (status_ == StatusNone)
        selectGameStatus();
    emit statusUpdated(gameStatus());
    return;
}

GameModel::~GameModel()
{
    while (!elementsList.isEmpty())
        delete elementsList.takeFirst();
}

GameModel::GameStatus GameModel::gameStatus() const
{
    if (!accepted_)
        return StatusWaitingAccept;
    return status_;
}

bool GameModel::selectGameStatus()
{
    if (status_ == StatusError || status_ == StatusBreak || status_ == StatusWin || status_ == StatusLose || status_ == StatusDraw)
        return false; // Эти статусы автоматически не меняются.
    GameStatus new_status;
    if (!accepted_) {
        new_status = StatusWaitingAccept;
    } else if (turnsCount_ == 0) {
        new_status = (my_el == GameElement::TypeBlack) ? StatusWaitingLocalAction : StatusWaitingOpponent;
    } else {
        bool my_last_turn = (elementsList.last()->type() == my_el);
        //if (turnsCount_ == 4 && switchColor)
        //    my_last_turn = !my_last_turn;
        new_status = (my_last_turn) ? StatusWaitingOpponent : StatusWaitingLocalAction;
    }
    if (status_ != new_status) {
        status_ = new_status;
        return true;
    }
    return false;
}

const GameElement *GameModel::getElement(int x, int y) const
{
    const int idx =  getElementIndex(x, y);
    if (idx == -1)
        return NULL;
    return elementsList.at(idx);

}

int GameModel::getElementIndex(int x, int y) const
{
    if (x >= 0 && x < boardSizeX_ && y >= 0 && y < boardSizeY_) {
        for (int i = 0, cnt = elementsList.size(); i < cnt; i++) {
            const GameElement *el = elementsList.at(i);
            if (el->x() == x && el->y() == y)
                return i;
        }
    }
    return -1;

}

/**
 * Добавление нового хода в модель
 */
bool GameModel::doTurn(int x, int y, bool local)
{
    lastErrorStr = QString();
    // Проверяем возможность хода
    if (!accepted_)
        return false; // Нет подтверждения
    if (local) {
        if (status_ != GameModel::StatusWaitingLocalAction)
            return false; // Не наш ход
    } else {
        if (status_ != GameModel::StatusWaitingOpponent)
            return false;
    }
    if (x < 0 || x >= boardSizeX_ || y < 0 || y >= boardSizeY_)
        return false;
    if (turnsCount_ == 0 && (x != 7 || y != 7)) {
        lastErrorStr = tr("The first turn can be only H8.");
        return false;
    }
    if (getElementIndex(x, y) != -1)
        return false; // Место уже занято
    // Определяем цвет элемента
    GameElement::ElementType type;
    if (local)
        type = my_el;
    else
        type = (my_el == GameElement::TypeBlack) ? GameElement::TypeWhite : GameElement::TypeBlack;
    // Create new element
    GameElement *el = new GameElement(type, x, y);
    if (!el)
        return false;
    // Insert new element into game model
    elementsList.push_back(el);
    if (type == GameElement::TypeBlack)
        blackCount_++;
    else
        whiteCount_++;
    turnsCount_++;
    if (local) {
        accepted_ = false;
    } else {
        if (checkForLose()) {
            status_ = StatusLose;
            emit statusUpdated(status_);
        } else if (checkForDraw()) {
            status_ = StatusDraw;
            emit statusUpdated(status_);
        }
    }
    if (selectGameStatus())
        emit statusUpdated(status_);
    return true;
}

/**
 * Переключение цвета камней на 4м ходу
 */
bool GameModel::doSwitchColor(bool local)
{
    lastErrorStr = QString();
    // Проверяем возможность хода
    if (!accepted_)
        return false; // Нет подтверждения
    if (local) {
        if (status_ != GameModel::StatusWaitingLocalAction)
            return false; // Не наш ход
    } else {
        if (status_ != GameModel::StatusWaitingOpponent)
            return false;
    }
    if (turnsCount_ != 3)
        return false;
    // Переключаем
    my_el = (my_el == GameElement::TypeBlack) ? GameElement::TypeWhite : GameElement::TypeBlack;
    switchColor = true;
    accepted_ = !local;
    turnsCount_++;
    if (selectGameStatus())
        emit statusUpdated(status_);
    return true;
}

bool GameModel::accept()
{
    if (!accepted_) {
        accepted_ = true;
        selectGameStatus();
        emit statusUpdated(status_);
        return true;
    }
    return false;
}

void GameModel::setErrorStatus()
{
    if (status_ != StatusError) {
        status_ = StatusError;
        accepted_ = true;
        emit statusUpdated(status_);
    }
}

void GameModel::breakGame()
{
    if (status_ == StatusWaitingAccept || status_ == StatusWaitingLocalAction || status_ == StatusWaitingOpponent) {
        status_ = StatusBreak;
        accepted_ = true;
        emit statusUpdated(status_);
    }
}

void GameModel::setWin()
{
    if (status_ == StatusWaitingAccept || status_ == StatusWaitingLocalAction || status_ == StatusWaitingOpponent) {
        status_ = StatusWin;
        accepted_ = true;
        emit statusUpdated(status_);
    }
}

void GameModel::setDraw()
{
    if (status_ == StatusWaitingAccept || status_ == StatusWaitingLocalAction || status_ == StatusWaitingOpponent) {
        status_ = StatusDraw;
        accepted_ = true;
        emit statusUpdated(status_);
    }
}

void GameModel::setLose()
{
    if (status_ == StatusWaitingAccept || status_ == StatusWaitingLocalAction || status_ == StatusWaitingOpponent) {
        status_ = StatusLose;
        accepted_ = true;
        emit statusUpdated(status_);
    }
}

bool GameModel::checkForLose()
{
    int max_x = boardSizeX_ - 1;
    int max_y = boardSizeY_ - 1;
    int last_x = lastX();
    int last_y = lastY();
    if (last_x < 0 || last_x >= max_x || last_y < 0 || last_y >= max_y)
        return false;
    if (turnsCount_ == 4 && switchColor)
        return false; // Переключение цвета, незачем проверять
    // Проверяем вверх
    int vert = 1;
    if (last_y > 0) {
        int tmp_y = last_y - 1;
        for (; tmp_y >= 0; tmp_y--) {
            const GameElement *el = getElement(last_x, tmp_y);
            if (!el || el->type() == my_el)
                break;
        }
        vert = last_y - tmp_y;
        if (vert > 5)
            return false;
    }
    // Проверяем вниз
    if (last_y < max_y) {
        int tmp_y = last_y + 1;
        for (; tmp_y <= max_y; tmp_y++) {
            const GameElement *el = getElement(last_x, tmp_y);
            if (!el || el->type() == my_el)
                break;
        }
        vert += tmp_y - last_y - 1;
        if (vert > 5)
            return false;
    }
    // Слева
    int horiz = 1;
    if (last_x > 0) {
        int tmp_x = last_x - 1;
        for (; tmp_x >= 0; tmp_x--) {
            const GameElement *el = getElement(tmp_x, last_y);
            if (!el || el->type() == my_el)
                break;
        }
        horiz = last_x - tmp_x;
        if (horiz > 5)
            return false;
    }
    // Справа
    if (last_x < max_x) {
        int tmp_x = last_x + 1;
        for (; tmp_x <= max_x; tmp_x++) {
            const GameElement *el = getElement(tmp_x, last_y);
            if (!el || el->type() == my_el)
                break;
        }
        horiz += tmp_x - last_x - 1;
        if (horiz > 5)
            return false;
    }
    // Лево вверх
    int diag1 = 1;
    if (last_x > 0 && last_y > 0) {
        int tmp_y = last_y - 1;
        for (int tmp_x = last_x - 1; tmp_x >= 0; tmp_x--) {
            const GameElement *el = getElement(tmp_x, tmp_y);
            if (!el || el->type() == my_el)
                break;
            tmp_y--;
            if (tmp_y < 0)
                break;
        }
        diag1 = last_y - tmp_y;
        if (diag1 > 5)
            return false;
    }
    // Право вниз
    if (last_x < max_x && last_y < max_y) {
        int tmp_y = last_y + 1;
        for (int tmp_x = last_x + 1; tmp_x <= max_x; tmp_x++) {
            const GameElement *el = getElement(tmp_x, tmp_y);
            if (!el || el->type() == my_el)
                break;
            tmp_y++;
            if (tmp_y > max_y)
                break;
        }
        diag1 += tmp_y - last_y - 1;
        if (diag1 > 5)
            return false;
    }
    // Право верх
    int diag2 = 1;
    if (last_x < max_x && last_y > 0) {
        int tmp_y = last_y - 1;
        for (int tmp_x = last_x + 1; tmp_x <= max_x; tmp_x++) {
            const GameElement *el = getElement(tmp_x, tmp_y);
            if (!el || el->type() == my_el)
                break;
            tmp_y--;
            if (tmp_y < 0)
                break;
        }
        diag2 = last_y - tmp_y;
        if (diag2 > 5)
            return false;
    }
    // Лево низ
    if (last_x > 0 && last_y < max_y) {
        int tmp_y = last_y + 1;
        for (int tmp_x = last_x - 1; tmp_x >= 0; tmp_x--) {
            const GameElement *el = getElement(tmp_x, tmp_y);
            if (!el || el->type() == my_el)
                break;
            tmp_y++;
            if (tmp_y > max_y)
                break;
        }
        diag2 += tmp_y - last_y - 1;
        if (diag2 > 5)
            return false;
    }
    if (vert == 5 || horiz == 5 || diag1 == 5 || diag2 == 5)
        return true;
    return false;
}

bool GameModel::checkForDraw()
{
    // Если доска заполнена
    return (turnsCount_ == (boardSizeX_ * boardSizeY_));
}

QString GameModel::toString() const
{
    QString res_str = "gomokugameplugin.save.1;\n";
    GameElement *lastEl = NULL;
    if (!elementsList.isEmpty())
        lastEl = elementsList.last();
    foreach (GameElement *el, elementsList) {
        if (el == lastEl && !accepted_)
            continue; // Не сохраняем не подтвержденное
        res_str.append(QString("Element:%1,%2,%3;\n")
            .arg(el->x()).arg(el->y())
            .arg((el->type() == GameElement::TypeBlack) ? "black" : "white"));
    }
    res_str.append(QString("SwitchColor:%1;\n").arg((switchColor) ? "yes" : "no"));
    res_str.append(QString("Color:%1;\n").arg((my_el == GameElement::TypeBlack) ? "black" : "white"));
    res_str.append(QString("Status:%1;\n").arg(statusString()));
    QString tmp_str = res_str;
    QString crcStr = QCryptographicHash::hash(tmp_str.replace("\n", "").toLatin1().data(), QCryptographicHash::Sha1).toHex().constData();
    res_str.append(QString("Sha1Sum:%1;\n").arg(crcStr));

    return res_str;
}

int GameModel::lastX() const
{
    if (elementsList.isEmpty())
        return -1;
    return elementsList.last()->x();
}

int GameModel::lastY() const
{
    if (elementsList.isEmpty())
        return -1;
    return elementsList.last()->y();
}

/**
 * Возвращает статус игры в виде строки
 */
QString GameModel::statusString() const
{
    QString stat_str;
    if (status_ == StatusError) {
        stat_str = "error";
    } else if (status_ == StatusWin) {
        stat_str = "win";
    } else if (status_ == StatusLose) {
        stat_str = "lose";
    } else if (status_ == StatusDraw) {
        stat_str = "draw";
    } else {
        stat_str = "play";
    }
    return stat_str;
}

bool GameModel::isLoaded() const
{
    return (loadedTurnsCount > 0 && loadedTurnsCount == turnsCount_);
}

QString GameModel::gameInfo() const
{
    QString text = "Game info:\n";
    text.append(QString("Black stones: %1\n").arg(blackCount_));
    text.append(QString("White stones: %1\n").arg(whiteCount_));
    text.append(QString("Your color: %1\n").arg((my_el == GameElement::TypeBlack) ? "black" : "white"));
    text.append(QString("SwitchColor: %1\n").arg((switchColor) ? "yes" : "no"));
    text.append(QString("Game status: %1").arg(statusString()));
    if (isLoaded()) {
        QString chksumStr;
        if (chksum == ChksumNone) {
            chksumStr = "none";
        } else if (chksum == ChksumCorrect) {
            chksumStr = "correct";
        } else if (chksum == ChksumIncorrect) {
            chksumStr = "!!! incorrect !!!";
        }
        text.append(QString("\nCheck sum: %1").arg(chksumStr));
    }
    return text;
}

/**
 * Информация о ходе. Информация возвращается в структуре
 */
GameModel::TurnInfo GameModel::turnInfo(int num) const
{
    TurnInfo res;
    res.x = 0;
    res.y = 0;
    res.my = GameElement::TypeNone;
    if (num > 0 && num <= turnsCount_) {
        int i = num - 1;
        bool myInvert = false;
        if (switchColor) {
            if (num >= 4) {
                i--;
                if (num == 4) {
                     // Этот ход - переключение цвета
                    res.x = -1;
                    res.y = -1;
                    res.my = (elementsList.at(i)->type() == my_el);
                    return res;
                }
            } else {
                myInvert = true;
            }
        }
        const GameElement *el = elementsList.at(i);
        res.x = el->x();
        res.y = el->y();
        res.my = (elementsList.at(i)->type() == my_el);
        if (myInvert)
            res.my = !res.my;
    }
    return res;
}
