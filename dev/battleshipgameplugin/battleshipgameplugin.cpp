/*
 * battleshipgameplugin.cpp - Battleship Game plugin
 * Copyright (C) 2014  Aleksey Andreev
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

#include "battleshipgameplugin.h"
#include "gamesessions.h"
#include "options.h"

#include <QFileDialog>

#define constVersion "0.0.1"
#define constShortPluginName "battleshipgameplugin"
#define constPluginName "Battleship Game Plugin"

BattleshipGamePlugin::BattleshipGamePlugin(QObject *parent) :
    QObject(parent), enabled_(false), psiTab(nullptr), psiIcon(nullptr), psiAccInfo(nullptr), psiContactInfo(nullptr),
    psiSender(nullptr), psiEvent(nullptr), psiSound(nullptr), psiPopup(nullptr)
{
    Options::psiOptions = nullptr;
}

QString BattleshipGamePlugin::name() const { return constPluginName; }

QString BattleshipGamePlugin::shortName() const { return constShortPluginName; }

QString BattleshipGamePlugin::version() const { return constVersion; }

QWidget *BattleshipGamePlugin::options()
{
    QWidget *options = new QWidget;
    ui_.setupUi(options);
    ui_.play_error->setIcon(psiIcon->getIcon("psi/play"));
    ui_.play_finish->setIcon(psiIcon->getIcon("psi/play"));
    ui_.play_move->setIcon(psiIcon->getIcon("psi/play"));
    ui_.play_start->setIcon(psiIcon->getIcon("psi/play"));
    ui_.select_error->setIcon(psiIcon->getIcon("psi/browse"));
    ui_.select_finish->setIcon(psiIcon->getIcon("psi/browse"));
    ui_.select_move->setIcon(psiIcon->getIcon("psi/browse"));
    ui_.select_start->setIcon(psiIcon->getIcon("psi/browse"));
    restoreOptions();
    connect(ui_.play_error, SIGNAL(clicked()), this, SLOT(testSound()));
    connect(ui_.play_finish, SIGNAL(clicked()), this, SLOT(testSound()));
    connect(ui_.play_move, SIGNAL(clicked()), this, SLOT(testSound()));
    connect(ui_.play_start, SIGNAL(clicked()), this, SLOT(testSound()));
    connect(ui_.select_error, SIGNAL(clicked()), this, SLOT(getSound()));
    connect(ui_.select_finish, SIGNAL(clicked()), this, SLOT(getSound()));
    connect(ui_.select_start, SIGNAL(clicked()), this, SLOT(getSound()));
    connect(ui_.select_move, SIGNAL(clicked()), this, SLOT(getSound()));
    return options;
}

bool BattleshipGamePlugin::enable()
{
    if (enabled_)
        return true;
    // Грузим иконку плагина
    QFile file(":/battleshipgameplugin/battleship");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray ico = file.readAll();
        psiIcon->addIcon("battleshipgameplugin/battleship", ico);
        file.close();
    }
    // Создаем соединения с менеджером игровых сессий
    GameSessionList *gsl = GameSessionList::instance();
    connect(gsl, SIGNAL(sendStanza(int, QString)), this, SLOT(sendGameStanza(int, QString)), Qt::QueuedConnection);
    connect(gsl, SIGNAL(doPopup(QString)), this, SLOT(doPopup(QString)), Qt::QueuedConnection);
    connect(gsl, SIGNAL(playSound(QString)), this, SLOT(playSound(QString)), Qt::QueuedConnection);
    connect(gsl, SIGNAL(doInviteEvent(int, QString, QString, QObject *, const char *)), this,
            SLOT(doPsiEvent(int, QString, QString, QObject *, const char *)), Qt::QueuedConnection);

    // Выставляем флаг и уходим
    enabled_ = true;
    return true;
}

bool BattleshipGamePlugin::disable()
{
    enabled_ = false;
    GameSessionList::reset();
    Options::reset();
    return true;
}

void BattleshipGamePlugin::applyOptions()
{
    Options *options = Options::instance();
    options->setOption(constDefSoundSettings, ui_.cb_sound_override->isChecked());
    options->setOption(constSoundStart, ui_.le_start->text());
    options->setOption(constSoundFinish, ui_.le_finish->text());
    options->setOption(constSoundMove, ui_.le_move->text());
    options->setOption(constSoundError, ui_.le_error->text());
    options->setOption(constDndDisable, ui_.cb_disable_dnd->isChecked());
    options->setOption(constConfDisable, ui_.cb_disable_conf->isChecked());
    options->setOption(constSaveWndPosition, ui_.cb_save_pos->isChecked());
    options->setOption(constSaveWndWidthHeight, ui_.cb_save_w_h->isChecked());
}

void BattleshipGamePlugin::restoreOptions()
{
    Options *options = Options::instance();
    ui_.cb_sound_override->setChecked(options->getOption(constDefSoundSettings).toBool());
    ui_.le_start->setText(options->getOption(constSoundStart).toString());
    ui_.le_finish->setText(options->getOption(constSoundFinish).toString());
    ui_.le_move->setText(options->getOption(constSoundMove).toString());
    ui_.le_error->setText(options->getOption(constSoundError).toString());
    ui_.cb_disable_dnd->setChecked(options->getOption(constDndDisable).toBool());
    ui_.cb_disable_conf->setChecked(options->getOption(constConfDisable).toBool());
    ui_.cb_save_pos->setChecked(options->getOption(constSaveWndPosition).toBool());
    ui_.cb_save_w_h->setChecked(options->getOption(constSaveWndWidthHeight).toBool());
}

QPixmap BattleshipGamePlugin::icon() const { return QPixmap(":/battleshipgameplugin/battleship"); }

/**
 * Получение списка ресурсов и вызов формы для отправки приглашения
 */
