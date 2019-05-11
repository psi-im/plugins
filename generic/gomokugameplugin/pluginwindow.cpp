/*
 * pluginwindow.cpp - Gomoku Game plugin
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

#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

#include "pluginwindow.h"
#include "ui_pluginwindow.h"
#include "common.h"
#include "options.h"
#include "gamemodel.h"

const QString fileFilter = "Gomoku save files (*.gmk)";

//-------------------------- HintElementWidget -------------------------

HintElementWidget::HintElementWidget(QWidget *parent) :
    QFrame(parent),
    hintElement(nullptr)
{
}

HintElementWidget::~HintElementWidget()
{
    if (hintElement)
        delete hintElement;
}

void HintElementWidget::setElementType(GameElement::ElementType type)
{
    if (hintElement)
        delete hintElement;
    hintElement = new GameElement(type, 0, 0);
    QFrame::update();
}

void HintElementWidget::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);
    if (!hintElement)
        return;
    QRect rect = this->rect();
    QPainter painter(this);
    hintElement->paint(&painter, rect);
}

//------------------------ PluginWindow --------------------------

PluginWindow::PluginWindow(const QString &full_jid, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PluginWindow),
    bmodel(nullptr),
    delegate(nullptr),
    gameActive(false)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    ui->lbOpponent->setText(full_jid);
}

PluginWindow::~PluginWindow()
{
    delete ui;
}

void PluginWindow::init(const QString &element)
{
    GameElement::ElementType elemType;
    if (element == "white") {
        elemType = GameElement::TypeWhite;
    } else {
        elemType = GameElement::TypeBlack;
    }
    // Инициируем модель доски
    if (bmodel == nullptr) {
        bmodel = new BoardModel(this);
        connect(bmodel, SIGNAL(changeGameStatus(GameModel::GameStatus)), this, SLOT(changeGameStatus(GameModel::GameStatus)));
        connect(bmodel, SIGNAL(setupElement(int, int)), this, SLOT(setupElement(int, int)));
        connect(bmodel, SIGNAL(lose()), this, SLOT(setLose()), Qt::QueuedConnection);
        connect(bmodel, SIGNAL(draw()), this, SLOT(setDraw()), Qt::QueuedConnection);
        connect(bmodel, SIGNAL(switchColor()), this, SIGNAL(switchColor()));
        connect(bmodel, SIGNAL(doPopup(const QString)), this, SIGNAL(doPopup(const QString)));
    }
    bmodel->init(new GameModel(elemType, 15, 15)); // GameModel убивается при уничтожении BoardModel
    ui->board->setModel(bmodel);
    // Создаем делегат
    if (delegate == nullptr) {
        delegate = new BoardDelegate(bmodel, ui->board); // Прописан родитель
    }
    // Инициируем BoardView
    ui->board->setItemDelegate(delegate);
    ui->board->reset();
    // Объекты GUI
    ui->hintElement->setElementType(elemType);
    ui->actionNewGame->setEnabled(false);
    ui->actionResign->setEnabled(true);
    ui->actionSwitchColor->setEnabled(false);
    ui->lsTurnsList->clear();
    //--
    emit playSound(constSoundStart);
    gameActive = true;
}

void PluginWindow::changeGameStatus(const GameModel::GameStatus status)
{
    int step = bmodel->turnNum();
    if (step == 4) {
        if (status == GameModel::StatusWaitingLocalAction && bmodel->myElementType() == GameElement::TypeWhite) {
            ui->actionSwitchColor->setEnabled(true);
        }
    } else if (step == 5) {
        ui->actionSwitchColor->setEnabled(false);
    }
    QString stat_str = "n/a";
    if (status == GameModel::StatusWaitingOpponent) {
        stat_str = tr("Waiting for opponent");
        ui->actionResign->setEnabled(true);
        emit changeGameSession("wait-opponent-command");
    } else if (status == GameModel::StatusWaitingAccept) {
        stat_str = tr("Waiting for accept");
        emit changeGameSession("wait-opponent-accept");
    } else if (status == GameModel::StatusWaitingLocalAction) {
        stat_str = tr("Your turn");
        emit changeGameSession("wait-game-window");
        ui->actionResign->setEnabled(true);
        emit playSound(constSoundMove);
    } else if (status == GameModel::StatusBreak) {
        stat_str = tr("End of game");
        endGame();
    } else if (status == GameModel::StatusError) {
        stat_str = tr("Error");
        endGame();
    } else if (status == GameModel::StatusWin) {
        stat_str = tr("Win!");
        endGame();
    } else if (status == GameModel::StatusLose) {
        stat_str = tr("Lose.");
        endGame();
    } else if (status == GameModel::StatusDraw) {
        stat_str = tr("Draw.");
        endGame();
    }
    ui->lbStatus->setText(stat_str);
}

void PluginWindow::endGame()
{
    gameActive = false;
    ui->actionResign->setEnabled(false);
    ui->actionNewGame->setEnabled(true);
    emit changeGameSession("none");
    emit playSound(constSoundFinish);
}

/**
 * В списке ходов сменился активный элемент
 */
