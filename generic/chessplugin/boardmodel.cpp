/*
 * boardmodel.cpp - plugin
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "boardmodel.h"

using namespace Chess;

BoardModel::BoardModel(Figure::GameType type, QObject *parent) :
    QAbstractTableModel(parent), myMove(false), waitForFigure(false), check(false), gameType_(Figure::NoGame),
    gameState_(-1)
{
    setGameType(type);
    setHeaders();
}

void BoardModel::setHeaders()
{
    vHeader.clear();
    hHeader.clear();
    if (gameType_ == Figure::WhitePlayer) {
        vHeader << "8"
                << "7"
                << "6"
                << "5"
                << "4"
                << "3"
                << "2"
                << "1";
        hHeader << "A"
                << "B"
                << "C"
                << "D"
                << "E"
                << "F"
                << "G"
                << "H";
    } else {
        vHeader << "1"
                << "2"
                << "3"
                << "4"
                << "5"
                << "6"
                << "7"
                << "8";
        hHeader << "H"
                << "G"
                << "F"
                << "E"
                << "D"
                << "C"
                << "B"
                << "A";
    }
}

QVariant BoardModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            return hHeader.at(section);
        } else {
            return vHeader.at(section);
        }
    } else
        return QVariant();
}

void BoardModel::setGameType(Figure::GameType type)
{
    gameType_ = type;
    if (type == Figure::WhitePlayer) {
        myMove = true;
    } else {
        myMove = false;
    }
}

Qt::ItemFlags BoardModel::flags(const QModelIndex & /*index*/) const
{
    Qt::ItemFlags flags = Qt::NoItemFlags;
    flags |= Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    return flags;
}

int BoardModel::columnCount(const QModelIndex & /*parent*/) const { return hHeader.size(); }

int BoardModel::rowCount(const QModelIndex & /* parent*/) const { return vHeader.size(); }

QVariant BoardModel::data(const QModelIndex &i, int role) const
{
    QModelIndex index = i;
    if (!index.isValid())
        return QVariant();

    if (gameType_ == Figure::BlackPlayer)
        index = invert(index);

    if (role == Qt::BackgroundRole) {
        if (index == kingIndex() && isCheck())
            return QVariant(QColor(0x9a, 0, 0));
        int i = index.column() + index.row();
        if (i & 1) {
            switch (gameState_) {
            case 1:
                return QVariant(QColor("green"));
            case 2:
                return QVariant(QColor(0xb4, 0xcc, 0xff));
            case 3:
                return QVariant(QColor(0x9a, 0, 0));
            default:
                return QVariant(QColor(0x74, 0x44, 0x0e));
            }
        } else
            return QVariant(QColor(0xff, 0xed, 0xc2));
    } else if (role == Qt::DisplayRole) {
        int x = index.column();
        int y = index.row();
        for (Figure *figure : whiteFigures_) {
            if (x == figure->positionX() && y == figure->positionY())
                return figure->getPixmap();
        }
        for (Figure *figure : blackFigures_) {
            if (x == figure->positionX() && y == figure->positionY())
                return figure->getPixmap();
        }
        return QVariant();
    }

    return QVariant();
}

