/*
 * gamesessions.cpp - Gomoku Game plugin
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <QTextDocument>
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
#include <QRandomGenerator>
#endif

#include "common.h"
#include "gamesessions.h"
#include "invatedialog.h"
#include "options.h"

GameSessions::GameSessions(QObject *parent) :
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    QObject(parent), stanzaId(QRandomGenerator::global()->generate() % 10000), // Получаем псевдослучайный стартовый id
#else
    QObject(parent), stanzaId(qrand() % 10000), // Получаем псевдослучайный стартовый id
#endif
    errorStr("")
{
    gameSessions.clear();
}

GameSessions::~GameSessions()
{
    while (!gameSessions.isEmpty()) {
        GameSession gs = gameSessions.first();
        if (!gs.wnd.isNull()) {
            gs.wnd.data()->close(); // В результате сигнала будет удалена соотв. запись сессии.
        } else {
            gameSessions.removeFirst();
        }
    }
}

GameSessions *GameSessions::instance_ = nullptr;

GameSessions *GameSessions::instance()
{
    if (!instance_) {
        instance_ = new GameSessions();
    }
    return instance_;
}

void GameSessions::reset()
{
    if (instance_) {
        delete instance_;
        instance_ = nullptr;
    }
}

/**
 * Обработка входящей станзы и вызов соответствующих методов
 */
bool GameSessions::processIncomingIqStanza(int account, const QDomElement &xml, const QString &acc_status,
                                           bool conf_priv)
{
    const QString iq_type = xml.attribute("type");
    if (iq_type == "set") {
        QDomElement childElem = xml.firstChildElement("create");
        if (!childElem.isNull() && childElem.namespaceURI() == "games:board"
            && childElem.attribute("type") == constProtoType) {
            const QString from    = xml.attribute("from");
            const QString id      = xml.attribute("id");
            const QString protoId = childElem.attribute("id");
            if (protoId != constProtoId) {
                sendErrorIq(account, from, id, "Incorrect protocol version");
                return true;
            }
            Options *options = Options::instance();
            if ((options->getOption(constDndDisable).toBool() && acc_status == "dnd")
                || (options->getOption(constConfDisable).toBool() && conf_priv)) {
                sendErrorIq(account, from, id, "");
                return true;
            }
            if (incomingInvitation(account, from, childElem.attribute("color"), id)) {
                emit doInviteEvent(account, from, tr("%1: Invitation from %2").arg(constPluginName).arg(from), this,
                                   SLOT(showInvitation(QString)));
            }
            return true;
        }
        if (activeCount() == 0) // Нет ни одной активной игровой сессии (наиболее вероятный исход большую часть времени)
            return false;       // Остальные проверки бессмысленны
        childElem = xml.firstChildElement("turn");
        if (!childElem.isNull() && childElem.namespaceURI() == "games:board"
            && childElem.attribute("type") == constProtoType) {
            const QString from    = xml.attribute("from");
            const QString id      = xml.attribute("id");
            const QString protoId = childElem.attribute("id");
            if (protoId != constProtoId) {
                sendErrorIq(account, from, id, "Incorrect protocol version");
                return true;
            }
            QDomElement turnChildElem = childElem.firstChildElement("move");
            if (!turnChildElem.isNull()) {
                return doTurnAction(account, from, id, turnChildElem.attribute("pos"));
            }
            turnChildElem = childElem.firstChildElement("resign");
            if (!turnChildElem.isNull()) {
                return youWin(account, from, id);
            }
            turnChildElem = childElem.firstChildElement("draw");
            if (!turnChildElem.isNull()) {
                return setDraw(account, from, id);
            }
            return false;
        }
        childElem = xml.firstChildElement("close");
        if (!childElem.isNull() && childElem.namespaceURI() == "games:board"
            && childElem.attribute("type") == constProtoType) {
            const QString from    = xml.attribute("from");
            const QString id      = xml.attribute("id");
            const QString protoId = childElem.attribute("id");
            if (protoId != constProtoId) {
                sendErrorIq(account, from, id, "Incorrect protocol version");
                return true;
            }
            return closeRemoteGameBoard(account, from, id);
        }
        childElem = xml.firstChildElement("load");
        if (!childElem.isNull() && childElem.namespaceURI() == "games:board"
            && childElem.attribute("type") == constProtoType) {
            const QString from    = xml.attribute("from");
            const QString id      = xml.attribute("id");
            const QString protoId = childElem.attribute("id");
            if (protoId != constProtoId) {
                sendErrorIq(account, from, id, "Incorrect protocol version");
                return true;
            }
            return remoteLoad(account, from, id, childElem.text());
        }
    } else if (iq_type == "result") {
        if (doResult(account, xml.attribute("from"), xml.attribute("id"))) {
            return true;
        }
    } else if (iq_type == "error") {
        if (doReject(account, xml.attribute("from"), xml.attribute("id"))) {
            return true;
        }
    }
    return false;
}

