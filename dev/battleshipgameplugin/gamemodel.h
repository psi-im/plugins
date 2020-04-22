/*
 * gamemodel.h - Battleship game plugin
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef GAMEMODEL_H
#define GAMEMODEL_H

#include <QObject>
#include <QRect>
#include <QStringList>

class GameShip : public QObject {
    Q_OBJECT
public:
    enum ShipType { ShipDirUnknown, ShipHorizontal, ShipVertical };
    GameShip(int len, const QString &digest = QString(), QObject *parent = nullptr);
    int      length() const { return length_; }
    int      position() const { return firstPos_; }
    ShipType direction() const { return direction_; }
    QString  digest() const { return digest_; }
    bool     isDestroyed() const { return destroyed_; }
    void     setDirection(ShipType dir);
    void     setPosition(int pos);
    void     setDigest(const QString &digest);
    void     setDestroyed(bool destr);
    int      nextPosition(int prev);

private:
    int      length_;
    ShipType direction_;
    int      firstPos_;
    bool     destroyed_;
    QString  digest_;
};

class GameBoard : public QObject {
    Q_OBJECT
public:
    enum CellStatus { CellFree, CellOccupied, CellUnknown, CellMiss, CellHit, CellMargin };
    struct GameCell {
        CellStatus status;
        int        ship;
        QString    digest;
        QString    seed;
        GameCell(CellStatus s) : status(s), ship(-1) { }
    };
    GameBoard(QObject *parent = nullptr);
    void               init(CellStatus s, bool genseed);
    void               makeShipRandomPosition();
    void               calculateCellsHash();
    bool               updateCellDigest(int pos, const QString &digest);
    bool               updateCell(int pos, CellStatus cs, const QString &seed);
    bool               updateShipDigest(int length, const QString &digest);
    void               shot(int pos);
    GameShip::ShipType shipDirection(int pos);
    int                findAndInitShip(int pos);
    QRect              shipRect(int snum, bool margin) const;
    void               setShipDestroy(int n, bool margin);
    const GameCell &   cell(int pos) const;
    QStringList        toStringList(bool covered) const;
    bool               isAllDestroyed() const;

private:
    static QString genSeed(int len);
    GameShip *     findShip(int length, const QString &digest);
    bool           isShipPositionLegal(int shipNum);
    void           fillShipMargin(int n);

private:
    QList<GameCell>   cells_;
    QList<GameShip *> ships_;

signals:
    void shipDestroyed(int snum);
};

class GameModel : public QObject {
    Q_OBJECT
public:
    enum GameStatus {
        StatusNone,
        StatusError,
        StatusBoardInit,
        StatusMyTurn,
        StatusWaitingTurnAccept,
        StatusWaitingOpponent,
        StatusWin,
        StatusLose,
        StatusDraw
    };
    GameModel(QObject *parent = nullptr);
    void             init();
    GameStatus       status() const { return status_; }
    void             setStatus(GameStatus s);
    void             setError();
    bool             initOpponentBoard(const QStringList &data);
    bool             uncoverOpponentBoard(const QStringList &data);
    const GameBoard &myBoard() const { return myBoard_; }
    const GameBoard &oppBoard() const { return opBoard_; }
    void             setOpponentDraw(bool draw);
    bool             isOpponentDraw() const { return oppDraw_; }
    void             setOpponentAcceptedDraw(bool accept);
    void             opponentResign();
    void             opponentTurn(int pos);
    bool             handleResult();
    bool             handleTurnResult(const QString &res, const QString &seed);
    QString          lastShotResult() const;
    QString          lastShotSeed() const;
    QStringList      getUncoveredBoard() const;

private:
    GameStatus status_;
    GameBoard  myBoard_;
    GameBoard  opBoard_;
    int        lastShot_;
    bool       draw_;
    bool       oppDraw_;
    bool       myAccept_;
    bool       oppResign_;
    bool       myResign_;
    bool       destroyed_;

private slots:
    void myShipDestroyed();

public slots:
    void sendCoveredBoard();
    void localTurn(int pos);
    void setLocalDraw(bool draw);
    void localAccept();
    void localResign();

signals:
    void acceptDraw();
    void statusChanged();
    void myBoardUpdated(int x, int y, int width, int height);
    void oppBoardUpdated(int x, int y, int width, int height);
    void gameEvent(QString data);
};

#endif // GAMEMODEL_H
