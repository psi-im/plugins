/*
 * watcherplugin.cpp - plugin
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

#include <QFileDialog>
#include <QDomElement>
#include <QHash>
#include <QAction>

#include "view.h"
#include "model.h"
#include "ui_options.h"
#include "watcheditem.h"
#include "edititemdlg.h"

#include "psiplugin.h"
#include "stanzafilter.h"
#include "popupaccessor.h"
#include "popupaccessinghost.h"
#include "optionaccessor.h"
#include "optionaccessinghost.h"
#include "iconfactoryaccessor.h"
#include "iconfactoryaccessinghost.h"
#include "menuaccessor.h"
#include "applicationinfoaccessor.h"
#include "applicationinfoaccessinghost.h"
#include "plugininfoprovider.h"
#include "activetabaccessinghost.h"
#include "activetabaccessor.h"
#include "contactinfoaccessinghost.h"
#include "contactinfoaccessor.h"
#include "accountinfoaccessinghost.h"
#include "accountinfoaccessor.h"
#include "soundaccessinghost.h"
#include "soundaccessor.h"
#include "toolbariconaccessor.h"

#define constVersion "0.4.6"

#define constSoundFile "sndfl"
#define constInterval "intrvl"
#define constCount "count"
#define constSndFiles "sndfiles"
#define constJids "jids"
#define constEnabledJids "enjids"
#define constWatchedItems "watcheditem"
#define constDisableSnd "dsblsnd"
#define constDisablePopupDnd "dsblpopupdnd"
#define constShowInContext "showincontext"

#define POPUP_OPTION_NAME "Watcher Plugin"


class Watcher : public QObject, public PsiPlugin, public PopupAccessor, public MenuAccessor, public PluginInfoProvider,
                public OptionAccessor, public StanzaFilter, public IconFactoryAccessor, public ApplicationInfoAccessor,
                public ActiveTabAccessor, public ContactInfoAccessor, public AccountInfoAccessor, public SoundAccessor,
                public ToolbarIconAccessor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.psi-plus.Watcher")
    Q_INTERFACES(PsiPlugin PopupAccessor OptionAccessor StanzaFilter IconFactoryAccessor AccountInfoAccessor
                 PluginInfoProvider MenuAccessor ApplicationInfoAccessor ActiveTabAccessor ContactInfoAccessor
                 SoundAccessor ToolbarIconAccessor)
public:
    Watcher();
    virtual QString name() const;
    virtual QString shortName() const;
    virtual QString version() const;
    virtual QWidget* options();
    virtual bool enable();
    virtual bool disable();
    virtual void optionChanged(const QString& option);
    virtual void applyOptions();
    virtual void restoreOptions();
    virtual QPixmap icon() const;

    virtual void setPopupAccessingHost(PopupAccessingHost* host);
    virtual void setOptionAccessingHost(OptionAccessingHost* host);
    virtual bool incomingStanza(int account, const QDomElement& xml);
    virtual bool outgoingStanza(int account, QDomElement& xml);
    virtual void setIconFactoryAccessingHost(IconFactoryAccessingHost* host);
    QList < QVariantHash > getAccountMenuParam();
    QList < QVariantHash > getContactMenuParam();
    virtual QAction* getContactAction(QObject* , int , const QString& );
    virtual QAction* getAccountAction(QObject* , int ) { return 0; }
    virtual void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host);
    virtual QString pluginInfo();
    virtual void setActiveTabAccessingHost(ActiveTabAccessingHost* host);
    virtual void setContactInfoAccessingHost(ContactInfoAccessingHost* host);
    virtual void setAccountInfoAccessingHost(AccountInfoAccessingHost* host);
    virtual void setSoundAccessingHost(SoundAccessingHost* host);

    QList<QVariantHash> getButtonParam() { return QList<QVariantHash>(); }
    QAction* getAction(QObject *parent, int account, const QString &contact);

    QAction* createAction(QObject *parent, const QString &contact);

private:
    OptionAccessingHost *psiOptions;
    PopupAccessingHost* popup;
    IconFactoryAccessingHost* icoHost;
    ApplicationInfoAccessingHost* appInfoHost;
    ActiveTabAccessingHost* activeTab;
    ContactInfoAccessingHost* contactInfo;
    AccountInfoAccessingHost* accInfo;
    SoundAccessingHost* sound_;
    bool enabled;
    QString soundFile;
    //int Interval;
    QPointer<QWidget> optionsWid;
    Model *model_;
    Ui::Options ui_;
    QList<WatchedItem*> items_;
    bool isSndEnable;
    bool disableSnd;
    bool disablePopupDnd;
    int popupId;
    QHash<QString, QAction*> actions_;
    bool showInContext_ = true;

    bool checkWatchedItem(const QString& from, const QString& body, WatchedItem *wi);

private slots:
    void checkSound(QModelIndex index = QModelIndex());
    void getSound(QModelIndex index = QModelIndex());
    void addLine();
    void delSelected();
    void Hack();
    void onOptionsClose();
    void playSound(const QString& soundFile);
    void showPopup(int account, const QString& jid, QString text);

    void addItemAct();
    void delItemAct();
    void editItemAct();
    void addNewItem(const QString& settings);
    void editCurrentItem(const QString& setting);
    void timeOut();
    void actionActivated();
    void removeFromActions(QObject *object);
};

Watcher::Watcher()
    : psiOptions(0)
    , popup(0)
    , icoHost(0)
    , appInfoHost(0)
    , activeTab(0)
    , contactInfo(0)
    , accInfo(0)
    , sound_(0)
    , enabled(false)
    , soundFile("sound/watcher.wav")
//, Interval(2)
    , model_(0)
    , isSndEnable(false)
    , disableSnd(true)
    , disablePopupDnd(true)
    , popupId(0)
{
}

QString Watcher::name() const {
    return "Watcher Plugin";
}

QString Watcher::shortName() const {
    return "watcher";
}

QString Watcher::version() const {
    return constVersion;
}

bool Watcher::enable() {
    if(psiOptions) {
        enabled = true;
        soundFile = psiOptions->getPluginOption(constSoundFile, QVariant(soundFile)).toString();
        disableSnd = psiOptions->getPluginOption(constDisableSnd, QVariant(disableSnd)).toBool();
        disablePopupDnd = psiOptions->getPluginOption(constDisablePopupDnd, QVariant(disablePopupDnd)).toBool();

        int interval = psiOptions->getPluginOption(constInterval, QVariant(3000)).toInt()/1000;
        popupId = popup->registerOption(POPUP_OPTION_NAME, interval, "plugins.options."+shortName()+"."+constInterval);

        QStringList jids = psiOptions->getPluginOption(constJids, QVariant(QStringList())).toStringList();
        QStringList soundFiles = psiOptions->getPluginOption(constSndFiles, QVariant(QStringList())).toStringList();
        QStringList enabledJids = psiOptions->getPluginOption(constEnabledJids, QVariant(QStringList())).toStringList();
        if (enabledJids.isEmpty()) {
            for (int i = 0; i < jids.size(); i++) {
                enabledJids << "true";
            }
        }

        if(!model_) {
            model_ = new Model(jids, soundFiles, enabledJids, this);
            connect(model_, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(Hack()));
        }

        items_.clear();
        QStringList list = psiOptions->getPluginOption(constWatchedItems).toStringList();
        foreach(const QString& settings, list) {
            WatchedItem* wi = new WatchedItem();
            wi->setSettings(settings);
            items_.push_back(wi);
            if(!wi->jid().isEmpty())
                wi->setText(wi->jid());
            else if(!wi->watchedText().isEmpty())
                wi->setText(wi->watchedText());
            else
                wi->setText(tr("Empty item"));
        }

        QStringList files;
        files << "watcher_on" << "watcher";
        foreach (QString filename, files) {

            QFile file(":/icons/" + filename + ".png");
            file.open(QIODevice::ReadOnly);
            QByteArray image = file.readAll();
            icoHost->addIcon("watcher/" + filename, image);
            file.close();
        }

        showInContext_ = psiOptions->getPluginOption(constShowInContext, QVariant(true)).toBool();
    }

    return enabled;
}

bool Watcher::disable() {
    delete model_;
    model_ = 0;

    qDeleteAll(items_);
    foreach (QAction* action, actions_) {
        action->disconnect();
        action->deleteLater();
    }
    items_.clear();
    actions_.clear();

    popup->unregisterOption(POPUP_OPTION_NAME);
    enabled = false;
    return true;
}

QWidget* Watcher::options() {
    if (!enabled) {
        return 0;
    }
    optionsWid = new QWidget();
    connect(optionsWid, SIGNAL(destroyed()), this, SLOT(onOptionsClose()));

    ui_.setupUi(optionsWid);

    restoreOptions();

    ui_.cb_hack->setVisible(false);
    ui_.tb_open->setIcon(icoHost->getIcon("psi/browse"));
    ui_.tb_test->setIcon(icoHost->getIcon("psi/play"));
    ui_.pb_add->setIcon(icoHost->getIcon("psi/addContact"));
    ui_.pb_del->setIcon(icoHost->getIcon("psi/remove"));
    ui_.pb_add_item->setIcon(icoHost->getIcon("psi/addContact"));
    ui_.pb_delete_item->setIcon(icoHost->getIcon("psi/remove"));
    ui_.pb_edit_item->setIcon(icoHost->getIcon("psi/action_templates_edit"));

    ui_.tableView->setModel(model_);
    ui_.tableView->init(icoHost);

    ui_.cb_showInContext->setChecked(showInContext_);

    connect(ui_.tableView, SIGNAL(checkSound(QModelIndex)), this, SLOT(checkSound(QModelIndex)));
    connect(ui_.tableView, SIGNAL(getSound(QModelIndex)), this, SLOT(getSound(QModelIndex)));
    connect(ui_.tb_test, SIGNAL(pressed()), this, SLOT(checkSound()));
    connect(ui_.tb_open, SIGNAL(pressed()), this, SLOT(getSound()));
    connect(ui_.pb_add, SIGNAL(released()), this, SLOT(addLine()));
    connect(ui_.pb_del, SIGNAL(released()), this, SLOT(delSelected()));

    connect(ui_.pb_add_item, SIGNAL(clicked()), this, SLOT(addItemAct()));
    connect(ui_.pb_delete_item, SIGNAL(clicked()), this, SLOT(delItemAct()));
    connect(ui_.pb_edit_item, SIGNAL(clicked()), this, SLOT(editItemAct()));
    connect(ui_.listWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(editItemAct()));

    return optionsWid;
}

void Watcher::addLine() {
    model_->addRow();
    Hack(); //activate apply button
}

void Watcher::delSelected() {
    ui_.tableView->deleteSelected();
    Hack(); //activate apply button
}

void Watcher::applyOptions() {
    soundFile = ui_.le_sound->text();
    psiOptions->setPluginOption(constSoundFile, QVariant(soundFile));

    disableSnd = ui_.cb_disable_snd->isChecked();
    psiOptions->setPluginOption(constDisableSnd, QVariant(disableSnd));

    //    Interval = ui_.sb_delay->value();
    //    psiOptions->setPluginOption(constInterval,QVariant(Interval));

    disablePopupDnd = ui_.cb_disableDnd->isChecked();
    psiOptions->setPluginOption(constDisablePopupDnd, QVariant(disablePopupDnd));

    model_->apply();
    psiOptions->setPluginOption(constEnabledJids, QVariant(model_->getEnabledJids()));
    psiOptions->setPluginOption(constJids, QVariant(model_->getWatchedJids()));
    psiOptions->setPluginOption(constSndFiles, QVariant(model_->getSounds()));

    foreach(WatchedItem *wi, items_)
    delete(wi);
    items_.clear();
    QStringList l;
    for(int i = 0; i < ui_.listWidget->count(); i++) {
        WatchedItem *wi = (WatchedItem*)ui_.listWidget->item(i);
        if(wi) {
            items_.push_back(wi->copy());
            l.push_back(wi->settingsString());
        }
    }

    psiOptions->setPluginOption(constWatchedItems, QVariant(l));

    showInContext_ = ui_.cb_showInContext->isChecked();

    psiOptions->setPluginOption(constShowInContext, QVariant(showInContext_));
}

void Watcher::restoreOptions() {
    ui_.le_sound->setText(soundFile);
    //    ui_.sb_delay->setValue(Interval);
    ui_.cb_disable_snd->setChecked(disableSnd);
    ui_.cb_disableDnd->setChecked(disablePopupDnd);
    model_->reset();
    foreach(WatchedItem* wi, items_) {
        ui_.listWidget->addItem(wi->copy());
    }
}

QPixmap Watcher::icon() const
{
    return QPixmap(":/icons/watcher.png");
}

bool Watcher::incomingStanza(int acc, const QDomElement &stanza) {
    if(enabled) {
        if(stanza.tagName() == "presence") {
            if(stanza.attribute("type") == "error")
                return false;

            QString from = stanza.attribute("from");
            if(from.isEmpty())
                return false;

            bool find = false;
            int index = model_->indexByJid(from);
            if(index >= 0) {
                if (model_->getEnabledJids().at(index) == "true") {
                    find = true;
                }
            }
            else {
                from = from.split("/").takeFirst();
                index = model_->indexByJid(from);
                if(index >= 0) {
                    if (model_->getEnabledJids().at(index) == "true") {
                        find = true;
                    }
                }
            }
            if(find) {
                QString status = stanza.firstChildElement("show").text();
                if(status.isEmpty()) {
                    if(stanza.attribute("type") == "unavailable") {
                        status = "offline";
                    }
                    else {
                        status = "online";
                        if(model_->statusByJid(from) != status && psiOptions->getGlobalOption("options.ui.notifications.sounds.enable").toBool()) {
                            QString snd = model_->soundByJid(from);
                            if(snd.isEmpty())
                                snd = soundFile;
                            playSound(snd);
                        }
                    }
                }
                if(model_->statusByJid(from) != status) {
                    model_->setStatusForJid(from, status);
                    status[0] = status[0].toUpper();
                    from = stanza.attribute("from"); // нужно быть уверенным, что у нас полный джид
                    const QString bare = from.split("/").first();
                    QString nick = contactInfo->name(acc, bare);
                    QString text;
                    if(!nick.isEmpty())
                        from = " [" + from + "]";
                    text = nick + from + tr(" change status to ") + status;
                    QMetaObject::invokeMethod(this, "showPopup", Qt::QueuedConnection,
                                              Q_ARG(int, acc),
                                              Q_ARG(const QString&, bare),
                                              Q_ARG(QString, text));
                }
            }
        }
        else if(stanza.tagName() == "message") {
            QString body = stanza.firstChildElement("body").text();
            if(!body.isEmpty()) {
                QString from = stanza.attribute("from");
                QString type = stanza.attribute("type");
                if(disableSnd) {
                    QString jid = activeTab->getJid();
                    if(jid.split("/").first().toLower() == from.split("/").first().toLower())
                        return false;
                }

                if(type == "groupchat") {
                    foreach(WatchedItem *wi, items_) {
                        if(!wi->groupChat())
                            continue;

                        if(checkWatchedItem(from, body, wi))
                            break;
                    }
                }
                else {
                    foreach(WatchedItem *wi, items_) {
                        if(wi->groupChat())
                            continue;

                        if(checkWatchedItem(from, body, wi))
                            break;
                    }
                }
            }
        }
    }
    return false;
}

bool Watcher::outgoingStanza(int /*account*/, QDomElement& /*xml*/) {
    return false;
}

