/*
 * gomokugameplugin.cpp - Gomoku Game plugin
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

#include "gomokugameplugin.h"
#include "common.h"
#include "gamesessions.h"
#include "options.h"
#include <QFileDialog>

#define constVersion "0.1.2"
#define constShortPluginName "gomokugameplugin"

GomokuGamePlugin::GomokuGamePlugin(QObject *parent) :
    QObject(parent), enabled_(false), psiTab(nullptr), psiIcon(nullptr), psiAccInfo(nullptr), psiContactInfo(nullptr),
    psiSender(nullptr), psiEvent(nullptr), psiSound(nullptr), psiPopup(nullptr)
{
    Options::psiOptions = nullptr;
}

QString GomokuGamePlugin::name() const { return constPluginName; }

QString GomokuGamePlugin::shortName() const { return constShortPluginName; }

QString GomokuGamePlugin::version() const { return constVersion; }

QWidget *GomokuGamePlugin::options()
{
    QWidget *options = new QWidget;
    ui_.setupUi(options);
    ui_.play_error->setIcon(psiIcon->getIcon("psi/play"));
    ui_.play_finish->setIcon(psiIcon->getIcon("psi/play"));
    ui_.play_move->setIcon(psiIcon->getIcon("psi/play"));
    ;
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

bool GomokuGamePlugin::enable()
{
    if (enabled_)
        return true;
    // Грузим иконку плагина
    QFile file(":/gomokugameplugin/gomoku");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray ico = file.readAll();
        psiIcon->addIcon("gomokugameplugin/gomoku", ico);
        file.close();
    }
    // Грузим настройки плагина
    // -- загрузятся по требованию
    // Создаем соединения с менеджером игровых сессий
    GameSessions *sessions = GameSessions::instance();
    connect(sessions, SIGNAL(sendStanza(int, QString)), this, SLOT(sendGameStanza(int, QString)), Qt::QueuedConnection);
    connect(sessions, SIGNAL(doPopup(const QString)), this, SLOT(doPopup(const QString)), Qt::QueuedConnection);
    connect(sessions, SIGNAL(playSound(const QString)), this, SLOT(playSound(const QString)), Qt::QueuedConnection);
    connect(sessions, SIGNAL(doInviteEvent(int, QString, QString, QObject *, const char *)), this,
            SLOT(doPsiEvent(int, QString, QString, QObject *, const char *)), Qt::QueuedConnection);
    // Выставляем флаг и уходим
    enabled_ = true;
    return true;
}

bool GomokuGamePlugin::disable()
{
    enabled_ = false;
    GameSessions::reset();
    Options::reset();
    return true;
}

void GomokuGamePlugin::applyOptions()
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

void GomokuGamePlugin::restoreOptions()
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

QPixmap GomokuGamePlugin::icon() const { return QPixmap(":/gomokugameplugin/img/gomoku_16.png"); }

/**
 * Получение списка ресурсов и вызов формы для отправки приглашения
 */
void GomokuGamePlugin::invite(int account, QString full_jid)
{
    QStringList jid_parse = full_jid.split("/");
    QString     jid       = jid_parse.takeFirst();
    if (jid.isEmpty())
        return;
    QStringList res_list;
    if (psiContactInfo->isPrivate(account, full_jid)) {
        // Это конференция
        if (jid_parse.size() == 0)
            return;
        res_list.push_back(jid_parse.join("/"));
    } else {
        // Получаем список ресурсов оппонента
        res_list = psiContactInfo->resources(account, jid);
    }
    // Отображение окна отправки приглашения
    GameSessions::instance()->invite(account, jid, res_list);
}

// ------------------------------------------ Slots ------------------------------------------

/**
 * Кто то кликнул по кнопке тулбара в окне чата
 */
void GomokuGamePlugin::toolButtonPressed()
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
    invite(account, psiTab->getJid());
}

/**
 * Кто то выбрал плагин в меню ростера
 */
void GomokuGamePlugin::menuActivated()
{
    if (!enabled_)
        return;
    int account = sender()->property("account").toInt();
    if (psiAccInfo->getStatus(account) == "offline")
        return;
    QString jid = sender()->property("jid").toString();
    invite(account, jid);
}

/**
 * Создания события для приглашения
 */
void GomokuGamePlugin::doPsiEvent(int account, QString from, QString text, QObject *receiver, const char *method)
{
    psiEvent->createNewEvent(account, from, text, receiver, method);
}

/**
 * Отсылка станзы по запросу игры
 */
void GomokuGamePlugin::sendGameStanza(int account, QString stanza)
{
    if (!enabled_ || psiAccInfo->getStatus(account) == "offline")
        return;
    psiSender->sendStanza(account, stanza);
}

void GomokuGamePlugin::testSound()
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

void GomokuGamePlugin::getSound()
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

void GomokuGamePlugin::doPopup(const QString &text)
{
    psiPopup->initPopup(text, tr(constPluginName), "gomokugameplugin/gomoku");
}

