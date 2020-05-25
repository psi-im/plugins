/*
 * stopspamplugin.cpp - plugin
 * Copyright (C) 2009-2011  Evgeny Khryukin
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

#include "accountinfoaccessinghost.h"
#include "accountinfoaccessor.h"
#include "applicationinfoaccessinghost.h"
#include "applicationinfoaccessor.h"
#include "contactinfoaccessinghost.h"
#include "contactinfoaccessor.h"
#include "eventfilter.h"
#include "iconfactoryaccessinghost.h"
#include "iconfactoryaccessor.h"
#include "optionaccessinghost.h"
#include "optionaccessor.h"
#include "plugininfoprovider.h"
#include "popupaccessinghost.h"
#include "popupaccessor.h"
#include "psiplugin.h"
#include "stanzafilter.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"

#include "deferredstanzasender.h"
#include "model.h"
#include "ui_options.h"
#include "view.h"
#include "viewer.h"

#define cVer "0.5.8" // Plugin version

#define constQuestion "qstn"
#define constAnswer "answr"
#define constUnblocked "UnblockedList"
#define constJids "dsblJids"
#define constselected "slctd"
#define constCounter "cntr"
#define constHeight "Height"
#define constWidth "Width"
#define constCongratulation "cngrtltn"
#define constPopupNotify "popupntf"
#define constInterval "intrvl"
#define constTimes "times"
#define constResetTime "resettm"
#define constLogHistory "lghstr"
#define constDefaultAct "dfltact"

#define constUseMuc "usemuc"
#define constAdmin "affadmin"
#define constModer "rolemoder"
#define constOwner "affowner"
#define constMember "affmember"
#define constParticipant "roleparticipant"
#define constNone "affnone"
#define constVisitor "rolevisitor"
#define constBlockAll "blockall"
#define constBlockAllMes "blockallmes"
#define constEnableBlockAllMes "enableblockallmes"
#define constLastUnblock "lastunblock"

#define POPUP_OPTION "Stop Spam Plugin"

class StopSpam : public QObject,
                 public PsiPlugin,
                 public OptionAccessor,
                 public StanzaSender,
                 public StanzaFilter,
                 public AccountInfoAccessor,
                 public ApplicationInfoAccessor,
                 public PopupAccessor,
                 public IconFactoryAccessor,
                 public PluginInfoProvider,
                 public EventFilter,
                 public ContactInfoAccessor {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.psi-plus.StopSpam" FILE "psiplugin.json")
    Q_INTERFACES(PsiPlugin OptionAccessor StanzaSender StanzaFilter AccountInfoAccessor ApplicationInfoAccessor
                     PopupAccessor IconFactoryAccessor PluginInfoProvider EventFilter ContactInfoAccessor)

public:
    StopSpam();
    virtual QString             name() const;
    virtual QString             shortName() const;
    virtual QString             version() const;
    virtual PsiPlugin::Priority priority();
    virtual QWidget *           options();
    virtual bool                enable();
    virtual bool                disable();
    virtual void                applyOptions();
    virtual void                restoreOptions();
    virtual QPixmap             icon() const;
    virtual void                setOptionAccessingHost(OptionAccessingHost *host);
    virtual void                optionChanged(const QString &) { }
    virtual void                setStanzaSendingHost(StanzaSendingHost *host);
    virtual bool                incomingStanza(int account, const QDomElement &xml);
    virtual bool                outgoingStanza(int account, QDomElement &xml);
    virtual void                setAccountInfoAccessingHost(AccountInfoAccessingHost *host);
    virtual void                setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host);
    virtual void                setPopupAccessingHost(PopupAccessingHost *host);
    virtual void                setIconFactoryAccessingHost(IconFactoryAccessingHost *host);
    virtual void                setContactInfoAccessingHost(ContactInfoAccessingHost *host);
    virtual QString             pluginInfo();

    virtual bool processEvent(int, QDomElement &) { return false; }
    virtual bool processMessage(int, const QString &, const QString &, const QString &) { return false; }
    virtual bool processOutgoingMessage(int account, const QString &fromJid, QString &body, const QString &type,
                                        QString &subject);
    virtual void logout(int) { }

private slots:
    void resetCounter();
    void view();
    void close(int w, int h);
    void changeWidgetsState();
    void addRow();
    void removeRow();
    void hack();
    void onOptionsClose();

private:
    bool findMucNS(const QDomElement &stanza);
    void updateCounter(const QDomElement &stanza, bool b);
    bool findAcc(int account, const QString &Jid, int &i);
    bool findMuc(const QString &mucJid, const QString &nick, int &i);
    void logHistory(const QDomElement &stanza);
    bool processMuc(int account, const QDomElement &stanza);

    bool                          enabled;
    OptionAccessingHost *         psiOptions;
    DefferedStanzaSender *        stanzaHost;
    AccountInfoAccessingHost *    accInfoHost;
    ApplicationInfoAccessingHost *appInfoHost;
    IconFactoryAccessingHost *    icoHost;
    PopupAccessingHost *          popup;
    ContactInfoAccessingHost *    contactInfo;

    QString      Question;  //вопрос
    QString      Answer;    // ответ
    QString      Unblocked; // прошедшие проверку
    QStringList  Jids;      // список джидов правил
    QVariantList selected;  // список вкл\выкл
    int          Counter;   // счетчик
    int          Height;    //высота и ширина
    int          Width;
    QString      Congratulation; // поздравление
    bool         DefaultAct;     // выключить, если не подошло ни одно правило
    int          Times;          // сколько раз слать
    int          ResetTime;      // через сколько сбросить счетчик
    bool         LogHistory;     // логировать в историю
    bool         UseMuc, BlockAll,
        EnableBlockAllMes; // включечить для конф, блокировать все приваты, слать сообщение, если блокируем все приваты
    bool    Admin, Owner, None, Member;  // аффилиации
    bool    Moder, Participant, Visitor; // роли
    QString BlockAllMes; // сообщение, которое шлется приватам если блокируем все приваты

    struct Blocked { // структура, необходимая для подсчета кол-ва сообщений для конкретного джида
        int       Acc;
        QString   Jid;
        int       count;
        QDateTime LastMes;
    };
    struct MucUser { // структура, описывающая посетителя конференции
        QString mucJid;
        QString nick;
        QString jid;
        QString role;
        QString affiliation;
    };
    QVector<Blocked>  BlockedJids;
    QPointer<ViewLog> viewer;
    Model *           model_;
    QVector<MucUser>  mucUsers_;
    QPointer<QWidget> options_;
    Ui::Options       ui_;
    int               popupId;
};

StopSpam::StopSpam() :
    enabled(false), psiOptions(nullptr), stanzaHost(nullptr), accInfoHost(nullptr), appInfoHost(nullptr),
    icoHost(nullptr), popup(nullptr), contactInfo(nullptr), Question("2+3=?"), Answer("5"), Unblocked(""), Counter(0),
    Height(500), Width(600), Congratulation("Congratulations! Now you can chat!"), DefaultAct(false), Times(2),
    ResetTime(5), LogHistory(false), UseMuc(false), BlockAll(false), EnableBlockAllMes(true), Admin(false),
    Owner(false), None(true), Member(false), Moder(false), Participant(true), Visitor(true),
    BlockAllMes("The private messages are blocked! Send your message to groupchat, please."), viewer(nullptr),
    model_(nullptr), options_(nullptr), popupId(0)
{
}

QString StopSpam::name() const { return "Stop Spam Plugin"; }

QString StopSpam::shortName() const { return "stopspam"; }

QString StopSpam::version() const { return cVer; }

PsiPlugin::Priority StopSpam::priority() { return PriorityHighest; }

bool StopSpam::enable()
{
    if (psiOptions) {
        enabled = true;

        BlockedJids.clear();
        mucUsers_.clear();

        Question       = psiOptions->getPluginOption(constQuestion, QVariant(Question)).toString();
        Answer         = psiOptions->getPluginOption(constAnswer, QVariant(Answer)).toString();
        Congratulation = psiOptions->getPluginOption(constCongratulation, QVariant(Congratulation)).toString();
        Unblocked      = psiOptions->getPluginOption(constUnblocked, QVariant(Unblocked)).toString();
        DefaultAct     = psiOptions->getPluginOption(constDefaultAct, QVariant(DefaultAct)).toBool();
        Height         = psiOptions->getPluginOption(constHeight, QVariant(Height)).toInt();
        Width          = psiOptions->getPluginOption(constWidth, QVariant(Width)).toInt();
        Times          = psiOptions->getPluginOption(constTimes, QVariant(Times)).toInt();
        ResetTime      = psiOptions->getPluginOption(constResetTime, QVariant(ResetTime)).toInt();
        LogHistory     = psiOptions->getPluginOption(constLogHistory, QVariant(LogHistory)).toBool();
        Counter        = psiOptions->getPluginOption(constCounter, QVariant(Counter)).toInt();

        UseMuc            = psiOptions->getPluginOption(constUseMuc, QVariant(UseMuc)).toBool();
        BlockAll          = psiOptions->getPluginOption(constBlockAll, QVariant(BlockAll)).toBool();
        Admin             = psiOptions->getPluginOption(constAdmin, QVariant(Admin)).toBool();
        Owner             = psiOptions->getPluginOption(constOwner, QVariant(Owner)).toBool();
        None              = psiOptions->getPluginOption(constNone, QVariant(None)).toBool();
        Member            = psiOptions->getPluginOption(constMember, QVariant(Member)).toBool();
        Moder             = psiOptions->getPluginOption(constModer, QVariant(Moder)).toBool();
        Participant       = psiOptions->getPluginOption(constParticipant, QVariant(Participant)).toBool();
        Visitor           = psiOptions->getPluginOption(constVisitor, QVariant(Visitor)).toBool();
        BlockAllMes       = psiOptions->getPluginOption(constBlockAllMes, QVariant(BlockAllMes)).toString();
        EnableBlockAllMes = psiOptions->getPluginOption(constEnableBlockAllMes, QVariant(EnableBlockAllMes)).toBool();

        QDate luTime = QDate::fromString(
            psiOptions->getPluginOption(constLastUnblock, QVariant(QDate::currentDate().toString("yyyyMMdd")))
                .toString(),
            "yyyyMMdd");
        if (!Unblocked.isEmpty() && luTime.daysTo(QDate::currentDate()) > 3) {
            Unblocked.clear();
            psiOptions->setPluginOption(constUnblocked, QVariant(Unblocked));
        }

        Jids     = psiOptions->getPluginOption(constJids, QVariant(Jids)).toStringList();
        selected = psiOptions->getPluginOption(constselected, QVariant(selected)).value<QVariantList>();
        model_   = new Model(Jids, selected, this);
        connect(model_, &Model::dataChanged, this, &StopSpam::hack);

        // register popup option
        int interval = psiOptions->getPluginOption(constInterval, QVariant(5000)).toInt() / 1000;
        popupId = popup->registerOption(POPUP_OPTION, interval, "plugins.options." + shortName() + "." + constInterval);
    }
    return enabled;
}

bool StopSpam::disable()
{
    delete viewer;
    viewer = nullptr;
    delete model_;
    model_ = nullptr;
    delete stanzaHost;
    stanzaHost = nullptr;

    popup->unregisterOption(POPUP_OPTION);
    enabled = false;
    return true;
}

void StopSpam::applyOptions()
{
    if (!options_)
        return;

    Question = ui_.te_question->toPlainText();
    psiOptions->setPluginOption(constQuestion, Question);

    Answer = ui_.le_answer->text();
    psiOptions->setPluginOption(constAnswer, Answer);

    Congratulation = ui_.te_congratulation->toPlainText();
    psiOptions->setPluginOption(constCongratulation, Congratulation);

    DefaultAct = ui_.cb_default_act->isChecked();
    psiOptions->setPluginOption(constDefaultAct, QVariant(DefaultAct));

    Times = ui_.sb_times->value();
    psiOptions->setPluginOption(constTimes, Times);

    ResetTime = ui_.sb_reset->value();
    psiOptions->setPluginOption(constResetTime, ResetTime);

    LogHistory = ui_.cb_log_history->isChecked();
    psiOptions->setPluginOption(constLogHistory, LogHistory);

    UseMuc = ui_.cb_enable_muc->isChecked();
    psiOptions->setPluginOption(constUseMuc, UseMuc);

    BlockAll = ui_.cb_block_privates->isChecked();
    psiOptions->setPluginOption(constBlockAll, BlockAll);

    Admin = ui_.cb_admin->isChecked();
    psiOptions->setPluginOption(constAdmin, Admin);

    Owner = ui_.cb_owner->isChecked();
    psiOptions->setPluginOption(constOwner, Owner);

    None = ui_.cb_none->isChecked();
    psiOptions->setPluginOption(constNone, None);

    Member = ui_.cb_member->isChecked();
    psiOptions->setPluginOption(constMember, Member);

    Moder = ui_.cb_moderator->isChecked();
    psiOptions->setPluginOption(constModer, Moder);

    Participant = ui_.cb_participant->isChecked();
    psiOptions->setPluginOption(constParticipant, Participant);

    Visitor = ui_.cb_visitor->isChecked();
    psiOptions->setPluginOption(constVisitor, Visitor);

    EnableBlockAllMes = ui_.cb_send_block_all_mes->isChecked();
    psiOptions->setPluginOption(constEnableBlockAllMes, EnableBlockAllMes);

    BlockAllMes = ui_.te_muc->toPlainText();
    psiOptions->setPluginOption(constBlockAllMes, BlockAllMes);

    model_->apply();
    Jids     = model_->getJids();
    selected = model_->enableFor();
    psiOptions->setPluginOption(constJids, Jids);
    psiOptions->setPluginOption(constselected, selected);
}

void StopSpam::restoreOptions()
{
    if (!options_)
        return;

    ui_.te_question->setText(Question);
    ui_.le_answer->setText(Answer);
    ui_.te_congratulation->setText(Congratulation);
    ui_.cb_default_act->setChecked(DefaultAct);
    ui_.sb_times->setValue(Times);
    ui_.sb_reset->setValue(ResetTime);
    ui_.cb_log_history->setChecked(LogHistory);
    ui_.cb_enable_muc->setChecked(UseMuc);
    ui_.cb_block_privates->setChecked(BlockAll);
    ui_.cb_admin->setChecked(Admin);
    ui_.cb_owner->setChecked(Owner);
    ui_.cb_none->setChecked(None);
    ui_.cb_member->setChecked(Member);
    ui_.cb_moderator->setChecked(Moder);
    ui_.cb_participant->setChecked(Participant);
    ui_.cb_visitor->setChecked(Visitor);
    ui_.cb_send_block_all_mes->setChecked(EnableBlockAllMes);
    ui_.te_muc->setText(BlockAllMes);
    ui_.le_number->setText(QString::number(Counter));

    model_->reset();
}

QPixmap StopSpam::icon() const { return QPixmap(":/icons/stopspam.png"); }

QWidget *StopSpam::options()
{
    if (!enabled) {
        return nullptr;
    }
    options_ = new QWidget();
    ui_.setupUi(options_);
    connect(options_, &QWidget::destroyed, this, &StopSpam::onOptionsClose);

    ui_.tv_rules->setModel(model_);
    ui_.tv_rules->init();

    connect(ui_.cb_send_block_all_mes, &QCheckBox::stateChanged, this, &StopSpam::changeWidgetsState);
    connect(ui_.cb_enable_muc, &QCheckBox::stateChanged, this, &StopSpam::changeWidgetsState);
    connect(ui_.cb_block_privates, &QCheckBox::stateChanged, this, &StopSpam::changeWidgetsState);

    connect(ui_.pb_add, &QPushButton::released, this, &StopSpam::addRow);
    connect(ui_.pb_del, &QPushButton::released, this, &StopSpam::removeRow);

    connect(ui_.pb_reset, &QPushButton::released, this, &StopSpam::resetCounter);
    connect(ui_.pb_view, &QPushButton::released, this, &StopSpam::view);

    restoreOptions();
    changeWidgetsState();

    return options_;
}

void StopSpam::setOptionAccessingHost(OptionAccessingHost *host) { psiOptions = host; }

void StopSpam::setIconFactoryAccessingHost(IconFactoryAccessingHost *host) { icoHost = host; }

void StopSpam::setPopupAccessingHost(PopupAccessingHost *host) { popup = host; }

void StopSpam::setStanzaSendingHost(StanzaSendingHost *host) { stanzaHost = new DefferedStanzaSender(host); }

void StopSpam::setAccountInfoAccessingHost(AccountInfoAccessingHost *host) { accInfoHost = host; }

void StopSpam::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host) { appInfoHost = host; }

void StopSpam::setContactInfoAccessingHost(ContactInfoAccessingHost *host) { contactInfo = host; }

bool StopSpam::incomingStanza(int account, const QDomElement &stanza)
{
    if (enabled) {
        if (stanza.tagName() == "iq") {
            QDomElement query = stanza.firstChildElement("query");
            if (!Unblocked.isEmpty() && !query.isNull() && query.namespaceURI() == "jabber:iq:roster") {
                QStringList Roster        = accInfoHost->getRoster(account);
                QStringList UnblockedList = Unblocked.split("\n");
                while (!Roster.isEmpty()) {
                    QString jid = Roster.takeFirst();
                    UnblockedList.removeOne(jid);
                }
                Unblocked = "";
                while (!UnblockedList.isEmpty()) {
                    QString jid = UnblockedList.takeFirst();
                    if (jid != "") {
                        Unblocked += jid + "\n";
                    }
                }
                psiOptions->setPluginOption(constUnblocked, QVariant(Unblocked));
            }
        }

        QString from = stanza.attribute("from");
        QString to   = stanza.attribute("to");
        QString valF = from.split("/").takeFirst();
        QString valT = to.split("/").takeFirst();

        if (valF.toLower() == valT.toLower() || valF.toLower() == accInfoHost->getJid(account).toLower())
            return false;

        if (!from.contains("@"))
            return false;

        // Нам необходимо сделать эту проверку здесь,
        // иначе мы рискуем вообще ее не сделать
        if (stanza.tagName() == "message") {
            bool        findInvite = false;
            QString     invFrom;
            QDomElement x = stanza.firstChildElement("x");
            while (!x.isNull()) {
                QDomElement invite = x.firstChildElement("invite");
                if (!invite.isNull()) {
                    findInvite = true;
                    invFrom    = invite.attribute("from");
                    break;
                }
                x = x.nextSiblingElement("x");
            }
            if (findInvite) { // invite to MUC
                QStringList r = accInfoHost->getRoster(account);
                if (r.contains(invFrom.split("/").first(), Qt::CaseInsensitive))
                    return false;
                else {
                    bool findRule = false;
                    for (int i = 0; i < Jids.size(); i++) {
                        QString jid_ = Jids.at(i);
                        if (jid_.isEmpty())
                            continue;
                        if (invFrom.contains(jid_, Qt::CaseInsensitive)) {
                            findRule = true;
                            if (!selected[i].toBool())
                                return false;
                            break;
                        }
                    }
                    if (!findRule && DefaultAct)
                        return false;
                    else {
                        updateCounter(stanza, false);
                        return true;
                    }
                }
            }
        }

        if (contactInfo->isConference(account, valF) || contactInfo->isPrivate(account, from) || findMucNS(stanza)) {
            if (UseMuc)
                return processMuc(account, stanza);
            else
                return false;
        }

        QStringList Roster = accInfoHost->getRoster(account);
        if (Roster.isEmpty() || Roster.contains("-1"))
            return false;
        if (Roster.contains(valF, Qt::CaseInsensitive))
            return false;

        QStringList UnblockedJids = Unblocked.split("\n");
        if (UnblockedJids.contains(valF, Qt::CaseInsensitive))
            return false;

        bool findRule = false;
        for (int i = 0; i < Jids.size(); i++) {
            QString jid_ = Jids.at(i);
            if (jid_.isEmpty())
                continue;
            if (from.contains(jid_, Qt::CaseInsensitive)) {
                findRule = true;
                if (!selected[i].toBool())
                    return false;
                break;
            }
        }
        if (!findRule && DefaultAct)
            return false;

        if (stanza.tagName() == "message") {
            QString subj = stanza.firstChildElement("subject").text();
            QString type = "";
            type         = stanza.attribute("type");
            if (type == "error" && subj == "StopSpam Question") {
                updateCounter(stanza, false);
                return true;
            }

            if (subj == "AutoReply" || subj == "StopSpam" || subj == "StopSpam Question")
                return false;

            if (type == "groupchat" || type == "error")
                return false;

            QDomElement captcha = stanza.firstChildElement("captcha");
            if (!captcha.isNull() && captcha.namespaceURI() == "urn:xmpp:captcha")
                return false; // CAPTCHA

            QDomElement Body = stanza.firstChildElement("body");
            if (!Body.isNull()) {
                QString BodyText = Body.text();
                if (BodyText == Answer) {
                    Unblocked += valF + "\n";
                    psiOptions->setPluginOption(constUnblocked, QVariant(Unblocked));
                    psiOptions->setPluginOption(constLastUnblock, QVariant(QDate::currentDate().toString("yyyyMMdd")));
                    stanzaHost->sendMessage(account, from, Congratulation, "StopSpam", "chat");
                    updateCounter(stanza, true);
                    if (LogHistory)
                        logHistory(stanza);
                    return true;
                } else {
                    int i = BlockedJids.size();
                    if (findAcc(account, valF, i)) {
                        Blocked &B = BlockedJids[i];
                        if (B.count < Times) {
                            stanzaHost->sendMessage(account, from, Question, "StopSpam Question", "chat");
                            updateCounter(stanza, false);
                            if (LogHistory)
                                logHistory(stanza);
                            B.count++;
                            B.LastMes = QDateTime::currentDateTime();
                            return true;
                        } else {
                            if (QDateTime::currentDateTime().secsTo(B.LastMes) >= -ResetTime * 60) {
                                updateCounter(stanza, false);
                                if (LogHistory)
                                    logHistory(stanza);
                                return true;
                            } else {
                                B.count   = 1;
                                B.LastMes = QDateTime::currentDateTime();
                                stanzaHost->sendMessage(account, from, Question, "StopSpam Question", "chat");
                                updateCounter(stanza, false);
                                if (LogHistory)
                                    logHistory(stanza);
                                return true;
                            }
                        }
                    } else {
                        Blocked B = { account, valF, 1, QDateTime::currentDateTime() };
                        BlockedJids << B;
                        stanzaHost->sendMessage(account, from, Question, "StopSpam Question", "chat");
                        updateCounter(stanza, false);
                        if (LogHistory)
                            logHistory(stanza);
                        return true;
                    }
                }
            }
            updateCounter(stanza, false);
            return true;
        }

        if (stanza.tagName() == "presence") {
            QString type = stanza.attribute("type");
            if (type == "subscribe") {
                stanzaHost->sendMessage(account, from, Question, "StopSpam Question", "chat");
                stanzaHost->sendStanza(account, "<presence type=\"unsubscribed\" to=\"" + valF + "\" />");
                updateCounter(stanza, false);
                if (LogHistory)
                    logHistory(stanza);
                return true;
            } else
                return false;
        }

        if (stanza.tagName() == "iq" && stanza.attribute("type") == "set") {
            QString msg = QString("<iq type=\"error\" id=\"%1\" ").arg(stanza.attribute("id"));
            if (!from.isEmpty())
                msg += QString("to=\"%1\"").arg(from);
            msg += " />";
            stanzaHost->sendStanza(account, msg);
            updateCounter(stanza, false);
            return true;
        }

        return false;
    }
    return false;
}

bool StopSpam::findMucNS(const QDomElement &stanza)
{
    bool         find     = false;
    QDomNodeList nodeList = stanza.elementsByTagName("x");
    for (int i = 0; i < nodeList.size(); i++) {
        QDomElement item = nodeList.at(i).toElement();
        if (!item.isNull() && item.namespaceURI().contains("http://jabber.org/protocol/muc")) {
            find = true;
            break;
        }
    }

    return find;
}

bool StopSpam::outgoingStanza(int /*account*/, QDomElement & /*xml*/) { return false; }

