/*
 * gamesessions.h - Gomoku Game plugin
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

#ifndef GAMESESSIONS_H
#define GAMESESSIONS_H

#include <QObject>
#include <QPointer>
#include <QDomElement>

#include "pluginwindow.h"

#define constProtoId    "gomoku_01"

namespace XML {
    QString escapeString(const QString &str);
    QString iqErrorString(const QString &jid, const QString &id);
}

class GameSessions : public QObject
{
Q_OBJECT

public:
    explicit GameSessions(QObject *parent = 0);
    ~GameSessions();
    static GameSessions *instance();
    static void reset();
    bool processIncomingIqStanza(int accont, const QDomElement& xml, const QString &acc_status, bool conf_priv);
    void invite(int account, const QString &jid, const QStringList &res_list, QWidget *parent = nullptr);
    int  activeCount() const;

private:
    enum SessionStatus {
        StatusNone,
        StatusInviteOutDialog,      // Окно приглашения с нашей стороны
        StatusInviteSend,           // Отправлено приглашение
        StatusInviteInDialog,       // Окно приглашения к нам (входящий инвайт)
        StatusWaitOpponentCommand,  // Ожидается входящая команда от оппонента (ход, сдача, загрузка игры и т.д.)
        StatusWaitGameWindow,       // Ожидается активность от окна игры
        StatusWaitOpponentAccept    // Ожидается входящий ответ от оппонента на нашу команду (Accept или Error)
    };
    struct GameSession {
        SessionStatus          status;
        int                    my_acc;
        QString                full_jid;
        QPointer<PluginWindow> wnd;
        QString                last_iq_id;
        QString                element;
    };
    QList<GameSession> gameSessions;
    int  stanzaId;
    static GameSessions *instance_;
    QString errorStr;

private:
    bool incomingInvitation(const int account, const QString &from, const QString &color, const QString &iq_id);
    bool doResult(const int account, const QString &from, const QString &iq_id);
    bool doReject(const int account, const QString &from, const QString &iq_id);
    bool doTurnAction(const int account, const QString &from, const QString &iq_id, const QString &value);
    bool youWin(const int account, const QString &from, const QString &iq_id);
    bool setDraw(const int account, const QString &from, const QString &iq_id);
    bool closeRemoteGameBoard(const int account, const QString &from, const QString &iq_id);
    bool remoteLoad(const int account, const QString &from, const QString &iq_id, const QString &value);
    bool regGameSession(const SessionStatus status, const int account, const QString &jid, const QString &id = "", const QString &element = "");
    void startGame(const int sess_index);
    int  findGameSessionByWnd(QObject *wnd) const;
    int  findGameSessionById(const int account, const QString &id) const;
    int  findGameSessionByJid(const int account, const QString &jid) const;
    int  findGameSessionByJid(const QString &jid) const;
    bool removeGameSession(const int account, const QString &jid);
    QString newId(const bool big_add = false);
    QString getLastError() const;
    void sendErrorIq(const int account, const QString &jid, const QString &id, const QString &/*err_str*/);

private slots:
    void showInvitation(const QString &from);
    void doInviteDialog(const int account, const QString &from);
    void sendInvite(const int account, const QString &full_jid, const QString &element);
    void cancelInvite(const int account, const QString &full_jid);
    void acceptInvite(const int, const QString &);
    void rejectInvite(const int, const QString &);
    void setSessionStatus(const QString &);
    void closeGameWindow(bool, int, int, int, int);
    void sendMove(const int, const int);
    void switchColor();
    void sendAccept();
    void sendError();
    void sendDraw();
    void youLose();
    void sendLoad(const QString &);
    void newGame();

signals:
    void sendStanza(int, QString);
    void doInviteEvent(int, QString, QString, QObject *, const char *);
    void doPopup(const QString);
    void playSound(const QString);

};

#endif // GAMESESSIONS_H