void BoardModel::reset()
{
    gameState_ = 0;
    qDeleteAll(whiteFigures_);
    whiteFigures_.clear();
    qDeleteAll(blackFigures_);
    blackFigures_.clear();

    for (int i = 0; i < 8; i++) {
        Figure *whitePawn = new Figure(Figure::WhitePlayer, Figure::White_Pawn, i, 6, this);
        whiteFigures_.append(whitePawn);
    }
    Figure *whiteKing    = new Figure(Figure::WhitePlayer, Figure::White_King, 4, 7, this);
    Figure *whiteQueen   = new Figure(Figure::WhitePlayer, Figure::White_Queen, 3, 7, this);
    Figure *whiteBishop  = new Figure(Figure::WhitePlayer, Figure::White_Bishop, 2, 7, this);
    Figure *whiteBishop2 = new Figure(Figure::WhitePlayer, Figure::White_Bishop, 5, 7, this);
    Figure *whiteKnight  = new Figure(Figure::WhitePlayer, Figure::White_Knight, 1, 7, this);
    Figure *whiteKnight2 = new Figure(Figure::WhitePlayer, Figure::White_Knight, 6, 7, this);
    Figure *whiteCastle  = new Figure(Figure::WhitePlayer, Figure::White_Castle, 0, 7, this);
    Figure *whiteCastle2 = new Figure(Figure::WhitePlayer, Figure::White_Castle, 7, 7, this);
    whiteFigures_.append(whiteKing);
    whiteFigures_.append(whiteQueen);
    whiteFigures_.append(whiteBishop);
    whiteFigures_.append(whiteBishop2);
    whiteFigures_.append(whiteKnight);
    whiteFigures_.append(whiteKnight2);
    whiteFigures_.append(whiteCastle);
    whiteFigures_.append(whiteCastle2);

    for (int i = 0; i < 8; i++) {
        Figure *blackPawn = new Figure(Figure::BlackPlayer, Figure::Black_Pawn, i, 1, this);
        blackFigures_.append(blackPawn);
    }
    Figure *blackKing    = new Figure(Figure::BlackPlayer, Figure::Black_King, 4, 0, this);
    Figure *blackQueen   = new Figure(Figure::BlackPlayer, Figure::Black_Queen, 3, 0, this);
    Figure *blackBishop  = new Figure(Figure::BlackPlayer, Figure::Black_Bishop, 2, 0, this);
    Figure *blackBishop2 = new Figure(Figure::BlackPlayer, Figure::Black_Bishop, 5, 0, this);
    Figure *blackKnight  = new Figure(Figure::BlackPlayer, Figure::Black_Knight, 1, 0, this);
    Figure *blackKnight2 = new Figure(Figure::BlackPlayer, Figure::Black_Knight, 6, 0, this);
    Figure *blackCastle  = new Figure(Figure::BlackPlayer, Figure::Black_Castle, 0, 0, this);
    Figure *blackCastle2 = new Figure(Figure::BlackPlayer, Figure::Black_Castle, 7, 0, this);
    blackFigures_.append(blackKing);
    blackFigures_.append(blackQueen);
    blackFigures_.append(blackBishop);
    blackFigures_.append(blackBishop2);
    blackFigures_.append(blackKnight);
    blackFigures_.append(blackKnight2);
    blackFigures_.append(blackCastle);
    blackFigures_.append(blackCastle2);

    beginResetModel();
    endResetModel();
}

bool BoardModel::moveRequested(QModelIndex oldIndex, QModelIndex newIndex)
{
    if (!oldIndex.isValid() || !newIndex.isValid())
        return false;

    check          = isCheck();
    Figure *figure = findFigure(oldIndex);
    if (figure) {
        if (figure->gameType() != gameType_ && myMove)
            return false;

        int x     = newIndex.column();
        int y     = newIndex.row();
        int move_ = canMove(figure, x, y);
        if (!move_)
            return false;
        Figure *newFigure = nullptr;
        if (move_ == 2) { // kill figure
            newFigure = findFigure(newIndex);
            if (newFigure) {
                int tmpX = newFigure->positionX();
                int tmpY = newFigure->positionY();
                newFigure->setPosition(-1, -1);
                figure->setPosition(x, y);
                if (isCheck()) {
                    figure->setPosition(oldIndex.column(), oldIndex.row());
                    newFigure->setPosition(tmpX, tmpY);
                    return false;
                }
                emit figureKilled(newFigure);
            }
        } else if (move_ == 3) { // kill with pawn
            int tmpX = lastMove.figure->positionX();
            int tmpY = lastMove.figure->positionY();
            lastMove.figure->setPosition(-1, -1);
            figure->setPosition(x, y);
            if (isCheck()) {
                figure->setPosition(oldIndex.column(), oldIndex.row());
                lastMove.figure->setPosition(tmpX, tmpY);
                return false;
            }
            emit figureKilled(lastMove.figure);
        } else if (move_ == 4) { // roc
            figure->setPosition(x, y);
            if (isCheck()) {
                figure->setPosition(oldIndex.column(), oldIndex.row());
                return false;
            }
            if (x == 6) {
                QModelIndex tmpIndex = createIndex(y, 7);
                newFigure            = findFigure(tmpIndex);
                newFigure->setPosition(5, y);
            } else if (x == 2) {
                QModelIndex tmpIndex = createIndex(y, 0);
                newFigure            = findFigure(tmpIndex);
                newFigure->setPosition(3, y);
            }
        } else { // move
            figure->setPosition(x, y);
            if (isCheck()) {
                figure->setPosition(oldIndex.column(), oldIndex.row());
                return false;
            }
        }

        figure->isMoved       = true;
        lastMove.oldIndex     = oldIndex;
        lastMove.newIndex     = newIndex;
        lastMove.figure       = figure;
        lastMove.killedFigure = newFigure;

        emit layoutChanged();

        if ((figure->type() == Figure::White_Pawn && y == 0) || (figure->type() == Figure::Black_Pawn && y == 7)) {
            if (myMove)
                emit needNewFigure(newIndex, figure->type() == Figure::White_Pawn ? "white" : "black");
            waitForFigure = true;
            tempIndex_    = oldIndex;
            return true;
        } else if (myMove)
            emit move(oldIndex.column(), 7 - oldIndex.row(), newIndex.column(), 7 - newIndex.row(),
                      ""); // 7- - for compatibility with tkabber

        moveTransfer();

        return true;
    }
    return false;
}

