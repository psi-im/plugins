/*
 * pluginwindow.h - Gomoku Game plugin
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

#ifndef PLUGINWINDOW_H
#define PLUGINWINDOW_H

#include <QCloseEvent>
#include <QMainWindow>

#include "boarddelegate.h"
#include "boardmodel.h"
#include "gameelement.h"

namespace Ui {
class PluginWindow;
}

using namespace GomokuGame;

class HintElementWidget : public QFrame {
    Q_OBJECT
public:
    HintElementWidget(QWidget *parent = nullptr);
    ~HintElementWidget();
    void setElementType(GameElement::ElementType type);

private:
    GameElement *hintElement;

protected:
    virtual void paintEvent(QPaintEvent *event);
};

class PluginWindow : public QMainWindow {
    Q_OBJECT
public:
    PluginWindow(const QString &full_jid, QWidget *parent = nullptr);
    ~PluginWindow();
    void init(const QString &element);

private:
    Ui::PluginWindow *ui;
    BoardModel       *bmodel;
    BoardDelegate    *delegate;
    bool              gameActive;

private:
    void endGame();
    void appendTurn(const int num, const int x, const int y, const bool my_turn);
    bool tryLoadGame(const QString &load_str, const bool local);
    void showDraw();

protected:
    virtual void closeEvent(QCloseEvent *event);

private slots:
    void changeGameStatus(const GameModel::GameStatus status);
    void turnSelected();
    void setupElement(const int x, const int y);
    void acceptStep();
    void setAccept();
    void setError();
    void setTurn(const int, const int);
    void setSwitchColor();
    void doSwitchColor();
    void setLose();
    void setDraw();
    void setResign();
    void setWin();
    void setClose();
    void newGame();
    void saveGame();
    void loadGame();
    void loadRemoteGame(const QString &);
    void opponentDraw();
    void setSkin();

signals:
    void changeGameSession(QString);
    void closeBoard(bool, int, int, int, int);
    void setElement(int, int);
    void accepted();
    void error();
    void lose();
    void draw();
    void switchColor();
    void load(QString);
    void sendNewInvite();
    void doPopup(const QString);
    void playSound(const QString);
};

#endif // PLUGINWINDOW_H
