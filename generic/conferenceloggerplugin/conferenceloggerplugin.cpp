/*
 * conferenceloggerplugin.cpp - plugin
 * Copyright (C) 2009-2010  Khryukin Evgeny
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

#include <QByteArray>
#include <QComboBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDomElement>

#include "psiplugin.h"
#include "stanzafilter.h"
#include "accountinfoaccessor.h"
#include "accountinfoaccessinghost.h"
#include "applicationinfoaccessor.h"
#include "applicationinfoaccessinghost.h"
#include "optionaccessor.h"
#include "optionaccessinghost.h"
#include "activetabaccessinghost.h"
#include "activetabaccessor.h"
#include "iconfactoryaccessor.h"
#include "iconfactoryaccessinghost.h"
#include "gctoolbariconaccessor.h"
#include "plugininfoprovider.h"

#include "viewer.h"

#define cVer "0.2.0"
#define constHeight "Height"
#define constWidth "Width"
#define constlastItem "lastItem"
#define constShortCut "shortcut"

class ConferenceLogger: public QObject, public PsiPlugin, public StanzaFilter, public AccountInfoAccessor, public ApplicationInfoAccessor, public OptionAccessor,
public ActiveTabAccessor, public GCToolbarIconAccessor, public IconFactoryAccessor, public PluginInfoProvider
{
	Q_OBJECT
	Q_INTERFACES(PsiPlugin StanzaFilter AccountInfoAccessor ApplicationInfoAccessor OptionAccessor
		     ActiveTabAccessor GCToolbarIconAccessor IconFactoryAccessor PluginInfoProvider)

public:
	ConferenceLogger();
	virtual QString name() const;
	virtual QString shortName() const;
	virtual QString version() const;
        virtual QWidget* options();
	virtual bool enable();
        virtual bool disable();
        virtual void applyOptions();
        virtual void restoreOptions(){};
        virtual bool incomingStanza(int account, const QDomElement& xml);
	virtual bool outgoingStanza(int account, QDomElement& xml);
        virtual void setAccountInfoAccessingHost(AccountInfoAccessingHost* host);
        virtual void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host);
        virtual void setOptionAccessingHost(OptionAccessingHost* host);
        virtual void optionChanged(const QString& /*option*/){};
        virtual void setActiveTabAccessingHost(ActiveTabAccessingHost* host);
        virtual void setIconFactoryAccessingHost(IconFactoryAccessingHost* host);
	virtual QList < QVariantHash > getGCButtonParam();
	virtual QAction* getGCAction(QObject* , int , const QString& ) { return 0; };
	virtual QString pluginInfo();


private:
        bool enabled;
        AccountInfoAccessingHost *AccInfoHost;
        ApplicationInfoAccessingHost *AppInfoHost;
        OptionAccessingHost *psiOptions;
        ActiveTabAccessingHost* activeTab;
        IconFactoryAccessingHost *IcoHost;
        QString HistoryDir;
        void Logger(QString room, QString from, QString MyJid, QString Text, QString Stamp);
        QComboBox *FilesBox;
        QPushButton *viewButton;
        int Height;
        int Width;
        QString lastItem;
        void showLog(QString filename);

private slots:
        void view();
        void viewFromOpt();
        void onClose(int, int);
};

Q_EXPORT_PLUGIN(ConferenceLogger);

ConferenceLogger::ConferenceLogger() {
        enabled = false;
        AppInfoHost = 0;
        AccInfoHost = 0;
        psiOptions = 0;
        IcoHost = 0;
        activeTab = 0;
        HistoryDir = "";
        FilesBox = 0;
        viewButton = 0;
        Height = 500;
        Width = 600;
        lastItem = "";
}

QString ConferenceLogger::name() const {
        return "Conference Logger Plugin";
}

QString ConferenceLogger::shortName() const {
        return "logger";
}

QString ConferenceLogger::version() const {
        return cVer;
}

bool ConferenceLogger::enable() {
	QFile file(":/conferenceloggerplugin/openlog.gif");
	if ( file.open(QIODevice::ReadOnly) ) {
		QByteArray image = file.readAll();
		IcoHost->addIcon("loggerplugin/openlog",image);
		file.close();
	} else {
		enabled = false;
		return enabled;
	}
	if(psiOptions) {
		enabled = true;
		HistoryDir = AppInfoHost->appHistoryDir();
		Height = psiOptions->getPluginOption(constHeight, QVariant(Height)).toInt();
		Width = psiOptions->getPluginOption(constWidth, QVariant(Width)).toInt();
		lastItem = psiOptions->getPluginOption(constlastItem, QVariant(lastItem)).toString();
	}
	return enabled;
}

bool ConferenceLogger::disable() {
        enabled = false;
	return true;
}

QWidget* ConferenceLogger::options() {
	if(!enabled) {
		return 0;
	}
	QWidget *options = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout(options);
	QLabel *label = new QLabel(tr("You can find your logs here:"));
	QLineEdit *path = new QLineEdit;
	path->setText(HistoryDir);
	path->setEnabled(false);
	FilesBox = new QComboBox();
	QDir dir(HistoryDir);
	foreach(QString file, dir.entryList(QDir::Files)) {
		if(file.contains("_in_")) {
			FilesBox->addItem(file);
		}
	}
	for(int i = FilesBox->count(); i > 0; --i) {
		if(FilesBox->itemText(i) == lastItem) {
			FilesBox->setCurrentIndex(i);
		}
	}
	QHBoxLayout *filesLayout = new QHBoxLayout();
	filesLayout->addWidget(new QLabel(tr("Logs:")));
	filesLayout->addWidget(FilesBox);
	filesLayout->addStretch();
	viewButton = new QPushButton(IcoHost->getIcon("psi/search"), tr("View Log"));
	connect(viewButton, SIGNAL(released()), SLOT(viewFromOpt()));
	QLabel *wikiLink = new QLabel(tr("<a href=\"http://psi-plus.com/wiki/plugins#conference_logger_plugin\">Wiki (Online)</a>"));
	wikiLink->setOpenExternalLinks(true);
	filesLayout->addWidget(viewButton);
	layout->addWidget(label);
	layout->addWidget(path);
	layout->addLayout(filesLayout);
	layout->addStretch();
	layout->addWidget(wikiLink);
	return options;
}