bool BoardModel::moveRequested(int oldX, int oldY, int newX, int newY)
{
    return moveRequested(createIndex(7 - oldY, oldX),
                         createIndex(7 - newY, newX)); // 7- - for compatibility with tkabber
}

bool BoardModel::isYourFigure(const QModelIndex &index) const
{
    Figure *figure = findFigure(index);
    if (!figure)
        return false;

    return gameType_ == figure->gameType();
}

Figure *BoardModel::findFigure(QModelIndex index) const
{
    Figure *figure_ = nullptr;
    int     x       = index.column();
    int     y       = index.row();
    for (Figure *figure : whiteFigures_) {
        if (x == figure->positionX() && y == figure->positionY()) {
            figure_ = figure;
            break;
        }
    }
    if (!figure_) {
        for (Figure *figure : blackFigures_) {
            if (x == figure->positionX() && y == figure->positionY()) {
                figure_ = figure;
                break;
            }
        }
    }
    return figure_;
}

QModelIndex BoardModel::kingIndex() const
{
    if (gameType_ == Figure::WhitePlayer)
        return findFigure(Figure::White_King, gameType_);
    return findFigure(Figure::Black_King, gameType_);
}

QModelIndex BoardModel::findFigure(Figure::FigureType type, Figure::GameType game) const
{
    QModelIndex index;
    if (game == Figure::WhitePlayer) {
        for (Figure *figure : whiteFigures_) {
            if (type == figure->type()) {
                index = createIndex(figure->positionY(), figure->positionX());
            }
        }
    } else {
        for (Figure *figure : blackFigures_) {
            if (type == figure->type()) {
                index = createIndex(figure->positionY(), figure->positionX());
            }
        }
    }

    return index;
}

