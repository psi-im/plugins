/*
 * pepchangenotifyplugin.cpp - plugin
 * Copyright (C) 2010  Evgeny Khryukin
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

#include "accountinfoaccessinghost.h"
#include "accountinfoaccessor.h"
#include "applicationinfoaccessinghost.h"
#include "applicationinfoaccessor.h"
#include "contactinfoaccessinghost.h"
#include "contactinfoaccessor.h"
#include "iconfactoryaccessinghost.h"
#include "iconfactoryaccessor.h"
#include "optionaccessinghost.h"
#include "optionaccessor.h"
#include "plugininfoprovider.h"
#include "popupaccessinghost.h"
#include "popupaccessor.h"
#include "psiplugin.h"
#include "soundaccessinghost.h"
#include "soundaccessor.h"
#include "stanzafilter.h"
#include <QDomElement>
#include <QFileDialog>

#include "ui_options.h"

#define constSoundFile "sndfl"
#define constInterval "intrvl"
#define constTune "tune"
#define constMood "mood"
#define constActivity "act"
// #define constGeoloc "geo"
#define constDisableDnd "dsbldnd"
#define constContactDelay "contactdelay"

#define POPUP_OPTION_NAME "PEP Change Notify Plugin"

// delays in secconds
const int connectDelay = 30;

class PepPlugin : public QObject,
                  public PsiPlugin,
                  public StanzaFilter,
                  public AccountInfoAccessor,
                  public OptionAccessor,
                  public PopupAccessor,
                  public PluginInfoProvider,
                  public SoundAccessor,
                  public ApplicationInfoAccessor,
                  public ContactInfoAccessor,
                  public IconFactoryAccessor {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.psi-plus.PepPlugin" FILE "psiplugin.json")
    Q_INTERFACES(PsiPlugin StanzaFilter AccountInfoAccessor OptionAccessor PopupAccessor SoundAccessor
                     PluginInfoProvider ApplicationInfoAccessor ContactInfoAccessor IconFactoryAccessor)

public:
    PepPlugin();
    virtual QString  name() const;
    virtual QWidget *options();
    virtual bool     enable();
    virtual bool     disable();
    virtual void     applyOptions();
    virtual void     restoreOptions();
    virtual bool     incomingStanza(int account, const QDomElement &xml);
    virtual bool     outgoingStanza(int account, QDomElement &xml);
    virtual void     setAccountInfoAccessingHost(AccountInfoAccessingHost *host);
    virtual void     setOptionAccessingHost(OptionAccessingHost *host);
    virtual void     optionChanged(const QString     &/*option*/) {};
    virtual void     setPopupAccessingHost(PopupAccessingHost *host);
    virtual void     setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host);
    virtual void     setContactInfoAccessingHost(ContactInfoAccessingHost *host);
    virtual void     setIconFactoryAccessingHost(IconFactoryAccessingHost *host);
    virtual void     setSoundAccessingHost(SoundAccessingHost *host);
    virtual QString  pluginInfo();

private:
    bool                          enabled;
    OptionAccessingHost          *psiOptions;
    AccountInfoAccessingHost     *accInfoHost;
    PopupAccessingHost           *popup;
    ApplicationInfoAccessingHost *appInfo;
    ContactInfoAccessingHost     *contactInfo;
    IconFactoryAccessingHost     *iconHost;
    SoundAccessingHost           *sound_;
    QString                       soundFile;
    // int interval;
    int               delay;
    bool              showMood, showTune, showActivity; //, showGeoloc;
    bool              disableDnd;
    int               popupId;
    QPointer<QWidget> options_;
    Ui::Options       ui_;

    struct ContactState {
        ContactState(const QString &j = QString()) : jid(j) { }
        enum Event {
            EventTune,
            EventMood,
            EventActivity
            //,EventGeolocation
        };
        QString            jid;
        QMap<Event, QTime> events;
        bool               operator==(const ContactState &s) { return jid == s.jid; }
    };
    QList<ContactState>   states_;
    QHash<int, QTime>     lastConnectionTime_; // <int account, QTime connection_time>
    QHash<QString, QTime> contactsOnlineTime_; // <JID, time when contact goes online>

    QList<ContactState>::iterator findContactStateIndex(const QString &jid);
    bool                          checkContactState(QList<ContactState>::iterator &it, ContactState::Event e);
    bool                          checkContactStatus(const QString &jid);
    bool                          processJid(const QString &jid, ContactState::Event e);
    void                          playSound(const QString &soundFile);
    void                          showPopup(const QString &title, const QString &text, const QString &icon);
    QDomElement                   getFirstChildElement(const QDomElement &elem);