bool Watcher::checkWatchedItem(const QString &from, const QString &body, WatchedItem *wi) {
    if(!wi->jid().isEmpty() && from.contains(QRegExp(wi->jid(),Qt::CaseInsensitive, QRegExp::Wildcard))) {
        isSndEnable = psiOptions->getGlobalOption("options.ui.notifications.sounds.enable").toBool();
        if(wi->alwaysUse() || isSndEnable) {
            psiOptions->setGlobalOption("options.ui.notifications.sounds.enable", QVariant(false));
            playSound(wi->sFile());
            QTimer::singleShot(500, this, SLOT(timeOut())); // включаем все звуки через секунду, чтобы не игралось два звука одновременно
            return true;
        }
    }
    if(!wi->watchedText().isEmpty()) {
        foreach(QString txt, wi->watchedText().split(QRegExp("\\s+"), QString::SkipEmptyParts)) {
            if(body.contains(QRegExp(txt, Qt::CaseInsensitive, QRegExp::Wildcard)) ) {
                psiOptions->setGlobalOption("options.ui.notifications.sounds.enable", QVariant(false));
                playSound(wi->sFile());
                QTimer::singleShot(500, this, SLOT(timeOut())); // включаем все звуки через секунду, чтобы не игралось два звука одновременно
                return true;
            }
        }
    }
    return false;
}