// return 0 - can't move;
// return 1 - can move;
// return 2 - kill someone;
// return 3 - strange pawn kill :)
// return 4 - roc
int BoardModel::canMove(Figure *figure, int newX, int newY) const
{
    int  positionX_ = figure->positionX();
    int  positionY_ = figure->positionY();
    bool isMoved    = figure->isMoved;
    switch (figure->type()) {
    case Figure::White_Pawn:
        if (qAbs(newX - positionX_) > 1)
            return 0;

        if ((positionY_ - newY) == 2 && !isMoved && newX == positionX_) {
            QModelIndex index = createIndex(positionY_ - 1, positionX_);
            if (findFigure(index))
                return 0;
            index = createIndex(newY, newX);
            if (findFigure(index))
                return 0;
            return 1;
        }

        if (positionY_ - newY == 1) {
            QModelIndex index = createIndex(newY, newX);
            Figure *    f     = findFigure(index);
            if (newX == positionX_) {
                if (!f)
                    return 1;
                else
                    return 0;
            }
            if (f && f->gameType() == Figure::BlackPlayer)
                return 2;
            if (lastMove.figure->type() == Figure::Black_Pawn
                && qAbs(lastMove.oldIndex.row() - lastMove.newIndex.row()) == 2 && lastMove.oldIndex.row() + 1 == newY
                && lastMove.oldIndex.column() == newX)
                return 3;
            return 0;
        }
        return 0;
    case Figure::Black_Pawn:
        if (qAbs(newX - positionX_) > 1)
            return 0;

        if ((positionY_ - newY) == -2 && !isMoved && newX == positionX_) {
            QModelIndex index = createIndex(positionY_ + 1, positionX_);
            if (findFigure(index))
                return 0;
            index = createIndex(newY, newX);
            if (findFigure(index))
                return 0;
            return 1;
        }

        if (positionY_ - newY == -1) {
            QModelIndex index = createIndex(newY, newX);
            Figure *    f     = findFigure(index);
            if (newX == positionX_) {
                if (!f)
                    return 1;
                else
                    return 0;
            }
            if (f && f->gameType() == Figure::WhitePlayer)
                return 2;
            if (lastMove.figure->type() == Figure::White_Pawn
                && qAbs(lastMove.oldIndex.row() - lastMove.newIndex.row()) == 2 && lastMove.oldIndex.row() - 1 == newY
                && lastMove.oldIndex.column() == newX)
                return 3;
            return 0;
        }
        return 0;
    case Figure::White_Castle:
        if ((newX == positionX_ && newY != positionY_) || (newX != positionX_ && newY == positionY_)) {
            if (positionX_ - newX > 0) {
                for (int i = positionX_ - newX - 1; i > 0; --i) {
                    QModelIndex index = createIndex(positionY_, positionX_ - i);
                    if (findFigure(index))
                        return 0;
                }
            } else if (newX - positionX_ > 0) {
                for (int i = newX - positionX_ - 1; i > 0; --i) {
                    QModelIndex index = createIndex(positionY_, positionX_ + i);
                    if (findFigure(index))
                        return 0;
                }
            } else if (positionY_ - newY > 0) {
                for (int i = positionY_ - newY - 1; i > 0; --i) {
                    QModelIndex index = createIndex(positionY_ - i, positionX_);
                    if (findFigure(index))
                        return 0;
                }
            } else if (newY - positionY_ > 0) {
                for (int i = newY - positionY_ - 1; i > 0; --i) {
                    QModelIndex index = createIndex(positionY_ + i, positionX_);
                    if (findFigure(index))
                        return 0;
                }
            }
            Figure *figure_ = findFigure(createIndex(newY, newX));
            if (!figure_)
                return 1;
            else
                return 2;
        }
        return 0;
    case Figure::Black_Castle:
        if ((newX == positionX_ && newY != positionY_) || (newX != positionX_ && newY == positionY_)) {
            if (positionX_ - newX > 0) {
                for (int i = positionX_ - newX - 1; i > 0; --i) {
                    QModelIndex index = createIndex(positionY_, positionX_ - i);
                    if (findFigure(index))
                        return 0;
                }
            } else if (newX - positionX_ > 0) {
                for (int i = newX - positionX_ - 1; i > 0; --i) {
                    QModelIndex index = createIndex(positionY_, positionX_ + i);
                    if (findFigure(index))
                        return 0;
                }
            } else if (positionY_ - newY > 0) {
                for (int i = positionY_ - newY - 1; i > 0; --i) {
                    QModelIndex index = createIndex(positionY_ - i, positionX_);
                    if (findFigure(index))
                        return 0;
                }
            } else if (newY - positionY_ > 0) {
                for (int i = newY - positionY_ - 1; i > 0; --i) {
                    QModelIndex index = createIndex(positionY_ + i, positionX_);
                    if (findFigure(index))
                        return 0;
                }
            }
            Figure *figure_ = findFigure(createIndex(newY, newX));
            if (!figure_)
                return 1;
            else
                return 2;
        }
        return 0;
    case Figure::White_King:
        if (qAbs(newX - positionX_) < 2 && qAbs(newY - positionY_) < 2) {
            Figure *figure_ = findFigure(createIndex(newY, newX));
            if (!figure_)
                return 1;
            else
                return 2;
        }
        if (check) {
            return 0;
        }
        if (!isMoved && newX == 6 && newY == 7) {
            Figure *figure_ = findFigure(createIndex(7, 7));
            if (!figure_) {
                return 0;
            }
            if (figure_->isMoved) {
                return 0;
            }
            figure_ = findFigure(createIndex(7, 5));
            if (figure_) {
                return 0;
            }
            figure_ = findFigure(createIndex(7, 6));
            if (figure_) {
                return 0;
            }
            figure->setPosition(5, 7);
            for (Figure *figure_ : blackFigures_) {
                if (figure_->positionX() != -1) {
                    if (canMove(figure_, 5, 7)) {
                        figure->setPosition(positionX_, positionY_);
                        return 0;
                    }
                }
            }
            return 4;
        }
        if (!isMoved && newX == 2 && newY == 7) {
            Figure *figure_ = findFigure(createIndex(7, 0));
            if (!figure_)
                return 0;
            if (figure_->isMoved)
                return 0;
            figure_ = findFigure(createIndex(7, 3));
            if (figure_)
                return 0;
            figure_ = findFigure(createIndex(7, 2));
            if (figure_)
                return 0;
            figure_ = findFigure(createIndex(7, 1));
            if (figure_)
                return 0;
            figure->setPosition(3, 7);
            for (Figure *figure_ : blackFigures_) {
                if (figure_->positionX() != -1) {
                    if (canMove(figure_, 3, 7)) {
                        figure->setPosition(positionX_, positionY_);
                        return 0;
                    }
                }
            }
            return 4;
        }
        return 0;
    case Figure::Black_King:
        if (qAbs(newX - positionX_) < 2 && qAbs(newY - positionY_) < 2) {
            Figure *figure_ = findFigure(createIndex(newY, newX));
            if (!figure_)
                return 1;
            else
                return 2;
        }
        if (check)
            return 0;
        if (!isMoved && newX == 6 && newY == 0) {
            Figure *figure_ = findFigure(createIndex(0, 7));
            if (!figure_)
                return 0;
            if (figure_->isMoved)
                return 0;
            figure_ = findFigure(createIndex(0, 5));
            if (figure_)
                return 0;
            figure_ = findFigure(createIndex(0, 6));
            if (figure_)
                return 0;
            figure->setPosition(5, 0);
            for (Figure *figure_ : whiteFigures_) {
                if (figure_->positionX() != -1) {
                    if (canMove(figure_, 5, 0)) {
                        figure->setPosition(positionX_, positionY_);
                        return 0;
                    }
                }
            }
            return 4;
        }
        if (!isMoved && newX == 2 && newY == 0) {
            Figure *figure_ = findFigure(createIndex(0, 0));
            if (!figure_)
                return 0;
            if (figure_->isMoved)
                return 0;
            figure_ = findFigure(createIndex(0, 3));
            if (figure_)
                return 0;
            figure_ = findFigure(createIndex(0, 2));
            if (figure_)
                return 0;
            figure_ = findFigure(createIndex(0, 1));
            if (figure_)
                return 0;
            figure->setPosition(3, 0);
            for (Figure *figure_ : whiteFigures_) {
                if (figure_->positionX() != -1) {
                    if (canMove(figure_, 3, 0)) {
                        figure->setPosition(positionX_, positionY_);
                        return 0;
                    }
                }
            }
            return 4;
        }
        return 0;
    case Figure::White_Bishop:
        if (qAbs(newX - positionX_) == qAbs(newY - positionY_)) {
            if (newX - positionX_ > 0) {
                if (newY - positionY_ > 0) {
                    for (int i = newX - positionX_ - 1; i > 0; --i) {
                        QModelIndex index = createIndex(positionY_ + i, positionX_ + i);
                        if (findFigure(index))
                            return 0;
                    }
                } else if (positionY_ - newY > 0) {
                    for (int i = newX - positionX_ - 1; i > 0; --i) {
                        QModelIndex index = createIndex(positionY_ - i, positionX_ + i);
                        if (findFigure(index))
                            return 0;
                    }
                }
            } else if (positionX_ - newX > 0) {
                if (newY - positionY_ > 0) {
                    for (int i = positionX_ - newX - 1; i > 0; --i) {
                        QModelIndex index = createIndex(positionY_ + i, positionX_ - i);
                        if (findFigure(index))
                            return 0;
                    }
                } else if (positionY_ - newY > 0) {
                    for (int i = positionX_ - newX - 1; i > 0; --i) {
                        QModelIndex index = createIndex(positionY_ - i, positionX_ - i);
                        if (findFigure(index))
                            return 0;
                    }
                }
            }
            Figure *figure_ = findFigure(createIndex(newY, newX));
            if (!figure_)
                return 1;
            else
                return 2;
        }
        return 0;
    case Figure::Black_Bishop:
        if (qAbs(newX - positionX_) == qAbs(newY - positionY_)) {
            if (newX - positionX_ > 0) {
                if (newY - positionY_ > 0) {
                    for (int i = newX - positionX_ - 1; i > 0; --i) {
                        QModelIndex index = createIndex(positionY_ + i, positionX_ + i);
                        if (findFigure(index))
                            return 0;
                    }
                } else if (positionY_ - newY > 0) {
                    for (int i = newX - positionX_ - 1; i > 0; --i) {
                        QModelIndex index = createIndex(positionY_ - i, positionX_ + i);
                        if (findFigure(index))
                            return 0;
                    }
                }
            } else if (positionX_ - newX > 0) {
                if (newY - positionY_ > 0) {
                    for (int i = positionX_ - newX - 1; i > 0; --i) {
                        QModelIndex index = createIndex(positionY_ + i, positionX_ - i);
                        if (findFigure(index))
                            return 0;
                    }
                } else if (positionY_ - newY > 0) {
                    for (int i = positionX_ - newX - 1; i > 0; --i) {
                        QModelIndex index = createIndex(positionY_ - i, positionX_ - i);
                        if (findFigure(index))
                            return 0;
                    }
                }
            }
            Figure *figure_ = findFigure(createIndex(newY, newX));
            if (!figure_)
                return 1;
            else
                return 2;
        }
        return 0;
    case Figure::White_Knight:
        if ((qAbs(newX - positionX_) == 2 && qAbs(newY - positionY_) == 1)
            || (qAbs(newX - positionX_) == 1 && qAbs(newY - positionY_) == 2)) {
            Figure *figure_ = findFigure(createIndex(newY, newX));
            if (!figure_)
                return 1;
            else
                return 2;
        }
        return 0;
    case Figure::Black_Knight:
        if ((qAbs(newX - positionX_) == 2 && qAbs(newY - positionY_) == 1)
            || (qAbs(newX - positionX_) == 1 && qAbs(newY - positionY_) == 2)) {
            Figure *figure_ = findFigure(createIndex(newY, newX));
            if (!figure_)
                return 1;
            else
                return 2;
        }
        return 0;
    case Figure::White_Queen:
        if ((newX == positionX_ && newY != positionY_) || (newX != positionX_ && newY == positionY_)) {
            if (positionX_ - newX > 0) {
                for (int i = positionX_ - newX - 1; i > 0; --i) {
                    QModelIndex index = createIndex(positionY_, positionX_ - i);
                    if (findFigure(index))
                        return 0;
                }
            } else if (newX - positionX_ > 0) {
                for (int i = newX - positionX_ - 1; i > 0; --i) {
                    QModelIndex index = createIndex(positionY_, positionX_ + i);
                    if (findFigure(index))
                        return 0;
                }
            } else if (positionY_ - newY > 0) {
                for (int i = positionY_ - newY - 1; i > 0; --i) {
                    QModelIndex index = createIndex(positionY_ - i, positionX_);
                    if (findFigure(index))
                        return 0;
                }
            } else if (newY - positionY_ > 0) {
                for (int i = newY - positionY_ - 1; i > 0; --i) {
                    QModelIndex index = createIndex(positionY_ + i, positionX_);
                    if (findFigure(index))
                        return 0;
                }
            }
            Figure *figure_ = findFigure(createIndex(newY, newX));
            if (!figure_)
                return 1;
            else
                return 2;
        }
        if (qAbs(newX - positionX_) == qAbs(newY - positionY_)) {
            if (newX - positionX_ > 0) {
                if (newY - positionY_ > 0) {
                    for (int i = newX - positionX_ - 1; i > 0; --i) {
                        QModelIndex index = createIndex(positionY_ + i, positionX_ + i);
                        if (findFigure(index))
                            return 0;
                    }
                } else if (positionY_ - newY > 0) {
                    for (int i = newX - positionX_ - 1; i > 0; --i) {
                        QModelIndex index = createIndex(positionY_ - i, positionX_ + i);
                        if (findFigure(index))
                            return 0;
                    }
                }
            } else if (positionX_ - newX > 0) {
                if (newY - positionY_ > 0) {
                    for (int i = positionX_ - newX - 1; i > 0; --i) {
                        QModelIndex index = createIndex(positionY_ + i, positionX_ - i);
                        if (findFigure(index))
                            return 0;
                    }
                } else if (positionY_ - newY > 0) {
                    for (int i = positionX_ - newX - 1; i > 0; --i) {
                        QModelIndex index = createIndex(positionY_ - i, positionX_ - i);
                        if (findFigure(index))
                            return 0;
                    }
                }
            }
            Figure *figure_ = findFigure(createIndex(newY, newX));
            if (!figure_)
                return 1;
            else
                return 2;
        }
        return 0;
    case Figure::Black_Queen:
        if ((newX == positionX_ && newY != positionY_) || (newX != positionX_ && newY == positionY_)) {
            if (positionX_ - newX > 0) {
                for (int i = positionX_ - newX - 1; i > 0; --i) {
                    QModelIndex index = createIndex(positionY_, positionX_ - i);
                    if (findFigure(index))
                        return 0;
                }
            } else if (newX - positionX_ > 0) {
                for (int i = newX - positionX_ - 1; i > 0; --i) {
                    QModelIndex index = createIndex(positionY_, positionX_ + i);
                    if (findFigure(index))
                        return 0;
                }
            } else if (positionY_ - newY > 0) {
                for (int i = positionY_ - newY - 1; i > 0; --i) {
                    QModelIndex index = createIndex(positionY_ - i, positionX_);
                    if (findFigure(index))
                        return 0;
                }
            } else if (newY - positionY_ > 0) {
                for (int i = newY - positionY_ - 1; i > 0; --i) {
                    QModelIndex index = createIndex(positionY_ + i, positionX_);
                    if (findFigure(index))
                        return 0;
                }
            }
            Figure *figure_ = findFigure(createIndex(newY, newX));
            if (!figure_)
                return 1;
            else
                return 2;
        }
        if (qAbs(newX - positionX_) == qAbs(newY - positionY_)) {
            if (newX - positionX_ > 0) {
                if (newY - positionY_ > 0) {
                    for (int i = newX - positionX_ - 1; i > 0; --i) {
                        QModelIndex index = createIndex(positionY_ + i, positionX_ + i);
                        if (findFigure(index))
                            return 0;
                    }
                } else if (positionY_ - newY > 0) {
                    for (int i = newX - positionX_ - 1; i > 0; --i) {
                        QModelIndex index = createIndex(positionY_ - i, positionX_ + i);
                        if (findFigure(index))
                            return 0;
                    }
                }
            } else if (positionX_ - newX > 0) {
                if (newY - positionY_ > 0) {
                    for (int i = positionX_ - newX - 1; i > 0; --i) {
                        QModelIndex index = createIndex(positionY_ + i, positionX_ - i);
                        if (findFigure(index))
                            return 0;
                    }
                } else if (positionY_ - newY > 0) {
                    for (int i = positionX_ - newX - 1; i > 0; --i) {
                        QModelIndex index = createIndex(positionY_ - i, positionX_ - i);
                        if (findFigure(index))
                            return 0;
                    }
                }
            }
            Figure *figure_ = findFigure(createIndex(newY, newX));
            if (!figure_)
                return 1;
            else
                return 2;
        }
        return 0;
    case Figure::None:
        return 0;
    }
    return false;
}