void BattleshipGamePlugin::inviteDlg(const int account, const QString &full_jid)
{
    QString bareJid = full_jid.section('/', 0, 0);
    // QStringList jid_parse = full_jid.split("/");
    // QString jid = jid_parse.takeFirst();
    if (bareJid.isEmpty())
        return;
    QStringList resList;
    if (psiContactInfo->isPrivate(account, full_jid)) {
        // This is conference
        QString res = full_jid.section('/', 1);
        if (res.isEmpty())
            return;
        resList.append(res);
    } else {
        // Получаем список ресурсов оппонента
        resList = psiContactInfo->resources(account, bareJid);
    }
    // Отображение окна отправки приглашения
    GameSessionList::instance()->invite(account, bareJid, resList);
}

// ------------------------------------------ Slots ------------------------------------------

/**
 * Кто-то кликнул по кнопке тулбара в окне чата
 */
void BattleshipGamePlugin::toolButtonPressed()
{
    if (!enabled_)
        return;
    // Получаем наш account id
    QString jid     = psiTab->getYourJid();
    int     account = -1;
    for (int i = 0;; i++) {
        QString str1 = psiAccInfo->getJid(i);
        if (str1 == jid) {
            account = i;
            break;
        }
        if (str1 == "-1")
            return;
    }
    // Проверяем статус аккаунта
    if (account == -1 || psiAccInfo->getStatus(account) == "offline")
        return;
    // --
    inviteDlg(account, psiTab->getJid());
}

/**
 * Кто-то выбрал плагин в меню ростера
 */
void BattleshipGamePlugin::menuActivated()
{
    if (!enabled_)
        return;
    int account = sender()->property("account").toInt();
    if (psiAccInfo->getStatus(account) == "offline")
        return;
    QString jid = sender()->property("jid").toString();
    inviteDlg(account, jid);
}

/**
 * Создания события для приглашения
 */
void BattleshipGamePlugin::doPsiEvent(int account, QString from, QString text, QObject *receiver, const char *method)
{
    psiEvent->createNewEvent(account, from, text, receiver, method);
}

/**
 * Отсылка станзы по запросу игры
 */
void BattleshipGamePlugin::sendGameStanza(const int account, const QString &stanza)
{
    if (enabled_ && psiAccInfo->getStatus(account) != "offline")
        psiSender->sendStanza(account, stanza);
}

void BattleshipGamePlugin::testSound()
{
    QObject *sender_ = sender();
    if (sender_ == (ui_.play_error)) {
        psiSound->playSound(ui_.le_error->text());
    } else if (sender_ == ui_.play_finish) {
        psiSound->playSound(ui_.le_finish->text());
    } else if (sender_ == ui_.play_move) {
        psiSound->playSound(ui_.le_move->text());
    } else if (sender_ == ui_.play_start) {
        psiSound->playSound(ui_.le_start->text());
    }
}

void BattleshipGamePlugin::getSound()
{
    QObject *  sender_ = sender();
    QLineEdit *le      = nullptr;
    if (sender_ == ui_.select_error) {
        le = ui_.le_error;
    } else if (sender_ == ui_.select_finish) {
        le = ui_.le_finish;
    } else if (sender_ == ui_.select_move) {
        le = ui_.le_move;
    } else if (sender_ == ui_.select_start) {
        le = ui_.le_start;
    }
    if (!le)
        return;
    QString file_name = QFileDialog::getOpenFileName(nullptr, tr("Choose a sound file"), "", tr("Sound (*.wav)"));
    if (file_name.isEmpty())
        return;
    le->setText(file_name);
}

void BattleshipGamePlugin::doPopup(const QString &text)
{
    psiPopup->initPopup(text, tr(constPluginName), "battleshipgameplugin/battleship");
}

