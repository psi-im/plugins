/*
 * gamesessionlist.cpp - Battleship game plugin
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

#ifndef GAMESESSIONLIST_H
#define GAMESESSIONLIST_H

#include <QDateTime>
#include <QDomElement>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QTimer>

#include "pluginwindow.h"

class GameSession;

namespace XML {
QString escapeString(const QString &str);
QString iqErrorString(const QString &jid, const QString &id);
}

class GameSessionList : public QObject {
    Q_OBJECT

public:
    static GameSessionList *instance();
    static void             reset();
    GameSession *           createSession(int account, const QString &jid, bool first, const QString &gameId);
    void                    updateGameKey(GameSession *gs);
    void                    removeGame(GameSession *gs);
    GameSession *           findGame(int account, const QString &jid, const QString &gameId);
    GameSession *           findGameByStanzaId(int account, const QString &jid, const QString &stanzaId);
    bool    processIncomingIqStanza(int account, const QDomElement &xml, const QString &accStatus, bool fromPrivate);
    void    invite(int account, const QString &jid, const QStringList &resList);
    QString getStanzaId(bool bigOffset);
    void    sendErrorIq(int account, const QString &jid, const QString &id);

private:
    GameSessionList(QObject *parent = nullptr);
    ~GameSessionList();
    QString        generateKey(int account, const QString &jid, const QString &gameId);
    static QString getErrorMessage(const QDomElement &xml);

private:
    static GameSessionList *      instance_;
    QHash<QString, GameSession *> list_;
    int                           stanzaId_;

signals:
    void sendStanza(int, QString);
    void doPopup(QString);
    void playSound(QString);
    void doInviteEvent(int, QString, QString, QObject *, const char *);
};

class GameSession : public QObject {
    Q_OBJECT

public:
    enum GameStage { StageNone, StageInvitation, StageInitBoard, StageShooting, StageShowBoard, StageEnd };
    enum GameStatus {
        StatusNone,
        StatusError,
        StatusWaitInviteConfirmation,
        StatusWaitBoardVerification,
        StatusWaitShotConfirmation,
        StatusWaitOpponent
    };
    using Timer       = QPointer<QTimer>;
    using InviteDlg   = QPointer<QDialog>;
    using BoardWidget = QPointer<PluginWindow>;
    void executeNextAction();
    void invite(const QStringList &resList);
    void initOpponentBoard(const QDomElement &xml);
    void checkOpponentBoard(const QDomElement &xml);
    void opponentTurn(const QDomElement &xml);
    bool handleTurnResult(const QDomElement &xml);

private:
    friend class GameSessionList;
    GameSession(GameSessionList *gsl, int account, const QString &jid, bool first, const QString &gameId);
    ~GameSession();
    void processIncomingInvite();
    void appendInvitationEvent();
    void generateGameId();
    void sendIqResponse(const QString &id);
    void sendStanzaResult(const QString &id, const QString &body = QString());
    void initBoard();
    void setError();
    void startGame();
    bool checkEndGame();
    bool isMyNextTurn();
    void sendUncoveredBoard();
    void setTimer();

private:
    GameSessionList *gsl_;
    GameStage        stage_;
    GameStatus       status_;
    int              account_;
    QString          jid_;
    bool             first_;
    QString          gameId_;
    QString          stanzaId_;
    QDateTime        modifTime_;
    Timer            timer_;
    InviteDlg        inviteDlg_;
    BoardWidget      boardWid_;
    bool             myBoardChecked_;
    bool             opBoardChecked_;
    bool             resign_;
    // int             lastTurnPos_;
    // bool            lastTurnMy_;
    QString lastTurnResult_;
    QString lastTurnSeed_;
    QString boardStatus_;

private slots:
    void sendInvite(QString jid, bool first);
    void acceptInvitation();
    void rejectInvitation();
    void endSession();
    void boardEvent(QString data);
    void timeout();

public slots:
    void showInvitationDialog();

signals:
    void sendStanza(int, QString);
    void doPopup(const QString);
    void playSound(const QString);
    void doInviteEvent(int, QString, QString, QObject *, const char *);
};

#endif // GAMESESSIONLIST_H
