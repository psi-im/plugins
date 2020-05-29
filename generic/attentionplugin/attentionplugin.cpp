/*
 * attentionplugin.cpp - plugin
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

#include <QDomElement>
#include <QFileDialog>

#include "accountinfoaccessinghost.h"
#include "accountinfoaccessor.h"
#include "activetabaccessinghost.h"
#include "activetabaccessor.h"
#include "applicationinfoaccessinghost.h"
#include "applicationinfoaccessor.h"
#include "iconfactoryaccessinghost.h"
#include "iconfactoryaccessor.h"
#include "menuaccessor.h"
#include "optionaccessinghost.h"
#include "optionaccessor.h"
#include "plugininfoprovider.h"
#include "popupaccessinghost.h"
#include "popupaccessor.h"
#include "psiplugin.h"
#include "soundaccessinghost.h"
#include "soundaccessor.h"
#include "stanzafilter.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"
#include "toolbariconaccessor.h"

#include "ui_options.h"

#define constSoundFile "sndfl"
#define constInterval "intrvl"
#define constInfPopup "infPopup"
#define constTimeout "timeout"
#define constDisableDnd "dsbldnd"

#define POPUP_OPTION "Attention Plugin"

class AttentionPlugin : public QObject,
                        public PsiPlugin,
                        public StanzaFilter,
                        public AccountInfoAccessor,
                        public OptionAccessor,
                        public ActiveTabAccessor,
                        public ToolbarIconAccessor,
                        public ApplicationInfoAccessor,
                        public IconFactoryAccessor,
                        public PopupAccessor,
                        public StanzaSender,
                        public MenuAccessor,
                        public PluginInfoProvider,
                        public SoundAccessor {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.psi-plus.AttentionPlugin" FILE "psiplugin.json")
    Q_INTERFACES(PsiPlugin StanzaFilter AccountInfoAccessor OptionAccessor ActiveTabAccessor ApplicationInfoAccessor
                     ToolbarIconAccessor IconFactoryAccessor PopupAccessor StanzaSender MenuAccessor PluginInfoProvider
                         SoundAccessor)

public:
    AttentionPlugin();
    virtual QString             name() const;
    virtual QWidget *           options();
    virtual bool                enable();
    virtual bool                disable();
    virtual void                applyOptions();
    virtual void                restoreOptions();
    virtual bool                incomingStanza(int account, const QDomElement &xml);
    virtual bool                outgoingStanza(int account, QDomElement &xml);
    virtual void                setAccountInfoAccessingHost(AccountInfoAccessingHost *host);
    virtual void                setOptionAccessingHost(OptionAccessingHost *host);
    virtual void                optionChanged(const QString &option);
    virtual void                setActiveTabAccessingHost(ActiveTabAccessingHost *host);
    virtual void                setIconFactoryAccessingHost(IconFactoryAccessingHost *host);
    virtual void                setPopupAccessingHost(PopupAccessingHost *host);
    virtual void                setStanzaSendingHost(StanzaSendingHost *host);
    virtual QList<QVariantHash> getButtonParam();
    virtual QAction *           getAction(QObject *, int, const QString &) { return nullptr; };
    virtual QList<QVariantHash> getAccountMenuParam();
    virtual QList<QVariantHash> getContactMenuParam();
    virtual QAction *           getContactAction(QObject *, int, const QString &) { return nullptr; };
    virtual QAction *           getAccountAction(QObject *, int) { return nullptr; };
    virtual void                setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host);
    virtual void                setSoundAccessingHost(SoundAccessingHost *host);
    virtual QString             pluginInfo();

private:
    bool                          enabled;
    OptionAccessingHost *         psiOptions;
    AccountInfoAccessingHost *    accInfoHost;
    ActiveTabAccessingHost *      activeTab;
    IconFactoryAccessingHost *    icoHost;
    PopupAccessingHost *          popup;
    StanzaSendingHost *           stanzaSender;
    ApplicationInfoAccessingHost *appInfo;
    SoundAccessingHost *          sound_;
    QString                       soundFile;
    int                           timeout_;
    bool                          infPopup, disableDnd;
    QTimer *                      nudgeTimer_;
    QPointer<QWidget>             nudgeWindow_;
    QPoint                        oldPoint_;
    QPointer<QWidget>             options_;
    int                           popupId;

    struct Blocked {
        int       Acc;
        QString   Jid;
        QDateTime LastMes;
    };
    QVector<Blocked> blockedJids_;

    Ui::Options ui_;

    enum { FakeAccount = 9999 };

    bool findAcc(int account, const QString &Jid, int &i);
    void sendAttention(int account, const QString &yourJid, const QString &jid);
    void nudge();
    void playSound(const QString &soundFile);
    void showPopup(int account, const QString &jid, const QString &text);

private slots:
    void checkSound();
    void getSound();
    void sendAttentionFromTab();
    void sendAttentionFromMenu();
    void nudgeTimerTimeout();
};

AttentionPlugin::AttentionPlugin() :
    enabled(false), psiOptions(nullptr), accInfoHost(nullptr), activeTab(nullptr), icoHost(nullptr), popup(nullptr),
    stanzaSender(nullptr), appInfo(nullptr), sound_(nullptr), soundFile("sound/attention.wav"), timeout_(30),
    infPopup(false), disableDnd(false), nudgeTimer_(nullptr), popupId(0)
{
}

QString AttentionPlugin::name() const { return "Attention Plugin"; }

bool AttentionPlugin::enable()
{
    QFile file(":/attentionplugin/attention.png");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray image = file.readAll();
        icoHost->addIcon("attentionplugin/attention", image);
        file.close();
    } else {
        enabled = false;
        return enabled;
    }
    if (psiOptions) {
        blockedJids_.clear();
        enabled    = true;
        soundFile  = psiOptions->getPluginOption(constSoundFile, QVariant(soundFile)).toString();
        timeout_   = psiOptions->getPluginOption(constTimeout, QVariant(timeout_)).toInt();
        infPopup   = psiOptions->getPluginOption(constInfPopup, QVariant(infPopup)).toBool();
        disableDnd = psiOptions->getPluginOption(constDisableDnd, QVariant(disableDnd)).toBool();
        popupId    = popup->registerOption(POPUP_OPTION,
                                        psiOptions->getPluginOption(constInterval, QVariant(4000)).toInt() / 1000,
                                        QLatin1String("plugins.options.attention.") + constInterval);

        QWidgetList wl = qApp->allWidgets();
        for (QWidget *w : wl) {
            if (w->objectName() == "MainWin") {
                nudgeWindow_ = w;
                break;
            }
        }
        nudgeTimer_ = new QTimer(this);
        nudgeTimer_->setInterval(50);
        connect(nudgeTimer_, &QTimer::timeout, this, &AttentionPlugin::nudgeTimerTimeout);
    }
    return enabled;
}

bool AttentionPlugin::disable()
{
    enabled = false;
    nudgeTimer_->stop();
    delete nudgeTimer_;
    nudgeTimer_ = nullptr;
    popup->unregisterOption(POPUP_OPTION);
    return true;
}

QWidget *AttentionPlugin::options()
{
    if (!enabled) {
        return nullptr;
    }
    options_ = new QWidget();
    ui_.setupUi(options_);

    ui_.tb_open->setIcon(icoHost->getIcon("psi/browse"));
    ui_.tb_test->setIcon(icoHost->getIcon("psi/play"));

    connect(ui_.tb_open, &QToolButton::clicked, this, &AttentionPlugin::getSound);
    connect(ui_.tb_test, &QToolButton::clicked, this, &AttentionPlugin::checkSound);

    restoreOptions();

    return options_;
}

bool AttentionPlugin::incomingStanza(int account, const QDomElement &stanza)
{
    if (enabled) {
        if (stanza.tagName() == "message" && stanza.attribute("type") == "headline"
            && !stanza.firstChildElement("attention").isNull()) {

            if (disableDnd && accInfoHost->getStatus(account) == "dnd")
                return false;

            QString from = stanza.attribute("from");

            int i = blockedJids_.size();
            if (findAcc(account, from, i)) {
                Blocked &B = blockedJids_[i];
                if (QDateTime::currentDateTime().secsTo(B.LastMes) > -timeout_) {
                    return false;
                } else {
                    B.LastMes = QDateTime::currentDateTime();
                }
            } else {
                Blocked B = { account, from, QDateTime::currentDateTime() };
                blockedJids_ << B;
            }

            const QString optAway      = "options.ui.notifications.passive-popups.suppress-while-away";
            QVariant      suppressAway = psiOptions->getGlobalOption(optAway);
            const QString optDnd       = "options.ui.notifications.passive-popups.suppress-while-dnd";
            QVariant      suppressDnd  = psiOptions->getGlobalOption(optDnd);
            int           interval     = popup->popupDuration(POPUP_OPTION);
            if (infPopup && (accInfoHost->getStatus(account) == "away" || accInfoHost->getStatus(account) == "xa")) {
                psiOptions->setGlobalOption(optAway, false);
                popup->setPopupDuration(POPUP_OPTION, -1);
            }
            psiOptions->setGlobalOption(optDnd, disableDnd);

            showPopup(account, from.split("/").first(), from + tr(" sends Attention message to you!"));
            psiOptions->setGlobalOption(optAway, suppressAway);
            psiOptions->setGlobalOption(optDnd, suppressDnd);
            popup->setPopupDuration(POPUP_OPTION, interval);

            if (psiOptions->getGlobalOption("options.ui.notifications.sounds.enable").toBool())
                playSound(soundFile);

            /*QTextEdit *te = activeTab->getEditBox();
              if(te)
              nudgeWindow_ = te->window();

              else
              nudgeWindow_ = qApp->activeWindow();*/

            if (nudgeWindow_ && nudgeWindow_->isVisible())
                nudge();
        }

        else if (stanza.tagName() == "iq" && stanza.attribute("type") == "get") {
            QDomElement query = stanza.firstChildElement("query");
            if (!query.isNull() && query.namespaceURI() == "http://jabber.org/protocol/disco#info") {
                if (query.attribute("node") == "https://psi-im.org#at-pl") {
                    QString reply = QString("<iq type=\"result\" to=\"%1\" id=\"%2\">"
                                            "<query xmlns=\"http://jabber.org/protocol/disco#info\" "
                                            "node=\"https://psi-im.org#at-pl\">"
                                            "<feature var=\"urn:xmpp:attention:0\"/></query></iq>")
                                        .arg(stanzaSender->escape(stanza.attribute("from")),
                                             stanzaSender->escape(stanza.attribute("id")));
                    stanzaSender->sendStanza(account, reply);
                    return true;
                }
            }
        }
    }
    return false;
}