void BattleshipGamePlugin::playSound(const QString &sound_id)
{
    Options *options = Options::instance();
    if (options->getOption(constDefSoundSettings).toBool()
        || Options::psiOptions->getGlobalOption("options.ui.notifications.sounds.enable").toBool()) {
        if (sound_id == constSoundMove)
            psiSound->playSound(options->getOption(constSoundMove).toString());
        else if (sound_id == constSoundStart)
            psiSound->playSound(options->getOption(constSoundStart).toString());
        else if (sound_id == constSoundFinish)
            psiSound->playSound(options->getOption(constSoundFinish).toString());
        else if (sound_id == constSoundError)
            psiSound->playSound(options->getOption(constSoundError).toString());
    }
}

// --------------------- Plugin info provider ---------------------------

QString BattleshipGamePlugin::pluginInfo()
{
    return tr("Author: ") + "Liuch\n" + tr("Email: ") + "liuch@mail.ru\n\n"
        + tr("This plugin allows you to play battleship with your friends.\n"
             "For sending commands, normal messages are used, so this plugin will always work wherever you are able to "
             "log in."
             "To invite a friend for a game, you can use contact menu item or the button on the toolbar in a chat "
             "window.");
}

// --------------------- Option accessor ---------------------------

void BattleshipGamePlugin::setOptionAccessingHost(OptionAccessingHost *host) { Options::psiOptions = host; }

void BattleshipGamePlugin::optionChanged(const QString & /*option*/) { }

// --------------------- Iconfactory accessor ---------------------------
void BattleshipGamePlugin::setIconFactoryAccessingHost(IconFactoryAccessingHost *host) { psiIcon = host; }

// --------------------- Toolbar icon accessor ---------------------------
QList<QVariantHash> BattleshipGamePlugin::getButtonParam()
{
    QList<QVariantHash> list;
    QVariantHash        hash;
    hash["tooltip"] = QVariant(tr("Battleship game"));
    hash["icon"]    = QVariant(QString("battleshipgameplugin/battleship"));
    hash["reciver"] = qVariantFromValue(qobject_cast<QObject *>(this));
    hash["slot"]    = QVariant(SLOT(toolButtonPressed()));
    list.push_back(hash);
    return list;
}

QAction *BattleshipGamePlugin::getAction(QObject * /*parent*/, int /*account*/, const QString & /*contact*/)
{
    return nullptr;
}

// --------------------- Activetab accessor ---------------------------

void BattleshipGamePlugin::setActiveTabAccessingHost(ActiveTabAccessingHost *host) { psiTab = host; }

// --------------------- Account info accessor ---------------------------

void BattleshipGamePlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost *host) { psiAccInfo = host; }

// --------------------- Contact info accessor ---------------------------

void BattleshipGamePlugin::setContactInfoAccessingHost(ContactInfoAccessingHost *host) { psiContactInfo = host; }

// --------------------- Stanza sender ---------------------------

void BattleshipGamePlugin::setStanzaSendingHost(StanzaSendingHost *host) { psiSender = host; }

// --------------------- Stanza filter ---------------------------

bool BattleshipGamePlugin::incomingStanza(int account, const QDomElement &xml)
{
    if (xml.tagName() == "iq") {
        QString acc_status = "";
        bool    confPriv   = false;
        if (xml.attribute("type") == "set") {
            acc_status = psiAccInfo->getStatus(account);
            confPriv   = psiContactInfo->isPrivate(account, xml.attribute("from"));
        }
        return GameSessionList::instance()->processIncomingIqStanza(account, xml, acc_status, confPriv);
    }
    return false;
}

bool BattleshipGamePlugin::outgoingStanza(int /*account*/, QDomElement & /*xml*/) { return false; }

// --------------------- Event creator ---------------------------

void BattleshipGamePlugin::setEventCreatingHost(EventCreatingHost *host) { psiEvent = host; }

// --------------------- Sound accessor ---------------------------

void BattleshipGamePlugin::setSoundAccessingHost(SoundAccessingHost *host) { psiSound = host; }

// --------------------- Menu accessor ---------------------------

QList<QVariantHash> BattleshipGamePlugin::getAccountMenuParam() { return QList<QVariantHash>(); }

QList<QVariantHash> BattleshipGamePlugin::getContactMenuParam()
{
    QList<QVariantHash> menu_list;
    QVariantHash        hash;
    hash["name"]    = QVariant(tr("Battleship game!"));
    hash["icon"]    = QVariant(QString("battleshipgameplugin/battleship"));
    hash["reciver"] = qVariantFromValue(qobject_cast<QObject *>(this));
    hash["slot"]    = QVariant(SLOT(menuActivated()));
    menu_list.push_back(hash);
    return menu_list;
}

QAction *BattleshipGamePlugin::getContactAction(QObject *, int, const QString &) { return nullptr; }

QAction *BattleshipGamePlugin::getAccountAction(QObject *, int) { return nullptr; }

// --------------------- Popup accessor ---------------------------

void BattleshipGamePlugin::setPopupAccessingHost(PopupAccessingHost *host) { psiPopup = host; }