bool StopSpam::processOutgoingMessage(int acc, const QString &fromJid, QString &body, const QString &type,
                                      QString & /*subject*/)
{
    if (enabled && type != "groupchat" && !body.isEmpty()) {
        QString bareJid;
        if (contactInfo->isPrivate(acc, fromJid)) {
            bareJid = fromJid;
        } else {
            bareJid = fromJid.split("/").first();
            if (contactInfo->inList(acc, bareJid))
                return false;
        }
        if (!Unblocked.split("\n").contains(bareJid, Qt::CaseInsensitive)) {
            Unblocked += bareJid + "\n";
            psiOptions->setPluginOption(constUnblocked, QVariant(Unblocked));
            psiOptions->setPluginOption(constLastUnblock, QVariant(QDate::currentDate().toString("yyyyMMdd")));
        }
    }
    return false;
}

void StopSpam::updateCounter(const QDomElement &stanza, bool b)
{
    ++Counter;
    psiOptions->setPluginOption(constCounter, QVariant(Counter));
    QString path = appInfoHost->appProfilesDir(ApplicationInfoAccessingHost::DataLocation);
    QFile   file(path + "/Blockedstanzas.log");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QString     date = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss");
        QTextStream out(&file);
        out.setCodec("UTF-8");
        // out.seek(file.size());
        out.setGenerateByteOrderMark(false);
        out << date << endl << stanza << endl;
    }

    if (!popup->popupDuration(POPUP_OPTION))
        return;

    if (!b) {
        QString popupText = tr("Block stanza from ") + stanza.attribute("from");
        popup->initPopup(popupText, tr("Stop Spam Plugin"), "psi/cancel", popupId);
    } else {
        QString popupText = stanza.attribute("from") + tr(" pass the test");
        popup->initPopup(popupText, tr("Stop Spam Plugin"), "psi/headline", popupId);
    }
}

