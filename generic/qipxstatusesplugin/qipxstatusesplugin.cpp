/*
 * qipxstatusesplugin.cpp - plugin
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
#include <QIcon>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore>

#include "contactstateaccessinghost.h"
#include "contactstateaccessor.h"
#include "plugininfoprovider.h"
#include "psiplugin.h"
#include "stanzafilter.h"

class QipXStatuses : public QObject,
                     public PsiPlugin,
                     public StanzaFilter,
                     public ContactStateAccessor,
                     public PluginInfoProvider {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.psi-plus.QipXStatuses" FILE "psiplugin.json")
    Q_INTERFACES(PsiPlugin StanzaFilter ContactStateAccessor PluginInfoProvider)

public:
    QipXStatuses();
    virtual QString  name() const;
    virtual QWidget *options();
    virtual bool     enable();
    virtual bool     disable();
    virtual void     applyOptions() { }
    virtual void     restoreOptions() { }
    virtual bool     incomingStanza(int account, const QDomElement &xml);
    virtual bool     outgoingStanza(int account, QDomElement &xml);
    virtual void     setContactStateAccessingHost(ContactStateAccessingHost *host);
    virtual QString  pluginInfo();

private:
    bool                       enabled;
    ContactStateAccessingHost *contactState;

    QDomElement activityToXml(QString type, QString specificType, QString text);
    QDomElement MoodToXml(QString type, QString text);
};

QipXStatuses::QipXStatuses()
{
    enabled      = false;
    contactState = nullptr;
}

QString QipXStatuses::name() const { return "Qip X-Statuses Plugin"; }

bool QipXStatuses::enable()
{
    enabled = true;
    return enabled;
}

bool QipXStatuses::disable()
{
    enabled = false;
    return true;
}

QWidget *QipXStatuses::options()
{
    if (!enabled)
        return nullptr;

    QWidget *    options = new QWidget;
    QVBoxLayout *layout  = new QVBoxLayout(options);
    QLabel *     wiki    = new QLabel;
    wiki->setText(tr("<a href=\"https://psi-plus.com/wiki/en:plugins#qip_x-statuses_plugin\">Wiki (Online)</a>"));
    wiki->setOpenExternalLinks(true);
    layout->addWidget(wiki);
    layout->addStretch();
    return options;
}

bool QipXStatuses::incomingStanza(int account, const QDomElement &stanza)
{
    if (enabled) {
        if (stanza.tagName() == "presence") {
            bool         xStat = false;
            QDomElement  emptyElem;
            QString      from      = stanza.attribute("from").split("/").first();
            QDomNodeList xElemList = stanza.elementsByTagName("x");
            QDomElement  xElem;
            for (int i = xElemList.size(); i > 0;) {
                xElem = xElemList.at(--i).toElement();
                if (xElem.namespaceURI() == "http://qip.ru/x-status") {
                    xStat           = true;
                    QString mood    = "";
                    QString act     = "";
                    QString specAct = "";
                    QString title   = "";
                    title           = xElem.firstChildElement("title").text();
                    int id          = xElem.attribute("id").toInt();
                    switch (id) {
                    case 1:
                        mood = "angry";
                        break;
                    case 2:
                        act     = "grooming";
                        specAct = "taking_a_bath";
                        break;
                    case 3:
                        mood = "tired";
                        break;
                    case 4:
                        act     = "relaxing";
                        specAct = "partying";
                        break;
                    case 5:
                        act     = "drinking";
                        specAct = "having_a_beer";
                        break;
                    case 6:
                        act     = "inactive";
                        specAct = "thinking";
                        break;
                    case 7:
                        act = "eating";
                        break;
                    case 8:
                        act     = "relaxing";
                        specAct = "watching_tv";
                        break;
                    case 9:
                        act     = "relaxing";
                        specAct = "socializing";
                        break;
                    case 10:
                        act     = "drinking";
                        specAct = "having_coffee";
                        break;
                    case 12:
                        act = "having_appointment";
                        break;
                    case 13:
                        act     = "relaxing";
                        specAct = "watching_a_movie";
                        break;
                    case 14:
                        mood = "happy";
                        break;
                    case 15:
                        act     = "talking";
                        specAct = "on_the_phone";
                        break;
                    case 16:
                        act     = "relaxing";
                        specAct = "gaming";
                        break;
                    case 17:
                        act     = "working";
                        specAct = "studying";
                        break;
                    case 18:
                        act     = "relaxing";
                        specAct = "shopping";
                        break;
                    case 19:
                        mood = "sick";
                        break;
                    case 20:
                        act     = "inactive";
                        specAct = "sleeping";
                        break;
                    case 21:
                        act     = "exercising";
                        specAct = "swimming";
                        break;
                    case 22:
                        act     = "relaxing";
                        specAct = "reading";
                        break;
                    case 23:
                        act = "working";
                        break;
                    case 24:
                        act     = "working";
                        specAct = "coding";
                        break;
                    case 25:
                        act     = "relaxing";
                        specAct = "going_out";
                        break;
                    case 26:
                        act     = "talking";
                        specAct = "on_video_phone";
                        break;
                    case 27:
                        act     = "talking";
                        specAct = "on_the_phone";
                        break;
                    case 28:
                        act     = "inactive";
                        specAct = "sleeping";
                        break;
                    case 29:
                        act = "grooming";
                        break;
                    case 30:
                        mood = "undefined";
                        break;
                    case 31:
                        act = "doing_chores";
                        break;
                    case 32:
                        mood = "in_love";
                        break;
                    case 33:
                        mood = "curious";
                        break;
                    case 34:
                        mood = "in_love";
                        break;
                    case 35:
                        act     = "working";
                        specAct = "writing";
                        break;
                    }

                    if (id != 11) {
                        if (!mood.isEmpty())
                            contactState->setMood(account, from, MoodToXml(mood, title));
                        else
                            contactState->setMood(account, from, emptyElem);
                        if (!act.isEmpty())
                            contactState->setActivity(account, from, activityToXml(act, specAct, title));
                        else
                            contactState->setActivity(account, from, emptyElem);

                        contactState->setTune(account, from, "");
                    } else {
                        contactState->setTune(account, from, title);
                        contactState->setActivity(account, from, emptyElem);
                        contactState->setMood(account, from, emptyElem);
                    }

                    break;
                }
            }
            if (!xStat) {
                QDomElement cElem = stanza.firstChildElement("c");
                if (!cElem.isNull() && cElem.namespaceURI() == "http://jabber.org/protocol/caps"
                    && cElem.attribute("node") == "http://qip.ru/caps") {
                    contactState->setMood(account, from, emptyElem);
                    contactState->setActivity(account, from, emptyElem);
                    contactState->setTune(account, from, "");
                }
            }
        }
    }
    return false;
}

bool QipXStatuses::outgoingStanza(int /*account*/, QDomElement & /*xml*/) { return false; }