/**
 * Вызов окна для приглашения к игре
 */
void GameSessions::invite(int account, const QString &jid, const QStringList &res_list, QWidget *parent)
{
    InvateDialog *dialog = new InvateDialog(account, jid, res_list, parent);
    connect(dialog, SIGNAL(acceptGame(int, QString, QString)), this, SLOT(sendInvite(int, QString, QString)));
    connect(dialog, SIGNAL(rejectGame(int, QString)), this, SLOT(cancelInvite(int, QString)));
    dialog->show();
}

/**
 * Отправка приглашения поиграть выбранному джиду
 */
void GameSessions::sendInvite(const int account, const QString &full_jid, const QString &element)
{
    QString new_id = newId(true);
    if (!regGameSession(StatusInviteSend, account, full_jid, new_id, element)) {
        emit doPopup(getLastError());
        return;
    }
    emit sendStanza(account,
                    QString("<iq type=\"set\" to=\"%1\" id=\"%2\"><create xmlns=\"games:board\" id=\"%3\" type=\"%4\" "
                            "color=\"%5\"></create></iq>")
                        .arg(XML::escapeString(full_jid))
                        .arg(new_id)
                        .arg(constProtoId)
                        .arg(constProtoType)
                        .arg(element));
}

/**
 * Отмена приглашения. Мы передумали приглашать. :)
 */
void GameSessions::cancelInvite(const int account, const QString &full_jid) { removeGameSession(account, full_jid); }

/**
 * Обработка и регистрация входящего приглашения
 */
bool GameSessions::incomingInvitation(const int account, const QString &from, const QString &color,
                                      const QString &iq_id)
{
    errorStr = "";
    if (color != "black" && color != "white") {
        errorStr = tr("Incorrect parameters");
    }
    if (regGameSession(StatusInviteInDialog, account, from, iq_id, color)) {
        const int idx = findGameSessionById(account, iq_id);
        if (!gameSessions.at(idx).wnd.isNull()) {
            QMetaObject::invokeMethod(this, "doInviteDialog", Qt::QueuedConnection, Q_ARG(int, account),
                                      Q_ARG(QString, from));
            return false;
        }
        return true;
    }
    sendErrorIq(account, from, iq_id, errorStr);
    return false;
}

/**
 * Отображение окна приглашения
 */
void GameSessions::showInvitation(const QString &from)
{
    int       account_ = 0;
    const int idx      = findGameSessionByJid(from);
    if (idx == -1 || gameSessions.at(idx).status != StatusInviteInDialog)
        return;
    account_ = gameSessions.at(idx).my_acc;
    doInviteDialog(account_, from);
}

/**
 * Отображение формы входящего приглашения.
 */
void GameSessions::doInviteDialog(const int account, const QString &from)
{
    const int idx = findGameSessionByJid(account, from);
    if (idx == -1 || gameSessions.at(idx).status != StatusInviteInDialog)
        return;
    InvitationDialog *wnd = new InvitationDialog(account, from, gameSessions.at(idx).element,
                                                 gameSessions.at(idx).last_iq_id, gameSessions.at(idx).wnd);
    connect(wnd, SIGNAL(accepted(int, QString)), this, SLOT(acceptInvite(int, QString)));
    connect(wnd, SIGNAL(rejected(int, QString)), this, SLOT(rejectInvite(int, QString)));
    wnd->show();
}

/**
 * Принимаем приглашение на игру
 */