bool StopSpam::findAcc(int account, const QString &Jid, int &i)
{
    for (; i > 0;) {
        Blocked Block = BlockedJids[--i];
        if (Block.Acc == account && Block.Jid == Jid) {
            return true;
        }
    }
    return false;
}

void StopSpam::resetCounter()
{
    Counter = 0;
    psiOptions->setPluginOption(constCounter, QVariant(Counter));
    ui_.le_number->setText("0");
}

void StopSpam::view()
{
    if (viewer)
        viewer->raise();
    else {
        const QString &&path
            = appInfoHost->appProfilesDir(ApplicationInfoAccessingHost::DataLocation) + "/Blockedstanzas.log";
        viewer = new ViewLog(path, icoHost);
        connect(viewer, &ViewLog::onClose, this, &StopSpam::close);
        if (!viewer->init())
            return;
        viewer->resize(Width, Height);
        viewer->show();
    }
}

void StopSpam::close(int width, int height)
{
    Height = height;
    Width  = width;
    psiOptions->setPluginOption(constHeight, QVariant(Height));
    psiOptions->setPluginOption(constWidth, QVariant(Width));
}

void StopSpam::logHistory(const QDomElement &stanza)
{
    QString folder   = appInfoHost->appHistoryDir();
    QString filename = stanza.attribute("from").split("/").takeFirst() + QString::fromUtf8(".history");
    filename.replace("%", "%25");
    filename.replace("_", "%5f");
    filename.replace("-", "%2d");
    filename.replace("@", "_at_");
    QFile file(folder + "/" + filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append))
        return;

    QString time = QDateTime::currentDateTime().toString("|yyyy-MM-ddThh:mm:ss|");
    QString type;
    if (stanza.tagName() == "presence")
        type = "3|";
    else
        type = "1|";
    QString body = stanza.firstChildElement("body").text();
    if (body.isEmpty())
        body = "subscribe";
    QString     outText = time + type + QString::fromUtf8("from|N---|") + body;
    QTextStream out(&file);
    out.setCodec("UTF-8");
    // out.seek(file.size());
    out.setGenerateByteOrderMark(false);
    out << outText << endl;
}