private slots:
    void checkSound();
    void getSound();
    void doNotification(const QString &title, const QString &text, const QString &icon);
};

PepPlugin::PepPlugin() :
    enabled(false), psiOptions(nullptr), accInfoHost(nullptr), popup(nullptr), appInfo(nullptr), contactInfo(nullptr),
    iconHost(nullptr), sound_(nullptr), soundFile("sound/pepnotify.wav")
    //, interval(5)
    ,
    delay(60), showMood(false), showTune(true), showActivity(false)
    //    , showGeoloc(false)
    ,
    disableDnd(false), popupId(0)
{
}

QString PepPlugin::name() const { return "PEP Change Notify Plugin"; }

bool PepPlugin::enable()
{
    states_.clear();
    lastConnectionTime_.clear();
    contactsOnlineTime_.clear();
    if (psiOptions) {
        enabled      = true;
        soundFile    = psiOptions->getPluginOption(constSoundFile, QVariant(soundFile)).toString();
        showMood     = psiOptions->getPluginOption(constMood, QVariant(showMood)).toBool();
        showTune     = psiOptions->getPluginOption(constTune, QVariant(showTune)).toBool();
        showActivity = psiOptions->getPluginOption(constActivity, QVariant(showActivity)).toBool();
        //        showGeoloc = psiOptions->getPluginOption(constGeoloc, QVariant(showGeoloc)).toBool();
        disableDnd = psiOptions->getPluginOption(constDisableDnd, QVariant(disableDnd)).toBool();
        delay      = psiOptions->getPluginOption(constContactDelay, QVariant(delay)).toInt();

        int interval = psiOptions->getPluginOption(constInterval, QVariant(5000)).toInt() / 1000;
        popupId      = popup->registerOption(POPUP_OPTION_NAME, interval,
                                             QLatin1String("plugins.options.pepplugin.") + constInterval);
    }
    return enabled;
}

bool PepPlugin::disable()
{
    states_.clear();
    lastConnectionTime_.clear();
    contactsOnlineTime_.clear();
    popup->unregisterOption(POPUP_OPTION_NAME);
    enabled = false;
    return true;
}

QWidget *PepPlugin::options()
{
    if (!enabled)
        return nullptr;

    options_ = new QWidget();
    ui_.setupUi(options_);

    ui_.cb_geoloc->setVisible(false); // FIXME

    ui_.pb_check->setIcon(iconHost->getIcon("psi/play"));
    ui_.pb_get->setIcon(iconHost->getIcon("psi/browse"));

    connect(ui_.pb_check, &QToolButton::clicked, this, &PepPlugin::checkSound);
    connect(ui_.pb_get, &QToolButton::clicked, this, &PepPlugin::getSound);

    restoreOptions();

    return options_;
}