bool ConferenceLogger::incomingStanza(int account, const QDomElement& stanza) {
	if (enabled) {
		if(stanza.tagName() == "message") {
			if(stanza.attribute("type") == "groupchat") {
				QString from = stanza.attribute("from");
				QStringList List = from.split("/");
				QString room = List.takeFirst();
				from = "";
				if(!List.isEmpty()) {
					from = List.join("/");
				}
				QString Stamp = "";
				Stamp = stanza.firstChildElement("x").attribute("stamp");
				QDomElement body = stanza.firstChildElement("body");
				if(!body.isNull()) {
					QString Text = body.text();
					QString MyJid = AccInfoHost->getJid(account);
					MyJid = MyJid.replace("@", "_at_");
					Logger(room, from, MyJid, Text, Stamp);
				}
			}
		}
	}
	return false;
}

bool ConferenceLogger::outgoingStanza(int /*account*/, QDomElement& /*xml*/)
{
	return false;
}

void ConferenceLogger::setAccountInfoAccessingHost(AccountInfoAccessingHost* host) {
	AccInfoHost = host;
}

void ConferenceLogger::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host) {
	AppInfoHost = host;
}

void ConferenceLogger::Logger(QString room, QString from, QString MyJid, QString Text, QString Stamp) {
	room = room.replace("@", "_at_");
	room = "_in_" + room;
	if(Stamp == "") {
		Stamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
	}
	else {
		Stamp.insert(4, "-");
		Stamp.insert(7, "-");
		Stamp.replace("T", " ");
	}
	QFile file(HistoryDir + QDir::separator() + MyJid + room);
	if(file.open(QIODevice::WriteOnly | QIODevice::Append)) {
		QTextStream out(&file);
		//out.seek(file.size());
		out.setCodec("UTF-8");
		out.setGenerateByteOrderMark(false);
		out << Stamp << "  " << from << ": " << Text << endl;
	}
}

void ConferenceLogger::applyOptions() {
	if (FilesBox == 0)  return;

	QVariant vlastItem(FilesBox->currentText());
	lastItem = vlastItem.toString();
	psiOptions->setPluginOption(constlastItem, vlastItem);
}

void ConferenceLogger::setOptionAccessingHost(OptionAccessingHost* host) {
	psiOptions = host;
}

void ConferenceLogger::viewFromOpt() {
	lastItem = FilesBox->currentText();
	psiOptions->setPluginOption(constlastItem, QVariant(lastItem));
	showLog(lastItem);
}

void ConferenceLogger::view() {
	if(!enabled) return;
	QString Jid = activeTab->getJid();
	QString YourJid = activeTab->getYourJid();
	if(Jid == "" || YourJid == "") {
		return;
	}
	Jid = Jid.replace("@", "_at_");
	QStringList List = YourJid.split("/");
	YourJid = List.takeFirst();
	YourJid = YourJid.replace("@", "_at_");
	QString FName = YourJid + "_in_" + Jid;
	QDir dir(HistoryDir);
	foreach(QString file, dir.entryList(QDir::Files)) {
		if(file == FName) {
			showLog(file);
			break;
		}
	}
}

void ConferenceLogger::showLog(QString filename) {
	filename = HistoryDir + "/" + filename;
	Viewer *v = new Viewer(filename, IcoHost);
	v->resize(Width, Height);
	if(!v->init()) {
		delete(v);
		return;
	}
	connect(v, SIGNAL(onClose(int,int)), this, SLOT(onClose(int,int)));
	v->show();
}

void ConferenceLogger::onClose(int w, int h) {
	Width = w;
	Height = h;
	psiOptions->setPluginOption(constWidth, QVariant(Width));
	psiOptions->setPluginOption(constHeight, QVariant(Height));
}

void ConferenceLogger::setActiveTabAccessingHost(ActiveTabAccessingHost* host) {
	activeTab = host;
}

void ConferenceLogger::setIconFactoryAccessingHost(IconFactoryAccessingHost *host) {
	IcoHost = host;
}

QList < QVariantHash > ConferenceLogger::getGCButtonParam() {
	QList< QVariantHash > l;
	QVariantHash hash;
	hash["tooltip"] = QVariant(tr("Conference History"));
	hash["icon"] = QVariant(QString("loggerplugin/openlog"));
	hash["reciver"] = qVariantFromValue(qobject_cast<QObject *>(this));
	hash["slot"] = QVariant(SLOT(view()));
	l.push_back(hash);
	return l;
}

QString ConferenceLogger::pluginInfo() {
	return tr("Author: ") +  "Dealer_WeARE\n"
			+ tr("Email: ") + "wadealer@gmail.com\n\n"
			+ trUtf8("This plugin is designed to save conference logs in which the Psi+ user sits.\n"
				 "Conferences logs can be viewed from the plugin settings or by clicking on the appropriate button on the toolbar in the active window/tab with conference.\n\n"
				 "Note: To work correctly, the option options.ui.chat.central-toolbar  must be set to true.");
}


#include "conferenceloggerplugin.moc"