void Watcher::timeOut() {
    psiOptions->setGlobalOption("options.ui.notifications.sounds.enable", QVariant(isSndEnable));
}

void Watcher::setPopupAccessingHost(PopupAccessingHost* host) {
    popup = host;
}

void Watcher::setIconFactoryAccessingHost(IconFactoryAccessingHost* host) {
    icoHost = host;
}

void Watcher::setActiveTabAccessingHost(ActiveTabAccessingHost *host) {
    activeTab = host;
}

void Watcher::setOptionAccessingHost(OptionAccessingHost *host) {
    psiOptions = host;
}

void Watcher::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host) {
    appInfoHost = host;
}

void Watcher::setContactInfoAccessingHost(ContactInfoAccessingHost *host) {
    contactInfo = host;
}

void Watcher::optionChanged(const QString &option) {
    Q_UNUSED(option);
}

void Watcher::setAccountInfoAccessingHost(AccountInfoAccessingHost *host) {
    accInfo = host;
}

void Watcher::setSoundAccessingHost(SoundAccessingHost *host) {
    sound_ = host;
}

QAction* Watcher::createAction(QObject *parent, const QString &contact)
{
    QStringList jids = model_->getWatchedJids();
    QAction *action;
    if (jids.contains(contact, Qt::CaseInsensitive) && model_->jidEnabled(contact)) {
        action = new QAction(QIcon(":/icons/watcher_on.png"), tr("Don't watch for JID"), parent);
        action->setProperty("watch", true);
    }
    else {
        action = new QAction(QIcon(":/icons/watcher.png"), tr("Watch for JID"), parent);
        action->setProperty("watch", false);
    }

    action->setProperty("jid", contact);
    connect(action, SIGNAL(triggered()), SLOT(actionActivated()));

    return action;
}