bool PepPlugin::incomingStanza(int account, const QDomElement &stanza)
{
    if (enabled) {
        if (stanza.tagName() == "presence") {
            QString type = stanza.attribute("type");
            QString jid  = stanza.attribute("from").split("/").first().toLower();
            if (type == "unavailable") {
                contactsOnlineTime_.remove(jid);
            } else if (!contactsOnlineTime_.contains(jid)) {
                contactsOnlineTime_.insert(jid, QTime::currentTime());
            }
            return false;
        }

        if (stanza.tagName() == "message") {
            if (lastConnectionTime_.value(account).secsTo(QTime::currentTime()) < connectDelay) {
                return false;
            }
            if (disableDnd && accInfoHost->getStatus(account) == "dnd") {
                return false;
            }

            QString jid = stanza.attribute("from").split("/").first().toLower();
            if (jid == accInfoHost->getJid(account).toLower())
                return false;

            QDomElement event = stanza.firstChildElement("event");
            if (!event.isNull() && event.namespaceURI().contains("http://jabber.org/protocol/pubsub")) {
                QDomElement items = event.firstChildElement("items");
                if (items.isNull())
                    return false;
                QDomElement item = items.firstChildElement("item");
                if (item.isNull())
                    return false;

                if (showTune) {
                    QDomElement tune = item.firstChildElement("tune");
                    if (!tune.isNull() && tune.namespaceURI() == "http://jabber.org/protocol/tune") {
                        if (!processJid(jid, ContactState::EventTune)) {
                            return false;
                        }
                        QString artist = tune.firstChildElement("artist").text();
                        QString title  = tune.firstChildElement("title").text();
                        if (!artist.isEmpty() || !title.isEmpty()) {
                            QString str = tr("Now listening: ");
                            if (artist.isEmpty()) {
                                str += title;
                            } else {
                                str += artist;
                                if (!title.isEmpty()) {
                                    str += " - " + title;
                                }
                            }
                            QMetaObject::invokeMethod(this, "doNotification", Qt::QueuedConnection,
                                                      Q_ARG(const QString &, contactInfo->name(account, jid)),
                                                      Q_ARG(const QString &, str), Q_ARG(const QString &, "pep/tune"));
                        }
                        return false;
                    }
                }
                if (showMood) {
                    QDomElement mood = item.firstChildElement("mood");
                    if (!mood.isNull() && mood.namespaceURI() == "http://jabber.org/protocol/mood") {
                        if (!processJid(jid, ContactState::EventMood)) {
                            return false;
                        }

                        QString type = getFirstChildElement(mood).tagName();
                        if (!type.isEmpty()) {
                            QString text = mood.firstChildElement("text").text();
                            QString str  = tr("Mood changed to \"%1").arg(type);
                            if (!text.isEmpty()) {
                                str += ": " + text;
                            }
                            str += "\"";
                            QMetaObject::invokeMethod(this, "doNotification", Qt::QueuedConnection,
                                                      Q_ARG(const QString &, contactInfo->name(account, jid)),
                                                      Q_ARG(const QString &, str),
                                                      Q_ARG(const QString &, "mood/" + type));
                        }
                        return false;
                    }
                }
                if (showActivity) {
                    QDomElement act = item.firstChildElement("activity");
                    if (!act.isNull() && act.namespaceURI() == "http://jabber.org/protocol/activity") {
                        if (!processJid(jid, ContactState::EventActivity)) {
                            return false;
                        }

                        QString     icon;
                        QString     type, secType;
                        QDomElement t = getFirstChildElement(act);
                        if (!t.isNull()) {
                            icon = type = t.tagName();
                            secType     = getFirstChildElement(t).tagName();
                            if (!secType.isEmpty())
                                icon += "_" + secType;
                        }
                        if (!type.isEmpty()) {
                            QString text = act.firstChildElement("text").text();
                            QString str  = tr("Activity changed to \"%1").arg(type);
                            if (!secType.isEmpty()) {
                                str += " - " + secType;
                            }
                            if (!text.isEmpty()) {
                                str += ": " + text;
                            }
                            str += "\"";
                            QMetaObject::invokeMethod(this, "doNotification", Qt::QueuedConnection,
                                                      Q_ARG(const QString &, contactInfo->name(account, jid)),
                                                      Q_ARG(const QString &, str),
                                                      Q_ARG(const QString &, "activities/" + icon));
                        }
                        return false;
                    }
                }
            }
        }
    }
    return false;
}

bool PepPlugin::outgoingStanza(int account, QDomElement &xml)
{
    if (enabled) {
        if (xml.tagName() == "iq" && xml.attribute("type") == "set" && !xml.firstChildElement("session").isNull()) {
            lastConnectionTime_.insert(account, QTime::currentTime());
        }
    }

    return false;
}

void PepPlugin::applyOptions()
{
    if (!options_)
        return;

    soundFile = ui_.le_sound->text();
    psiOptions->setPluginOption(constSoundFile, QVariant(soundFile));

    //    interval = ui_.sb_interval->value();
    //    psiOptions->setPluginOption(constInterval, QVariant(interval));

    showActivity = ui_.cb_activity->isChecked();
    psiOptions->setPluginOption(constActivity, QVariant(showActivity));

    //    showGeoloc = ui_.cb_geoloc->isChecked();
    //    psiOptions->setPluginOption(constGeoloc, QVariant(showGeoloc));

    showMood = ui_.cb_mood->isChecked();
    psiOptions->setPluginOption(constMood, QVariant(showMood));

    showTune = ui_.cb_tune->isChecked();
    psiOptions->setPluginOption(constTune, QVariant(showTune));

    disableDnd = ui_.cb_disable_dnd->isChecked();
    psiOptions->setPluginOption(constDisableDnd, QVariant(disableDnd));

    delay = ui_.sb_delay->value();
    psiOptions->setPluginOption(constContactDelay, QVariant(delay));
}