QMultiMap<QModelIndex, int> BoardModel::availableMoves(Figure *figure) const
{
    QMultiMap<QModelIndex, int> map;
    for (int x = 0; x <= 7; x++) {
        for (int y = 0; y <= 7; y++) {
            if (isYourFigure(index(y, x)))
                continue;
            int move = canMove(figure, x, y);
            if (move) {
                map.insert(index(y, x), move);
            }
        }
    }
    return map;
}

int BoardModel::checkGameState()
{
    int state = 0;
    check     = isCheck();
    if (gameType_ == Figure::WhitePlayer) {
        for (Figure *figure : qAsConst(whiteFigures_)) {
            if (figure->positionX() != -1) {
                QMultiMap<QModelIndex, int> moves = availableMoves(figure);
                if (!moves.isEmpty()) {
                    QList<QModelIndex> ml = moves.keys();
                    for (auto index : ml) {
                        if (doTestMove(figure, index, moves.value(index))) {
                            return state; // in progress
                        }
                    }
                }
            }
        }
    } else {
        for (Figure *figure : qAsConst(blackFigures_)) {
            if (figure->positionX() != -1) {
                QMultiMap<QModelIndex, int> moves = availableMoves(figure);
                if (!moves.isEmpty()) {
                    QList<QModelIndex> ml = moves.keys();
                    for (auto index : ml) {
                        if (doTestMove(figure, index, moves.value(index))) {
                            return state; // in progress
                        }
                    }
                }
            }
        }
    }

    state = isCheck() ? 2 : 1;
    return state;
}

