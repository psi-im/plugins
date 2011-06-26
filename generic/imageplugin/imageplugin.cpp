/*
 * imageplugin.cpp - plugin
 * Copyright (C) 2009-2010  VampiRus
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

#include "psiplugin.h"
#include "activetabaccessinghost.h"
#include "activetabaccessor.h"
#include "toolbariconaccessor.h"
#include "iconfactoryaccessor.h"
#include "iconfactoryaccessinghost.h"
#include "gctoolbariconaccessor.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"
#include "accountinfoaccessinghost.h"
#include "accountinfoaccessor.h"
#include "plugininfoprovider.h"
#include <QByteArray>
#include <QFile>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

#define constVersion "0.1.1"

class ImagePlugin : public QObject, public PsiPlugin, public ToolbarIconAccessor
, public GCToolbarIconAccessor, public StanzaSender, public IconFactoryAccessor
, public ActiveTabAccessor, public PluginInfoProvider, public AccountInfoAccessor
{
        Q_OBJECT
        Q_INTERFACES(PsiPlugin ToolbarIconAccessor GCToolbarIconAccessor
	StanzaSender ActiveTabAccessor
	IconFactoryAccessor AccountInfoAccessor PluginInfoProvider)
public:
        virtual QString name() const;
        virtual QString shortName() const;
        virtual QString version() const;
        virtual QWidget* options();
        virtual bool enable();
        virtual bool disable();

        virtual void applyOptions() {};
        virtual void restoreOptions() {};
	virtual QList < QVariantHash > getButtonParam();
	virtual QAction* getAction(QObject* , int , const QString& ) { return 0; };
	virtual QList < QVariantHash > getGCButtonParam();
	virtual QAction* getGCAction(QObject* , int , const QString& ) { return 0; };
        virtual void setIconFactoryAccessingHost(IconFactoryAccessingHost* host);
        virtual void setStanzaSendingHost(StanzaSendingHost *host);
        virtual void setActiveTabAccessingHost(ActiveTabAccessingHost* host);
        virtual void setAccountInfoAccessingHost(AccountInfoAccessingHost* host);
	virtual QString pluginInfo();

private:
        IconFactoryAccessingHost* iconhost;
        StanzaSendingHost* stanzaSender;
        ActiveTabAccessingHost* activeTab;
        AccountInfoAccessingHost* accInfo;
	bool enabled;
        QHash<QString,int> accounts_;
private slots:
        void fileSelect();
        
};

Q_EXPORT_PLUGIN(ImagePlugin);


QString ImagePlugin::name() const {
	return "Image Plugin";
}

QString ImagePlugin::shortName() const {
        return "image";
}
QString ImagePlugin::version() const {
        return constVersion;
}
bool ImagePlugin::enable(){
	QFile file(":/imageplugin/imageplugin.gif");
	if ( file.open(QIODevice::ReadOnly) ) {
		QByteArray image = file.readAll();
		iconhost->addIcon("imageplugin/icon",image);
		file.close();
		enabled = true;
	} else {
		enabled = false;
	}
	return enabled;
}
bool ImagePlugin::disable(){
	enabled = false;
        return true;
}
QWidget* ImagePlugin::options(){
        if (!enabled) {
		return 0;
	}
        QWidget *optionsWid = new QWidget();
        QVBoxLayout *vbox= new QVBoxLayout(optionsWid);
        QLabel *wikiLink = new QLabel(tr("<a href=\"http://psi-plus.com/wiki/plugins#image_plugin\">Wiki (Online)</a>"),optionsWid);
	wikiLink->setOpenExternalLinks(true);
        vbox->addWidget(wikiLink);
        vbox->addStretch();
        return optionsWid;
}

QList< QVariantHash > ImagePlugin::getButtonParam(){
	QVariantHash hash;
	hash["tooltip"] = QVariant(tr("Send Image"));
	hash["icon"] = QVariant(QString("imageplugin/icon"));
	hash["reciver"] = qVariantFromValue(qobject_cast<QObject *>(this));
	hash["slot"] = QVariant(SLOT(fileSelect()));
	QList< QVariantHash > l;
	l.push_back(hash);
	return l;
}

QList< QVariantHash > ImagePlugin::getGCButtonParam(){
	return getButtonParam();
}

void ImagePlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost* host){
    accInfo = host;
}

void ImagePlugin::setIconFactoryAccessingHost(IconFactoryAccessingHost* host){
        iconhost = host;
}

void ImagePlugin::setStanzaSendingHost(StanzaSendingHost *host){
        stanzaSender = host;
}

void ImagePlugin::setActiveTabAccessingHost(ActiveTabAccessingHost* host){
        activeTab = host;
}

void ImagePlugin::fileSelect(){
    if (!enabled) return;
    QString fileName("");
    QString jid = activeTab->getYourJid();
    QString jidToSend = activeTab->getJid();
    int account = 0;
    QString tmpJid("");
    while (jid != (tmpJid = accInfo->getJid(account))){
        ++account;
        if (tmpJid == "-1") return;
    }
    if ("offline" == accInfo->getStatus(account)){return;}
    fileName = QFileDialog::getOpenFileName(0,tr("Open Image"),"",tr("Images (*.png *.gif *.jpg *.jpeg *.ico)"));
    if ("" != fileName){
	QFile file(fileName);
	if ( file.open(QIODevice::ReadOnly) ) {
	    QByteArray image = file.readAll();
            QString imageBase64(QUrl::toPercentEncoding(image.toBase64()));
            if(image.length()>61440){
                QMessageBox msgBox;
                msgBox.setText(tr("The image size is too large."));
                msgBox.setInformativeText("image size must be less than 60 kb");
                msgBox.exec();
                return;
            }
	    QString mType = QLatin1String(sender()->parent()->metaObject()->className()) == "PsiChatDlg"? "chat" : "groupchat";
            QString msgHtml = "<message type=\""+mType+"\" to=\""+jidToSend+"\" id=\""+stanzaSender->uniqueId(account)+"\" ><body>"+tr("Image :)")+"</body><html xmlns=\"http://jabber.org/protocol/xhtml-im\">";
            msgHtml +="<body xmlns=\"http://www.w3.org/1999/xhtml\">";
            msgHtml += "<br/><img src=\"data:image/"+fileName.right(fileName.length() - fileName.lastIndexOf(".") - 1)+";base64,"+
                    imageBase64+"\" alt=\"img\"/> ";
            msgHtml += "</body></html></message>";
            stanzaSender->sendStanza(account, msgHtml);
	    file.close();
        }
    }
}

QString ImagePlugin::pluginInfo() {
	return tr("Author: ") +  "VampiRUS\n\n"
			+ trUtf8("This plugin is designed to send images to roster contacts.\n"
			 "Your contact's client must be support XEP-0071: XHTML-IM and support the data:URI scheme.\n"
			 "Note: To work correctly, the option options.ui.chat.central-toolbar  must be set to true.");
}

#include "imageplugin.moc"

