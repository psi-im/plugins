/*
 * historykeeperplugin.cpp - plugin
 * Copyright (C) 2010  Khryukin Evgeny
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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <QtGui>
#include <QtCore>
#include "qdebug.h"

#include "psiplugin.h"
#include "applicationinfoaccessor.h"
#include "applicationinfoaccessinghost.h"
#include "optionaccessor.h"
#include "optionaccessinghost.h"
#include "menuaccessor.h"
#include "plugininfoprovider.h"

#include "tooltip.h"

#define cVer "0.0.5"
#define constClearHistoryFor "clear-history-for"



class HistoryKeeperPlugin: public QObject, public PsiPlugin, public ApplicationInfoAccessor, public OptionAccessor, public MenuAccessor, public PluginInfoProvider
{
	Q_OBJECT
        Q_INTERFACES(PsiPlugin OptionAccessor ApplicationInfoAccessor MenuAccessor PluginInfoProvider)

public:
        HistoryKeeperPlugin();
	virtual QString name() const;
	virtual QString shortName() const;
	virtual QString version() const;
        virtual QWidget* options();
	virtual bool enable();
        virtual bool disable();
        virtual void applyOptions();
        virtual void restoreOptions();
        virtual void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host);
        virtual void setOptionAccessingHost(OptionAccessingHost* host);
        virtual void optionChanged(const QString& /*option*/) {};
	virtual QList < QVariantHash > getAccountMenuParam();
	virtual QList < QVariantHash > getContactMenuParam();
	virtual QAction* getContactAction(QObject* , int , const QString& ) { return 0; };
	virtual QAction* getAccountAction(QObject* , int ) { return 0; };
	virtual QString pluginInfo();


private:
        bool enabled;
        OptionAccessingHost* psiOptions; 
        ApplicationInfoAccessingHost *appInfo;
        QTextEdit *contactsWidget;
        QStringList contacts;

        void removeHistory();
        QString nameToFilename(QString name);

private slots:
        void addContact(QString jid);
        void removeContact(QString jid);
        void showToolTip();
        void checked(QString jid, bool check);

    };

Q_EXPORT_PLUGIN(HistoryKeeperPlugin);

HistoryKeeperPlugin::HistoryKeeperPlugin() {
        enabled = false;        
        appInfo = 0;
        psiOptions = 0;
        contactsWidget = 0;
        contacts.clear();
    }

QString HistoryKeeperPlugin::name() const {
        return "History Keeper Plugin";
    }

QString HistoryKeeperPlugin::shortName() const {
        return "historykeeper";
}

QString HistoryKeeperPlugin::version() const {
        return cVer;
}

bool HistoryKeeperPlugin::enable() {
    if(psiOptions) {
        enabled = true;
        contacts = psiOptions->getPluginOption(constClearHistoryFor, QVariant(contacts)).toStringList();
    }
    return enabled;
}

bool HistoryKeeperPlugin::disable() {
        removeHistory();
        enabled = false;
	return true;
}

void HistoryKeeperPlugin::removeHistory() {
    if(!enabled)
        return;

    QString historyDir(appInfo->appHistoryDir());
    foreach(QString jid, contacts) {
        jid = nameToFilename(jid);
        QString fileName = historyDir + QDir::separator() + jid;
        QFile file(fileName);
        if(file.open(QIODevice::ReadWrite)) {
            qDebug("Removing file %s", qPrintable(fileName));
            file.remove();
        }
    }
}

QString HistoryKeeperPlugin::nameToFilename(QString name) {    
    name = name.replace("%", "%25");
    name = name.replace("_", "%5f");
    name = name.replace("-", "%2d");
    name = name.replace("@", "_at_");
    name += QString::fromUtf8(".history");
    return name;
}

QWidget* HistoryKeeperPlugin::options() {
    if(!enabled) {
    return 0;
}
    QWidget *options = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(options);

    contactsWidget = new QTextEdit();
    QString text;
    foreach(QString contact, contacts) {
        text += contact + "\n";
    }
    contactsWidget->setMaximumWidth(300);
    contactsWidget->setText(text);

    QLabel *wikiLink = new QLabel(tr("<a href=\"http://psi-plus.com/wiki/plugins#history_keeper_plugin\">Wiki (Online)</a>"));
    wikiLink->setOpenExternalLinks(true);

    layout->addWidget(new QLabel(tr("Remove history for contacts:")));
    layout->addWidget(contactsWidget);
    layout->addWidget(wikiLink);

    return options;
}

void HistoryKeeperPlugin::addContact(QString jid) {
    if(!contacts.contains(jid)) {
        contacts.append(jid);
        psiOptions->setPluginOption(constClearHistoryFor, QVariant(contacts));
    }
}

void HistoryKeeperPlugin::removeContact(QString jid) {
    if(contacts.contains(jid)) {
        contacts.removeAt(contacts.indexOf(jid));
        psiOptions->setPluginOption(constClearHistoryFor, QVariant(contacts));
    }
}

void HistoryKeeperPlugin::showToolTip() {
    QString jid = sender()->property("jid").toString();
    bool checked = contacts.contains(jid);
    KeeperToolTip *tooltip = new KeeperToolTip(jid, checked, sender());
    connect(tooltip, SIGNAL(check(QString,bool)), this, SLOT(checked(QString,bool)));
    tooltip->show();
}

void HistoryKeeperPlugin::checked(QString jid, bool check) {
    if(check)
       addContact(jid) ;
    else
        removeContact(jid);
}

void HistoryKeeperPlugin::applyOptions() {
    if(!contactsWidget) return;

    contacts = contactsWidget->toPlainText().split(QRegExp("\\s+"), QString::SkipEmptyParts);
    psiOptions->setPluginOption(constClearHistoryFor, QVariant(contacts));
}

void HistoryKeeperPlugin::restoreOptions() {
    if(!contactsWidget) return;

    QString text;
    foreach(QString contact, contacts) {
        text += contact + "\n";
    }
    contactsWidget->setText(text);
}

void HistoryKeeperPlugin::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host) {
     appInfo = host;
 }

void HistoryKeeperPlugin::setOptionAccessingHost(OptionAccessingHost* host) {
    psiOptions = host;
}

QList < QVariantHash > HistoryKeeperPlugin::getAccountMenuParam() {
    return QList < QVariantHash >();
}

QList < QVariantHash > HistoryKeeperPlugin::getContactMenuParam() {
        QVariantHash hash;
        hash["icon"] = QVariant(QString("psi/clearChat"));
        hash["name"] = QVariant(tr("Clear history on exit"));
        hash["reciver"] = qVariantFromValue(qobject_cast<QObject *>(this));
        hash["slot"] = QVariant(SLOT(showToolTip()));
	QList< QVariantHash > l;
	l.push_back(hash);
        return l;
}

QString HistoryKeeperPlugin::pluginInfo() {
	return tr("Author: ") +  "Dealer_WeARE\n"
			+ tr("Email: ") + "wadealer@gmail.com\n\n"
			+ trUtf8("This plugin is designed to remove the history of selected contacts when the Psi+ is closed.\n"
			 "You can select or deselect a contact for history removal from the context menu of a contact or via the plugin options.");
}

#include "historykeeperplugin.moc"