bool AttentionPlugin::outgoingStanza(int /*account*/, QDomElement &xml)
{
    if (enabled) {
        if (xml.tagName() == "iq" && xml.attribute("type") == "result") {
            QDomNodeList list = xml.elementsByTagNameNS("http://jabber.org/protocol/disco#info", "query");
            if (!list.isEmpty()) {
                QDomElement query = list.at(0).toElement();
                if (!query.hasAttribute("node")) {
                    QDomDocument doc     = xml.ownerDocument();
                    QDomElement  feature = doc.createElement("feature");
                    feature.setAttribute("var", "urn:xmpp:attention:0");
                    query.appendChild(feature);
                }
            }
        } else if (xml.tagName() == "presence") {
            QDomNodeList list = xml.elementsByTagNameNS("http://jabber.org/protocol/caps", "c");
            if (!list.isEmpty()) {
                QDomElement c = list.at(0).toElement();
                if (c.hasAttribute("ext")) {
                    QString ext = c.attribute("ext");
                    ext += " at-pl";
                    c.setAttribute("ext", ext);
                }
            }
        }
    }
    return false;
}

void AttentionPlugin::applyOptions()
{
    if (!options_)
        return;

    soundFile = ui_.le_sound->text();
    psiOptions->setPluginOption(constSoundFile, soundFile);

    timeout_ = ui_.sb_count->value();
    psiOptions->setPluginOption(constTimeout, timeout_);

    infPopup = ui_.cb_dontHide->isChecked();
    psiOptions->setPluginOption(constInfPopup, infPopup);

    disableDnd = ui_.cb_disableDND->isChecked();
    psiOptions->setPluginOption(constDisableDnd, disableDnd);
}