void PepPlugin::restoreOptions()
{
    if (!options_)
        return;

    ui_.le_sound->setText(soundFile);
    //    ui_.sb_interval->setValue(interval);
    ui_.cb_activity->setChecked(showActivity);
    //    ui_.cb_geoloc->setChecked(showGeoloc);
    ui_.cb_mood->setChecked(showMood);
    ui_.cb_tune->setChecked(showTune);
    ui_.cb_disable_dnd->setChecked(disableDnd);
    ui_.sb_delay->setValue(delay);
}

void PepPlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost *host) { accInfoHost = host; }

void PepPlugin::setOptionAccessingHost(OptionAccessingHost *host) { psiOptions = host; }

void PepPlugin::setPopupAccessingHost(PopupAccessingHost *host) { popup = host; }

void PepPlugin::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host) { appInfo = host; }

void PepPlugin::setContactInfoAccessingHost(ContactInfoAccessingHost *host) { contactInfo = host; }

void PepPlugin::setIconFactoryAccessingHost(IconFactoryAccessingHost *host) { iconHost = host; }

void PepPlugin::setSoundAccessingHost(SoundAccessingHost *host) { sound_ = host; }

void PepPlugin::playSound(const QString &f) { sound_->playSound(f); }

void PepPlugin::getSound()
{
    QString fileName = QFileDialog::getOpenFileName(nullptr, tr("Choose a sound file"), "", tr("Sound (*.wav)"));
    if (fileName.isEmpty())
        return;
    ui_.le_sound->setText(fileName);
}

void PepPlugin::checkSound() { playSound(ui_.le_sound->text()); }

void PepPlugin::showPopup(const QString &title, const QString &text, const QString &icon)
{
    QVariant suppressDnd = psiOptions->getGlobalOption("options.ui.notifications.passive-popups.suppress-while-dnd");
    psiOptions->setGlobalOption("options.ui.notifications.passive-popups.suppress-while-dnd", disableDnd);

    int interval = popup->popupDuration(POPUP_OPTION_NAME);
    if (interval) {
        popup->initPopup(text.toHtmlEscaped(), title.toHtmlEscaped(), icon, popupId);
    }
    psiOptions->setGlobalOption("options.ui.notifications.passive-popups.suppress-while-dnd", suppressDnd);
}

void PepPlugin::doNotification(const QString &title, const QString &text, const QString &icon)
{
    showPopup(title, text, icon);
    if (psiOptions->getGlobalOption("options.ui.notifications.sounds.enable").toBool()) {
        playSound(soundFile);
    }
}

QList<PepPlugin::ContactState>::iterator PepPlugin::findContactStateIndex(const QString &jid)
{
    QList<ContactState>::iterator it = states_.begin();
    ContactState                  s(jid);
    for (; it != states_.end(); ++it) {
        if (s == *it) {
            break;
        }
    }
    if (it == states_.end()) {
        it = states_.insert(--it, s);
    }
    return it;
}

bool PepPlugin::checkContactState(QList<ContactState>::iterator &it, ContactState::Event e)
{
    QTime time = QTime::currentTime();
    if ((*it).events.contains(e)) {
        QTime oldTime = (*it).events.value(e);
        if (oldTime.secsTo(time) < delay) {
            return false;
        }
    }
    (*it).events.insert(e, time);
    return true;
}

bool PepPlugin::checkContactStatus(const QString &jid)
{
    if (!contactsOnlineTime_.contains(
            jid)) { // такое может произойти, если плагин включили, когда аккаунт уже был в онлайне
        return true; // а вообще, ивенты от контактов не должны приходить раньше презенсов.
    }                // если такое произойдет - таймаут не сработает
    QTime contactTime = contactsOnlineTime_.value(jid);
    return (contactTime.secsTo(QTime::currentTime()) < delay) ? false : true;
}

bool PepPlugin::processJid(const QString &jid, ContactState::Event e)
{
    if (!checkContactStatus(jid)) {
        return false;
    }
    QList<ContactState>::iterator it = findContactStateIndex(jid);
    if (!checkContactState(it, e)) {
        return false;
    }
    return true;
}

QDomElement PepPlugin::getFirstChildElement(const QDomElement &elem)
{
    QDomElement newElem;
    QDomNode    node = elem.firstChild();
    while (!node.isNull()) {
        if (!node.isElement()) {
            node = node.nextSibling();
            continue;
        }

        newElem = node.toElement();
        break;
    }

    return newElem;
}

QString PepPlugin::pluginInfo()
{
    return tr(
        "This plugin shows popup notifications when users from your roster changes their mood, tune or activity.");
}

#include "pepchangenotifyplugin.moc"