void GomokuGamePlugin::playSound(const QString &sound_id)
{
    Options *options = Options::instance();
    if (options->getOption(constDefSoundSettings).toBool()
        || Options::psiOptions->getGlobalOption("options.ui.notifications.sounds.enable").toBool()) {
        if (sound_id == constSoundMove) {
            psiSound->playSound(options->getOption(constSoundMove).toString());
        } else if (sound_id == constSoundStart) {
            psiSound->playSound(options->getOption(constSoundStart).toString());
        } else if (sound_id == constSoundFinish) {
            psiSound->playSound(options->getOption(constSoundFinish).toString());
        } else if (sound_id == constSoundError) {
            psiSound->playSound(options->getOption(constSoundError).toString());
        }
    }
}

// --------------------- Plugin info provider ---------------------------

QString GomokuGamePlugin::pluginInfo()
{
    return tr(
        "This plugin allows you to play gomoku with your friends.\n"
        "For sending commands, normal messages are used, so this plugin will always work wherever you are able to "
        "log in."
        "To invite a friend for a game, you can use contact menu item or the button on the toolbar in a chat "
        "window.");
}

// --------------------- Option accessor ---------------------------

void GomokuGamePlugin::setOptionAccessingHost(OptionAccessingHost *host) { Options::psiOptions = host; }

void GomokuGamePlugin::optionChanged(const QString & /*option*/) { }

// --------------------- Iconfactory accessor ---------------------------
void GomokuGamePlugin::setIconFactoryAccessingHost(IconFactoryAccessingHost *host) { psiIcon = host; }

// --------------------- Toolbar icon accessor ---------------------------
QList<QVariantHash> GomokuGamePlugin::getButtonParam()
{
    QList<QVariantHash> list;
    QVariantHash        hash;
    hash["tooltip"] = QVariant(tr("Gomoku game"));
    hash["icon"]    = QVariant(QString("gomokugameplugin/gomoku"));
    hash["reciver"] = QVariant::fromValue(qobject_cast<QObject *>(this));
    hash["slot"]    = QVariant(SLOT(toolButtonPressed()));
    list.push_back(hash);
    return list;
}

QAction *GomokuGamePlugin::getAction(QObject * /*parent*/, int /*account*/, const QString & /*contact*/)
{
    return nullptr;
}

// --------------------- Activetab accessor ---------------------------

void GomokuGamePlugin::setActiveTabAccessingHost(ActiveTabAccessingHost *host) { psiTab = host; }

// --------------------- Account info accessor ---------------------------

void GomokuGamePlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost *host) { psiAccInfo = host; }

// --------------------- Contact info accessor ---------------------------

void GomokuGamePlugin::setContactInfoAccessingHost(ContactInfoAccessingHost *host) { psiContactInfo = host; }

// --------------------- Stanza sender ---------------------------

void GomokuGamePlugin::setStanzaSendingHost(StanzaSendingHost *host) { psiSender = host; }

// --------------------- Stanza filter ---------------------------

bool GomokuGamePlugin::incomingStanza(int account, const QDomElement &xml)
{
    if (xml.tagName() == "iq") {
        QString acc_status = "";
        bool    confPriv   = false;
        if (xml.attribute("type") == "set") {
            acc_status = psiAccInfo->getStatus(account);
            confPriv   = psiContactInfo->isPrivate(account, xml.attribute("from"));
        }
        return GameSessions::instance()->processIncomingIqStanza(account, xml, acc_status, confPriv);
    }
    return false;
}

bool GomokuGamePlugin::outgoingStanza(int /*account*/, QDomElement & /*xml*/) { return false; }

// --------------------- Event creator ---------------------------

void GomokuGamePlugin::setEventCreatingHost(EventCreatingHost *host) { psiEvent = host; }

// --------------------- Sound accessor ---------------------------

void GomokuGamePlugin::setSoundAccessingHost(SoundAccessingHost *host) { psiSound = host; }

// --------------------- Menu accessor ---------------------------

QList<QVariantHash> GomokuGamePlugin::getAccountMenuParam() { return QList<QVariantHash>(); }

QList<QVariantHash> GomokuGamePlugin::getContactMenuParam()
{
    QList<QVariantHash> menu_list;
    QVariantHash        hash;
    hash["name"]    = QVariant(tr("Gomoku game!"));
    hash["icon"]    = QVariant(QString("gomokugameplugin/gomoku"));
    hash["reciver"] = QVariant::fromValue(qobject_cast<QObject *>(this));
    hash["slot"]    = QVariant(SLOT(menuActivated()));
    menu_list.push_back(hash);
    return menu_list;
}

QAction *GomokuGamePlugin::getContactAction(QObject *, int, const QString &) { return nullptr; }

QAction *GomokuGamePlugin::getAccountAction(QObject *, int) { return nullptr; }

// --------------------- Popup accessor ---------------------------

void GomokuGamePlugin::setPopupAccessingHost(PopupAccessingHost *host) { psiPopup = host; }