void AttentionPlugin::restoreOptions()
{
    if (!options_)
        return;

    ui_.le_sound->setText(soundFile);
    ui_.sb_count->setValue(timeout_);
    ui_.cb_dontHide->setChecked(infPopup);
    ui_.cb_disableDND->setChecked(disableDnd);
}

void AttentionPlugin::optionChanged(const QString &option) { Q_UNUSED(option); }

void AttentionPlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost *host) { accInfoHost = host; }

void AttentionPlugin::setOptionAccessingHost(OptionAccessingHost *host) { psiOptions = host; }

void AttentionPlugin::setActiveTabAccessingHost(ActiveTabAccessingHost *host) { activeTab = host; }

void AttentionPlugin::setIconFactoryAccessingHost(IconFactoryAccessingHost *host) { icoHost = host; }

void AttentionPlugin::setStanzaSendingHost(StanzaSendingHost *host) { stanzaSender = host; }

void AttentionPlugin::setPopupAccessingHost(PopupAccessingHost *host) { popup = host; }

void AttentionPlugin::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host) { appInfo = host; }

void AttentionPlugin::setSoundAccessingHost(SoundAccessingHost *host) { sound_ = host; }

QList<QVariantHash> AttentionPlugin::getButtonParam()
{
    QList<QVariantHash> l;
    QVariantHash        hash;
    hash["tooltip"] = QVariant(tr("Send Attention"));
    hash["icon"]    = QVariant(QString("attentionplugin/attention"));
    hash["reciver"] = QVariant::fromValue(qobject_cast<QObject *>(this));
    hash["slot"]    = QVariant(SLOT(sendAttentionFromTab()));
    l.push_back(hash);
    return l;
}