void PluginWindow::turnSelected()
{
    QListWidgetItem *item = ui->lsTurnsList->currentItem();
    if (item) {
        bmodel->setSelect(item->data(Qt::UserRole).toInt(), item->data(Qt::UserRole + 1).toInt());
    }
}

/**
 * Пришел сигнал с модели доски об установки игроком нового элемента
 */
void PluginWindow::setupElement(const int x, const int y)
{
    appendTurn(bmodel->turnNum() - 1, x, y, true);
    emit setElement(x, y);
}

/**
 * Добавление хода в список ходов
 */
void PluginWindow::appendTurn(const int num, const int x, const int y, const bool my_turn)
{
    QString str1;
    if (my_turn) {
        str1 = tr("You");
    } else {
        str1 = tr("Opp", "Opponent");
    }
    QString msg;
    if (x == -1 && y == -1) {
        msg = tr("%1: %2 - swch", "Switch color").arg(num).arg(str1);
    } else {
        msg = QString("%1: %2 - %3%4").arg(num).arg(str1)
            .arg(horHeaderString.at(x))
            .arg(QString::number(y + 1));
    }
    QListWidgetItem *item = new QListWidgetItem(msg, ui->lsTurnsList);
    item->setData(Qt::UserRole, x);
    item->setData(Qt::UserRole + 1, y);
    //item->setFlags((item->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsUserCheckable);
    ui->lsTurnsList->addItem(item);
    ui->lsTurnsList->setCurrentItem(item);
}

/**
 * Подтверждение последнего хода в списке
 */
void PluginWindow::acceptStep()
{
    // Визуальное отображение подтвержденного хода
}

/**
 * Пришло подтверждение от оппонента
 */
void PluginWindow::setAccept()
{
    bmodel->setAccept();
    acceptStep();
}

/**
 * Пришел ход от противника
 */
void PluginWindow::setTurn(const int x, const int y)
{
    if (bmodel) {
        if (bmodel->opponentTurn(x, y)) {
            appendTurn(bmodel->turnNum() - 1, x, y, false);
            emit accepted();
            if (bmodel->turnNum() == 4) { // Ходы сквозные, значит 4й всегда белые
                ui->actionSwitchColor->setEnabled(true);
                doSwitchColor();
            }
            return;
        }
    }
    emit error();
}

/**
 * Оппонент поменял цвет
 */
void PluginWindow::setSwitchColor()
{
    if (bmodel->doSwitchColor(false)) {
        ui->hintElement->setElementType(GameElement::TypeWhite);
        appendTurn(bmodel->turnNum() - 1, -1, -1, false);
        emit accepted();
    } else {
        emit error();
    }
}

/**
 * Пришла ошибка от противника
 */
void PluginWindow::setError()
{
    bmodel->setError();
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setIcon(QMessageBox::Warning);
    msgBox->setWindowTitle(tr("Gomoku Plugin"));
    msgBox->setText(tr("Game Error!"));
    msgBox->setStandardButtons(QMessageBox::Ok);
    msgBox->setWindowModality(Qt::WindowModal);
    msgBox->exec();
    delete msgBox;
}

/**
 * Оппонент закрыл игровую доску.
 */
void PluginWindow::setClose()
{
    bmodel->setClose();
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setIcon(QMessageBox::Warning);
    msgBox->setWindowTitle(tr("Gomoku Plugin"));
    msgBox->setText(tr("Your opponent has closed the board!\n You can still save the game."));
    msgBox->setStandardButtons(QMessageBox::Ok);
    msgBox->setWindowModality(Qt::WindowModal);
    msgBox->exec();
    delete msgBox;
}

/**
 * Реакция на закрытие нашей доски
 */
void PluginWindow::closeEvent (QCloseEvent *event)
{
    emit closeBoard(gameActive, y(), x(), width(), height()); // Отправляем сообщение оппоненту
    gameActive = false;
    event->accept();
}

/**
 * Предлагаем сменить цвет
 */
void PluginWindow::doSwitchColor()
{
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setIcon(QMessageBox::Question);
    msgBox->setWindowTitle(tr("Gomoku Plugin"));
    msgBox->setText(tr("You want to switch color?"));
    msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox->setDefaultButton(QMessageBox::No);
    msgBox->setWindowModality(Qt::WindowModal);
    int res = msgBox->exec();
    delete msgBox;
    if (res == QMessageBox::Yes) {
        if (bmodel->doSwitchColor(true)) {
            ui->hintElement->setElementType(GameElement::TypeBlack);
            appendTurn(bmodel->turnNum() - 1, -1, -1, true);
        }
    }
}

/**
 * Мы проиграли выставляем сигнал и показываем сообщение
 */
void PluginWindow::setLose()
{
    emit lose();
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setIcon(QMessageBox::Information);
    msgBox->setWindowTitle(tr("Gomoku Plugin"));
    msgBox->setText(tr("You Lose."));
    msgBox->setStandardButtons(QMessageBox::Ok);
    msgBox->setWindowModality(Qt::WindowModal);
    msgBox->exec();
    delete msgBox;
}