void QipXStatuses::setContactStateAccessingHost(ContactStateAccessingHost *host) { contactState = host; }

QDomElement QipXStatuses::activityToXml(QString type, QString specificType, QString text)
{
    QDomDocument doc;
    QDomElement  activity = doc.createElementNS("http://jabber.org/protocol/activity", "activity");

    if (!type.isEmpty()) {
        QDomElement el = doc.createElement(type);

        if (!specificType.isEmpty()) {
            QDomElement elChild = doc.createElement(specificType);
            el.appendChild(elChild);
        }

        activity.appendChild(el);
    }

    if (!text.isEmpty()) {
        QDomElement el = doc.createElement("text");
        QDomText    t  = doc.createTextNode(text);
        el.appendChild(t);
        activity.appendChild(el);
    }

    return activity;
}

QDomElement QipXStatuses::MoodToXml(QString type, QString text)
{
    QDomDocument doc;
    QDomElement  mood = doc.createElementNS("http://jabber.org/protocol/mood", "mood");

    if (!type.isEmpty()) {
        QDomElement el = doc.createElement(type);
        mood.appendChild(el);
    }

    if (!text.isEmpty()) {
        QDomElement el = doc.createElement("text");
        QDomText    t  = doc.createTextNode(text);
        el.appendChild(t);
        mood.appendChild(el);
    }

    return mood;
}

QString QipXStatuses::pluginInfo()
{
    return tr("This plugin is designed to display x-statuses of contacts using the QIP Infium jabber client.");
}

#include "qipxstatusesplugin.moc"