bool StopSpam::processMuc(int account, const QDomElement &stanza)
{
    if (stanza.tagName() == "presence") {
        QStringList jidList = stanza.attribute("from").split("/");
        int         i       = mucUsers_.size();
        if (findMuc(jidList.first(), jidList.last(), i)) {
            MucUser &    mu       = mucUsers_[i];
            QDomNodeList nodeList = stanza.elementsByTagName("x");
            for (int i = nodeList.size(); i > 0;) {
                QDomNode node = nodeList.at(--i).firstChild();
                while (!node.isNull()) {
                    QDomElement item = node.toElement();
                    if (item.tagName() == "item") {
                        mu.affiliation = item.attribute("affiliation");
                        mu.role        = item.attribute("role");
                        mu.jid         = item.attribute("jid");
                        break;
                    }
                    node = node.nextSibling();
                }
            }
        } else {
            MucUser mu;
            mu.mucJid             = jidList.first();
            mu.nick               = jidList.last();
            QDomNodeList nodeList = stanza.elementsByTagName("x");
            for (int i = nodeList.size(); i > 0;) {
                QDomNode node = nodeList.at(--i).firstChild();
                while (!node.isNull()) {
                    QDomElement item = node.toElement();
                    if (item.tagName() == "item") {
                        mu.affiliation = item.attribute("affiliation");
                        mu.role        = item.attribute("role");
                        mu.jid         = item.attribute("jid");
                        break;
                    }
                    node = node.nextSibling();
                }
            }
            mucUsers_ << mu;
        }
    } else if (stanza.tagName() == "message" && stanza.attribute("type") == "chat") {
        QDomElement subj = stanza.firstChildElement("subject");
        if (subj.text() == "StopSpam" || subj.text() == "StopSpam Question")
            return false;

        QString valF = stanza.attribute("from");
        if (contactInfo->isConference(account, valF))
            return false;

        MucUser     mu;
        QStringList jidList = valF.split("/");
        int         i       = mucUsers_.size();
        if (findMuc(jidList.first(), jidList.last(), i)) {
            mu = mucUsers_[i];
        } else {
            mu.affiliation = "";
            mu.jid         = "";
            mu.mucJid      = "";
            mu.nick        = "";
            mu.role        = "";
        }

        bool find = false;

        if (mu.affiliation == "owner" && !Owner)
            find = true;
        else if (mu.affiliation == "admin" && !Admin)
            find = true;
        else if (mu.affiliation == "none" && !None)
            find = true;
        else if (mu.affiliation == "member" && !Member)
            find = true;
        if (find)
            return false;

        if (mu.role == "moderator" && !Moder)
            find = true;
        else if (mu.role == "participant" && !Participant)
            find = true;
        else if (mu.role == "visitor" && !Visitor)
            find = true;
        if (find)
            return false;

        QStringList UnblockedJids = Unblocked.split("\n");
        if (UnblockedJids.contains(valF, Qt::CaseInsensitive))
            return false;

        for (int i = 0; i < Jids.size(); i++) {
            QString jid_ = Jids.at(i);
            if (jid_.isEmpty())
                continue;
            if (mu.jid.contains(jid_, Qt::CaseInsensitive) || mu.nick.contains(jid_, Qt::CaseInsensitive)
                || mu.mucJid.contains(jid_, Qt::CaseInsensitive)) {
                if (!selected[i].toBool())
                    return false;
                break;
            }
        }

        QDomElement Body = stanza.firstChildElement("body");
        if (Body.isNull())
            return false;

        if (BlockAll) {
            updateCounter(stanza, false);

            if (EnableBlockAllMes)
                stanzaHost->sendMessage(account, valF, BlockAllMes, "StopSpam", "chat");

            return true;
        }

        QString BodyText = Body.text();
        if (BodyText == Answer) {
            Unblocked += valF + "\n";
            QVariant vUnblocked(Unblocked);
            psiOptions->setPluginOption(constUnblocked, vUnblocked);
            psiOptions->setPluginOption(constLastUnblock, QVariant(QDate::currentDate().toString("yyyyMMdd")));
            stanzaHost->sendMessage(account, valF, Congratulation, "StopSpam", "chat");
            updateCounter(stanza, true);
            return true;
        } else {
            int i = BlockedJids.size();
            if (findAcc(account, valF, i)) {
                Blocked &B = BlockedJids[i];
                if (B.count < Times) {
                    stanzaHost->sendMessage(account, valF, Question, "StopSpam Question", "chat");
                    updateCounter(stanza, false);
                    B.count++;
                    B.LastMes = QDateTime::currentDateTime();
                    return true;
                } else {
                    if (QDateTime::currentDateTime().secsTo(B.LastMes) >= -ResetTime * 60) {
                        updateCounter(stanza, false);
                        return true;
                    } else {
                        B.count   = 1;
                        B.LastMes = QDateTime::currentDateTime();
                        stanzaHost->sendMessage(account, valF, Question, "StopSpam Question", "chat");
                        updateCounter(stanza, false);
                        return true;
                    }
                }
            } else {
                Blocked B = { account, valF, 1, QDateTime::currentDateTime() };
                BlockedJids << B;
                stanzaHost->sendMessage(account, valF, Question, "StopSpam Question", "chat");
                updateCounter(stanza, false);
                return true;
            }
        }
    }

    return false;
}