QAction* Watcher::getAction(QObject *parent, int /*account*/, const QString &contact)
{
    if (!enabled) {
        return 0;
    }

    if (!actions_.contains(contact)) {
        QAction *action = createAction(parent, contact);
        connect(action, SIGNAL(destroyed(QObject*)), SLOT(removeFromActions(QObject*)));
        actions_[contact] = action;
    }
    return actions_[contact];
}

void Watcher::actionActivated()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (action->property("watch").toBool()) {
        action->setProperty("watch", false);
        action->setIcon(QIcon(":/icons/watcher.png"));
        action->setText(tr("Watch for JID"));
        model_->setJidEnabled(action->property("jid").toString(), false);
    }
    else {
        action->setProperty("watch", true);
        action->setIcon(QIcon(":/icons/watcher_on.png"));
        action->setText(tr("Don't watch for JID"));
        model_->setJidEnabled(action->property("jid").toString(), true);
    }
    model_->apply();
    psiOptions->setPluginOption(constEnabledJids, QVariant(model_->getEnabledJids()));
    psiOptions->setPluginOption(constJids, QVariant(model_->getWatchedJids()));
    psiOptions->setPluginOption(constSndFiles, QVariant(model_->getSounds()));
}

void Watcher::removeFromActions(QObject *object)
{
    actions_.remove(object->property("jid").toString());
}