bool BoardModel::doTestMove(Figure *figure, QModelIndex newIndex, int move)
{
    int     oldX = figure->positionX();
    int     oldY = figure->positionY();
    int     x    = newIndex.column();
    int     y    = newIndex.row();
    int     tmpX;
    int     tmpY;
    bool    ch;
    Figure *newFigure = nullptr;
    switch (move) {
    case 1:
        figure->setPosition(x, y);
        ch = isCheck();
        figure->setPosition(oldX, oldY);
        return ch ? false : true;
        break;
    case 2:
        newFigure = findFigure(newIndex);
        if (newFigure) {
            tmpX = newFigure->positionX();
            tmpY = newFigure->positionY();
            newFigure->setPosition(-1, -1);
            figure->setPosition(x, y);
            ch = isCheck();
            figure->setPosition(oldX, oldY);
            newFigure->setPosition(tmpX, tmpY);
            return ch ? false : true;
        }
        return false;
        break;
    case 3:
        tmpX = lastMove.figure->positionX();
        tmpY = lastMove.figure->positionY();
        lastMove.figure->setPosition(-1, -1);
        figure->setPosition(x, y);
        ch = isCheck();
        figure->setPosition(oldX, oldY);
        lastMove.figure->setPosition(tmpX, tmpY);
        return ch ? false : true;
        break;
    case 4:
        figure->setPosition(x, y);
        ch = isCheck();
        figure->setPosition(oldX, oldY);
        return ch ? false : true;
        break;
    }
    return false;
}

