/*
 * noughtsandcrossesplugin.cpp - Psi plugin to play noughts and crosses
 * Copyright (C) 2006  Kevin Smith
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

#include <QDebug>
#include <QGridLayout>
#include <QLabel>
#include <QtCore>
#include <QtGui>

#include "eventfilter.h"
#include "psiplugin.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"

#include "tictac.h"

class NoughtsAndCrossesPlugin : public QObject, public PsiPlugin, public EventFilter, public StanzaSender {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.psi-plus.NoughtsAndCrosses" FILE "psiplugin.json")
    Q_INTERFACES(PsiPlugin EventFilter StanzaSender)

public:
    NoughtsAndCrossesPlugin();

    virtual QString  name() const;
    virtual QWidget *options();
    virtual bool     enable();
    virtual bool     disable();

    virtual void setStanzaSendingHost(StanzaSendingHost *host);
    virtual bool processEvent(int account, QDomElement &e);
    virtual bool processMessage(int account, const QString &fromJid, const QString &body, const QString &subject);
    virtual bool processOutgoingMessage(int account, const QString &fromJid, QString &body, const QString &type,
                                        QString &subject)
    {
        Q_UNUSED(account);
        Q_UNUSED(fromJid);
        Q_UNUSED(body);
        Q_UNUSED(type);
        Q_UNUSED(subject);
        return false;
    };
    virtual void    logout(int account) { Q_UNUSED(account); };
    virtual void    applyOptions() {};
    virtual void    restoreOptions() {};
    virtual QString pluginInfo();

private slots:
    void stopGame();
    void myTurn(int space);
    void theirTurn(int space);
    void gameOver(TicTacGameBoard::State state);

private:
    void startGame(QString jid, int size, bool meFirst, int account);

    QWidget           *optionsWid;
    TicTacToe         *game;
    QString            playingWith;
    int                account_;
    bool               enabled_;
    StanzaSendingHost *stanzaSender_;
};

// Q_EXPORT_PLUGIN(NoughtsAndCrossesPlugin);

NoughtsAndCrossesPlugin::NoughtsAndCrossesPlugin()
{
    game          = NULL;
    enabled_      = false;
    stanzaSender_ = 0;
    optionsWid    = 0;
}

QString NoughtsAndCrossesPlugin::name() const { return "NoughtsAndCrosses Plugin"; }

QWidget *NoughtsAndCrossesPlugin::options()
{
    if (!enabled_) {
        return 0;
    }
    optionsWid          = new QWidget();
    QGridLayout *layout = new QGridLayout(optionsWid);
    layout->addWidget(new QLabel(tr("Send command: noughtsandcrosses start")));
    return optionsWid;
}

bool NoughtsAndCrossesPlugin::enable()
{
    if (stanzaSender_) {
        enabled_ = true;
    }
    return enabled_;
}

bool NoughtsAndCrossesPlugin::disable()
{
    stopGame();
    enabled_ = false;
    return true;
}

void NoughtsAndCrossesPlugin::setStanzaSendingHost(StanzaSendingHost *host) { stanzaSender_ = host; }

bool NoughtsAndCrossesPlugin::processMessage(int account, const QString &fromJid, const QString &message,
                                             const QString &subject)
{
    // FIXME(mck)
    QString fromDisplay = fromJid;

    Q_UNUSED(subject);

    if (!enabled_) {
        return false;
    }

    QString reply;
    qDebug("naughtsandcrosses message");
    if (!message.startsWith("noughtsandcrosses", Qt::CaseInsensitive))
        return false;
    qDebug("message for us in noughtsandcrosses");
    if (game && fromJid != playingWith) {
        reply = QString("<message to=\"%1\" type=\"chat\"><body>already playing with %2, sorry</body></message>")
                    .arg(fromJid, playingWith);
        stanzaSender_->sendStanza(account, reply);
        return true;
    }
    QString command = QString(message).toLower();
    command.remove(0, 18);
    qDebug() << QString("noughtsandcrosses command string %1").arg(command);
    if (command == QString("start")) {
        if (game)
            return true;
        qWarning() << QString("Received message '%1', launching nac with %2").arg(message, fromDisplay);
        QString reply;
        reply = QString("<message to=\"%1\" type=\"chat\"><body>noughtsandcrosses starting</body></message>")
                    .arg(fromJid);
        stanzaSender_->sendStanza(account, reply);
        startGame(fromJid, 3, false, account);
    } else if (command == QString("starting")) {
        if (game)
            return true;
        qWarning() << QString("Received message '%1', launching nac with %2").arg(message, fromDisplay);
        QString reply;
        reply
            = QString(
                  "<message to=\"%1\" type=\"chat\"><body>starting noughts and crosses, I go first :)</body></message>")
                  .arg(fromJid);
        stanzaSender_->sendStanza(account, reply);
        startGame(fromJid, 3, true, account);
    } else if (!game) {
        return true;
    } else if (command.startsWith("move")) {
        command.remove(0, 5);

        int space = command.toInt();
        qDebug() << QString("noughtsandcrosses move to space %1").arg(space);
        theirTurn(space);
    }
    return true;
}

void NoughtsAndCrossesPlugin::startGame(QString jid, int size, bool meFirst, int account)
{
    game = new TicTacToe(meFirst, size);
    game->setWindowTitle(QString("Noughts and Crosses with %1").arg(jid));
    playingWith = jid;
    game->show();
    account_ = account;
    connect(game, &TicaTacToe::closing, this, &NoughtsAndCrossesPlugin::stopGame);
    connect(game, &TicaTacToe::myMove, this, &NoughtsAndCrossesPlugin::myTurn);
    connect(game, &TicaTacToe::gameOverSignal, this, &NoughtsAndCrossesPlugin::gameOver);
}

void NoughtsAndCrossesPlugin::stopGame()
{
    delete game;
    game = NULL;
}

void NoughtsAndCrossesPlugin::gameOver(TicTacGameBoard::State state)
{
    QString reply;
    QString winner;
    switch (state) {
    case TicTacGameBoard::HumanWon:
        winner = "I";
        break;
    case TicTacGameBoard::ComputerWon:
        winner = "You";
        break;
    case TicTacGameBoard::NobodyWon:
        winner = "It was a draw, no-one";
        break;
    default:
        winner = "ERROR!!!";
    }
    reply = QString("<message to=\"%1\" type=\"chat\"><body>%2 won. Good game.</body></message>")
                .arg(playingWith, winner);
    stanzaSender_->sendStanza(account_, reply);
}

void NoughtsAndCrossesPlugin::myTurn(int space)
{
    qDebug() << QString("my turn: %1").arg(space);
    if (!game)
        return;
    QString reply;
    reply = QString("<message to=\"%1\" type=\"chat\"><body>noughtsandcrosses move %2</body></message>")
                .arg(playingWith, space);
    stanzaSender_->sendStanza(account_, reply);
}

void NoughtsAndCrossesPlugin::theirTurn(int space)
{
    qDebug() << QString("their turn: %1").arg(space);
    if (!game)
        return;
    game->theirMove(space);
}

bool NoughtsAndCrossesPlugin::processEvent(int acc, QDomElement &e)
{
    Q_UNUSED(acc);
    Q_UNUSED(e);
    return false;
}

QString NoughtsAndCrossesPlugin::pluginInfo() { return tr("Psi plugin to play noughts and crosses"); }

#include "noughtsandcrossesplugin.moc"