void Watcher::playSound(const QString& f) {
    sound_->playSound(f);
}

void Watcher::getSound(QModelIndex index) {
    if(ui_.tb_open->isDown()) {
        QString fileName = QFileDialog::getOpenFileName(0,tr("Choose a sound file"),
                                                        psiOptions->getPluginOption(constLastFile, QVariant("")).toString(),
                                                        tr("Sound (*.wav)"));
        if(fileName.isEmpty()) return;
        QFileInfo fi(fileName);
        psiOptions->setPluginOption(constLastFile, QVariant(fi.absolutePath()));
        ui_.le_sound->setText(fileName);
    } else {
        QString fileName = QFileDialog::getOpenFileName(0,tr("Choose a sound file"),
                                                        psiOptions->getPluginOption(constLastFile, QVariant("")).toString(),
                                                        tr("Sound (*.wav)"));
        if(fileName.isEmpty()) return;
        QFileInfo fi(fileName);
        psiOptions->setPluginOption(constLastFile, QVariant(fi.absolutePath()));
        const QModelIndex editIndex = model_->index(index.row(), 2, QModelIndex());
        model_->setData(editIndex, QVariant(fileName));
    }
}

void Watcher::checkSound(QModelIndex index) {
    if(ui_.tb_test->isDown()) {
        playSound(ui_.le_sound->text());
    } else {
        playSound(model_->tmpSoundFile(index));
    }
}