bool BoardModel::isCheck() const
{
    if (!myMove)
        return false;

    bool        check_ = false;
    QModelIndex king   = kingIndex();
    int         kingX  = king.column();
    int         kingY  = king.row();
    if (gameType_ == Figure::WhitePlayer) {
        for (Figure *figure : blackFigures_) {
            if (figure->positionX() != -1) {
                if (canMove(figure, kingX, kingY) == 2) {
                    check_ = true;
                    break;
                }
            }
        }
    } else if (gameType_ == Figure::BlackPlayer) {
        for (Figure *figure : whiteFigures_) {
            if (figure->positionX() != -1) {
                if (canMove(figure, kingX, kingY) == 2) {
                    check_ = true;
                    break;
                }
            }
        }
    }
    return check_;
}

QModelIndex BoardModel::invert(QModelIndex index) const { return createIndex(7 - index.row(), 7 - index.column()); }

void BoardModel::updateFigure(QModelIndex index, const QString &figure)
{
    Figure *oldFigure = findFigure(index);
    if ((gameType_ == Figure::WhitePlayer && myMove) || (gameType_ == Figure::BlackPlayer && !myMove)) {
        if (figure == "queen") {
            oldFigure->setType(Figure::White_Queen);
        } else if (figure == "rook") {
            oldFigure->setType(Figure::White_Castle);
        } else if (figure == "bishop") {
            oldFigure->setType(Figure::White_Bishop);
        } else if (figure == "knight") {
            oldFigure->setType(Figure::White_Knight);
        }
    } else {
        if (figure == "queen") {
            oldFigure->setType(Figure::Black_Queen);
        } else if (figure == "rook") {
            oldFigure->setType(Figure::Black_Castle);
        } else if (figure == "bishop") {
            oldFigure->setType(Figure::Black_Bishop);
        } else if (figure == "knight") {
            oldFigure->setType(Figure::Black_Knight);
        }
    }
    if (myMove)
        emit move(tempIndex_.column(), 7 - tempIndex_.row(), index.column(), 7 - index.row(),
                  figure); // 7- - for compatibility with tkabber

    moveTransfer();
    waitForFigure = false;

    emit layoutChanged();
}