void StopSpam::changeWidgetsState()
{
    ui_.gb_affiliations->setEnabled(ui_.cb_enable_muc->isChecked());
    ui_.gb_rules->setEnabled(ui_.cb_enable_muc->isChecked());
    ui_.cb_block_privates->setEnabled(ui_.cb_enable_muc->isChecked());
    ui_.cb_send_block_all_mes->setEnabled(ui_.cb_enable_muc->isChecked() && ui_.cb_block_privates->isChecked());
    ui_.te_muc->setEnabled(ui_.cb_enable_muc->isChecked() && ui_.cb_block_privates->isChecked()
                           && ui_.cb_send_block_all_mes->isChecked());
}

void StopSpam::addRow()
{
    model_->addRow();
    hack();
}

void StopSpam::removeRow()
{
    if (model_->rowCount() > 1) {
        QModelIndex index = ui_.tv_rules->currentIndex();
        if (index.isValid()) {
            model_->deleteRow(index.row());
            hack();
        }
    }
}

void StopSpam::hack()
{
    ui_.cb_admin->toggle();
    ui_.cb_admin->toggle();
}

bool StopSpam::findMuc(const QString &mucJid, const QString &nick, int &i)
{
    for (; i > 0;) {
        MucUser mu = mucUsers_[--i];
        if (mu.mucJid == mucJid && mu.nick == nick) {
            return true;
        }
    }
    return false;
}

