/*
 * juickplugin.cpp - plugin
 * Copyright (C) 2009-2010 Kravtsov Nikolai, Khryukin Evgeny
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <QtCore>
#include <QtGui>
#include <QDomElement>
#include <stdlib.h>
#include <time.h>


#include "psiplugin.h"
#include "eventfilter.h"
#include "stanzafilter.h"
#include "optionaccessor.h"
#include "optionaccessinghost.h"
#include "activetabaccessor.h"
#include "activetabaccessinghost.h"
#include "applicationinfoaccessor.h"
#include "applicationinfoaccessinghost.h"
#include "plugininfoprovider.h"

#include "http.h"
#include "juickjidlist.h"

#define constuserColor "usercolor"
#define consttagColor "tagcolor"
#define constmsgColor "idcolor"
#define constQcolor "quotecolor"
#define constLcolor "linkcolor"
#define constUbold "userbold"
#define constTbold "tagbold"
#define constMbold "idbold"
#define constQbold "quotebold"
#define constLbold "linkbold"
#define constUitalic "useritalic"
#define constTitalic "tagitalic"
#define constMitalic "iditalic"
#define constQitalic "quoteitalic"
#define constLitalic "linkitalic"
#define constUunderline "userunderline"
#define constTunderline "tagunderline"
#define constMunderline "idunderline"
#define constQunderline "quoteunderline"
#define constLunderline "linkunderline"
#define constIdAsResource "idAsResource"
#define constShowPhoto "showphoto"
#define constShowAvatars "showavatars"
#define constWorkInGroupchat "workingroupchat"


#define constVersion "0.10.6"
#define constPluginName "Juick Plugin"

class JuickPlugin : public QObject, public PsiPlugin, public EventFilter, public OptionAccessor, public ActiveTabAccessor,
					public StanzaFilter, public ApplicationInfoAccessor, public PluginInfoProvider
{
	Q_OBJECT
	Q_INTERFACES(PsiPlugin EventFilter OptionAccessor ActiveTabAccessor StanzaFilter
				 ApplicationInfoAccessor PluginInfoProvider)

		public:
		JuickPlugin();
	virtual QString name() const;
	virtual QString shortName() const;
	virtual QString version() const;
	virtual QWidget* options();
	virtual bool enable();
	virtual bool disable();
	virtual void applyOptions();
	virtual void restoreOptions();
	virtual bool processEvent(int account, QDomElement& e);
	virtual bool processMessage(int account, const QString& fromJid, const QString& body, const QString& subject);
	virtual bool processOutgoingMessage(int , const QString& , QString& , const QString& , QString& ) { return false; }
	virtual void logout(int ) {};
	// OptionAccessor
	virtual void setOptionAccessingHost(OptionAccessingHost* host);
	virtual void optionChanged(const QString& option);
	//ActiveTabAccessor
	virtual void setActiveTabAccessingHost(ActiveTabAccessingHost* host);
	//ApplicationInfoAccessor
	virtual void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host);
	virtual QString pluginInfo();
	//virtual void setAccountInfoAccessingHost(AccountInfoAccessingHost* host);

	virtual bool incomingStanza(int account, const QDomElement& stanza);
	virtual bool outgoingStanza(int account, QDomElement& stanza);
	void elementFromString(QDomElement& body,QDomDocument e, QString& msg, QString jid, QString resource = "");
	void nl2br(QDomElement& body,QDomDocument e, QString msg);
	void addPlus(QDomElement& body,QDomDocument e, QString msg, QString jid, QString resource = "");
	void addSubscribe(QDomElement& body,QDomDocument e, QString msg, QString jid, QString resource = "");
	void addHttpLink(QDomElement& body,QDomDocument e, QString msg);
	void addTagLink(QDomElement& body,QDomDocument e, QString tag, QString jid);
	void addUserLink(QDomElement& body,QDomDocument e, QString nick, QString altText, QString pattern, QString jid);
	void addMessageId(QDomElement& body,QDomDocument e, QString mId, QString altText,QString pattern, QString jid, QString resource = "");
	void addUnsubscribe(QDomElement& body,QDomDocument e, QString msg, QString jid, QString resource = "");
	void addDelete(QDomElement& body,QDomDocument e, QString msg, QString jid, QString resource = "");
	void addFavorite(QDomElement& body,QDomDocument e, QString msg, QString jid, QString resource = "");
private:
	bool enabled;
	QCheckBox *ubButton;
	QCheckBox *uiButton;
	QCheckBox *uuButton;
	QCheckBox *tbButton;
	QCheckBox *tiButton;
	QCheckBox *tuButton;
	QCheckBox *mbButton;
	QCheckBox *miButton;
	QCheckBox *muButton;
	QCheckBox *qbButton;
	QCheckBox *qiButton;
	QCheckBox *quButton;
	QCheckBox *lbButton;
	QCheckBox *liButton;
	QCheckBox *luButton;
	QToolButton *ucButton;
	QToolButton *tcButton;
	QToolButton *mcButton;
	QToolButton *qcButton;
	QToolButton *lcButton;
	QColor userColor;
	QColor tagColor;
	QColor msgColor;
	QColor quoteColor;
	QColor lineColor;
	QCheckBox *asResourceButton;
	QCheckBox *showPhotoButton;
	QCheckBox *showAvatarsButton;
	QCheckBox *groupChatButton;
	OptionAccessingHost* psiOptions;
	ActiveTabAccessingHost* activeTab;
	ApplicationInfoAccessingHost* applicationInfo;
	bool userBold;
	bool tagBold;
	bool msgBold;
	bool quoteBold;
	bool lineBold;
	bool userItalic;
	bool tagItalic;
	bool msgItalic;
	bool quoteItalic;
	bool lineItalic;
	bool userUnderline;
	bool tagUnderline;
	bool msgUnderline;
	bool quoteUnderline;
	bool lineUnderline;
	QString juick;
	QString jubo;
	QString idStyle;
	QString userStyle;
	QString tagStyle;
	QString quoteStyle;
	QString linkStyle;
	QRegExp tagRx;
	QRegExp pmRx;
	QRegExp postRx;
	QRegExp replyRx;
	QRegExp regx;
	QRegExp rpostRx;
	QRegExp threadRx;
	QRegExp userRx;
	QRegExp singleMsgRx;
	QRegExp lastMsgRx;
	QRegExp juboRx;
	QRegExp msgPostRx;
	QRegExp delMsgRx;
	QRegExp delReplyRx;
	QRegExp idRx;
	QRegExp nickRx;
	QRegExp recomendRx;
	QString userLinkPattern;
	QString messageLinkPattern;
	QString altTextUser;
	QString altTextMsg;
	QString commonLinkColor;
	bool idAsResource;
	bool showPhoto;
	bool showAvatars;
	bool workInGroupChat;
	QString showAllmsgString;
	QString replyMsgString;
	QString userInfoString;
	QString subscribeString;
	QString showLastTenString;
	QString unsubscribeString;
	QStringList jidList_;
	QPointer<QWidget> optionsWid;

private slots:
	void chooseColor(QAbstractButton*);
	void clearCache();
	void updateJidList(QStringList jids);
	void requestJidList();
};

Q_EXPORT_PLUGIN(JuickPlugin);

JuickPlugin::JuickPlugin():enabled(false)
		, juick("juick@juick.com")
		, jubo("jubo@nologin.ru")
		, idStyle("")
		, userStyle("")
		, tagStyle("")
		, quoteStyle("")
		, linkStyle("")
		, tagRx		  ("^\\s*(?!\\*\\S+\\*)(\\*\\S+)")
		, pmRx		  ("^\\nPrivate message from (@.+):(.*)$")
		, postRx		  ("\\n@(\\S*):( \\*[^\\n]*){0,1}\\n(.*)\\n\\n(#\\d+)\\s(http://\\S*)\\n$")
		, replyRx		  ("\\nReply by @(.*):\\n>(.{,50})\\n\\n(.*)\\n\\n(#\\d+/\\d+)\\s(http://\\S*)\\n$")
		, regx		  ("(\\s+)(#\\d+(?:\\S+)|#\\d+/\\d+(?:\\S+)|@\\S+|_[^\\n]+_|\\*[^\\n]+\\*|/[^\\n]+/|http://\\S+|ftp://\\S+|https://\\S+){1}(\\s+)")
		, rpostRx		  ("\\nReply posted.\\n(#.*)\\s(http://\\S*)\\n$")
		, threadRx	  ("^\\n@(\\S*):( \\*[^\\n]*){0,1}\\n(.*)\\n(#\\d+)\\s(http://juick.com/\\S+)\\n(.*)")
		, userRx		  ("^\\nBlog: http://.*")
		, singleMsgRx	  ("^\\n@(\\S*):( \\*[^\\n]*){0,1}\\n(.*)\\n(#\\d+) (\\((?:.*; )\\d+ repl(?:ies|y)\\) ){0,1}(http://juick.com/\\S+)\\n$")
		, lastMsgRx	  ("^\\n(Last (?:popular ){0,1}messages:)(.*)")
		, juboRx		  ("^\\n([^\\n]*)\\n@(\\S*):( [^\\n]*){0,1}\\n(.*)\\n(#\\d+)\\s(http://juick.com/\\S+)\\n$")
		, msgPostRx	  ("\\nNew message posted.\\n(#.*)\\s(http://\\S*)\\n$")
		, delMsgRx	  ("^\\nMessage #\\d+ deleted.\\n$")
		, delReplyRx	  ("^\\nReply #\\d+/\\d+ deleted.\\n$")
		, idRx		  ("(#\\d+)(/\\d+){0,1}(\\S+){0,1}")
		, nickRx		  ("(@[\\w\\-\\.@\\|]*)(\\b.*)")
		, recomendRx	  ("^\\nRecommended by @(\\S*):\\n@(\\S*):( \\*[^\\n]*){0,1}\\n(.*)\\n\\n(#\\d+) (\\(\\d+ repl(?:ies|y)\\) ){0,1}(http://\\S*)\\n$")
{
	userColor = QColor(0, 85, 255);
	tagColor = QColor(131, 145, 145);
	msgColor = QColor(87, 165, 87);
	quoteColor = QColor(187, 187, 187);
	lineColor = QColor(0, 0, 255);
	userBold = true;
	tagBold = false;
	msgBold = false;
	quoteBold = false;
	lineBold = false;
	userItalic = false;
	tagItalic = true;
	msgItalic = false;
	quoteItalic = false;
	lineItalic = false;
	userUnderline = false;
	tagUnderline = false;
	msgUnderline = true;
	quoteUnderline = false;
	lineUnderline = true;
	idAsResource = false;
	showPhoto = false;
	showAvatars = false;
	workInGroupChat = false;
	pmRx.setMinimal(true);
	replyRx.setMinimal(true);
	regx.setMinimal(true);
	postRx.setMinimal(true);
	singleMsgRx.setMinimal(true);
	juboRx.setMinimal(true);
	showAllmsgString = tr("Show all messages");
	replyMsgString = tr("Reply");
	userInfoString = tr("Show %1's info and last 10 messages");
	subscribeString = tr("Subscribe");
	showLastTenString = tr("Show last 10 messages with tag %1");
	unsubscribeString = tr("Unsubscribe");
	jidList_ = QStringList() << "juick@juick.com" << "jubo@nologin.ru";

}

QString JuickPlugin::name() const
{
	return constPluginName;
}

QString JuickPlugin::shortName() const
{
	return "juick";
}

QString JuickPlugin::version() const
{
	return constVersion;
}

QWidget* JuickPlugin::options()
{
	if (!enabled) {
		return 0;
	}
	optionsWid = new QWidget();
	QVBoxLayout *vbox= new QVBoxLayout(optionsWid);
	QVBoxLayout *leftvbox= new QVBoxLayout;
	QVBoxLayout *rightvbox= new QVBoxLayout;
	QHBoxLayout *hbox= new QHBoxLayout;

	QHBoxLayout *ejl = new QHBoxLayout;
	QPushButton *requestJids = new QPushButton(tr("Edit JIDs"));
	ejl->addStretch();
	ejl->addWidget(requestJids);
	ejl->addStretch();
	connect(requestJids, SIGNAL(released()), SLOT(requestJidList()));
	vbox->addLayout(ejl);

	QGridLayout *layout= new QGridLayout;
	layout->addWidget(new QLabel(tr("@username"),optionsWid),1,0);
	layout->addWidget(new QLabel(tr("*tag"),optionsWid),2,0);
	layout->addWidget(new QLabel(tr("#message id"),optionsWid),3,0);
	layout->addWidget(new QLabel(tr(">quote"),optionsWid),4,0);
	layout->addWidget(new QLabel(tr("http://link"),optionsWid),5,0);
	layout->addWidget(new QLabel(tr("bold"),optionsWid),0,1);
	layout->addWidget(new QLabel(tr("italic"),optionsWid),0,2);
	layout->addWidget(new QLabel(tr("underline"),optionsWid),0,3);
	layout->addWidget(new QLabel(tr("color"),optionsWid),0,4);
	ubButton = new QCheckBox(optionsWid);
	uiButton = new QCheckBox(optionsWid);
	uuButton = new QCheckBox(optionsWid);
	tbButton = new QCheckBox(optionsWid);
	tiButton = new QCheckBox(optionsWid);
	tuButton = new QCheckBox(optionsWid);
	mbButton = new QCheckBox(optionsWid);
	miButton = new QCheckBox(optionsWid);
	muButton = new QCheckBox(optionsWid);
	qbButton = new QCheckBox(optionsWid);
	qiButton = new QCheckBox(optionsWid);
	quButton = new QCheckBox(optionsWid);
	lbButton = new QCheckBox(optionsWid);
	liButton = new QCheckBox(optionsWid);
	luButton = new QCheckBox(optionsWid);
	ucButton = new QToolButton(optionsWid);
	tcButton = new QToolButton(optionsWid);
	mcButton = new QToolButton(optionsWid);
	qcButton = new QToolButton(optionsWid);
	lcButton = new QToolButton(optionsWid);
	layout->addWidget(ubButton,1,1);
	layout->addWidget(uiButton,1,2);
	layout->addWidget(uuButton,1,3);
	layout->addWidget(ucButton,1,4);
	layout->addWidget(tbButton,2,1);
	layout->addWidget(tiButton,2,2);
	layout->addWidget(tuButton,2,3);
	layout->addWidget(tcButton,2,4);
	layout->addWidget(mbButton,3,1);
	layout->addWidget(miButton,3,2);
	layout->addWidget(muButton,3,3);
	layout->addWidget(mcButton,3,4);
	layout->addWidget(qbButton,4,1);
	layout->addWidget(qiButton,4,2);
	layout->addWidget(quButton,4,3);
	layout->addWidget(qcButton,4,4);
	layout->addWidget(lbButton,5,1);
	layout->addWidget(liButton,5,2);
	layout->addWidget(luButton,5,3);
	layout->addWidget(lcButton,5,4);
	QButtonGroup *b_color = new QButtonGroup;
	b_color->addButton(ucButton);
	b_color->addButton(tcButton);
	b_color->addButton(mcButton);
	b_color->addButton(qcButton);
	b_color->addButton(lcButton);
	ucButton->setStyleSheet(QString("background-color: %1;").arg(userColor.name()));
	tcButton->setStyleSheet(QString("background-color: %1;").arg(tagColor.name()));
	mcButton->setStyleSheet(QString("background-color: %1;").arg(msgColor.name()));
	qcButton->setStyleSheet(QString("background-color: %1;").arg(quoteColor.name()));
	lcButton->setStyleSheet(QString("background-color: %1;").arg(lineColor.name()));
	ucButton->setProperty("psi_color",userColor);
	tcButton->setProperty("psi_color",tagColor);
	mcButton->setProperty("psi_color",msgColor);
	qcButton->setProperty("psi_color",quoteColor);
	lcButton->setProperty("psi_color",lineColor);
	ubButton->setChecked(userBold);
	tbButton->setChecked(tagBold);
	mbButton->setChecked(msgBold);
	qbButton->setChecked(quoteBold);
	lbButton->setChecked(lineBold);
	uiButton->setChecked(userItalic);
	tiButton->setChecked(tagItalic);
	miButton->setChecked(msgItalic);
	qiButton->setChecked(quoteItalic);
	liButton->setChecked(lineItalic);
	uuButton->setChecked(userUnderline);
	tuButton->setChecked(tagUnderline);
	muButton->setChecked(msgUnderline);
	quButton->setChecked(quoteUnderline);
	luButton->setChecked(lineUnderline);
	connect(b_color, SIGNAL(buttonClicked(QAbstractButton*)), SLOT(chooseColor(QAbstractButton*)));
	vbox->addLayout(layout);
	asResourceButton = new QCheckBox(tr("Use message Id as resource"),optionsWid);
	asResourceButton->setChecked(idAsResource);
	leftvbox->addWidget(asResourceButton);
	showPhotoButton = new QCheckBox(tr("Show Photo"),optionsWid);
	showPhotoButton->setChecked(showPhoto);
	leftvbox->addWidget(showPhotoButton);
	showAvatarsButton = new QCheckBox(tr("Show Avatars"),optionsWid);
	showAvatarsButton->setChecked(showAvatars);
	leftvbox->addWidget(showAvatarsButton);
	groupChatButton = new QCheckBox(tr("Replaces message id with a link\nto this message in juick@conference.jabber.ru"),optionsWid);
	groupChatButton->setChecked(workInGroupChat);
	leftvbox->addWidget(groupChatButton);
	QLabel *wikiLink = new QLabel(tr("<a href=\"http://psi-plus.com/wiki/plugins#juick_plugin\">Wiki (Online)</a>"),optionsWid);
	wikiLink->setOpenExternalLinks(true);

	QPushButton *clearCacheButton = new QPushButton(tr("Clear avatar cache"), optionsWid);
	connect(clearCacheButton,SIGNAL(released()),SLOT(clearCache()));
	rightvbox->addWidget(clearCacheButton);
	hbox->addLayout(leftvbox);
	hbox->addLayout(rightvbox);
	vbox->addLayout(hbox);
	vbox->addWidget(wikiLink);

	return optionsWid;
}

bool JuickPlugin::enable()
{
	srand(time(NULL));
	enabled = true;
	QVariant vuserColor(userColor);
	vuserColor = psiOptions->getPluginOption(constuserColor);
	if (!vuserColor.isNull()) {
		userColor = vuserColor.toString();

	}
	QVariant vtagColor(tagColor);
	vtagColor = psiOptions->getPluginOption(consttagColor);
	if (!vtagColor.isNull()) {
		tagColor = vtagColor.toString();
	}
	QVariant vmsgColor(msgColor);
	vmsgColor = psiOptions->getPluginOption(constmsgColor);
	if (!vmsgColor.isNull()) {
		msgColor = vmsgColor.toString();
	}
	QVariant vQcolor(quoteColor);
	vQcolor = psiOptions->getPluginOption(constQcolor);
	if (!vQcolor.isNull()) {
		quoteColor = vQcolor.toString();
	}
	QVariant vLcolor(lineColor);
	vLcolor = psiOptions->getPluginOption(constLcolor);
	if (!vLcolor.isNull()) {
		lineColor = vLcolor.toString();
	}
	//bold
	QVariant vUbold(userBold);
	vUbold = psiOptions->getPluginOption(constUbold);
	if (!vUbold.isNull()) {
		userBold = vUbold.toBool();

	}
	QVariant vTbold(tagBold);
	vTbold = psiOptions->getPluginOption(constTbold);
	if (!vTbold.isNull()) {
		tagBold = vTbold.toBool();
	}
	QVariant vMbold(msgBold);
	vMbold = psiOptions->getPluginOption(constMbold);
	if (!vMbold.isNull()) {
		msgBold = vMbold.toBool();
	}
	QVariant vQbold(quoteBold);
	vQbold = psiOptions->getPluginOption(constQbold);
	if (!vQbold.isNull()) {
		quoteBold = vQbold.toBool();
	}
	QVariant vLbold(lineBold);
	vLbold = psiOptions->getPluginOption(constLbold);
	if (!vLbold.isNull()) {
		lineBold = vLbold.toBool();
	}
	//italic
	QVariant vUitalic(userItalic);
	vUitalic = psiOptions->getPluginOption(constUitalic);
	if (!vUitalic.isNull()) {
		userItalic = vUitalic.toBool();

	}
	QVariant vTitalic(tagItalic);
	vTitalic = psiOptions->getPluginOption(constTitalic);
	if (!vTitalic.isNull()) {
		tagItalic = vTitalic.toBool();
	}
	QVariant vMitalic(msgItalic);
	vMitalic = psiOptions->getPluginOption(constMitalic);
	if (!vMitalic.isNull()) {
		msgItalic = vMitalic.toBool();
	}
	QVariant vQitalic(quoteItalic);
	vQitalic = psiOptions->getPluginOption(constQitalic);
	if (!vQitalic.isNull()) {
		quoteItalic = vQitalic.toBool();
	}
	QVariant vLitalic(lineItalic);
	vLitalic = psiOptions->getPluginOption(constLitalic);
	if (!vLitalic.isNull()) {
		lineItalic = vLitalic.toBool();
	}
	//underline
	QVariant vUunderline(userUnderline);
	vUunderline = psiOptions->getPluginOption(constUunderline);
	if (!vUunderline.isNull()) {
		userUnderline = vUunderline.toBool();

	}
	QVariant vTunderline(tagUnderline);
	vTunderline = psiOptions->getPluginOption(constTunderline);
	if (!vTunderline.isNull()) {
		tagUnderline = vTunderline.toBool();
	}
	QVariant vMunderline(msgUnderline);
	vMunderline = psiOptions->getPluginOption(constMunderline);
	if (!vMunderline.isNull()) {
		msgUnderline = vMunderline.toBool();
	}
	QVariant vQunderline(quoteUnderline);
	vQunderline = psiOptions->getPluginOption(constQunderline);
	if (!vQunderline.isNull()) {
		quoteUnderline = vQunderline.toBool();
	}
	QVariant vLunderline(lineUnderline);
	vLunderline = psiOptions->getPluginOption(constLunderline);
	if (!vLunderline.isNull()) {
		lineUnderline = vLunderline.toBool();
	}
	QVariant vIdAsResource(idAsResource);
	vIdAsResource = psiOptions->getPluginOption(constIdAsResource);
	if (!vIdAsResource.isNull()) {
		idAsResource = vIdAsResource.toBool();
	}
	commonLinkColor =  psiOptions->getGlobalOption("options.ui.look.colors.chat.link-color").toString();
	QVariant vShowPhoto(showPhoto);
	vShowPhoto = psiOptions->getPluginOption(constShowPhoto);
	if (!vShowPhoto.isNull()) {
		showPhoto = vShowPhoto.toBool();
	}
	QVariant vShowAvatars(showAvatars);
	vShowAvatars = psiOptions->getPluginOption(constShowAvatars);
	if (!vShowAvatars.isNull()) {
		showAvatars = vShowAvatars.toBool();
	}
	QVariant vWorkInGroupchat(workInGroupChat);
	vWorkInGroupchat = psiOptions->getPluginOption(constWorkInGroupchat);
	if (!vWorkInGroupchat.isNull()) {
		workInGroupChat = vWorkInGroupchat.toBool();
	}

	jidList_ = psiOptions->getPluginOption("constJidList",QVariant(jidList_)).toStringList();

	applicationInfo->getProxyFor(constPluginName); // init proxy settings for Juick plugin

	if (showAvatars) {
		QString avDir = applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars";
		QDir dir(avDir);
		QDir perDir(avDir+"juick/per");
		if(!perDir.exists())
			dir.mkpath("juick/per");
	}
	return true;
}

bool JuickPlugin::disable()
{
	enabled = false;
	return true;
}

void JuickPlugin::applyOptions(){
	if (!optionsWid)
		return;

	userColor = ucButton->property("psi_color").value<QColor>();
	tagColor = tcButton->property("psi_color").value<QColor>();
	msgColor = mcButton->property("psi_color").value<QColor>();
	quoteColor = qcButton->property("psi_color").value<QColor>();
	lineColor = lcButton->property("psi_color").value<QColor>();
	QVariant vuserColor(userColor);
	psiOptions->setPluginOption(constuserColor, vuserColor);
	QVariant vtagColor(tagColor);
	psiOptions->setPluginOption(consttagColor, vtagColor);
	QVariant vmsgColor(msgColor);
	psiOptions->setPluginOption(constmsgColor, vmsgColor);
	QVariant vQcolor(quoteColor);
	psiOptions->setPluginOption(constQcolor, vQcolor);
	QVariant vLcolor(lineColor);
	psiOptions->setPluginOption(constLcolor, vLcolor);
	//bold
	userBold = ubButton->isChecked();
	tagBold = tbButton->isChecked();
	msgBold = mbButton->isChecked();
	quoteBold = qbButton->isChecked();
	lineBold = lbButton->isChecked();
	QVariant vUbold(userBold);
	psiOptions->setPluginOption(constUbold, vUbold);
	QVariant vTbold(tagBold);
	psiOptions->setPluginOption(constTbold, vTbold);
	QVariant vMbold(msgBold);
	psiOptions->setPluginOption(constMbold, vMbold);
	QVariant vQbold(quoteBold);
	psiOptions->setPluginOption(constQbold, vQbold);
	QVariant vLbold(lineBold);
	psiOptions->setPluginOption(constLbold, vLbold);
	//italic
	userItalic = uiButton->isChecked();
	tagItalic = tiButton->isChecked();
	msgItalic = miButton->isChecked();
	quoteItalic = qiButton->isChecked();
	lineItalic = liButton->isChecked();
	QVariant vUitalic(userItalic);
	psiOptions->setPluginOption(constUitalic, vUitalic);
	QVariant vTitalic(tagItalic);
	psiOptions->setPluginOption(constTitalic, vTitalic);
	QVariant vMitalic(msgItalic);
	psiOptions->setPluginOption(constMitalic, vMitalic);
	QVariant vQitalic(quoteItalic);
	psiOptions->setPluginOption(constQitalic, vQitalic);
	QVariant vLitalic(lineItalic);
	psiOptions->setPluginOption(constLitalic, vLitalic);
	//underline
	userUnderline = uuButton->isChecked();
	tagUnderline = tuButton->isChecked();
	msgUnderline = muButton->isChecked();
	quoteUnderline = quButton->isChecked();
	lineUnderline = luButton->isChecked();
	QVariant vUunderline(userUnderline);
	psiOptions->setPluginOption(constUunderline, vUunderline);
	QVariant vTunderline(tagUnderline);
	psiOptions->setPluginOption(constTunderline, vTunderline);
	QVariant vMunderline(msgUnderline);
	psiOptions->setPluginOption(constMunderline, vMunderline);
	QVariant vQunderline(quoteUnderline);
	psiOptions->setPluginOption(constQunderline, vQunderline);
	QVariant vLunderline(lineUnderline);
	psiOptions->setPluginOption(constLunderline, vLunderline);
	//asResource
	idAsResource = asResourceButton->isChecked();
	QVariant vIdAsResource(idAsResource);
	psiOptions->setPluginOption(constIdAsResource, vIdAsResource);
	showPhoto = showPhotoButton->isChecked();
	QVariant vShowPhoto(showPhoto);
	psiOptions->setPluginOption(constShowPhoto, vShowPhoto);
	showAvatars = showAvatarsButton->isChecked();
	if (showAvatars == true){
		bool cache = false;
		QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars");
		dir.mkpath("juick/per");
		if (dir.exists("juick")){
			cache = true;
		} else {
			QMessageBox::warning(0, tr("Warning"),tr("can't create folder %1 \ncaching avatars will be not available").arg(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick"));
		}
	}
	QVariant vShowAvatars(showAvatars);
	psiOptions->setPluginOption(constShowAvatars, vShowAvatars);
	workInGroupChat = groupChatButton->isChecked();
	QVariant vWorkInGroupchat(workInGroupChat);
	psiOptions->setPluginOption(constWorkInGroupchat, vWorkInGroupchat);

	psiOptions->setPluginOption("constJidList",QVariant(jidList_));

}

void JuickPlugin::restoreOptions(){
	if (!optionsWid)
		return;


	QVariant vuserColor(userColor);
	vuserColor = psiOptions->getPluginOption(constuserColor);
	if (!vuserColor.isNull()) {
		ucButton->setStyleSheet(QString("background-color: %1;").arg(vuserColor.toString()));
	}
	QVariant vtagColor(tagColor);
	vtagColor = psiOptions->getPluginOption(consttagColor);
	if (!vtagColor.isNull()) {
		tcButton->setStyleSheet(QString("background-color: %1;").arg(vtagColor.toString()));
	}
	QVariant vmsgColor(msgColor);
	vmsgColor = psiOptions->getPluginOption(constmsgColor);
	if (!vmsgColor.isNull()) {
		mcButton->setStyleSheet(QString("background-color: %1;").arg(vmsgColor.toString()));
	}
	QVariant vQcolor(quoteColor);
	vQcolor = psiOptions->getPluginOption(constQcolor);
	if (!vQcolor.isNull()) {
		qcButton->setStyleSheet(QString("background-color: %1;").arg(vQcolor.toString()));
	}
	QVariant vLcolor(lineColor);
	vLcolor = psiOptions->getPluginOption(constLcolor);
	if (!vLcolor.isNull()) {
		lcButton->setStyleSheet(QString("background-color: %1;").arg(vLcolor.toString()));
	}
	//bold
	QVariant vUbold(userBold);
	vUbold = psiOptions->getPluginOption(constUbold);
	if (!vUbold.isNull()) {
		ubButton->setChecked(vUbold.toBool());
	}
	QVariant vTbold(tagBold);
	vTbold = psiOptions->getPluginOption(constTbold);
	if (!vTbold.isNull()) {
		tbButton->setChecked(vTbold.toBool());
	}
	QVariant vMbold(msgBold);
	vMbold = psiOptions->getPluginOption(constMbold);
	if (!vMbold.isNull()) {
		mbButton->setChecked(vMbold.toBool());
	}
	QVariant vQbold(quoteBold);
	vQbold = psiOptions->getPluginOption(constQbold);
	if (!vQbold.isNull()) {
		qbButton->setChecked(vQbold.toBool());
	}
	QVariant vLbold(lineBold);
	vLbold = psiOptions->getPluginOption(constLbold);
	if (!vLbold.isNull()) {
		lbButton->setChecked(vLbold.toBool());
	}
	//italic
	QVariant vUitalic(userItalic);
	vUitalic = psiOptions->getPluginOption(constUitalic);
	if (!vUitalic.isNull()) {
		uiButton->setChecked(vUitalic.toBool());
	}
	QVariant vTitalic(tagItalic);
	vTitalic = psiOptions->getPluginOption(constTitalic);
	if (!vTitalic.isNull()) {
		tiButton->setChecked(vTitalic.toBool());
	}
	QVariant vMitalic(msgItalic);
	vMitalic = psiOptions->getPluginOption(constMitalic);
	if (!vMitalic.isNull()) {
		miButton->setChecked(vMitalic.toBool());
	}
	QVariant vQitalic(quoteItalic);
	vQitalic = psiOptions->getPluginOption(constQitalic);
	if (!vQitalic.isNull()) {
		qiButton->setChecked(vQitalic.toBool());
	}
	QVariant vLitalic(lineItalic);
	vLitalic = psiOptions->getPluginOption(constLitalic);
	if (!vLitalic.isNull()) {
		liButton->setChecked(vLitalic.toBool());
	}
	//underline
	QVariant vUunderline(userUnderline);
	vUunderline = psiOptions->getPluginOption(constUunderline);
	if (!vUunderline.isNull()) {
		uuButton->setChecked(vUunderline.toBool());
	}
	QVariant vTunderline(tagUnderline);
	vTunderline = psiOptions->getPluginOption(constTunderline);
	if (!vTunderline.isNull()) {
		tuButton->setChecked(vTunderline.toBool());
	}
	QVariant vMunderline(msgUnderline);
	vMunderline = psiOptions->getPluginOption(constMunderline);
	if (!vMunderline.isNull()) {
		muButton->setChecked(vMunderline.toBool());
	}
	QVariant vQunderline(quoteUnderline);
	vQunderline = psiOptions->getPluginOption(constQunderline);
	if (!vQunderline.isNull()) {
		quButton->setChecked(vQunderline.toBool());
	}
	QVariant vLunderline(lineUnderline);
	vLunderline = psiOptions->getPluginOption(constLunderline);
	if (!vLunderline.isNull()) {
		luButton->setChecked(vLunderline.toBool());
	}
	QVariant vIdAsResource(idAsResource);
	vIdAsResource = psiOptions->getPluginOption(constIdAsResource);
	if (!vIdAsResource.isNull()) {
		asResourceButton->setChecked(vIdAsResource.toBool());
	}
	QVariant vShowPhoto(showPhoto);
	vShowPhoto = psiOptions->getPluginOption(constShowPhoto);
	if (!vShowPhoto.isNull()) {
		showPhotoButton->setChecked(vShowPhoto.toBool());
	}
	QVariant vShowAvatars(showAvatars);
	vShowAvatars = psiOptions->getPluginOption(constShowAvatars);
	if (!vShowAvatars.isNull()) {
		showAvatarsButton->setChecked(vShowAvatars.toBool());
	}
	QVariant vWorkInGroupchat(workInGroupChat);
	vWorkInGroupchat = psiOptions->getPluginOption(constWorkInGroupchat);
	if (!vWorkInGroupchat.isNull()) {
		groupChatButton->setChecked(vWorkInGroupchat.toBool());
	}
}

void JuickPlugin::requestJidList()
{
	JuickJidList *jjl = new JuickJidList(jidList_, optionsWid);
	connect(jjl, SIGNAL(listUpdated(QStringList)), SLOT(updateJidList(QStringList)));
	jjl->show();
}

void JuickPlugin::updateJidList(QStringList jids) {
	jidList_ = jids;
	//HACK
	if(optionsWid) {
		ubButton->toggle();
		ubButton->toggle();
	}
}

bool JuickPlugin::processMessage(int account, const QString& fromJid, const QString& body, const QString& subject){
	Q_UNUSED(account);
	Q_UNUSED(fromJid);
	Q_UNUSED(body);
	Q_UNUSED(subject);
	return false;
}
bool JuickPlugin::processEvent(int account, QDomElement& e){
	Q_UNUSED(account);
	if (!enabled){
		return false;
	}
	QString jidToSend(juick);
	QString jid = e.childNodes().at(3).firstChild().nodeValue();
	QString usernameJ = jid.split("@").first();
	if (/*jid == juick || jid == jubo*/ jidList_.contains(jid, Qt::CaseInsensitive)
		|| usernameJ == "juick%juick.com"|| usernameJ == "jubo%nologin.ru") {
		if (usernameJ == "juick%juick.com"){
			jidToSend = jid;
		}
		if (usernameJ == "jubo%nologin.ru"){
			jidToSend = "juick%juick.com@"+jid.split("@").last();
		}
		userLinkPattern = "xmpp:%1?message;type=chat;body=%2+";
		altTextUser = userInfoString;
		if ( jid == jubo ){
			messageLinkPattern = "xmpp:%1%3?message;type=chat;body=%2";
			altTextMsg = replyMsgString;
		} else {
			messageLinkPattern = "xmpp:%1%3?message;type=chat;body=%2+";
			altTextMsg = showAllmsgString;
		}
		QString topTag("Top 20 tags:");
		QString resource("");
		//добавляем перевод строки для обработки ссылок и номеров сообщений в конце сообщения
		QString msg = "\n" + e.lastChild().firstChild().firstChild().nodeValue()+"\n";
		//juick bug
		msg.replace("&gt;",">");
		msg.replace("&lt;","<");
		QDomElement element =  e.ownerDocument().createElement("html");
		element.setAttribute("xmlns","http://jabber.org/protocol/xhtml-im");
		QDomElement body = e.ownerDocument().createElement("body");
		body.setAttribute("xmlns","http://www.w3.org/1999/xhtml");
		//Задаём стили
		idStyle = "color: " + msgColor.name() + ";";
		if (msgBold) {
			idStyle += "font-weight: bold;";
		}
		if (msgItalic) {
			idStyle += "font-style: italic;";
		}
		if (!msgUnderline) {
			idStyle += "text-decoration: none;";
		}
		userStyle = "color: " + userColor.name() + ";";
		if (userBold) {
			userStyle += "font-weight: bold;";
		}
		if (userItalic) {
			userStyle += "font-style: italic;";
		}
		if (!userUnderline) {
			userStyle += "text-decoration: none;";
		}
		tagStyle = "color: " + tagColor.name() + ";";
		if (tagBold) {
			tagStyle += "font-weight: bold;";
		}
		if (tagItalic) {
			tagStyle += "font-style: italic;";
		}
		if (!tagUnderline) {
			tagStyle += "text-decoration: none;";
		}
		quoteStyle = "color: " + quoteColor.name() + ";";
		if (quoteBold) {
			quoteStyle += "font-weight: bold;";
		}
		if (quoteItalic) {
			quoteStyle += "font-style: italic;";
		}
		if (!quoteUnderline) {
			quoteStyle += "text-decoration: none;";
		}
		quoteStyle += "margin: 5px;";
		linkStyle = "color: " + lineColor.name() + ";";
		if (lineBold) {
			linkStyle += "font-weight: bold;";
		}
		if (lineItalic) {
			linkStyle += "font-style: italic;";
		}
		if (!lineUnderline) {
			linkStyle += "text-decoration: none;";
		}
		QString res("");
		//HELP
		if (msg.indexOf("\nNICK mynickname - Set a nickname\n\n") != -1){
			nl2br(body,e.ownerDocument(),msg);
			if (idAsResource == true) {
				QStringList tmp = activeTab->getJid().split('/');
				if (tmp.count() > 1 && jid == tmp.first()){
					resource = tmp.last();
				}
			}
			msg =  "";
		}
		if (! e.lastChild().firstChild().nextSibling().isNull() && e.lastChild().firstChild().nextSibling().nodeName() == "x"){
			//photo post
			if (showPhoto && postRx.indexIn(msg) != -1){
				QString resLink("");
				if (idAsResource == true) {
					resource = postRx.cap(4);
					resLink = "/" + resource;
					resLink.replace("#","%23");
				}
				QDomNode domUrl = e.lastChild().firstChild().nextSibling().firstChild().firstChild();
				QString url = domUrl.nodeValue();
				if(url.split('.').last() == "jpg"){
					QUrl photoUrl(url);
					Http *http = new Http(this);
					http->setHost(photoUrl.host());

					//-----by Dealer_WeARE-----------
					Proxy prx = applicationInfo->getProxyFor(constPluginName);
					http->setProxyHostPort(prx.host, prx.port, prx.user, prx.pass, prx.type);

					QByteArray preview = http->get(QString(photoUrl.path()).replace("/photos-1024/","/ps/"));
					http->deleteLater();
					body.appendChild(e.ownerDocument().createElement("br"));
					addUserLink(body, e.ownerDocument(), "@" + postRx.cap(1), altTextUser ,"xmpp:%1?message;type=chat;body=%2+",jidToSend);
					body.appendChild(e.ownerDocument().createTextNode(": "));
					if (postRx.cap(2) != ""){
						//добавляем теги
						foreach (QString tag,postRx.cap(2).trimmed().split(" ")){
							addTagLink(body, e.ownerDocument(), tag, jidToSend);
						}
					}
					QDomElement table = e.ownerDocument().createElement("table");
					QDomElement tableRow = e.ownerDocument().createElement("tr");
					QDomElement td1 = e.ownerDocument().createElement("td");
					td1.setAttribute("valign","top");
					QDomElement td2 = e.ownerDocument().createElement("td");
					QDomElement link = e.ownerDocument().createElement("a");
					link.setAttribute("href",url);
					QDomElement img = e.ownerDocument().createElement("img");
					QString imgdata = "data:image/jpg;base64,"+QString(QUrl::toPercentEncoding(preview.toBase64()));
					img.setAttribute("src",imgdata);
					link.appendChild(img);
					td1.appendChild(link);
					QString newMsg = " " + postRx.cap(3) + " ";
					elementFromString(td2,e.ownerDocument(),newMsg, jidToSend);
					tableRow.appendChild(td1);
					tableRow.appendChild(td2);
					table.appendChild(tableRow);
					body.appendChild(table);
					addMessageId(body, e.ownerDocument(),postRx.cap(4), replyMsgString, "xmpp:%1%3?message;type=chat;body=%2",jidToSend, resLink);
					//ссылка на сообщение
					body.appendChild(e.ownerDocument().createTextNode(" "));
					addPlus(body, e.ownerDocument(), postRx.cap(4),jidToSend, resLink);
					body.appendChild(e.ownerDocument().createTextNode(" "));
					addSubscribe(body, e.ownerDocument(), postRx.cap(4),jidToSend, resLink);
					body.appendChild(e.ownerDocument().createTextNode(" "));
					addFavorite(body, e.ownerDocument(), postRx.cap(4),jidToSend, resLink);
					body.appendChild(e.ownerDocument().createTextNode(" "));
					addHttpLink(body, e.ownerDocument(), postRx.cap(5));
					msg = "";
				}
			}
			//удаление вложения, пока шлётся ссылка в сообщении
			e.lastChild().removeChild(e.lastChild().firstChild().nextSibling());
		}
		if ((jid == jubo || usernameJ == "jubo%nologin.ru") && juboRx.indexIn(msg) != -1){
			//Jubo bot
			body.appendChild(e.ownerDocument().createTextNode(juboRx.cap(1)));
			body.appendChild(e.ownerDocument().createElement("br"));
			addUserLink(body, e.ownerDocument(), "@" + juboRx.cap(2), altTextUser ,"xmpp:%1?message;type=chat;body=%2+", jidToSend);
			body.appendChild(e.ownerDocument().createTextNode(": "));
			if (juboRx.cap(3) != ""){
				//добавляем теги
				foreach (QString tag,juboRx.cap(3).trimmed().split(" ")){
					addTagLink(body, e.ownerDocument(), tag, jidToSend);
				}
			}
			//обрабатываем текст сообщения
			QString newMsg = " " + juboRx.cap(4) + " ";
			if (showAvatars){
				QDomElement table = e.ownerDocument().createElement("table");
				QDomElement tableRow = e.ownerDocument().createElement("tr");
				QDomElement td1 = e.ownerDocument().createElement("td");
				td1.setAttribute("valign","top");
				QDomElement td2 = e.ownerDocument().createElement("td");
				QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick/per");
				if (dir.exists()){
					QDomElement img = e.ownerDocument().createElement("img");
					img.setAttribute("src", QString(QUrl::fromLocalFile(QString("%1/@%2").arg(dir.absolutePath()).arg(juboRx.cap(2))).toEncoded()));
					///*/
					td1.appendChild(img);
				}
				elementFromString(td2,e.ownerDocument(),newMsg,jidToSend);
				tableRow.appendChild(td1);
				tableRow.appendChild(td2);
				table.appendChild(tableRow);
				body.appendChild(table);
			} else {
				body.appendChild(e.ownerDocument().createElement("br"));
				//обрабатываем текст сообщения
				elementFromString(body,e.ownerDocument(), newMsg, jidToSend);
			}
			//xmpp ссылка на сообщение
			addMessageId(body, e.ownerDocument(),juboRx.cap(5), replyMsgString, "xmpp:%1%3?message;type=chat;body=%2", jidToSend);
			//ссылка на сообщение
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addPlus(body, e.ownerDocument(), juboRx.cap(5), jidToSend);
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addSubscribe(body, e.ownerDocument(), juboRx.cap(5),jidToSend);
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addFavorite(body, e.ownerDocument(), juboRx.cap(5),jidToSend);
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addHttpLink(body, e.ownerDocument(), juboRx.cap(6));
			msg = "";
		} else if (lastMsgRx.indexIn(msg) != -1){
			//last 10 messages
			body.appendChild(e.ownerDocument().createTextNode(lastMsgRx.cap(1)));
			msg = lastMsgRx.cap(2);
			while (singleMsgRx.indexIn(msg) != -1){
				body.appendChild(e.ownerDocument().createElement("br"));
				addUserLink(body, e.ownerDocument(), "@" + singleMsgRx.cap(1), altTextUser ,"xmpp:%1?message;type=chat;body=%2+", jidToSend);
				body.appendChild(e.ownerDocument().createTextNode(": "));
				if (singleMsgRx.cap(2) != ""){
					//добавляем теги
					foreach (QString tag,singleMsgRx.cap(2).trimmed().split(" ")){
						addTagLink(body, e.ownerDocument(), tag, jidToSend);
					}
				}
				body.appendChild(e.ownerDocument().createElement("br"));
				//обрабатываем текст сообщения
				QString newMsg = " " + singleMsgRx.cap(3) + " ";
				elementFromString(body,e.ownerDocument(), newMsg, jidToSend);
				//xmpp ссылка на сообщение
				addMessageId(body, e.ownerDocument(),singleMsgRx.cap(4), replyMsgString, "xmpp:%1%3?message;type=chat;body=%2", jidToSend);
				body.appendChild(e.ownerDocument().createTextNode(" "));
				addPlus(body, e.ownerDocument(), singleMsgRx.cap(4),jidToSend);
				//ссылка на сообщение
				body.appendChild(e.ownerDocument().createTextNode(" "));
				addSubscribe(body, e.ownerDocument(), singleMsgRx.cap(4),jidToSend);
				body.appendChild(e.ownerDocument().createTextNode(" "));
				addFavorite(body, e.ownerDocument(), singleMsgRx.cap(4),jidToSend);
				body.appendChild(e.ownerDocument().createTextNode(" "+singleMsgRx.cap(5)));
				addHttpLink(body, e.ownerDocument(), singleMsgRx.cap(6));
				body.appendChild(e.ownerDocument().createElement("br"));
				msg = msg.right(msg.size() - singleMsgRx.matchedLength());
			}
			body.removeChild(body.lastChild());
		} else if (msg.indexOf(topTag) != -1){
			//Если это топ тегов
			body.appendChild(e.ownerDocument().createTextNode(topTag));
			body.appendChild(e.ownerDocument().createElement("br"));
			msg = msg.right(msg.size() - topTag.size() - 1);
			while (tagRx.indexIn(msg, 0) != -1){
				addTagLink(body, e.ownerDocument(), tagRx.cap(1), jidToSend);
				body.appendChild(e.ownerDocument().createElement("br"));
				msg = msg.right(msg.size() - tagRx.matchedLength());
			}
		} else if (recomendRx.indexIn(msg) != -1){
			//разбор рекомендации
			QString resLink("");
			if (idAsResource == true) {
				resource = recomendRx.cap(5);
				resLink = "/" + resource;
				resLink.replace("#","%23");
			}
			body.appendChild(e.ownerDocument().createElement("br"));
			body.appendChild(e.ownerDocument().createTextNode(tr("Recommended by ")));
			addUserLink(body, e.ownerDocument(), "@" + recomendRx.cap(1), altTextUser ,"xmpp:%1?message;type=chat;body=%2+", jidToSend);
			body.appendChild(e.ownerDocument().createTextNode(":"));
			body.appendChild(e.ownerDocument().createElement("br"));
			addUserLink(body, e.ownerDocument(), "@" + recomendRx.cap(2), altTextUser ,"xmpp:%1?message;type=chat;body=%2+", jidToSend);
			body.appendChild(e.ownerDocument().createTextNode(": "));
			if (recomendRx.cap(3) != ""){
				//добавляем теги
				foreach (QString tag,recomendRx.cap(3).trimmed().split(" ")){
					addTagLink(body, e.ownerDocument(), tag, jidToSend);
				}
			}
			//mood
			QRegExp moodRx("\\*mood:\\s(\\S*)\\s(.*)\\n(.*)");
			//geo
			QRegExp geoRx("\\*geo:\\s(.*)\\n(.*)");
			//tune
			QRegExp tuneRx("\\*tune:\\s(.*)\\n(.*)");
			if (moodRx.indexIn(recomendRx.cap(4)) != -1){
				body.appendChild(e.ownerDocument().createElement("br"));
				QDomElement bold = e.ownerDocument().createElement("b");
				bold.appendChild(e.ownerDocument().createTextNode("mood: "));
				body.appendChild(bold);
				QDomElement img = e.ownerDocument().createElement("icon");
				img.setAttribute("name","mood/"+moodRx.cap(1).left(moodRx.cap(1).size()-1).toLower());
				img.setAttribute("text",moodRx.cap(1));
				body.appendChild(img);
				body.appendChild(e.ownerDocument().createTextNode(" "+moodRx.cap(2)));
				msg = " " + moodRx.cap(3) + " ";
			} else if(geoRx.indexIn(recomendRx.cap(4)) != -1) {
				body.appendChild(e.ownerDocument().createElement("br"));
				QDomElement bold = e.ownerDocument().createElement("b");
				bold.appendChild(e.ownerDocument().createTextNode("geo: "+ geoRx.cap(1) ));
				body.appendChild(bold);
				msg = " " + geoRx.cap(2) + " ";
			} else if(tuneRx.indexIn(recomendRx.cap(4)) != -1) {
				body.appendChild(e.ownerDocument().createElement("br"));
				QDomElement bold = e.ownerDocument().createElement("b");
				bold.appendChild(e.ownerDocument().createTextNode("tune: "+ tuneRx.cap(1) ));
				body.appendChild(bold);
				msg = " " + tuneRx.cap(2) + " ";
			}
			else{
				msg = " " + recomendRx.cap(4) + " ";
			}
			if (showAvatars){
				QDomElement table = e.ownerDocument().createElement("table");
				QDomElement tableRow = e.ownerDocument().createElement("tr");
				QDomElement td1 = e.ownerDocument().createElement("td");
				td1.setAttribute("valign","top");
				QDomElement td2 = e.ownerDocument().createElement("td");
				QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick/per");
				if (dir.exists()){
					QDomElement img = e.ownerDocument().createElement("img");
					img.setAttribute("src", QString(QUrl::fromLocalFile(QString("%1/@%2").arg(dir.absolutePath()).arg(recomendRx.cap(2))).toEncoded()));
					td1.appendChild(img);
				}///
				elementFromString(td2,e.ownerDocument(),msg, jidToSend);
				tableRow.appendChild(td1);
				tableRow.appendChild(td2);
				table.appendChild(tableRow);
				body.appendChild(table);
			} else {
				body.appendChild(e.ownerDocument().createElement("br"));
				//обрабатываем текст сообщения
				elementFromString(body,e.ownerDocument(),msg,  jidToSend);
			}
			//xmpp ссылка на сообщение
			addMessageId(body, e.ownerDocument(),recomendRx.cap(5), replyMsgString, "xmpp:%1%3?message;type=chat;body=%2",jidToSend,resLink);
			//ссылка на сообщение
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addPlus(body, e.ownerDocument(), recomendRx.cap(5),jidToSend, resLink);
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addSubscribe(body, e.ownerDocument(), recomendRx.cap(5),jidToSend,resLink);
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addFavorite(body, e.ownerDocument(), recomendRx.cap(5),jidToSend,resLink);
			body.appendChild(e.ownerDocument().createTextNode(" "));
			body.appendChild(e.ownerDocument().createTextNode(recomendRx.cap(6)));
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addHttpLink(body, e.ownerDocument(), recomendRx.cap(7));
			msg = "";
		} else if (postRx.indexIn(msg) != -1){
			//разбор сообщения
			QString resLink("");
			if (idAsResource == true) {
				resource = postRx.cap(4);
				resLink = "/" + resource;
				resLink.replace("#","%23");
			}
			body.appendChild(e.ownerDocument().createElement("br"));
			addUserLink(body, e.ownerDocument(), "@" + postRx.cap(1), altTextUser ,"xmpp:%1?message;type=chat;body=%2+", jidToSend);
			body.appendChild(e.ownerDocument().createTextNode(": "));
			if (postRx.cap(2) != ""){
				//добавляем теги
				foreach (QString tag,postRx.cap(2).trimmed().split(" ")){
					addTagLink(body, e.ownerDocument(), tag, jidToSend);
				}
			}
			//mood
			QRegExp moodRx("\\*mood:\\s(\\S*)\\s(.*)\\n(.*)");
			//geo
			QRegExp geoRx("\\*geo:\\s(.*)\\n(.*)");
			//tune
			QRegExp tuneRx("\\*tune:\\s(.*)\\n(.*)");
			if (moodRx.indexIn(postRx.cap(3)) != -1){
				body.appendChild(e.ownerDocument().createElement("br"));
				QDomElement bold = e.ownerDocument().createElement("b");
				bold.appendChild(e.ownerDocument().createTextNode("mood: "));
				body.appendChild(bold);
				QDomElement img = e.ownerDocument().createElement("icon");
				img.setAttribute("name","mood/"+moodRx.cap(1).left(moodRx.cap(1).size()-1).toLower());
				img.setAttribute("text",moodRx.cap(1));
				body.appendChild(img);
				body.appendChild(e.ownerDocument().createTextNode(" "+moodRx.cap(2)));
				msg = " " + moodRx.cap(3) + " ";
			} else if(geoRx.indexIn(postRx.cap(3)) != -1) {
				body.appendChild(e.ownerDocument().createElement("br"));
				QDomElement bold = e.ownerDocument().createElement("b");
				bold.appendChild(e.ownerDocument().createTextNode("geo: "+ geoRx.cap(1) ));
				body.appendChild(bold);
				msg = " " + geoRx.cap(2) + " ";
			} else if(tuneRx.indexIn(postRx.cap(3)) != -1) {
				body.appendChild(e.ownerDocument().createElement("br"));
				QDomElement bold = e.ownerDocument().createElement("b");
				bold.appendChild(e.ownerDocument().createTextNode("tune: "+ tuneRx.cap(1) ));
				body.appendChild(bold);
				msg = " " + tuneRx.cap(2) + " ";
			}
			else{
				msg = " " + postRx.cap(3) + " ";
			}
			if (showAvatars){
				QDomElement table = e.ownerDocument().createElement("table");
				QDomElement tableRow = e.ownerDocument().createElement("tr");
				QDomElement td1 = e.ownerDocument().createElement("td");
				td1.setAttribute("valign","top");
				QDomElement td2 = e.ownerDocument().createElement("td");
				QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick/per");
				if (dir.exists()){
					QDomElement img = e.ownerDocument().createElement("img");
					img.setAttribute("src", QString(QUrl::fromLocalFile(QString("%1/@%2").arg(dir.absolutePath()).arg(postRx.cap(1))).toEncoded()));
					td1.appendChild(img);
				}
				elementFromString(td2,e.ownerDocument(),msg, jidToSend);
				tableRow.appendChild(td1);
				tableRow.appendChild(td2);
				table.appendChild(tableRow);
				body.appendChild(table);
			} else {
				body.appendChild(e.ownerDocument().createElement("br"));
				//обрабатываем текст сообщения
				elementFromString(body,e.ownerDocument(),msg,  jidToSend);
			}
			//xmpp ссылка на сообщение
			addMessageId(body, e.ownerDocument(),postRx.cap(4), replyMsgString, "xmpp:%1%3?message;type=chat;body=%2",jidToSend,resLink);
			//ссылка на сообщение
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addPlus(body, e.ownerDocument(), postRx.cap(4),jidToSend, resLink);
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addSubscribe(body, e.ownerDocument(), postRx.cap(4),jidToSend,resLink);
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addFavorite(body, e.ownerDocument(), postRx.cap(4),jidToSend,resLink);
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addHttpLink(body, e.ownerDocument(), postRx.cap(5));
			msg = "";
		} else if (replyRx.indexIn(msg) != -1){
			//обработка реплеев
			QString resLink("");
			QString replyId(replyRx.cap(4));
			if (idAsResource == true) {
				resource = replyId.left(replyId.indexOf("/"));
				resLink = "/" + resource;
				resLink.replace("#","%23");
			}
			body.appendChild(e.ownerDocument().createElement("br"));
			addUserLink(body, e.ownerDocument(), "@" + replyRx.cap(1), altTextUser ,"xmpp:%1?message;type=chat;body=%2+", jidToSend);
			body.appendChild(e.ownerDocument().createTextNode(tr(" replied:")));
			//цитата
			QDomElement blockquote = e.ownerDocument().createElement("blockquote");
			blockquote.setAttribute("style",quoteStyle);
			blockquote.appendChild(e.ownerDocument().createTextNode(replyRx.cap(2)));
			//обрабатываем текст сообщения
			msg = " " + replyRx.cap(3) + " ";
			if (showAvatars){
				QDomElement table = e.ownerDocument().createElement("table");
				QDomElement tableRow = e.ownerDocument().createElement("tr");
				QDomElement td1 = e.ownerDocument().createElement("td");
				td1.setAttribute("valign","top");
				QDomElement td2 = e.ownerDocument().createElement("td");
				QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick/per");
				if (dir.exists()){
					QDomElement img = e.ownerDocument().createElement("img");
					img.setAttribute("src", QString(QUrl::fromLocalFile(QString("%1/@%2").arg(dir.absolutePath()).arg(replyRx.cap(1))).toEncoded()));
					td1.appendChild(img);
				}
				td2.appendChild(blockquote);
				elementFromString(td2,e.ownerDocument(),msg,jidToSend);
				tableRow.appendChild(td1);
				tableRow.appendChild(td2);
				table.appendChild(tableRow);
				body.appendChild(table);
			} else {
				body.appendChild(blockquote);
				elementFromString(body,e.ownerDocument(),msg,jidToSend);
			}
			//xmpp ссылка на сообщение
			addMessageId(body, e.ownerDocument(),replyId, replyMsgString, "xmpp:%1%3?message;type=chat;body=%2",jidToSend,resLink);
			//ссылка на сообщение
			body.appendChild(e.ownerDocument().createTextNode(" "));
			QString msgId = replyId.split("/").first();
			addUnsubscribe(body, e.ownerDocument(), replyId,jidToSend, resLink);
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addPlus(body, e.ownerDocument(), msgId, jidToSend, resLink);
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addHttpLink(body, e.ownerDocument(), replyRx.cap(5));
			msg = "";
		} else if (rpostRx.indexIn(msg) != -1) {
			//Reply posted
			QString resLink("");
			if (idAsResource == true) {
				QString tmp(rpostRx.cap(1));
				resource = tmp.left(tmp.indexOf("/"));
				resLink = "/" + resource;
				resLink.replace("#","%23");
			}
			body.appendChild(e.ownerDocument().createElement("br"));
			body.appendChild(e.ownerDocument().createTextNode(tr("Reply posted.")));
			body.appendChild(e.ownerDocument().createElement("br"));
			//xmpp ссылка на сообщение
			addMessageId(body, e.ownerDocument(),rpostRx.cap(1), replyMsgString, "xmpp:%1%3?message;type=chat;body=%2",jidToSend,resLink);
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addDelete(body, e.ownerDocument(),rpostRx.cap(1),jidToSend,resLink);
			body.appendChild(e.ownerDocument().createTextNode(" "));
			//ссылка на сообщение
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addHttpLink(body, e.ownerDocument(), rpostRx.cap(2));
			msg = "";

		} else if (msgPostRx.indexIn(msg) != -1){
			//New message posted
			QString resLink("");
			if (idAsResource == true) {
				QStringList tmp = activeTab->getJid().split('/');
				if (tmp.count() > 1 && jid == tmp.first()){
					resource = tmp.last();
					resLink = "/" + resource;
				} else {
					QString tmp(msgPostRx.cap(1));
					resLink = "/" + tmp.left(tmp.indexOf("/"));
				}
				resLink.replace("#","%23");

			}
			body.appendChild(e.ownerDocument().createElement("br"));
			body.appendChild(e.ownerDocument().createTextNode(tr("New message posted.")));
			body.appendChild(e.ownerDocument().createElement("br"));
			//xmpp ссылка на сообщение
			addMessageId(body, e.ownerDocument(),msgPostRx.cap(1), replyMsgString, "xmpp:%1%3?message;type=chat;body=%2",jidToSend);
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addDelete(body, e.ownerDocument(),msgPostRx.cap(1),jidToSend);
			body.appendChild(e.ownerDocument().createTextNode(" "));
			//ссылка на сообщение
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addHttpLink(body, e.ownerDocument(), msgPostRx.cap(2));
			msg = "";
		}else if (threadRx.indexIn(msg) != -1){
			//Show All Messages
			QString resLink("");
			if (idAsResource == true) {
				resource = threadRx.cap(4);
				resLink = "/" + resource;
				resLink.replace("#","%23");
				res = resLink;
			}
			body.appendChild(e.ownerDocument().createElement("br"));
			addUserLink(body, e.ownerDocument(), "@" + threadRx.cap(1), altTextUser ,"xmpp:%1?message;type=chat;body=%2+",jidToSend);
			body.appendChild(e.ownerDocument().createTextNode(": "));
			if (threadRx.cap(2) != ""){
				//добавляем теги
				foreach (QString tag,threadRx.cap(2).trimmed().split(" ")){
					addTagLink(body, e.ownerDocument(), tag,jidToSend);
				}
			}
			body.appendChild(e.ownerDocument().createElement("br"));
			//обрабатываем текст сообщения
			QString newMsg(" " + threadRx.cap(3) + " ");
			elementFromString(body,e.ownerDocument(),newMsg,jidToSend);
			//xmpp ссылка на сообщение
			addMessageId(body, e.ownerDocument(),threadRx.cap(4), replyMsgString, "xmpp:%1%3?message;type=chat;body=%2",jidToSend,resLink);
			//ссылка на сообщение
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addSubscribe(body, e.ownerDocument(), threadRx.cap(4),jidToSend, resLink);
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addFavorite(body, e.ownerDocument(), threadRx.cap(4),jidToSend, resLink);
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addHttpLink(body, e.ownerDocument(), threadRx.cap(5));
			msg = msg.right(msg.size() - threadRx.matchedLength() + threadRx.cap(6).length());
		} else if (singleMsgRx.indexIn(msg) != -1) {
			//просмотр отдельного поста
			body.appendChild(e.ownerDocument().createElement("br"));
			addUserLink(body, e.ownerDocument(), "@" + singleMsgRx.cap(1), altTextUser ,"xmpp:%1?message;type=chat;body=%2+",jidToSend);
			body.appendChild(e.ownerDocument().createTextNode(": "));
			if (singleMsgRx.cap(2) != ""){
				//добавляем теги
				foreach (QString tag,singleMsgRx.cap(2).trimmed().split(" ")){
					addTagLink(body, e.ownerDocument(), tag,jidToSend);
				}
			}
			body.appendChild(e.ownerDocument().createElement("br"));
			//обрабатываем текст сообщения
			QString newMsg = " " + singleMsgRx.cap(3) + " ";
			elementFromString(body,e.ownerDocument(), newMsg,jidToSend);
			//xmpp ссылка на сообщение
			if (singleMsgRx.cap(5) == ""){
				messageLinkPattern = "xmpp:%1%3?message;type=chat;body=%2";
				altTextMsg = replyMsgString;
			}
			addMessageId(body, e.ownerDocument(), singleMsgRx.cap(4), altTextMsg, messageLinkPattern,jidToSend);
			//ссылка на сообщение
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addSubscribe(body, e.ownerDocument(), singleMsgRx.cap(4),jidToSend);
			body.appendChild(e.ownerDocument().createTextNode(" "));
			addFavorite(body, e.ownerDocument(), singleMsgRx.cap(4),jidToSend);
			body.appendChild(e.ownerDocument().createTextNode(" "+singleMsgRx.cap(5)));
			addHttpLink(body, e.ownerDocument(), singleMsgRx.cap(6));
			msg = "";
		} else if (msg.indexOf("Recommended blogs:") != -1){
			//если команда @
			userLinkPattern = "xmpp:%1?message;type=chat;body=S %2";
			altTextUser =tr("Subscribe to %1's blog");
		} else if (pmRx.indexIn(msg) != -1) {
			//Если PM
			userLinkPattern = "xmpp:%1?message;type=chat;body=PM %2";
			altTextUser =tr("Send personal message to %1");
		} else if (userRx.indexIn(msg) != -1) {
			//Если информация о пользователе
			userLinkPattern = "xmpp:%1?message;type=chat;body=S %2";
			altTextUser =tr("Subscribe to %1's blog");
		} else if (msg == "\nPONG\n"
				   || msg == "\nSubscribed!\n"
				   || msg == "\nUnsubscribed!\n"
				   || msg == "\nPrivate message sent.\n"
				   || msg == "\nInvalid request.\n"
				   || msg == "\nMessage added to your favorites.\n"
				   || msg == "\nMessage, you are replying to, not found.\n"
				   || msg == "\nThis nickname is already taken by someone\n"
				   || msg == "\nUser not found.\n"
				   || delMsgRx.indexIn(msg) != -1
				   || delReplyRx.indexIn(msg) != -1 ){
			msg = msg.left(msg.size() - 1);
		}
		if (idAsResource == true && resource == "" && (jid != jubo || usernameJ != "jubo%nologin.ru")) {
			QStringList tmp = activeTab->getJid().split('/');
			if (tmp.count() > 1 && jid == tmp.first()){
				resource = tmp.last();
			}
		}
		//обработка по умолчанию
		elementFromString(body,e.ownerDocument(),msg,jidToSend,res);
		element.appendChild(body);
		e.lastChild().appendChild(element);
		if (resource != ""){
			QDomElement domMsg = e.lastChildElement();
			QString from = domMsg.attribute("from");
			from.replace(QRegExp("(.*)/.*"),"\\1/"+resource);
			domMsg.setAttribute("from",from);
		}
	}
	return false;
}
void JuickPlugin::chooseColor(QAbstractButton* button)
{
	QColor c;
	c = button->property("psi_color").value<QColor>();
	c = QColorDialog::getColor(c);
	if(c.isValid()) {
		button->setProperty("psi_color", c);
		button->setStyleSheet(QString("background-color: %1").arg(c.name()));
		//HACK
		ubButton->toggle();
		ubButton->toggle();

	}
}
void JuickPlugin::clearCache(){
	QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick");
	QStringList fileNames = dir.entryList(QStringList(QString("*")));
	foreach(QString file,fileNames){
		QFile::remove(dir.absolutePath()+"/"+file);
	}
}
void JuickPlugin::setOptionAccessingHost(OptionAccessingHost* host)
{
	psiOptions = host;
}

