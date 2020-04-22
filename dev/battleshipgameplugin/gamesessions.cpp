/*
 * gamesessionlist.cpp - Battleship Game plugin
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

#include <QTextDocument>

#include "gamesessions.h"
#include "invitedialog.h"
#include "options.h"

#define TIMEOUT_INTERVAL_SECS 60 * 60 * 1 // One hour

GameSessionList::GameSessionList(QObject *parent) : QObject(parent), stanzaId_(qrand() % 10000) { }

GameSessionList::~GameSessionList()
{
    //    qDeleteAll(list_); because error: 'virtual GameSession::~GameSession()' is private
    QList<GameSession *> vals = list_.values();
    while (!vals.isEmpty())
        delete vals.takeFirst();
}

GameSessionList *GameSessionList::instance_ = nullptr;

GameSessionList *GameSessionList::instance()
{
    if (!instance_)
        instance_ = new GameSessionList();
    return instance_;
}

void GameSessionList::reset()
{
    if (instance_) {
        delete instance_;
        instance_ = nullptr;
    }
}

GameSession *GameSessionList::createSession(int account, const QString &jid, bool first, const QString &gameId)
{
    if (findGame(account, jid, gameId))
        return nullptr;
    GameSession *gs                          = new GameSession(this, account, jid, first, gameId);
    list_[generateKey(account, jid, gameId)] = gs;
    connect(gs, SIGNAL(sendStanza(int, QString)), this, SIGNAL(sendStanza(int, QString)));
    connect(gs, SIGNAL(doPopup(QString)), this, SIGNAL(doPopup(QString)));
    connect(gs, SIGNAL(playSound(QString)), this, SIGNAL(playSound(QString)));
    connect(gs, SIGNAL(doInviteEvent(int, QString, QString, QObject *, const char *)), this,
            SIGNAL(doInviteEvent(int, QString, QString, QObject *, const char *)));
    return gs;
}

void GameSessionList::removeGame(GameSession *gs)
{
    list_.remove(list_.key(gs));
    gs->deleteLater();
}

GameSession *GameSessionList::findGame(int account, const QString &jid, const QString &gameId)
{
    QString      key = generateKey(account, jid, gameId);
    GameSession *gs  = list_.value(key, NULL);
    return gs;
}

GameSession *GameSessionList::findGameByStanzaId(int account, const QString &jid, const QString &stanzaId)
{
    QList<GameSession *> l = list_.values();
    foreach (GameSession *gs, l)
        if (gs->account_ == account && gs->jid_ == jid && gs->stanzaId_ == stanzaId)
            return gs;
    return nullptr;
}

bool GameSessionList::processIncomingIqStanza(int account, const QDomElement &xml, const QString &accStatus,
                                              bool fromPrivate)
{
    const QString from   = xml.attribute("from");
    const QString iqType = xml.attribute("type");
    if (iqType == "set") {
        QDomElement childEl = xml.firstChildElement();
        if (childEl.isNull() || childEl.namespaceURI() != "games:board" || childEl.attribute("type") != "battleship")
            return false;

        const QString gameId = childEl.attribute("id");
        if (gameId.isEmpty())
            return true;

        GameSession * gs      = nullptr;
        const QString tagName = childEl.tagName();
        if (tagName == "create") {
            Options *opt = Options::instance();
            if ((!opt->getOption(constDndDisable).toBool() || accStatus != "dnd")
                && (!fromPrivate || !opt->getOption(constConfDisable).toBool())) {
                bool          err      = true;
                bool          first    = true;
                const QString firstStr = childEl.attribute("first");
                if (firstStr.toLower() == "true") {
                    first = false;
                    err   = false;
                } else if (firstStr.toLower() == "false")
                    err = false;
                if (!err)
                    gs = createSession(account, from, first, gameId);
            }
        } else if ((gs = findGame(account, from, gameId)) != nullptr) {
            if (tagName == "board") {
                if (!gs->opBoardChecked_) {
                    if (gs->stage_ == GameSession::StageInitBoard)
                        gs->initOpponentBoard(childEl);
                    else if (gs->stage_ == GameSession::StageShowBoard) {
                        gs->checkOpponentBoard(childEl);
                    }
                    if (!gs->opBoardChecked_)
                        gs->status_ = GameSession::StatusError;
                } else
                    gs->status_ = GameSession::StatusError;
            } else if (tagName == "turn") {
                if (gs->stage_ == GameSession::StageShooting && gs->status_ == GameSession::StatusWaitOpponent)
                    gs->opponentTurn(childEl);
                else
                    gs->status_ = GameSession::StatusError;
            }
        }

        if (gs) {
            if (gs->stage_ != GameSession::StageNone)
                gs->sendIqResponse(xml.attribute("id"));
            else
                gs->stanzaId_ = xml.attribute("id");
            gs->executeNextAction();
        } else
            sendErrorIq(account, from, xml.attribute("id"));
        return true;
    } else {
        GameSession *gs = findGameByStanzaId(account, from, xml.attribute("id"));
        if (gs) {
            if (iqType == "result") {
                bool err = true;
                switch (gs->stage_) {
                case GameSession::StageInvitation:
                    if (gs->status_ == GameSession::StatusWaitInviteConfirmation) {
                        gs->status_ = GameSession::StatusNone;
                        err         = false;
                    }
                    break;
                case GameSession::StageInitBoard:
                    break;
                case GameSession::StageShowBoard:
                    if (gs->status_ == GameSession::StatusWaitBoardVerification) {
                        gs->myBoardChecked_ = true;
                        gs->status_         = GameSession::StatusNone;
                        err                 = false;
                    }
                    break;
                case GameSession::StageShooting:
                    if (gs->status_ == GameSession::StatusWaitShotConfirmation) {
                        gs->status_ = GameSession::StatusNone;
                        err         = !gs->handleTurnResult(xml);
                    }
                    break;
                case GameSession::StageEnd:
                    err = false;
                    break;
                default:
                    break;
                }
                if (err)
                    gs->status_ = GameSession::StatusError;

            } else if (gs->stage_ == GameSession::StageInvitation) {
                QString msg    = tr("From: %1<br />The game was rejected").arg(from);
                QString errMsg = getErrorMessage(xml);
                if (!errMsg.isEmpty())
                    msg.append(QString(" (%1)").arg(errMsg));
                doPopup(msg);
                gs->endSession();
                gs = nullptr;
            } else
                gs->status_ = GameSession::StatusError;
            if (gs)
                gs->executeNextAction();
            return true;
        }
    }
    return false;
}

void GameSessionList::invite(int account, const QString &jid, const QStringList &resList)
{
    GameSession *gs = createSession(account, jid, true, QString());
    if (gs)
        gs->invite(resList);
}

void GameSessionList::sendErrorIq(int account, const QString &jid, const QString &id)
{
    emit sendStanza(account, XML::iqErrorString(jid, id));
}

QString GameSessionList::generateKey(int account, const QString &jid, const QString &gameId)
{
    return QString("%1:%2:%3").arg(QString::number(account)).arg(jid).arg(gameId);
}

QString GameSessionList::getStanzaId(bool bigOffset)
{
    if (bigOffset)
        stanzaId_ += (qrand() % 50) + 5;
    else
        stanzaId_ += (qrand() % 5) + 2;
    return "bsg_" + QString::number(stanzaId_);
}

void GameSessionList::updateGameKey(GameSession *gs)
{
    list_.remove(list_.key(gs));
    list_[generateKey(gs->account_, gs->jid_, gs->gameId_)] = gs;
}

QString GameSessionList::getErrorMessage(const QDomElement &xml)
{
    QDomElement el = xml.firstChildElement("error");
    if (!el.isNull()) {
        el = el.firstChildElement("error-message");
        if (!el.isNull())
            return el.text();
    }
    return QString();
}

// ---------------- XML --------------------

QString XML::escapeString(const QString &str) { return str.toHtmlEscaped().replace("\"", "&quot;"); }

QString XML::iqErrorString(const QString &jid, const QString &id)
{
    QString stanza = QString("<iq type=\"error\" to=\"%1\" id=\"%2\">\n<error type=\"cancel\" code=\"407\">\n"
                             "<error-message>Not Acceptable</error-message>\n</error></iq>\n")
                         .arg(XML::escapeString(jid))
                         .arg(XML::escapeString(id));
    return stanza;
}

// -------------- GameSession -------------------

GameSession::GameSession(GameSessionList *gsl, int account, const QString &jid, bool first, const QString &gameId) :
    QObject(nullptr), gsl_(gsl), stage_(StageNone), status_(StatusNone), account_(account), jid_(jid), first_(first),
    gameId_(gameId), modifTime_(QDateTime::currentDateTime()), inviteDlg_(nullptr), boardWid_(nullptr),
    myBoardChecked_(false), opBoardChecked_(false), resign_(false)
{
}

GameSession::~GameSession()
{
    if (!inviteDlg_.isNull())
        inviteDlg_->close();
    if (!boardWid_.isNull())
        boardWid_->close();
}

void GameSession::executeNextAction()
{
    if (stage_ == StageEnd)
        return;

    bool modif = false;
    while (true) {
        GameStage  old_stage  = stage_;
        GameStatus old_status = status_;
        if (status_ != StatusError) {
            switch (stage_) {
            case StageNone:
                stage_  = StageInvitation;
                status_ = StatusWaitInviteConfirmation;
                processIncomingInvite();
                break;
            case StageInvitation:
                if (status_ == StatusNone) {
                    myBoardChecked_ = false;
                    opBoardChecked_ = false;
                    stage_          = StageInitBoard;
                }
                break;
            case StageInitBoard:
                if (status_ == StatusNone) {
                    if (myBoardChecked_) {
                        if (opBoardChecked_) {
                            stage_ = StageShooting;
                            startGame();
                        }
                    } else {
                        status_ = StatusWaitBoardVerification;
                        initBoard();
                    }
                }
                break;
            case StageShooting:
                if (status_ == StatusNone) {
                    if (checkEndGame()) {
                        stage_          = StageShowBoard;
                        status_         = StatusNone;
                        myBoardChecked_ = false;
                        opBoardChecked_ = false;
                    } else if (!isMyNextTurn())
                        status_ = StatusWaitOpponent;
                }
                break;
            case StageShowBoard:
                if (status_ == StatusNone) {
                    if (myBoardChecked_) {
                        if (opBoardChecked_)
                            stage_ = StageEnd;
                    } else {
                        status_ = StatusWaitBoardVerification;
                        sendUncoveredBoard();
                    }
                }
                break;
            case StageEnd:
                if (status_ == StatusNone) {
                    checkEndGame();
                    if (status_ == StatusNone)
                        status_ = StatusError;
                }
                break;
            }
        } else if (stage_ != StageEnd) {
            setError();
            stage_ = StageEnd;
        }
        if (old_stage == stage_ && old_status == status_)
            break;
        modif = true;
    }
    if (modif)
        modifTime_ = QDateTime::currentDateTime();
    if (inviteDlg_.isNull() && boardWid_.isNull()) {
        if (stage_ == StageEnd)
            endSession();
        else if (timer_.isNull())
            setTimer();
    }
}

void GameSession::processIncomingInvite()
{
    if (!boardWid_.isNull())
        showInvitationDialog();
    else
        appendInvitationEvent();
}

void GameSession::appendInvitationEvent()
{
    emit doInviteEvent(account_, jid_, tr("%1: Invitation from %2").arg("Battleship Game Plugin").arg(jid_), this,
                       SLOT(showInvitationDialog()));
}

void GameSession::showInvitationDialog()
{
    inviteDlg_ = new InvitationDialog(jid_, first_, boardWid_.data());
    connect(inviteDlg_.data(), SIGNAL(accepted()), this, SLOT(acceptInvitation()));
    connect(inviteDlg_.data(), SIGNAL(rejected()), this, SLOT(rejectInvitation()));
    inviteDlg_->show();
}

void GameSession::acceptInvitation()
{
    status_ = StatusNone;
    sendStanzaResult(stanzaId_);
    executeNextAction();
}

void GameSession::rejectInvitation()
{
    GameSessionList::instance()->sendErrorIq(account_, jid_, stanzaId_);
    endSession();
}

void GameSession::invite(const QStringList &resList)
{
    QWidget *parent = nullptr;
    if (!boardWid_.isNull())
        parent = boardWid_.data();
    InviteDialog *dlg = new InviteDialog(jid_.section('/', 0, 0), resList, parent);
    connect(dlg, SIGNAL(acceptGame(QString, bool)), this, SLOT(sendInvite(QString, bool)));
    connect(dlg, SIGNAL(rejected()), this, SLOT(endSession()));
    inviteDlg_ = dlg;
    dlg->show();
}

void GameSession::sendInvite(QString jid, bool first)
{
    first_       = first;
    jid_         = jid;
    modifTime_   = QDateTime::currentDateTime();
    boardStatus_ = QString();
    generateGameId();
    gsl_->updateGameKey(this);

    stage_         = StageInvitation;
    status_        = StatusWaitInviteConfirmation;
    stanzaId_      = gsl_->getStanzaId(true);
    QString stanza = QString("<iq type=\"set\" to=\"%1\" id=\"%2\">"
                             "<create xmlns=\"games:board\" id=\"%3\" type=\"battleship\" first=\"%4\" /></iq>\n")
                         .arg(XML::escapeString(jid))
                         .arg(stanzaId_)
                         .arg(XML::escapeString(gameId_))
                         .arg((first) ? "true" : "false");
    emit sendStanza(account_, stanza);
}

void GameSession::generateGameId()
{
    gameId_
        = "battleship_" + QString::number(qrand(), 16) + QString::number(qrand(), 16) + QString::number(qrand(), 16);
}

void GameSession::endSession()
{
    if (boardWid_.isNull())
        gsl_->removeGame(this);
    else if (stage_ != StageEnd) {
        stage_  = StageEnd;
        status_ = StatusError;
    }
}

void GameSession::setError()
{
    status_ = StatusError;
    if (!boardWid_.isNull())
        boardWid_.data()->setError();
}

void GameSession::sendIqResponse(const QString &id)
{
    if (status_ == StatusError)
        gsl_->sendErrorIq(account_, jid_, id);
    else {
        QString body;
        if (stage_ == StageShooting && !resign_) {
            body = QString("<turn xmlns=\"games:board\" type=\"battleship\" id=\"%1\">\n"
                           "<shot result=\"%2\" seed=\"%3\"/>\n</turn>\n")
                       .arg(XML::escapeString(gameId_))
                       .arg(lastTurnResult_)
                       .arg(XML::escapeString(lastTurnSeed_));
        }
        sendStanzaResult(id, body);
    }
}

void GameSession::sendStanzaResult(const QString &id, const QString &body)
{
    QString stanza
        = QString("<iq type=\"result\" to=\"%1\" id=\"%2\"").arg(XML::escapeString(jid_)).arg(XML::escapeString(id));
    if (body.isEmpty())
        stanza.append("/>\n");
    else
        stanza.append(">\n" + body + "</iq>\n");
    emit sendStanza(account_, stanza);
}

void GameSession::initBoard()
{
    if (boardWid_.isNull()) {
        boardWid_ = new PluginWindow(jid_);
        connect(boardWid_.data(), SIGNAL(gameEvent(QString)), this, SLOT(boardEvent(QString)));
        connect(boardWid_.data(), SIGNAL(destroyed()), this, SLOT(endSession()));
    }
    boardWid_->initBoard();
    boardWid_->show();
}

void GameSession::initOpponentBoard(const QDomElement &xml)
{
    opBoardChecked_ = false;
    bool             cells[100];
    int              cellsCnt = 0;
    QList<short int> ships;
    for (int i = 0; i < 100; ++i)
        cells[i] = false;
    ships.append(5);
    ships.append(4);
    ships.append(3);
    ships.append(2);
    ships.append(2);
    ships.append(1);
    ships.append(1);
    QStringList data("init-opp-board");
    QDomElement el = xml.firstChildElement();
    while (!el.isNull()) {
        const QString nm = el.nodeName();
        if (nm == "cell") {
            int     row  = el.attribute("row").toInt();
            int     col  = el.attribute("col").toInt();
            QString hash = el.attribute("hash");
            if (row < 0 || row >= 10 || col < 0 || col >= 10 || hash.length() != 40)
                return;
            int pos = row * 10 + col;
            if (cells[pos])
                return;
            data.append(QString("cell;%1;%2").arg(pos).arg(hash));
            cells[pos] = true;
            ++cellsCnt;
        } else if (nm == "ship") {
            int     len  = el.attribute("length").toInt();
            QString hash = el.attribute("hash");
            if (!ships.removeOne(len) || hash.length() != 40)
                return;
            data.append(QString("ship;%1;%2").arg(len).arg(hash));
        }
        el = el.nextSiblingElement();
    }
    if (cellsCnt == 100 && ships.empty() && !boardWid_.isNull())
        opBoardChecked_ = (boardWid_->dataExchange(data).first() == "ok");
}

void GameSession::checkOpponentBoard(const QDomElement &xml)
{
    opBoardChecked_ = false;
    bool cells[100];
    int  cellsCnt = 0;
    for (int i = 0; i < 100; ++i)
        cells[i] = false;
    QStringList data("check-opp-board");
    QDomElement el = xml.firstChildElement();
    while (!el.isNull()) {
        if (el.nodeName() == "cell") {
            int     row  = el.attribute("row").toInt();
            int     col  = el.attribute("col").toInt();
            QString seed = el.attribute("seed");
            if (row < 0 || row >= 10 || col < 0 || col >= 10 || seed.isEmpty())
                return;
            int pos = row * 10 + col;
            if (cells[pos])
                return;
            QString ship = el.attribute("ship").toLower();
            if (ship == "true")
                ship = "1";
            else if (ship != "1")
                ship = "0";
            data.append(QString("%1;%2;%3").arg(pos).arg(ship).arg(seed));
            cells[pos] = true;
            ++cellsCnt;
        }
        el = el.nextSiblingElement();
    }
    if (cellsCnt == 100 && !boardWid_.isNull())
        opBoardChecked_ = (boardWid_->dataExchange(data).first() == "ok");
}

void GameSession::opponentTurn(const QDomElement &xml)
{
    bool        err    = false;
    int         pos    = -1;
    int         draw   = 0;
    int         resign = 0;
    int         accept = 0;
    QDomElement el     = xml.firstChildElement();
    while (!el.isNull()) {
        const QString tagName = el.tagName();
        if (tagName == "shot") {
            err                  = true;
            const QString rowStr = el.attribute("row");
            const QString colStr = el.attribute("col");
            if (rowStr.isEmpty() || colStr.isEmpty() || pos != -1)
                break;
            int row = rowStr.toInt();
            int col = colStr.toInt();
            if (row < 0 || row >= 10 || col < 0 || col >= 10)
                break;
            pos = row * 10 + col;
            err = false;
        } else if (tagName == "draw")
            ++draw;
        else if (tagName == "accept")
            ++accept;
        else if (tagName == "resign")
            ++resign;
        el = el.nextSiblingElement();
    }
    if (!err && draw + accept + resign <= 1 && (pos != -1 || resign + accept != 0)) {
        QStringList data("turn");
        if (draw)
            data.append("draw");
        if (accept)
            data.append("accept");
        if (resign)
            data.append("resign");
        if (pos != -1)
            data.append(QString("shot;%1").arg(pos));
        QStringList res;
        if (!boardWid_.isNull())
            res = boardWid_->dataExchange(data);
        if (res.takeFirst() == "ok") {
            while (!res.isEmpty()) {
                QString s = res.takeFirst();
                QString t = s.section(';', 0, 0);
                if (t == "result") {
                    lastTurnResult_ = s.section(';', 1, 1);
                    lastTurnSeed_   = s.section(';', 2);
                } else if (t == "status") {
                    boardStatus_ = s.section(';', 1);
                }
            }
            status_ = StatusNone;
        } else
            status_ = StatusError;
    } else
        status_ = StatusError;
}

bool GameSession::handleTurnResult(const QDomElement &xml)
{
    if (boardWid_.isNull())
        return false;

    QStringList data("turn-result");

    QDomElement el = xml.firstChildElement("turn");
    if (!el.isNull()) {
        if (el.namespaceURI() != "games:board" || el.attribute("type") != "battleship" || el.attribute("id") != gameId_)
            return false;

        el = el.firstChildElement("shot");
        if (el.isNull())
            return false;

        QString res = el.attribute("result");
        if (res != "miss" && res != "hit" && res != "destroy")
            return false;

        QString seed = el.attribute("seed");
        data.append(QString("shot-result;%1;%2").arg(res).arg(seed));
    }
    QStringList res = boardWid_->dataExchange(data);
    QString     s   = res.takeFirst();
    if (s == "ok") {
        while (!res.isEmpty()) {
            s = res.takeFirst();
            if (s.section(';', 0, 0) == "status") {
                boardStatus_ = s.section(';', 1);
                break;
            }
        }
        return true;
    }
    return false;
}

void GameSession::boardEvent(QString data)
{
    QStringList dataList = data.split('\n');
    QString     dataStr  = dataList.takeFirst();
    QString     body;
    if (dataStr == "covered-board") {
        body = QString("<board xmlns=\"games:board\" type=\"battleship\" id=\"%1\">\n").arg(gameId_);
        while (!dataList.isEmpty()) {
            dataStr     = dataList.takeFirst();
            QString str = dataStr.section(';', 0, 0);
            if (str == "cell") {
                int     pos  = dataStr.section(';', 1, 1).toInt();
                int     row  = pos / 10;
                int     col  = pos % 10;
                QString hash = dataStr.section(';', 2);
                body.append(QString("<cell row=\"%1\" col=\"%2\" hash=\"%3\"/>\n").arg(row).arg(col).arg(hash));
            } else if (str == "ship") {
                int     len  = dataStr.section(';', 1, 1).toInt();
                QString hash = dataStr.section(';', 2);
                body.append(QString("<ship length=\"%1\" hash=\"%2\"/>\n").arg(len).arg(hash));
            }
        }
        body.append("</board>\n");
    } else if (dataStr == "turn") {
        int  pos    = -1;
        bool draw   = false;
        bool accept = false;
        bool resign = false;
        while (!dataList.isEmpty()) {
            dataStr     = dataList.takeFirst();
            QString str = dataStr.section(';', 0, 0);
            if (str == "pos")
                pos = dataStr.section(';', 1).toInt();
            else if (str == "draw")
                draw = true;
            else if (str == "accept")
                accept = true;
            else if (str == "resign")
                resign = true;
        }
        body = QString("<turn xmlns=\"games:board\" type=\"battleship\" id=\"%1\">\n").arg(XML::escapeString(gameId_));
        if (pos != -1) {
            int row = pos / 10;
            int col = pos % 10;
            body.append(QString("<shot row=\"%1\" col=\"%2\"/>\n").arg(row).arg(col));
        }
        if (draw)
            body.append("<draw/>\n");
        if (accept)
            body.append("<accept/>\n");
        if (resign)
            body.append("<resign/>\n");
        body.append("</turn>\n");
        status_ = StatusWaitShotConfirmation;
        if (accept || resign)
            boardStatus_ = "end";
    } else if (dataStr == "new-game") {
        invite(QStringList(jid_.section('/', 1)));
        return;
    }
    stanzaId_      = gsl_->getStanzaId(false);
    QString stanza = QString("<iq type=\"set\" to=\"%1\" id=\"%2\">\n").arg(XML::escapeString(jid_)).arg(stanzaId_);
    stanza.append(body);
    stanza.append("</iq>\n");
    emit sendStanza(account_, stanza);
}

void GameSession::startGame()
{
    if (!boardWid_.isNull()) {
        QStringList data("start");
        if (first_)
            data.append("first");
        QStringList res = boardWid_->dataExchange(data);
        if (res.takeFirst() == "ok") {
            while (!res.isEmpty()) {
                QString s = res.takeFirst();
                if (s.section(';', 0, 0) == "status") {
                    boardStatus_ = s.section(';', 1);
                    break;
                }
            }
        } else
            boardStatus_.clear();
    }
}

bool GameSession::checkEndGame() { return (boardStatus_ == "end"); }

bool GameSession::isMyNextTurn() { return (boardStatus_ == "turn"); }

void GameSession::sendUncoveredBoard()
{
    if (boardWid_.isNull())
        return;
    QStringList res = boardWid_->dataExchange(QStringList("get-uncovered-board"));
    QString     body;
    while (!res.isEmpty()) {
        QString dataStr = res.takeFirst();
        int     pos     = dataStr.section(';', 0, 0).toInt();
        int     row     = pos / 10;
        int     col     = pos % 10;
        QString ship    = dataStr.section(';', 1, 1);
        QString seed    = dataStr.section(';', 2);
        body.append(QString("<cell row=\"%1\" col=\"%2\" ship=\"%3\" seed=\"%4\"/>\n")
                        .arg(row)
                        .arg(col)
                        .arg(ship)
                        .arg(XML::escapeString(seed)));
    }
    stanzaId_      = gsl_->getStanzaId(false);
    QString stanza = QString("<iq type=\"set\" to=\"%1\" id=\"%2\">\n").arg(XML::escapeString(jid_)).arg(stanzaId_);
    stanza.append(
        QString("<board xmlns=\"games:board\" type=\"battleship\" id=\"%1\">\n").arg(XML::escapeString(gameId_)));
    stanza.append(body);
    stanza.append("</board>\n</iq>\n");
    emit sendStanza(account_, stanza);
}

void GameSession::setTimer()
{
    timer_ = new QTimer(this);
    timer_->setSingleShot(true);
    connect(timer_.data(), SIGNAL(timeout()), this, SLOT(timeout()));
    timer_->setInterval(TIMEOUT_INTERVAL_SECS * 1000);
}

void GameSession::timeout()
{
    QDateTime currTime = QDateTime::currentDateTime();
    if (inviteDlg_.isNull() && boardWid_.isNull()) {
        if (modifTime_.secsTo(currTime) >= TIMEOUT_INTERVAL_SECS)
            endSession();
    } else
        delete timer_;
}