void BoardModel::moveTransfer() { myMove = !myMove; }

static inline Figure::FigureType rankFigure(int i)
{
    switch (i) {
    case 1:
        return Figure::White_Pawn;
    case 2:
        return Figure::White_Castle;
    case 3:
        return Figure::White_Bishop;
    case 4:
        return Figure::White_King;
    case 5:
        return Figure::White_Queen;
    case 6:
        return Figure::White_Knight;
    case 7:
        return Figure::Black_Pawn;
    case 8:
        return Figure::Black_Castle;
    case 9:
        return Figure::Black_Bishop;
    case 10:
        return Figure::Black_King;
    case 11:
        return Figure::Black_Queen;
    case 12:
        return Figure::Black_Knight;
    default:
        return Figure::None;
    }
    return Figure::None;
}

QString BoardModel::saveString() const
{
    QString save;
    for (Figure *figure : whiteFigures_) {
        save += QString("%1,%2,%3,%4;")
                    .arg(QString::number(figure->type()), QString::number(figure->positionY()),
                         QString::number(figure->positionX()),
                         figure->isMoved ? QString::number(1) : QString::number(0));
    }
    for (Figure *figure : blackFigures_) {
        save += QString("%1,%2,%3,%4;")
                    .arg(QString::number(figure->type()), QString::number(figure->positionY()),
                         QString::number(figure->positionX()),
                         figure->isMoved ? QString::number(1) : QString::number(0));
    }
    save += myMove ? QString::number(1) : QString::number(0);
    save += ";" + QString::number(gameType_) + ";";
    return save;
}

void BoardModel::loadSettings(const QString &settings, bool myLoad)
{
    reset();

    QStringList figuresSettings = settings.split(";");
    for (Figure *figure : qAsConst(whiteFigures_)) {
        if (!figuresSettings.isEmpty()) {
            QStringList set = figuresSettings.takeFirst().split(",");
            figure->setType(rankFigure(set.takeFirst().toInt()));
            figure->setPosition(set.takeFirst().toInt(), set.takeFirst().toInt());
            figure->isMoved = set.takeFirst().toInt();
        }
    }
    for (Figure *figure : qAsConst(blackFigures_)) {
        if (!figuresSettings.isEmpty()) {
            QStringList set = figuresSettings.takeFirst().split(",");
            figure->setType(rankFigure(set.takeFirst().toInt()));
            figure->setPosition(set.takeFirst().toInt(), set.takeFirst().toInt());
            figure->isMoved = set.takeFirst().toInt();
        }
    }
    if (!figuresSettings.isEmpty())
        myMove = myLoad ? figuresSettings.takeFirst().toInt() : !figuresSettings.takeFirst().toInt();
    if (!figuresSettings.isEmpty()) {
        int i = figuresSettings.takeFirst().toInt();
        switch (i) {
        case 1:
            gameType_ = myLoad ? Figure::WhitePlayer : Figure::BlackPlayer;
            break;
        case 2:
            gameType_ = myLoad ? Figure::BlackPlayer : Figure::WhitePlayer;
            break;
        default:
            gameType_ = Figure::NoGame;
            break;
        }
        setHeaders();
    }

    emit layoutChanged();
}

void BoardModel::updateView() { emit layoutChanged(); }