void GameSessions::acceptInvite(const int account, const QString &id)
{
    const int idx = findGameSessionById(account, id);
    if (idx != -1) {
        if (gameSessions.at(idx).status == StatusInviteInDialog) {
            QString my_el             = (gameSessions.at(idx).element == "black") ? "white" : "black";
            gameSessions[idx].element = my_el;
            startGame(idx);
            QString stanza = QString("<iq type=\"result\" to=\"%1\" id=\"%2\"><create xmlns=\"games:board\" "
                                     "type=\"%3\" id=\"%4\"/></iq>")
                                 .arg(XML::escapeString(gameSessions.at(idx).full_jid))
                                 .arg(XML::escapeString(id))
                                 .arg(constProtoType)
                                 .arg(constProtoId);
            emit sendStanza(account, stanza);
        } else {
            sendErrorIq(account, gameSessions.at(idx).full_jid, id, getLastError());
            emit doPopup(tr("You are already playing!"));
        }
    }
}

/**
 * Отклоняем приглашение на игру
 */
void GameSessions::rejectInvite(const int account, const QString &id)
{
    const int idx = findGameSessionById(account, id);
    if (idx != -1 && gameSessions.at(idx).status == StatusInviteInDialog) {
        QString jid = gameSessions.at(idx).full_jid;
        if (gameSessions.at(idx).wnd.isNull()) {
            removeGameSession(account, jid);
        } else {
            gameSessions[idx].status = StatusNone;
        }
        sendErrorIq(account, jid, id, getLastError());
    }
}

/**
 * Инициализация сессии игры и игровой доски
 */
void GameSessions::startGame(const int sess_index)
{
    newId(true);
    GameSession *sess = &gameSessions[sess_index];
    if (sess->wnd.isNull()) {
        PluginWindow *wnd = new PluginWindow(sess->full_jid, nullptr);
        connect(wnd, SIGNAL(changeGameSession(QString)), this, SLOT(setSessionStatus(QString)));
        connect(wnd, SIGNAL(closeBoard(bool, int, int, int, int)), this,
                SLOT(closeGameWindow(bool, int, int, int, int)));
        connect(wnd, SIGNAL(setElement(int, int)), this, SLOT(sendMove(int, int)));
        connect(wnd, SIGNAL(switchColor()), this, SLOT(switchColor()));
        connect(wnd, SIGNAL(accepted()), this, SLOT(sendAccept()));
        connect(wnd, SIGNAL(error()), this, SLOT(sendError()));
        connect(wnd, SIGNAL(lose()), this, SLOT(youLose()));
        connect(wnd, SIGNAL(draw()), this, SLOT(sendDraw()));
        connect(wnd, SIGNAL(load(QString)), this, SLOT(sendLoad(QString)));
        connect(wnd, SIGNAL(sendNewInvite()), this, SLOT(newGame()));
        connect(wnd, SIGNAL(doPopup(const QString)), this, SIGNAL(doPopup(const QString)));
        connect(wnd, SIGNAL(playSound(const QString)), this, SIGNAL(playSound(const QString)));
        sess->wnd        = wnd;
        Options *options = Options::instance();
        if (options->getOption(constSaveWndPosition).toBool()) {
            const int topPos = options->getOption(constWindowTop).toInt();
            if (topPos > 0) {
                const int leftPos = options->getOption(constWindowLeft).toInt();
                if (leftPos > 0) {
                    sess->wnd->move(leftPos, topPos);
                }
            }
        }
        if (options->getOption(constSaveWndWidthHeight).toBool()) {
            const int width = options->getOption(constWindowWidth).toInt();
            if (width > 0) {
                const int height = options->getOption(constWindowHeight).toInt();
                if (height > 0) {
                    sess->wnd->resize(width, height);
                }
            }
        }
    }
    sess->status = StatusNone;
    sess->wnd->init(sess->element);
    sess->wnd->show();
}

/**
 * Обработка iq ответа result для игры.
 */
bool GameSessions::doResult(const int account, const QString &from, const QString &iq_id)
{
    if (iq_id.isEmpty())
        return false;
    const int idx = findGameSessionById(account, iq_id);
    if (idx != -1) {
        GameSession *sess = &gameSessions[idx];
        if (sess->full_jid == from) {
            if (sess->status == StatusInviteSend) {
                startGame(idx);
                return true;
            }
            if (sess->status == StatusWaitOpponentAccept) {
                if (!sess->wnd.isNull()) {
                    QMetaObject::invokeMethod(sess->wnd.data(), "setAccept", Qt::QueuedConnection);
                    return true;
                }
            }
        }
    }
    // Видимо не наша станза
    return false;
}

/**
 * Отклонение игры или приглашения на основании iq ответа об ошибке
 */