void JuickPlugin::setActiveTabAccessingHost(ActiveTabAccessingHost* host){
	activeTab = host;
}
void JuickPlugin::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host){
	applicationInfo = host;
}
/*
  void JuickPlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost *host) {
  accInfo = host;
  }
*/
void JuickPlugin::optionChanged(const QString& option)
{
	Q_UNUSED(option);
}
bool JuickPlugin::incomingStanza(int account, const QDomElement& stanza){
	Q_UNUSED(account);
	QString jid("");
	QString usernameJ("");
	if (enabled && stanza.tagName() == "message" ){
		jid = stanza.attribute("from").split('/').first();
		if (workInGroupChat == true && jid == "juick@conference.jabber.ru") {
			QString msg = stanza.firstChild().nextSibling().firstChild().nodeValue();
			msg.replace(QRegExp("#(\\d+)"),"http://juick.com/\\1");
			stanza.firstChild().nextSibling().firstChild().setNodeValue(msg);
		}
	}
	if (enabled && showAvatars && stanza.tagName() == "message"
		&& (jid = stanza.attribute("from").split('/').first(), usernameJ = jid.split("@").first(), /*jid == juick||jid == jubo*/ jidList_.contains(jid)
			||usernameJ=="juick%juick.com"||usernameJ=="jubo%nologin.ru")){
		QDomNodeList childs = stanza.childNodes();
		int size = childs.size();
		for(int i = 0; i< size; ++i){
			QDomElement element = childs.item(i).toElement();
			if (!element.isNull() && element.tagName() == "juick"){
				QDomElement userElement = element.firstChildElement("user");
				QString uid = userElement.attribute("uid");
				QString unick = "@" + userElement.attribute("uname");
				QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick");
				if (dir.exists()){
					QStringList fileName = dir.entryList(QStringList(QString(unick + ";*")));
					if (!fileName.empty()){
						QString fname = fileName.last();
						bool ok;
						QString day = fname.split(';').last();
						if (QDate::currentDate().dayOfYear() - day.toInt(&ok,10) > (rand() %10 + 11)){
							Http *http = new Http(this);
							http->setHost("i.juick.com");

							//-----by Dealer_WeARE-----------				
							Proxy prx = applicationInfo->getProxyFor(constPluginName);
							http->setProxyHostPort(prx.host, prx.port, prx.user, prx.pass, prx.type);

							QByteArray img = http->get("/as/"+uid+".png");
							if(img.isEmpty())
								img = http->get("/a/"+uid+".png");
							http->deleteLater();
							QFile::remove(dir.absolutePath()+"/"+fname);
							fname = QString("%1;%2").arg(unick).arg(QDate::currentDate().dayOfYear());
							QFile file(QString("%1/%2").arg(dir.absolutePath()).arg(fname));
							if(!file.open(QIODevice::WriteOnly)){
								QMessageBox::warning(0, tr("Warning"),tr("Cannot write to file %1:\n%2.")
													 .arg(file.fileName())
													 .arg(file.errorString()));
							} else {
								file.write(img);
								file.close();
							}
							QDir dirPer(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick/per");
							if (!dirPer.exists()){
								dirPer.mkdir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick/per");
							}
							QFile filePer(QString("%1/per/%2").arg(dir.absolutePath()).arg(unick));
							if(!filePer.open(QIODevice::WriteOnly)){
								QMessageBox::warning(0, tr("Warning"),tr("Cannot write to file %1:\n%2.")
													 .arg(filePer.fileName())
													 .arg(filePer.errorString()));
							} else {
								filePer.write(img);
								filePer.close();
							}
						}
					} else {
						Http *http = new Http(this);
						http->setHost("i.juick.com");

						//-----by Dealer_WeARE-----------
						Proxy prx = applicationInfo->getProxyFor(constPluginName);
						http->setProxyHostPort(prx.host, prx.port, prx.user, prx.pass, prx.type);

						QByteArray img = http->get("/as/"+uid+".png");
						if(img.isEmpty())
							img = http->get("/a/"+uid+".png");
						http->deleteLater();
						QFile file(QString("%1/%2;%3").arg(dir.absolutePath()).arg(unick).arg(QDate::currentDate().dayOfYear()));
						QFile filePer(QString("%1/per/%2").arg(dir.absolutePath()).arg(unick));
						if(!file.open(QIODevice::WriteOnly)){
							QMessageBox::warning(0, tr("Warning"),tr("Cannot write to file %1:\n%2.")
												 .arg(file.fileName())
												 .arg(file.errorString()));
						} else {
							file.write(img);
							file.close();
						}
						QDir dirPer(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick/per");
						if (!dirPer.exists()){
							dirPer.mkdir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick/per");
						}
						if(!filePer.open(QIODevice::WriteOnly)){
							QMessageBox::warning(0, tr("Warning"),tr("Cannot write to file %1:\n%2.")
												 .arg(filePer.fileName())
												 .arg(filePer.errorString()));
						} else {
							filePer.write(img);
							filePer.close();
						}
					}
				}
				break;
			}
		}
	}
	return false;
}

bool JuickPlugin::outgoingStanza(int /*account*/, QDomElement& /*stanza*/)
{
	return false;
}

void JuickPlugin::elementFromString(QDomElement& body,QDomDocument e, QString& msg,QString jid, QString resource){
	int new_pos = 0;
	int pos = 0;
	while ((new_pos = regx.indexIn(msg, pos)) != -1){
		QString before = msg.mid(pos,new_pos-pos+regx.cap(1).length());
		int quoteSize = 0;
		nl2br(body,e,before.right(before.size() - quoteSize));
		QString seg = regx.cap(2);
		switch (seg.at(0).toAscii()){
		case '#':{
			idRx.indexIn(seg);
			if (idRx.cap(2) != "" ){
				//для #1234/12 - +ненужен
				messageLinkPattern = "xmpp:%1%3?message;type=chat;body=%2";
				altTextMsg = replyMsgString;
			}
			addMessageId(body, e.ownerDocument(),idRx.cap(1)+idRx.cap(2), altTextMsg, messageLinkPattern,jid, resource);
			body.appendChild(e.ownerDocument().createTextNode(idRx.cap(3)));
			break;}
		case '@':{
			nickRx.indexIn(seg);
			addUserLink(body, e.ownerDocument(), nickRx.cap(1), altTextUser ,userLinkPattern,jid);
			body.appendChild(e.ownerDocument().createTextNode(nickRx.cap(2)));
			//tag
			if (nickRx.cap(2) == ":" && (regx.cap(1) == "\n" || regx.cap(1) == "\n\n")){
				body.appendChild(e.ownerDocument().createTextNode(" "));
				QString tagMsg = msg.right(msg.size()-(new_pos+regx.matchedLength()-regx.cap(3).size()));
				for (int i=0; i < 6; ++i){
					if (tagRx.indexIn(tagMsg, 0) != -1){
						addTagLink(body, e.ownerDocument(), tagRx.cap(1),jid);
						tagMsg = tagMsg.right(tagMsg.size() - tagRx.matchedLength());
						new_pos += tagRx.matchedLength();
					} else {
						break;
					}
				}
				new_pos += regx.cap(3).size() - 1;
			}
			break;}
		case '*':{
			QDomElement bold = e.ownerDocument().createElement("b");
			bold.appendChild(e.ownerDocument().createTextNode(seg.mid(1,seg.size()-2)));
			body.appendChild(bold);
			break;}
		case '_':{
			QDomElement under = e.ownerDocument().createElement("u");
			under.appendChild(e.ownerDocument().createTextNode(seg.mid(1,seg.size()-2)));
			body.appendChild(under);
			break;}
		case '/':{
			QDomElement italic = e.ownerDocument().createElement("i");
			italic.appendChild(e.ownerDocument().createTextNode(seg.mid(1,seg.size()-2)));
			body.appendChild(italic);
			break;}
		case 'h':
		case 'f':{
			QDomElement ahref = e.ownerDocument().createElement("a");
			ahref.setAttribute("style","color:" + commonLinkColor + ";");
			ahref.setAttribute("href",seg);
			ahref.appendChild(e.ownerDocument().createTextNode(seg));
			body.appendChild(ahref);
			break;}
		default:{}
		}
		pos = new_pos+regx.matchedLength()-regx.cap(3).size();
		new_pos = pos;
	}
	nl2br(body, e , msg.right(msg.size()-pos));
	body.appendChild(e.ownerDocument().createElement("br"));
}
void JuickPlugin::nl2br(QDomElement& body,QDomDocument e, QString msg){
	foreach (QString str, msg.split("\n")) {
		body.appendChild(e.createTextNode(str));
		body.appendChild(e.createElement("br"));
	}
	body.removeChild(body.lastChild());
}

void JuickPlugin::addPlus(QDomElement& body,QDomDocument e, QString msg, QString jid, QString resource){
	QDomElement plus = e.createElement("a");
	plus.setAttribute("style",idStyle);
	plus.setAttribute("title",showAllmsgString);
	plus.setAttribute("href",QString("xmpp:%1%3?message;type=chat;body=%2+").arg(jid).arg(msg.replace("#","%23")).arg(resource));
	plus.appendChild(e.ownerDocument().createTextNode("+"));
	body.appendChild(plus);
}
void JuickPlugin::addSubscribe(QDomElement& body,QDomDocument e, QString msg, QString jid, QString resource){
	QDomElement subscribe = e.createElement("a");
	subscribe.setAttribute("style",idStyle);
	subscribe.setAttribute("title",subscribeString);
	subscribe.setAttribute("href",QString("xmpp:%1%3?message;type=chat;body=S %2").arg(jid).arg(msg.replace("#","%23")).arg(resource));
	subscribe.appendChild(e.ownerDocument().createTextNode("S"));
	body.appendChild(subscribe);
}
void JuickPlugin::addHttpLink(QDomElement& body,QDomDocument e, QString msg){
	QDomElement ahref = e.createElement("a");
	ahref.setAttribute("href",msg);
	ahref.setAttribute("style",linkStyle);
	ahref.appendChild(e.ownerDocument().createTextNode(msg));
	body.appendChild(ahref);
}
void JuickPlugin::addTagLink(QDomElement& body,QDomDocument e, QString tag, QString jid){
	QDomElement taglink = e.createElement("a");
	taglink.setAttribute("style",tagStyle);
	taglink.setAttribute("title",showLastTenString.arg(tag));
	taglink.setAttribute("href",QString("xmpp:%1?message;type=chat;body=%2").arg(jid).arg(tag));
	taglink.appendChild(e.ownerDocument().createTextNode( tag));
	body.appendChild(taglink);
	body.appendChild(e.ownerDocument().createTextNode(" "));
}
void JuickPlugin::addUserLink(QDomElement& body,QDomDocument e, QString nick, QString altText, QString pattern, QString jid){
	QDomElement ahref = e.createElement("a");
	ahref.setAttribute("style", userStyle);
	ahref.setAttribute("title", altText.arg(nick));
	ahref.setAttribute("href", pattern.arg(jid).arg(nick));
	ahref.appendChild(e.ownerDocument().createTextNode(nick));
	body.appendChild(ahref);
}
void JuickPlugin::addMessageId(QDomElement& body,QDomDocument e, QString mId, QString altText,QString pattern, QString jid, QString resource){
	QDomElement ahref = e.createElement("a");
	ahref.setAttribute("style",idStyle);
	ahref.setAttribute("title",altText);
	ahref.setAttribute("href",QString(pattern).arg(jid).arg(mId.replace("#","%23")).arg(resource));
	ahref.appendChild(e.ownerDocument().createTextNode(mId.replace("%23","#")));
	body.appendChild(ahref);
}
void JuickPlugin::addUnsubscribe(QDomElement& body,QDomDocument e, QString msg, QString jid, QString resource){
	QDomElement unsubscribe = e.createElement("a");
	unsubscribe.setAttribute("style",idStyle);
	unsubscribe.setAttribute("title",unsubscribeString);
	unsubscribe.setAttribute("href",QString("xmpp:%1%3?message;type=chat;body=U %2").arg(jid).arg(msg.left(msg.indexOf("/")).replace("#","%23")).arg(resource));
	unsubscribe.appendChild(e.ownerDocument().createTextNode("U"));
	body.appendChild(unsubscribe);
}
void JuickPlugin::addDelete(QDomElement& body,QDomDocument e, QString msg, QString jid, QString resource){
	QDomElement unsubscribe = e.createElement("a");
	unsubscribe.setAttribute("style",idStyle);
	unsubscribe.setAttribute("title",tr("Delete"));
	unsubscribe.setAttribute("href",QString("xmpp:%1%3?message;type=chat;body=D %2").arg(jid).arg(msg.replace("#","%23")).arg(resource));
	unsubscribe.appendChild(e.ownerDocument().createTextNode("D"));
	body.appendChild(unsubscribe);
}
void JuickPlugin::addFavorite(QDomElement& body,QDomDocument e, QString msg, QString jid, QString resource){
	QDomElement unsubscribe = e.createElement("a");
	unsubscribe.setAttribute("style",idStyle);
	unsubscribe.setAttribute("title",tr("Add to favorites"));
	unsubscribe.setAttribute("href",QString("xmpp:%1%3?message;type=chat;body=! %2").arg(jid).arg(msg.replace("#","%23")).arg(resource));
	unsubscribe.appendChild(e.ownerDocument().createTextNode("!"));
	body.appendChild(unsubscribe);
}

QString JuickPlugin::pluginInfo() {
	return tr("Author: ") +	 "VampiRUS\n\n"
	+ trUtf8("This plugin is designed to work efficiently and comfortably with the Juick microblogging service.\n"
			 "Currently, the plugin is able to: \n"
			 "* Coloring @nick, *tag and #message_id in messages from the juick@juick.com bot\n"
			 "* Detect >quotes in messages\n"
			 "* Enable clickable @nick, *tag, #message_id and other control elements to insert them into the typing area\n\n"
			 "Note: To work correctly, the option options.html.chat.render	must be set to true. ");
}
#include "juickplugin.moc"