/**
 * Ничья выставляем сигнал и показываем сообщение
 */
void PluginWindow::setDraw()
{
    emit draw();
    showDraw();
}

/**
 * Мы сдались (выбран пункт меню "Resign" на доске)
 */
void PluginWindow::setResign()
{
    bmodel->setResign();
}

/**
 * Сообщение о том что оппонент проиграл, т.е. мы выиграли
 */
void PluginWindow::setWin()
{
    bmodel->setWin();
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setIcon(QMessageBox::Information);
    msgBox->setWindowTitle(tr("Gomoku Plugin"));
    msgBox->setText(tr("You Win!"));
    msgBox->setStandardButtons(QMessageBox::Ok);
    msgBox->setWindowModality(Qt::WindowModal);
    msgBox->exec();
    delete msgBox;
}

/**
 * Обработчик начала новой игры. Запрос у пользователя и отсылка сигнала
 */
void PluginWindow::newGame()
{
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setIcon(QMessageBox::Question);
    msgBox->setWindowTitle(tr("Gomoku Plugin"));
    msgBox->setText(tr("You really want to begin new game?"));
    msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox->setWindowModality(Qt::WindowModal);
    int res = msgBox->exec();
    delete msgBox;
    if (res == QMessageBox::Yes) {
        emit sendNewInvite();
    }
}

/**
 * Обработчик сохранения игры
 */
void PluginWindow::saveGame()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save game"), "", fileFilter);
    if (fileName.isEmpty())
        return;
    if (fileName.right(4) != ".gmk")
        fileName.append(".gmk");
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream out(&file);
        out.setCodec("UTF-8");
        out.setGenerateByteOrderMark(false);
        out << bmodel->saveToString();
    }
}

/**
 * Обработчик загрузки игры с локального файла
 */
void PluginWindow::loadGame()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load game"), "", fileFilter);
    if (fileName.isEmpty())
        return;
    QFile file(fileName);
    if(file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        in.setCodec("UTF-8");
        QString saved_str = in.readAll();
        saved_str.replace("\n", "");
        if (tryLoadGame(saved_str, true)) {
            emit load(saved_str);
        }
    }
}

/**
 * Обработчик загрузки игры, посланной оппонентом
 */
void PluginWindow::loadRemoteGame(const QString &load_str)
{
    if (tryLoadGame(load_str, false)) {
        emit accepted();
        return;
    }
    emit error();
}

/**
 * Попытка создать модель игры по данным из строки load_str
 * При удачной попытке модель игровой доски инициируется с новыми данными игры
 */
bool PluginWindow::tryLoadGame(const QString &load_str, const bool local)
{
    if (!load_str.isEmpty()) {
        GameModel *gm = new GameModel(load_str, local);
        if (gm->isValid()) {
            QString info = gm->gameInfo();
            QMessageBox *msgBox = new QMessageBox(this);
            msgBox->setIcon(QMessageBox::Question);
            msgBox->setWindowTitle(tr("Gomoku Plugin"));
            info.append("\n").append(tr("You really want to begin loaded game?"));
            msgBox->setText(info);
            msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox->setWindowModality(Qt::WindowModal);
            int res = msgBox->exec();
            delete msgBox;
            if (res == QMessageBox::Yes) {
                // Инициализация модели игровой доски новыми данными
                bmodel->init(gm);
                ui->hintElement->setElementType(gm->myElementType());
                // Загрузка списка ходов
                ui->lsTurnsList->clear();
                for (int i = 1, cnt = gm->turnsCount(); i <= cnt; i++) {
                    GameModel::TurnInfo turn = gm->turnInfo(i);
                    appendTurn(i, turn.x, turn.y, turn.my);
                }
                return true;
            }
        }
        delete gm;
    }
    return false;
}

/**
 * Выбрано новое оформление (Скин)
 */
void PluginWindow::setSkin()
{
    QObject *sender_ = sender();
    if (sender_ == ui->actionSkin0) {
        ui->actionSkin0->setChecked(true);
        ui->actionSkin1->setChecked(false);
        delegate->setSkin(0);
    } else if (sender_ == ui->actionSkin1) {
        ui->actionSkin1->setChecked(true);
        ui->actionSkin0->setChecked(false);
        delegate->setSkin(1);
    }
    ui->board->repaint();
}

/**
 * Обработчик предложения оппонентом ничьей
 */
void PluginWindow::opponentDraw()
{
    bmodel->opponentDraw();
    showDraw();
}

/**
 * Отображение стандартного диалогового окна, сообщающего об ничьей
 */
void PluginWindow::showDraw()
{
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setIcon(QMessageBox::Information);
    msgBox->setWindowTitle(tr("Gomoku Plugin"));
    msgBox->setText(tr("Draw."));
    msgBox->setStandardButtons(QMessageBox::Ok);
    msgBox->setWindowModality(Qt::WindowModal);
    msgBox->exec();
    delete msgBox;
}