bool GameSessions::doReject(const int account, const QString &from, const QString &iq_id)
{
    if (iq_id.isEmpty())
        return false;
    // Сначала ищем в запросах
    const int idx = findGameSessionById(account, iq_id);
    if (idx != -1) {
        GameSession *sess = &gameSessions[idx];
        if (sess->full_jid == from) {
            SessionStatus status = sess->status;
            if (status == StatusInviteSend) {
                if (sess->wnd.isNull()) {
                    removeGameSession(account, from);
                } else {
                    gameSessions[idx].status = StatusNone;
                }
                emit doPopup(tr("From: %1<br />The game was rejected").arg(from));
                return true;
            }
            if (!sess->wnd.isNull()) {
                QMetaObject::invokeMethod(sess->wnd.data(), "setError", Qt::QueuedConnection);
                emit doPopup(tr("From: %1<br />Game error.").arg(from));
            } else {
                removeGameSession(account, from);
            }
            return true;
        }
    }
    return false;
}

/**
 * Обработка действия оппонента
 */
bool GameSessions::doTurnAction(const int account, const QString &from, const QString &iq_id, const QString &value)
{
    if (iq_id.isEmpty())
        return false;
    const int idx = findGameSessionByJid(account, from);
    if (idx == -1)
        return false;
    GameSession *sess = &gameSessions[idx];
    if (sess->full_jid == from) {
        if (!sess->wnd.isNull()) {
            if (value == "switch-color") {
                sess->last_iq_id = iq_id;
                QMetaObject::invokeMethod(sess->wnd.data(), "setSwitchColor", Qt::QueuedConnection);
                return true;
            } else {
                QStringList val_lst = value.split(",");
                if (val_lst.size() == 2) {
                    bool fOk;
                    int  x = val_lst.at(0).trimmed().toInt(&fOk);
                    if (fOk) {
                        int y = val_lst.at(1).trimmed().toInt(&fOk);
                        if (fOk) {
                            sess->last_iq_id = iq_id;
                            QMetaObject::invokeMethod(sess->wnd.data(), "setTurn", Qt::QueuedConnection, Q_ARG(int, x),
                                                      Q_ARG(int, y));
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

/**
 * Пришло сообщение оппонента о нашей победе
 */
bool GameSessions::youWin(const int account, const QString &from, const QString &iq_id)
{
    const int idx = findGameSessionByJid(account, from);
    if (idx == -1)
        return false;
    GameSession *sess = &gameSessions[idx];
    sess->last_iq_id  = iq_id;
    // Отправляем подтверждение получения станзы
    QString stanza
        = QString("<iq type=\"result\" to=\"%1\" id=\"%2\"><turn type=\"%3\" id=\"%4\" xmlns=\"games:board\"/></iq>")
              .arg(XML::escapeString(from))
              .arg(XML::escapeString(iq_id))
              .arg(constProtoType)
              .arg(constProtoId);
    emit sendStanza(account, stanza);
    // Отправляем окну уведомление о выигрыше
    QMetaObject::invokeMethod(sess->wnd.data(), "setWin", Qt::QueuedConnection);
    return true;
}

/**
 * Пришло собщение оппонента о ничьей
 */
bool GameSessions::setDraw(const int account, const QString &from, const QString &iq_id)
{
    const int idx = findGameSessionByJid(account, from);
    if (idx != -1) {
        GameSession *sess = &gameSessions[idx];
        sess->last_iq_id  = iq_id;
        // Отправляем подтверждение
        QString stanza
            = QString(
                  "<iq type=\"result\" to=\"%1\" id=\"%2\"><turn type=\"%3\" id=\"%4\" xmlns=\"games:board\"/></iq>")
                  .arg(XML::escapeString(from))
                  .arg(XML::escapeString(iq_id))
                  .arg(constProtoType)
                  .arg(constProtoId);
        emit sendStanza(account, stanza);
        // Уведомляем окно
        QMetaObject::invokeMethod(sess->wnd.data(), "opponentDraw", Qt::QueuedConnection);
        return true;
    }
    return false;
}

/**
 * Оппонент закрыл доску
 */
bool GameSessions::closeRemoteGameBoard(const int account, const QString &from, const QString &iq_id)
{
    const int idx = findGameSessionByJid(account, from);
    if (idx == -1)
        return false;
    GameSession *sess = &gameSessions[idx];
    if (sess->full_jid != from)
        return false;
    sess->last_iq_id = iq_id;
    QString stanza
        = QString("<iq type=\"result\" to=\"%1\" id=\"%2\"><turn type=\"%3\" id=\"%4\" xmlns=\"games:board\"/></iq>")
              .arg(XML::escapeString(from))
              .arg(XML::escapeString(iq_id))
              .arg(constProtoType)
              .arg(constProtoId);
    emit sendStanza(account, stanza);
    // Отправляем окну уведомление о закрытии окна
    QMetaObject::invokeMethod(gameSessions.at(idx).wnd.data(), "setClose", Qt::QueuedConnection);
    return true;
}

/**
 * Оппонент прислал загруженную игру
 */
bool GameSessions::remoteLoad(const int account, const QString &from, const QString &iq_id, const QString &value)
{
    const int idx = findGameSessionByJid(account, from);
    if (idx == -1)
        return false;
    gameSessions[idx].last_iq_id = iq_id;
    QMetaObject::invokeMethod(gameSessions.at(idx).wnd.data(), "loadRemoteGame", Qt::QueuedConnection,
                              Q_ARG(QString, value));
    return true;
}

/**
 * Возвращает количество активных игровых сессий
 */
int GameSessions::activeCount() const
{
    int cnt = 0;
    for (int i = 0, size = gameSessions.size(); i < size; i++) {
        if (gameSessions.at(i).status != StatusNone)
            cnt++;
    }
    return cnt;
}

/**
 * Установка статуса сессии игры. Статус может устанавливать только сама игра (окно),
 * т.к. только игра может достоверно знать кто ходит дальше/снова а кто ждет хода и т.д.
 * Этот статус только сессии передачи данных игры и отвечает за минимальную целостность и безопасность.
 * Не имеет ни какого отношения к статусу самой игры.
 */
void GameSessions::setSessionStatus(const QString &status_str)
{
    // Ищем сессию по отославшему статус объекту
    const int idx = findGameSessionByWnd(sender());
    if (idx == -1)
        return;
    // Выставляем статус
    if (status_str == "wait-opponent-command") {
        gameSessions[idx].status = StatusWaitOpponentCommand;
    } else if (status_str == "wait-game-window") {
        gameSessions[idx].status = StatusWaitGameWindow;
    } else if (status_str == "wait-opponent-accept") {
        gameSessions[idx].status = StatusWaitOpponentAccept;
    } else if (status_str == "none") {
        gameSessions[idx].status = StatusNone;
    }
}

/**
 * Мы закрыли игровую доску. Посылаем сигнал оппоненту.
 * Note: Сообщение о закрытии доски отправляется оппоненту только если игра не закончена
 * и не произошла ошибка (например подмена команды)
 */
void GameSessions::closeGameWindow(bool send_for_opponent, int top, int left, int width, int height)
{
    const int idx = findGameSessionByWnd(sender());
    if (idx == -1)
        return;
    if (send_for_opponent) {
        QString id_str               = newId();
        gameSessions[idx].last_iq_id = id_str;
        emit sendStanza(
            gameSessions.at(idx).my_acc,
            QString(
                "<iq type=\"set\" to=\"%1\" id=\"%2\"><close xmlns=\"games:board\" id=\"%3\" type=\"%4\"></close></iq>")
                .arg(XML::escapeString(gameSessions.at(idx).full_jid))
                .arg(id_str)
                .arg(constProtoId)
                .arg(constProtoType));
    }
    gameSessions.removeAt(idx);
    Options *options = Options::instance();
    options->setOption(constWindowTop, top);
    options->setOption(constWindowLeft, left);
    options->setOption(constWindowWidth, width);
    options->setOption(constWindowHeight, height);
}

/**
 * Мы походили, отправляем станзу нашего хода оппоненту
 */
void GameSessions::sendMove(const int x, const int y)
{
    const int idx = findGameSessionByWnd(sender());
    if (idx == -1)
        return;
    QString id_str               = newId();
    gameSessions[idx].last_iq_id = id_str;
    QString stanza = QString("<iq type=\"set\" to=\"%1\" id=\"%2\"><turn xmlns=\"games:board\" type=\"%3\" "
                             "id=\"%4\"><move pos=\"%5,%6\"></move></turn></iq>")
                         .arg(XML::escapeString(gameSessions.at(idx).full_jid))
                         .arg(id_str)
                         .arg(constProtoType)
                         .arg(constProtoId)
                         .arg(x)
                         .arg(y);
    emit sendStanza(gameSessions.at(idx).my_acc, stanza);
}

/**
 * Мы меняем цвет камней
 */
void GameSessions::switchColor()
{
    const int idx = findGameSessionByWnd(sender());
    if (idx == -1)
        return;
    QString id_str               = newId();
    gameSessions[idx].last_iq_id = id_str;
    QString stanza = QString("<iq type=\"set\" to=\"%1\" id=\"%2\"><turn xmlns=\"games:board\" type=\"%3\" "
                             "id=\"%4\"><move pos=\"switch-color\"></move></turn></iq>")
                         .arg(XML::escapeString(gameSessions.at(idx).full_jid))
                         .arg(id_str)
                         .arg(constProtoType)
                         .arg(constProtoId);
    emit sendStanza(gameSessions.at(idx).my_acc, stanza);
}

/**
 * Отправка подтверждения сопернику его хода
 */
void GameSessions::sendAccept()
{
    int idx = findGameSessionByWnd(sender());
    if (idx == -1)
        return;
    QString to_jid = gameSessions.at(idx).full_jid;
    if (to_jid.isEmpty())
        return;
    QString stanza
        = QString("<iq type=\"result\" to=\"%1\" id=\"%2\"><turn type=\"%3\" id=\"%4\" xmlns=\"games:board\"/></iq>")
              .arg(XML::escapeString(to_jid))
              .arg(XML::escapeString(gameSessions.at(idx).last_iq_id))
              .arg(constProtoType)
              .arg(constProtoId);
    emit sendStanza(gameSessions.at(idx).my_acc, stanza);
}

/**
 * Отправка оппоненту сообщения об ошибке
 */
void GameSessions::sendError()
{
    int idx = findGameSessionByWnd(sender());
    if (idx == -1)
        return;
    QString to_jid = gameSessions.at(idx).full_jid;
    if (to_jid.isEmpty())
        return;
    QString id_str               = newId();
    gameSessions[idx].last_iq_id = id_str;
    sendErrorIq(gameSessions.at(idx).my_acc, to_jid, id_str, getLastError());
}

/**
 * Отправка оппоненту запрос на ничью
 */
void GameSessions::sendDraw()
{
    const int idx = findGameSessionByWnd(sender());
    if (idx != -1) {
        GameSession *sess   = &gameSessions[idx];
        QString      new_id = newId();
        sess->last_iq_id    = new_id;
        QString stanza      = QString("<iq type=\"set\" to=\"%1\" id=\"%2\"><turn xmlns=\"games:board\" type=\"%3\" "
                                 "id=\"%4\"><draw/></turn></iq>")
                             .arg(XML::escapeString(sess->full_jid))
                             .arg(new_id)
                             .arg(constProtoType)
                             .arg(constProtoId);
        emit sendStanza(sess->my_acc, stanza);
    }
}

void GameSessions::youLose()
{
    const int idx = findGameSessionByWnd(sender());
    if (idx == -1)
        return;
    QString to_str = gameSessions.at(idx).full_jid;
    if (to_str.isEmpty())
        return;
    QString id_str               = newId();
    gameSessions[idx].last_iq_id = id_str;
    QString stanza = QString("<iq type=\"set\" to=\"%1\" id=\"%2\"><turn xmlns=\"games:board\" type=\"%3\" "
                             "id=\"%4\"><resign/></turn></iq>")
                         .arg(XML::escapeString(to_str))
                         .arg(id_str)
                         .arg(constProtoType)
                         .arg(constProtoId);
    emit sendStanza(gameSessions.at(idx).my_acc, stanza);
}

/**
 * Посылаем оппоненту загруженную игру
 */
void GameSessions::sendLoad(const QString &save_str)
{
    const int idx = findGameSessionByWnd(sender());
    if (idx == -1)
        return;
    QString to_str = gameSessions.at(idx).full_jid;
    if (to_str.isEmpty())
        return;
    QString new_id               = newId();
    gameSessions[idx].last_iq_id = new_id;
    QString stanza
        = QString(
              "<iq type=\"set\" to=\"%1\" id=\"%2\"><load xmlns=\"games:board\" id=\"%3\" type=\"%4\">%5</load></iq>")
              .arg(XML::escapeString(to_str))
              .arg(new_id)
              .arg(constProtoId)
              .arg(constProtoType)
              .arg(save_str);
    emit sendStanza(gameSessions.at(idx).my_acc, stanza);
}

/**
 * Запрос новой игры через игровую доску
 */
void GameSessions::newGame()
{
    const int idx = findGameSessionByWnd(sender());
    if (idx != -1) {
        GameSession *sess   = &gameSessions[idx];
        sess->status        = StatusNone;
        QStringList tmpList = sess->full_jid.split("/");
        QString     jid     = tmpList.takeFirst();
        if (tmpList.size() != 0) {
            invite(sess->my_acc, jid, QStringList(tmpList.join("/")), sess->wnd);
        }
    }
}

// -----------------------------------------

bool GameSessions::regGameSession(const SessionStatus status, const int account, const QString &jid, const QString &id,
                                  const QString &element)
{
    const int cnt = gameSessions.size();
    errorStr      = "";
    for (int i = 0; i < cnt; i++) {
        GameSession *sess = &gameSessions[i];
        if (sess->my_acc == account && sess->full_jid == jid) {
            if (sess->status != StatusNone) {
                errorStr = tr("You are already playing!");
                return false;
            }
            sess->status     = status;
            sess->last_iq_id = id;
            sess->element    = element;
            return true;
        }
    }
    GameSession session;
    session.status     = status;
    session.my_acc     = account;
    session.full_jid   = jid;
    session.last_iq_id = id;
    session.wnd        = nullptr;
    session.element    = element;
    gameSessions.push_back(session);
    return true;
}

int GameSessions::findGameSessionByWnd(QObject *wnd) const
{
    int res = -1;
    int cnt = gameSessions.size();
    for (int i = 0; i < cnt; i++) {
        if (gameSessions.at(i).wnd == wnd) {
            res = i;
            break;
        }
    }
    return res;
}

int GameSessions::findGameSessionById(const int account, const QString &id) const
{
    int res = -1;
    int cnt = gameSessions.size();
    for (int i = 0; i < cnt; i++) {
        if (gameSessions.at(i).last_iq_id == id && gameSessions.at(i).my_acc == account) {
            res = i;
            break;
        }
    }
    return res;
}

int GameSessions::findGameSessionByJid(const int account, const QString &jid) const
{
    int cnt = gameSessions.size();
    for (int i = 0; i < cnt; i++) {
        if (gameSessions.at(i).my_acc == account) {
            if (gameSessions.at(i).full_jid == jid) {
                return i;
            }
        }
    }
    return -1;
}

int GameSessions::findGameSessionByJid(const QString &jid) const
{
    int cnt = gameSessions.size();
    for (int i = 0; i < cnt; i++) {
        if (gameSessions.at(i).full_jid == jid) {
            return i;
        }
    }
    return -1;
}

bool GameSessions::removeGameSession(const int account, const QString &jid)
{
    const int idx = findGameSessionByJid(account, jid);
    if (idx != -1) {
        QPointer<PluginWindow> wnd = gameSessions.at(idx).wnd;
        if (!wnd.isNull()) {
            delete (wnd);
        }
        gameSessions.removeAt(idx);
        return true;
    }
    return false;
}

void GameSessions::sendErrorIq(const int account, const QString &jid, const QString &id, const QString & /*err_str*/)
{
    emit sendStanza(account, XML::iqErrorString(jid, id));
}

QString XML::escapeString(const QString &str) { return str.toHtmlEscaped().replace("\"", "&quot;"); }

QString XML::iqErrorString(const QString &jid, const QString &id)
{
    QString stanza = QString("<iq type=\"error\" to=\"%1\" id=\"%2\"><error type=\"cancel\" code=\"403\"/></iq>")
                         .arg(XML::escapeString(jid))
                         .arg(XML::escapeString(id));
    return stanza;
}

QString GameSessions::newId(const bool big_add)
{
    stanzaId += 1;
    if (big_add) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
        stanzaId += (QRandomGenerator::global()->generate() % 50) + 4;
    } else {
        stanzaId += (QRandomGenerator::global()->generate() % 5) + 1;
#else
        stanzaId += (qrand() % 50) + 4;
    } else {
        stanzaId += (qrand() % 5) + 1;
#endif
    }
    return "gg_" + QString::number(stanzaId);
}

QString GameSessions::getLastError() const { return errorStr; }