void AttentionPlugin::playSound(const QString &f) { sound_->playSound(f); }

void AttentionPlugin::getSound()
{
    QString fileName = QFileDialog::getOpenFileName(nullptr, tr("Choose a sound file"), "", tr("Sound (*.wav)"));
    if (fileName.isEmpty())
        return;
    ui_.le_sound->setText(fileName);
}

void AttentionPlugin::checkSound() { playSound(ui_.le_sound->text()); }

void AttentionPlugin::showPopup(int account, const QString &jid, const QString &text)
{
    if (account == FakeAccount) {
        popup->initPopup(text, tr("Attention Plugin"), "attentionplugin/attention", popupId);
    } else {
        popup->initPopupForJid(account, jid, text, tr("Attention Plugin"), "attentionplugin/attention", popupId);
    }
}

void AttentionPlugin::sendAttention(int account, const QString &yourJid, const QString &jid)
{

    if (accInfoHost->getStatus(account) == "offline")
        return;

    QString msg
        = QString(
              "<message from=\"%1\" to=\"%2\" type=\"headline\"><attention xmlns='urn:xmpp:attention:0'/></message>")
              .arg(yourJid)
              .arg(jid);
    stanzaSender->sendStanza(account, msg);

    showPopup(FakeAccount, QString(), tr("You sent Attention message to %1").arg(jid));
}

void AttentionPlugin::sendAttentionFromTab()
{
    if (!enabled)
        return;
    QString yourJid = activeTab->getYourJid();
    QString jid     = activeTab->getJid();
    QString tmpJid("");
    int     account = 0;
    while (yourJid != (tmpJid = accInfoHost->getJid(account))) {
        ++account;
        if (tmpJid == "-1")
            return;
    }

    sendAttention(account, yourJid, jid);
}

void AttentionPlugin::sendAttentionFromMenu()
{
    int     acc     = sender()->property("account").toInt();
    QString jid     = sender()->property("jid").toString();
    QString yourJid = accInfoHost->getJid(acc);

    sendAttention(acc, yourJid, jid);
}

bool AttentionPlugin::findAcc(int account, const QString &Jid, int &i)
{
    for (; i > 0;) {
        Blocked Block = blockedJids_[--i];
        if (Block.Acc == account && Block.Jid == Jid) {
            return true;
        }
    }
    return false;
}

QList<QVariantHash> AttentionPlugin::getAccountMenuParam() { return QList<QVariantHash>(); }

QList<QVariantHash> AttentionPlugin::getContactMenuParam()
{
    QVariantHash hash;
    hash["icon"]    = QVariant(QString("attentionplugin/attention"));
    hash["name"]    = QVariant(tr("Send Attention"));
    hash["reciver"] = QVariant::fromValue(qobject_cast<QObject *>(this));
    hash["slot"]    = QVariant(SLOT(sendAttentionFromMenu()));
    QList<QVariantHash> l;
    l.push_back(hash);
    return l;
}

void AttentionPlugin::nudge()
{
    if (!nudgeWindow_ || !nudgeTimer_ || nudgeTimer_->isActive())
        return;

    oldPoint_ = nudgeWindow_->pos();
    nudgeTimer_->start();
}

void AttentionPlugin::nudgeTimerTimeout()
{
    static uint count = 0;

    if (!nudgeWindow_) {
        nudgeTimer_->stop();
        count = 0;
        return;
    }
    if (count < 40) {
        int    rH = qrand() % 10, rW = qrand() % 10;
        QPoint newPoint(oldPoint_.x() + rH, oldPoint_.y() + rW);
        nudgeWindow_->move(newPoint);
        count++;
    } else {
        count = 0;
        nudgeTimer_->stop();
        nudgeWindow_->move(oldPoint_);
    }
}

QString AttentionPlugin::pluginInfo()
{
    return tr(
        "This plugin is designed to send and receive special messages such as Attentions.\n"
        "To work correctly, the plugin requires that the client of the other part supports XEP-0224 (for example: "
        "Pidgin, Miranda IM with Nudge plugin).");
}


#include "attentionplugin.moc"