void StopSpam::onOptionsClose() { model_->reset(); }

QString StopSpam::pluginInfo()
{
    return tr(
        "This plugin is designed to block spam messages and other unwanted information from Psi users."
        "The functionality of the plugin is based on the principle of \"question - answer\".\n"
        "With the plugin settings you can:\n"
        "* Define a security question and the answer\n"
        "* Define the set of rules that define whether to the trigger plugin for a contact\n"
        "* Define the text messages sent in the case of the correct answer\n"
        "* Enable notification through popups\n"
        "* Enable the saving of blocked messages in the history of the contact\n"
        "* Define the number of subject parcels\n"
        "* Set the time interval after which to reset the number of how many questions will be sent\n"
        "* Enable blocking of private messages in groupchats\n"
        "* Choose for which ranks and roles of groupchat participants blocking messages will be disabled\n"
        "* Enable deadlocks in private messages to participants who do not fall into the exceptions list for the "
        "roles and ranks which include blocking.\n\n"

        "The rules are checked from top to bottom. If the rule is Enabled - stopspam is triggered, otherwise - "
        "stopspam is not triggered."
        " In the case where none of the rules triggered stopspam for roster messages, you can specify whether the "
        "plugin will activate or not."
        " For private messages from the same groupchat, it will always work.\n"
        "Question and answer as well as a list of rules is common for ordinary messages and for private messages "
        "in groupchats.\n"
        "When a user has passed, the test will send a re-authorization request. It should be noted in the "
        "messages that are sent back"
        " the security question was correctly answered.\n"
        "The plugin keeps a log of blocked messages, which you can view through the plugin settings. The "
        "\"Reset\" button deletes the log"
        " and resets the counter of blocked messages.\n\n"
        "WARNING!!! Before registering a new transport, it is recommended to add its jid to transport exceptions. "
        "This is due to the fact"
        " that after the transport registration, authorization requests for all contacts will be sent and if the "
        "transport was not added to"
        " as an exception, the plugin will block all the requests.");
}

#include "stopspamplugin.moc"