void Watcher::showPopup(int account, const QString& jid, QString text) {
    QVariant suppressDnd = psiOptions->getGlobalOption("options.ui.notifications.passive-popups.suppress-while-dnd");
    psiOptions->setGlobalOption("options.ui.notifications.passive-popups.suppress-while-dnd", disablePopupDnd);

    int interval = popup->popupDuration(POPUP_OPTION_NAME);
    if(interval) {
        const QString statusMes = contactInfo->statusMessage(account, jid);
        if(!statusMes.isEmpty()) {
            text += tr("<br>Status Message: %1").arg(statusMes);
        }
        popup->initPopupForJid(account, jid, text, tr("Watcher Plugin"), "psi/search", popupId);
    }
    psiOptions->setGlobalOption("options.ui.notifications.passive-popups.suppress-while-dnd", suppressDnd);
}

void Watcher::Hack() {
    if (!optionsWid.isNull()) {
        ui_.cb_hack->toggle();
    }
}

void Watcher::onOptionsClose() {
    model_->reset();
}

QList < QVariantHash > Watcher::getAccountMenuParam() {
    return QList < QVariantHash >();
}

QList < QVariantHash > Watcher::getContactMenuParam() {
    return QList < QVariantHash >();
}

QAction* Watcher::getContactAction(QObject *p, int /*account*/, const QString &jid) {
    if (!enabled || !showInContext_) {
        return 0;
    }

    return createAction(p, jid);
}

void Watcher::addItemAct() {
    EditItemDlg *eid = new EditItemDlg(icoHost, psiOptions, optionsWid);
    connect(eid, SIGNAL(testSound(QString)), this, SLOT(playSound(QString)));
    connect(eid, SIGNAL(dlgAccepted(QString)), this, SLOT(addNewItem(QString)));
    eid->show();
}

void Watcher::addNewItem(const QString& settings) {
    WatchedItem *wi = new WatchedItem(ui_.listWidget);
    wi->setSettings(settings);
    if(!wi->jid().isEmpty())
        wi->setText(wi->jid());
    else if(!wi->watchedText().isEmpty())
        wi->setText(wi->watchedText());
    else
        wi->setText(tr("Empty item"));
    Hack();
}

void Watcher::delItemAct() {
    WatchedItem *wi = (WatchedItem*)ui_.listWidget->currentItem();
    if(wi) {
        int index = items_.indexOf(wi);
        if(index != -1)
            items_.removeAt(index);

        delete(wi);
        Hack();
    }
}

void Watcher::editItemAct() {
    WatchedItem *wi = (WatchedItem*)ui_.listWidget->currentItem();
    if(wi) {
        EditItemDlg *eid = new EditItemDlg(icoHost, psiOptions, optionsWid);
        eid->init(wi->settingsString());
        connect(eid, SIGNAL(testSound(QString)), this, SLOT(playSound(QString)));
        connect(eid, SIGNAL(dlgAccepted(QString)), this, SLOT(editCurrentItem(QString)));
        eid->show();
    }
}

void Watcher::editCurrentItem(const QString& settings) {
    WatchedItem *wi = (WatchedItem*)ui_.listWidget->currentItem();
    if(wi) {
        wi->setSettings(settings);
        if(!wi->jid().isEmpty())
            wi->setText(wi->jid());
        else if(!wi->watchedText().isEmpty())
            wi->setText(wi->watchedText());
        else
            wi->setText(tr("Empty item"));
        Hack();
    }
}

QString Watcher::pluginInfo() {
    return tr("Author: ") +     "Dealer_WeARE\n"
    + tr("Email: ") + "wadealer@gmail.com\n\n"
    + trUtf8("This plugin is designed to monitor the status of specific roster contacts, as well as for substitution of standard sounds of incoming messages.\n"
             "On the first tab set up a list of contacts for the status of which is monitored. When the status of such contacts changes a popup window will be shown"
             " and when the status changes to online a custom sound can be played."
             "On the second tab is configured list of items, the messages are being monitored. Each element can contain a regular expression"
             " to check for matches with JID, from which the message arrives, a list of regular expressions to check for matches with the text"
             " of an incoming message, the path to sound file which will be played in case of coincidence, as well as the setting, whether the sound"
             " is played always, even if the global sounds off. ");
}

#include "watcherplugin.moc"
