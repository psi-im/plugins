/*
 * juickplugin.cpp - plugin
 * Copyright (C) 2009-2012 Kravtsov Nikolai, Khryukin Evgeny
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
#include <QtWebKit/QWebView>
#include <QtWebKit/QWebFrame>

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
#include "toolbariconaccessor.h"

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


#define constVersion "0.11.0"
#define constPluginName "Juick Plugin"

static const int avatarsUpdateInterval = 10;

class JuickPlugin : public QObject, public PsiPlugin, public EventFilter, public OptionAccessor, public ActiveTabAccessor,
					public StanzaFilter, public ApplicationInfoAccessor, public PluginInfoProvider, public ToolbarIconAccessor
{
	Q_OBJECT
	Q_INTERFACES(PsiPlugin EventFilter OptionAccessor ActiveTabAccessor StanzaFilter
				 ApplicationInfoAccessor PluginInfoProvider ToolbarIconAccessor)

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
	virtual bool processMessage(int , const QString& , const QString& , const QString& ) { return false; }
	virtual bool processOutgoingMessage(int , const QString& , QString& , const QString& , QString& ) { return false; }
	virtual void logout(int ) {}
	// OptionAccessor
	virtual void setOptionAccessingHost(OptionAccessingHost* host);
	virtual void optionChanged(const QString& ) {}
	//ActiveTabAccessor
	virtual void setActiveTabAccessingHost(ActiveTabAccessingHost* host);
	//ApplicationInfoAccessor
	virtual void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host);
	virtual QString pluginInfo();
	//virtual void setAccountInfoAccessingHost(AccountInfoAccessingHost* host);
	virtual QList < QVariantHash > getButtonParam() { return QList < QVariantHash >(); }
	virtual QAction* getAction(QObject* parent, int account, const QString& contact);

	virtual bool incomingStanza(int account, const QDomElement& stanza);
	virtual bool outgoingStanza(int , QDomElement& ) { return false; }
	void elementFromString(QDomElement* body, QDomDocument* e, const QString &msg, const QString &jid, const QString &resource = "");
	void nl2br(QDomElement* body, QDomDocument* e,const QString& msg);
	void addPlus(QDomElement* body, QDomDocument* e, const QString &msg, const QString &jid, const QString &resource = "");
	void addSubscribe(QDomElement* body, QDomDocument* e, const QString &msg, const QString &jid, const QString &resource = "");
	void addHttpLink(QDomElement* body, QDomDocument* e, const QString &msg);
	void addTagLink(QDomElement* body, QDomDocument* e, const QString &tag, const QString &jid);
	void addUserLink(QDomElement* body, QDomDocument* e, const QString& nick, const QString& altText, const QString& pattern, const QString& jid);
	void addMessageId(QDomElement* body, QDomDocument* e, const QString& mId, const QString& altText, const QString& pattern, const QString& jid, const QString& resource = "");
	void addUnsubscribe(QDomElement* body, QDomDocument* e, const QString &msg, const QString &jid, const QString &resource = "");
	void addDelete(QDomElement* body ,QDomDocument* e, const QString& msg, const QString& jid, const QString& resource = "");
	void addFavorite(QDomElement* body, QDomDocument* e, const QString &msg, const QString &jid, const QString &resource = "");

private slots:
	void chooseColor(QAbstractButton*);
	void clearCache();
	void updateJidList(const QStringList& jids);
	void requestJidList();
	void getAvatar(const QString& uid, const QString &unick);
	void photoReady(const QByteArray& ba);
	void removeWidget();

private:
	void createAvatarsDir();
	void getPhoto(const QUrl &url);

private:
	bool enabled;
	OptionAccessingHost* psiOptions;
	ActiveTabAccessingHost* activeTab;
	ApplicationInfoAccessingHost* applicationInfo;
	QCheckBox *ubButton, *uiButton, *uuButton, *tbButton, *tiButton, *tuButton;
	QCheckBox *mbButton, *miButton, *muButton, *qbButton, *qiButton, *quButton;
	QCheckBox *lbButton, *liButton, *luButton;
	QToolButton *ucButton, *tcButton, *mcButton, *qcButton, *lcButton;
	QColor userColor, tagColor, msgColor, quoteColor, lineColor;
	QCheckBox *asResourceButton, *showPhotoButton, *showAvatarsButton, *groupChatButton;
	bool userBold,tagBold,msgBold,quoteBold,lineBold;
	bool userItalic,tagItalic,msgItalic,quoteItalic,lineItalic;
	bool userUnderline,tagUnderline,msgUnderline,quoteUnderline,lineUnderline;
	QString juick,jubo;
	QString idStyle,userStyle,tagStyle,quoteStyle,linkStyle;
	QRegExp tagRx,pmRx,postRx,replyRx,regx,rpostRx,threadRx,userRx;
	QRegExp singleMsgRx,lastMsgRx,juboRx,msgPostRx,delMsgRx,delReplyRx,idRx,nickRx,recomendRx;
	QString userLinkPattern,messageLinkPattern,altTextUser,altTextMsg,commonLinkColor;
	bool idAsResource,showPhoto,showAvatars,workInGroupChat;
	QString showAllmsgString,replyMsgString,userInfoString,subscribeString,showLastTenString,unsubscribeString;
	QStringList jidList_;
	QPointer<QWidget> optionsWid;
	QList<QWidget*> logs_;
};

Q_EXPORT_PLUGIN(JuickPlugin)

JuickPlugin::JuickPlugin()
	: enabled(false)
	, psiOptions(0), activeTab(0), applicationInfo(0)
	, userColor(0, 85, 255), tagColor(131, 145, 145), msgColor(87, 165, 87), quoteColor(187, 187, 187), lineColor(0, 0, 255)
	, userBold(true), tagBold(false), msgBold(false), quoteBold(false), lineBold(false)
	, userItalic(false), tagItalic(true), msgItalic(false), quoteItalic(false), lineItalic(false)
	, userUnderline(false), tagUnderline(false), msgUnderline(true), quoteUnderline(false), lineUnderline(true)
	, juick("juick@juick.com"), jubo("jubo@nologin.ru")
	, idStyle(""), userStyle(""), tagStyle(""), quoteStyle(""), linkStyle("")
	, tagRx		("^\\s*(?!\\*\\S+\\*)(\\*\\S+)")
	, pmRx		("^\\nPrivate message from (@.+):(.*)$")
	, postRx	("\\n@(\\S*):( \\*[^\\n]*){0,1}\\n(.*)\\n\\n(#\\d+)\\s(http://\\S*)\\n$")
	, replyRx	("\\nReply by @(.*):\\n>(.{,50})\\n\\n(.*)\\n\\n(#\\d+/\\d+)\\s(http://\\S*)\\n$")
	, regx		("(\\s+)(#\\d+(?:\\S+)|#\\d+/\\d+(?:\\S+)|@\\S+|_[^\\n]+_|\\*[^\\n]+\\*|/[^\\n]+/|http://\\S+|ftp://\\S+|https://\\S+){1}(\\s+)")
	, rpostRx	("\\nReply posted.\\n(#.*)\\s(http://\\S*)\\n$")
	, threadRx	("^\\n@(\\S*):( \\*[^\\n]*){0,1}\\n(.*)\\n(#\\d+)\\s(http://juick.com/\\S+)\\n(.*)")
	, userRx	("^\\nBlog: http://.*")
	, singleMsgRx	("^\\n@(\\S*):( \\*[^\\n]*){0,1}\\n(.*)\\n(#\\d+) (\\((?:.*; )\\d+ repl(?:ies|y)\\) ){0,1}(http://juick.com/\\S+)\\n$")
	, lastMsgRx	("^\\n(Last (?:popular ){0,1}messages:)(.*)")
	, juboRx	("^\\n([^\\n]*)\\n@(\\S*):( [^\\n]*){0,1}\\n(.*)\\n(#\\d+)\\s(http://juick.com/\\S+)\\n$")
	, msgPostRx	("\\nNew message posted.\\n(#.*)\\s(http://\\S*)\\n$")
	, delMsgRx	("^\\nMessage #\\d+ deleted.\\n$")
	, delReplyRx	("^\\nReply #\\d+/\\d+ deleted.\\n$")
	, idRx		("(#\\d+)(/\\d+){0,1}(\\S+){0,1}")
	, nickRx	("(@[\\w\\-\\.@\\|]*)(\\b.*)")
	, recomendRx	("^\\nRecommended by @(\\S*):\\n@(\\S*):( \\*[^\\n]*){0,1}\\n(.*)\\n\\n(#\\d+) (\\(\\d+ repl(?:ies|y)\\) ){0,1}(http://\\S*)\\n$")
	, idAsResource(false), showPhoto(false), showAvatars(false), workInGroupChat(false)
	, showAllmsgString(tr("Show all messages"))
	, replyMsgString(tr("Reply"))
	, userInfoString(tr("Show %1's info and last 10 messages"))
	, subscribeString(tr("Subscribe"))
	, showLastTenString(tr("Show last 10 messages with tag %1"))
	, unsubscribeString(tr("Unsubscribe"))
{
	pmRx.setMinimal(true);
	replyRx.setMinimal(true);
	regx.setMinimal(true);
	postRx.setMinimal(true);
	singleMsgRx.setMinimal(true);
	juboRx.setMinimal(true);
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
	enabled = true;

	userColor = psiOptions->getPluginOption(constuserColor, userColor).toString();
	tagColor = psiOptions->getPluginOption(consttagColor, tagColor).toString();
	msgColor  = psiOptions->getPluginOption(constmsgColor, msgColor).toString();
	quoteColor = psiOptions->getPluginOption(constQcolor, quoteColor).toString();
	lineColor = psiOptions->getPluginOption(constLcolor, lineColor).toString();

	//bold
	userBold = psiOptions->getPluginOption(constUbold, userBold).toBool();
	tagBold = psiOptions->getPluginOption(constTbold, tagBold).toBool();
	msgBold = psiOptions->getPluginOption(constMbold, msgBold).toBool();
	quoteBold = psiOptions->getPluginOption(constQbold, quoteBold).toBool();
	lineBold = psiOptions->getPluginOption(constLbold, lineBold).toBool();

	//italic
	userItalic = psiOptions->getPluginOption(constUitalic, userItalic).toBool();
	tagItalic = psiOptions->getPluginOption(constTitalic, tagItalic).toBool();
	msgItalic = psiOptions->getPluginOption(constMitalic, msgItalic).toBool();
	quoteItalic = psiOptions->getPluginOption(constQitalic, quoteItalic).toBool();
	lineItalic = psiOptions->getPluginOption(constLitalic, lineItalic).toBool();

	//underline
	userUnderline = psiOptions->getPluginOption(constUunderline, userUnderline).toBool();
	tagUnderline = psiOptions->getPluginOption(constTunderline, tagUnderline).toBool();
	msgUnderline = psiOptions->getPluginOption(constMunderline, msgUnderline).toBool();
	quoteUnderline = psiOptions->getPluginOption(constQunderline, quoteUnderline).toBool();
	lineUnderline = psiOptions->getPluginOption(constLunderline, lineUnderline).toBool();

	idAsResource = psiOptions->getPluginOption(constIdAsResource, idAsResource).toBool();
	commonLinkColor =  psiOptions->getGlobalOption("options.ui.look.colors.chat.link-color").toString();
	showPhoto = psiOptions->getPluginOption(constShowPhoto, showPhoto).toBool();
	showAvatars = psiOptions->getPluginOption(constShowAvatars, showAvatars).toBool();
	workInGroupChat = psiOptions->getPluginOption(constWorkInGroupchat, workInGroupChat).toBool();
	jidList_ = psiOptions->getPluginOption("constJidList",QVariant(jidList_)).toStringList();
	applicationInfo->getProxyFor(constPluginName); // init proxy settings for Juick plugin

	if (showAvatars) {
		createAvatarsDir();
	}

	return true;
}

bool JuickPlugin::disable()
{
	enabled = false;
	logs_.clear();
	QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick/photos");
	foreach(const QString& file, dir.entryList(QDir::Files)) {
		QFile::remove(dir.absolutePath()+"/"+file);
	}
	return true;
}

void JuickPlugin::createAvatarsDir()
{
	QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars");
	dir.mkpath("juick/photos");
	if (!dir.exists("juick"))
	{
		QMessageBox::warning(0, tr("Warning"),tr("can't create folder %1 \ncaching avatars will be not available")
				     .arg(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick"));
	}
}

void JuickPlugin::applyOptions()
{
	if (!optionsWid)
		return;

	userColor = ucButton->property("psi_color").value<QColor>();
	tagColor = tcButton->property("psi_color").value<QColor>();
	msgColor = mcButton->property("psi_color").value<QColor>();
	quoteColor = qcButton->property("psi_color").value<QColor>();
	lineColor = lcButton->property("psi_color").value<QColor>();
	psiOptions->setPluginOption(constuserColor, userColor);
	psiOptions->setPluginOption(consttagColor, tagColor);
	psiOptions->setPluginOption(constmsgColor, msgColor);
	psiOptions->setPluginOption(constQcolor, quoteColor);
	psiOptions->setPluginOption(constLcolor, lineColor);

	//bold
	userBold = ubButton->isChecked();
	tagBold = tbButton->isChecked();
	msgBold = mbButton->isChecked();
	quoteBold = qbButton->isChecked();
	lineBold = lbButton->isChecked();
	psiOptions->setPluginOption(constUbold, userBold);
	psiOptions->setPluginOption(constTbold, tagBold);
	psiOptions->setPluginOption(constMbold, msgBold);
	psiOptions->setPluginOption(constQbold, quoteBold);
	psiOptions->setPluginOption(constLbold, lineBold);

	//italic
	userItalic = uiButton->isChecked();
	tagItalic = tiButton->isChecked();
	msgItalic = miButton->isChecked();
	quoteItalic = qiButton->isChecked();
	lineItalic = liButton->isChecked();
	psiOptions->setPluginOption(constUitalic, userItalic);
	psiOptions->setPluginOption(constTitalic, tagItalic);
	psiOptions->setPluginOption(constMitalic, msgItalic);
	psiOptions->setPluginOption(constQitalic, quoteItalic);
	psiOptions->setPluginOption(constLitalic, lineItalic);

	//underline
	userUnderline = uuButton->isChecked();
	tagUnderline = tuButton->isChecked();
	msgUnderline = muButton->isChecked();
	quoteUnderline = quButton->isChecked();
	lineUnderline = luButton->isChecked();
	psiOptions->setPluginOption(constUunderline, userUnderline);
	psiOptions->setPluginOption(constTunderline, tagUnderline);
	psiOptions->setPluginOption(constMunderline, msgUnderline);
	psiOptions->setPluginOption(constQunderline, quoteUnderline);
	psiOptions->setPluginOption(constLunderline, lineUnderline);

	//asResource
	idAsResource = asResourceButton->isChecked();
	psiOptions->setPluginOption(constIdAsResource, idAsResource);
	showPhoto = showPhotoButton->isChecked();
	psiOptions->setPluginOption(constShowPhoto, showPhoto);
	showAvatars = showAvatarsButton->isChecked();
	if (showAvatars)
		createAvatarsDir();
	psiOptions->setPluginOption(constShowAvatars, showAvatars);
	workInGroupChat = groupChatButton->isChecked();
	psiOptions->setPluginOption(constWorkInGroupchat, workInGroupChat);
	psiOptions->setPluginOption("constJidList",QVariant(jidList_));

}

void JuickPlugin::restoreOptions()
{
	if (!optionsWid)
		return;

	ucButton->setStyleSheet(QString("background-color: %1;").arg(userColor.name()));
	tcButton->setStyleSheet(QString("background-color: %1;").arg(tagColor.name()));
	mcButton->setStyleSheet(QString("background-color: %1;").arg(msgColor.name()));
	qcButton->setStyleSheet(QString("background-color: %1;").arg(quoteColor.name()));
	lcButton->setStyleSheet(QString("background-color: %1;").arg(lineColor.name()));

	//bold
	ubButton->setChecked(userBold);
	tbButton->setChecked(tagBold);
	mbButton->setChecked(msgBold);
	qbButton->setChecked(quoteBold);
	lbButton->setChecked(lineBold);

	//italic
	uiButton->setChecked(userItalic);
	tiButton->setChecked(tagItalic);
	miButton->setChecked(msgItalic);
	qiButton->setChecked(quoteItalic);
	liButton->setChecked(lineItalic);

	//underline
	uuButton->setChecked(userUnderline);
	tuButton->setChecked(tagUnderline);
	muButton->setChecked(msgUnderline);
	quButton->setChecked(quoteUnderline);
	luButton->setChecked(lineUnderline);

	asResourceButton->setChecked(idAsResource);
	showPhotoButton->setChecked(showPhoto);
	showAvatarsButton->setChecked(showAvatars);
	groupChatButton->setChecked(workInGroupChat);
}

void JuickPlugin::requestJidList()
{
	JuickJidList *jjl = new JuickJidList(jidList_, optionsWid);
	connect(jjl, SIGNAL(listUpdated(QStringList)), SLOT(updateJidList(QStringList)));
	jjl->show();
}

void JuickPlugin::updateJidList(const QStringList& jids)
{
	jidList_ = jids;
	//HACK
	if(optionsWid) {
		ubButton->toggle();
		ubButton->toggle();
	}
}

bool JuickPlugin::processEvent(int account, QDomElement& e)
{
	Q_UNUSED(account);
	if (!enabled){
		return false;
	}

	QDomDocument doc = e.ownerDocument();
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
		QDomElement element =  doc.createElement("html");
		element.setAttribute("xmlns","http://jabber.org/protocol/xhtml-im");
		QDomElement body = doc.createElement("body");
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
			nl2br(&body, &doc,msg);
			if (idAsResource) {
				QStringList tmp = activeTab->getJid().split('/');
				if (tmp.count() > 1 && jid == tmp.first()){
					resource = tmp.last();
				}
			}
			msg =  "";
		}
		if (! e.lastChild().firstChild().nextSibling().isNull() && e.lastChild().firstChild().nextSibling().nodeName() == "x"){
			//photo post
			if (showPhoto && postRx.indexIn(msg) != -1) {
				QString resLink("");
				if (idAsResource) {
					resource = postRx.cap(4);
					resLink = "/" + resource;
					resLink.replace("#","%23");
				}
				QDomNode domUrl = e.lastChild().firstChild().nextSibling().firstChild().firstChild();
				QString url = domUrl.nodeValue();
				if(url.split('.').last() == "jpg") {
					QUrl photoUrl(url);
					getPhoto(photoUrl);
					body.appendChild(doc.createElement("br"));
					addUserLink(&body, &doc, "@" + postRx.cap(1), altTextUser ,"xmpp:%1?message;type=chat;body=%2+",jidToSend);
					body.appendChild(doc.createTextNode(": "));
					if (!postRx.cap(2).isEmpty()){
						//добавляем теги
						foreach (const QString& tag, postRx.cap(2).trimmed().split(" ")) {
							addTagLink(&body, &doc, tag, jidToSend);
						}
					}
					QDomElement table = doc.createElement("table");
					QDomElement tableRow = doc.createElement("tr");
					QDomElement td1 = doc.createElement("td");
					td1.setAttribute("valign","top");
					QDomElement td2 = doc.createElement("td");
					QDomElement link = doc.createElement("a");
					link.setAttribute("href",url);
					QDomElement img = doc.createElement("img");
					QString imgdata = photoUrl.path().replace("/", "%");//"data:image/jpg;base64,"+QString(QUrl::toPercentEncoding(preview.toBase64()));
					QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick/photos");
					img.setAttribute("src", QString(QUrl::fromLocalFile(QString("%1/%2").arg(dir.absolutePath()).arg(imgdata)).toEncoded()));
					link.appendChild(img);
					td1.appendChild(link);
					QString newMsg = " " + postRx.cap(3) + " ";
					elementFromString(&td2, &doc,newMsg, jidToSend);
					tableRow.appendChild(td1);
					tableRow.appendChild(td2);
					table.appendChild(tableRow);
					body.appendChild(table);
					addMessageId(&body, &doc,postRx.cap(4), replyMsgString, "xmpp:%1%3?message;type=chat;body=%2",jidToSend, resLink);
					//ссылка на сообщение
					body.appendChild(doc.createTextNode(" "));
					addPlus(&body, &doc, postRx.cap(4),jidToSend, resLink);
					body.appendChild(doc.createTextNode(" "));
					addSubscribe(&body, &doc, postRx.cap(4),jidToSend, resLink);
					body.appendChild(doc.createTextNode(" "));
					addFavorite(&body, &doc, postRx.cap(4),jidToSend, resLink);
					body.appendChild(doc.createTextNode(" "));
					addHttpLink(&body, &doc, postRx.cap(5));
					msg = "";
				}
			}
			//удаление вложения, пока шлётся ссылка в сообщении
			e.lastChild().removeChild(e.lastChild().firstChild().nextSibling());
		}
		if ((jid == jubo || usernameJ == "jubo%nologin.ru") && juboRx.indexIn(msg) != -1) {
			//Jubo bot
			body.appendChild(doc.createTextNode(juboRx.cap(1)));
			body.appendChild(doc.createElement("br"));
			addUserLink(&body, &doc, "@" + juboRx.cap(2), altTextUser ,"xmpp:%1?message;type=chat;body=%2+", jidToSend);
			body.appendChild(doc.createTextNode(": "));
			if (!juboRx.cap(3).isEmpty()){
				//добавляем теги
				foreach (const QString& tag,juboRx.cap(3).trimmed().split(" ")){
					addTagLink(&body, &doc, tag, jidToSend);
				}
			}
			//обрабатываем текст сообщения
			QString newMsg = " " + juboRx.cap(4) + " ";
			if (showAvatars) {
				QDomElement table = doc.createElement("table");
				QDomElement tableRow = doc.createElement("tr");
				QDomElement td1 = doc.createElement("td");
				td1.setAttribute("valign","top");
				QDomElement td2 = doc.createElement("td");
				QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick");
				if (dir.exists()) {
					QDomElement img = doc.createElement("img");
					img.setAttribute("src", QString(QUrl::fromLocalFile(QString("%1/@%2;").arg(dir.absolutePath()).arg(juboRx.cap(2))).toEncoded()));
					///*/
					td1.appendChild(img);
				}
				elementFromString(&td2, &doc,newMsg,jidToSend);
				tableRow.appendChild(td1);
				tableRow.appendChild(td2);
				table.appendChild(tableRow);
				body.appendChild(table);
			} else {
				body.appendChild(doc.createElement("br"));
				//обрабатываем текст сообщения
				elementFromString(&body, &doc, newMsg, jidToSend);
			}
			//xmpp ссылка на сообщение
			addMessageId(&body, &doc,juboRx.cap(5), replyMsgString, "xmpp:%1%3?message;type=chat;body=%2", jidToSend);
			//ссылка на сообщение
			body.appendChild(doc.createTextNode(" "));
			addPlus(&body, &doc, juboRx.cap(5), jidToSend);
			body.appendChild(doc.createTextNode(" "));
			addSubscribe(&body, &doc, juboRx.cap(5),jidToSend);
			body.appendChild(doc.createTextNode(" "));
			addFavorite(&body, &doc, juboRx.cap(5),jidToSend);
			body.appendChild(doc.createTextNode(" "));
			addHttpLink(&body, &doc, juboRx.cap(6));
			msg = "";
		} else if (lastMsgRx.indexIn(msg) != -1) {
			//last 10 messages
			body.appendChild(doc.createTextNode(lastMsgRx.cap(1)));
			msg = lastMsgRx.cap(2);
			while (singleMsgRx.indexIn(msg) != -1){
				body.appendChild(doc.createElement("br"));
				addUserLink(&body, &doc, "@" + singleMsgRx.cap(1), altTextUser ,"xmpp:%1?message;type=chat;body=%2+", jidToSend);
				body.appendChild(doc.createTextNode(": "));
				if (!singleMsgRx.cap(2).isEmpty()){
					//добавляем теги
					foreach (const QString& tag, singleMsgRx.cap(2).trimmed().split(" ")){
						addTagLink(&body, &doc, tag, jidToSend);
					}
				}
				body.appendChild(doc.createElement("br"));
				//обрабатываем текст сообщения
				QString newMsg = " " + singleMsgRx.cap(3) + " ";
				elementFromString(&body, &doc, newMsg, jidToSend);
				//xmpp ссылка на сообщение
				addMessageId(&body, &doc,singleMsgRx.cap(4), replyMsgString, "xmpp:%1%3?message;type=chat;body=%2", jidToSend);
				body.appendChild(doc.createTextNode(" "));
				addPlus(&body, &doc, singleMsgRx.cap(4),jidToSend);
				//ссылка на сообщение
				body.appendChild(doc.createTextNode(" "));
				addSubscribe(&body, &doc, singleMsgRx.cap(4),jidToSend);
				body.appendChild(doc.createTextNode(" "));
				addFavorite(&body, &doc, singleMsgRx.cap(4),jidToSend);
				body.appendChild(doc.createTextNode(" "+singleMsgRx.cap(5)));
				addHttpLink(&body, &doc, singleMsgRx.cap(6));
				body.appendChild(doc.createElement("br"));
				msg = msg.right(msg.size() - singleMsgRx.matchedLength());
			}
			body.removeChild(body.lastChild());
		} else if (msg.indexOf(topTag) != -1){
			//Если это топ тегов
			body.appendChild(doc.createTextNode(topTag));
			body.appendChild(doc.createElement("br"));
			msg = msg.right(msg.size() - topTag.size() - 1);
			while (tagRx.indexIn(msg, 0) != -1){
				addTagLink(&body, &doc, tagRx.cap(1), jidToSend);
				body.appendChild(doc.createElement("br"));
				msg = msg.right(msg.size() - tagRx.matchedLength());
			}
		} else if (recomendRx.indexIn(msg) != -1){
			//разбор рекомендации
			QString resLink("");
			if (idAsResource) {
				resource = recomendRx.cap(5);
				resLink = "/" + resource;
				resLink.replace("#","%23");
			}
			body.appendChild(doc.createElement("br"));
			body.appendChild(doc.createTextNode(tr("Recommended by ")));
			addUserLink(&body, &doc, "@" + recomendRx.cap(1), altTextUser ,"xmpp:%1?message;type=chat;body=%2+", jidToSend);
			body.appendChild(doc.createTextNode(":"));
			body.appendChild(doc.createElement("br"));
			addUserLink(&body, &doc, "@" + recomendRx.cap(2), altTextUser ,"xmpp:%1?message;type=chat;body=%2+", jidToSend);
			body.appendChild(doc.createTextNode(": "));
			if (!recomendRx.cap(3).isEmpty()) {
				//добавляем теги
				foreach (const QString& tag,recomendRx.cap(3).trimmed().split(" ")){
					addTagLink(&body, &doc, tag, jidToSend);
				}
			}
			//mood
			QRegExp moodRx("\\*mood:\\s(\\S*)\\s(.*)\\n(.*)");
			//geo
			QRegExp geoRx("\\*geo:\\s(.*)\\n(.*)");
			//tune
			QRegExp tuneRx("\\*tune:\\s(.*)\\n(.*)");
			if (moodRx.indexIn(recomendRx.cap(4)) != -1){
				body.appendChild(doc.createElement("br"));
				QDomElement bold = doc.createElement("b");
				bold.appendChild(doc.createTextNode("mood: "));
				body.appendChild(bold);
				QDomElement img = doc.createElement("icon");
				img.setAttribute("name","mood/"+moodRx.cap(1).left(moodRx.cap(1).size()-1).toLower());
				img.setAttribute("text",moodRx.cap(1));
				body.appendChild(img);
				body.appendChild(doc.createTextNode(" "+moodRx.cap(2)));
				msg = " " + moodRx.cap(3) + " ";
			} else if(geoRx.indexIn(recomendRx.cap(4)) != -1) {
				body.appendChild(doc.createElement("br"));
				QDomElement bold = doc.createElement("b");
				bold.appendChild(doc.createTextNode("geo: "+ geoRx.cap(1) ));
				body.appendChild(bold);
				msg = " " + geoRx.cap(2) + " ";
			} else if(tuneRx.indexIn(recomendRx.cap(4)) != -1) {
				body.appendChild(doc.createElement("br"));
				QDomElement bold = doc.createElement("b");
				bold.appendChild(doc.createTextNode("tune: "+ tuneRx.cap(1) ));
				body.appendChild(bold);
				msg = " " + tuneRx.cap(2) + " ";
			}
			else{
				msg = " " + recomendRx.cap(4) + " ";
			}
			if (showAvatars){
				QDomElement table = doc.createElement("table");
				QDomElement tableRow = doc.createElement("tr");
				QDomElement td1 = doc.createElement("td");
				td1.setAttribute("valign","top");
				QDomElement td2 = doc.createElement("td");
				QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick");
				if (dir.exists()){
					QDomElement img = doc.createElement("img");
					img.setAttribute("src", QString(QUrl::fromLocalFile(QString("%1/@%2;").arg(dir.absolutePath()).arg(recomendRx.cap(2))).toEncoded()));
					td1.appendChild(img);
				}
				elementFromString(&td2, &doc,msg, jidToSend);
				tableRow.appendChild(td1);
				tableRow.appendChild(td2);
				table.appendChild(tableRow);
				body.appendChild(table);
			} else {
				body.appendChild(doc.createElement("br"));
				//обрабатываем текст сообщения
				elementFromString(&body, &doc,msg,  jidToSend);
			}
			//xmpp ссылка на сообщение
			addMessageId(&body, &doc,recomendRx.cap(5), replyMsgString, "xmpp:%1%3?message;type=chat;body=%2",jidToSend,resLink);
			//ссылка на сообщение
			body.appendChild(doc.createTextNode(" "));
			addPlus(&body, &doc, recomendRx.cap(5),jidToSend, resLink);
			body.appendChild(doc.createTextNode(" "));
			addSubscribe(&body, &doc, recomendRx.cap(5),jidToSend,resLink);
			body.appendChild(doc.createTextNode(" "));
			addFavorite(&body, &doc, recomendRx.cap(5),jidToSend,resLink);
			body.appendChild(doc.createTextNode(" "));
			body.appendChild(doc.createTextNode(recomendRx.cap(6)));
			body.appendChild(doc.createTextNode(" "));
			addHttpLink(&body, &doc, recomendRx.cap(7));
			msg = "";
		} else if (postRx.indexIn(msg) != -1){
			//разбор сообщения
			QString resLink("");
			if (idAsResource) {
				resource = postRx.cap(4);
				resLink = "/" + resource;
				resLink.replace("#","%23");
			}
			body.appendChild(doc.createElement("br"));
			addUserLink(&body, &doc, "@" + postRx.cap(1), altTextUser ,"xmpp:%1?message;type=chat;body=%2+", jidToSend);
			body.appendChild(doc.createTextNode(": "));
			if (!postRx.cap(2).isEmpty()) {
				//добавляем теги
				foreach (const QString& tag,postRx.cap(2).trimmed().split(" ")){
					addTagLink(&body, &doc, tag, jidToSend);
				}
			}
			//mood
			QRegExp moodRx("\\*mood:\\s(\\S*)\\s(.*)\\n(.*)");
			//geo
			QRegExp geoRx("\\*geo:\\s(.*)\\n(.*)");
			//tune
			QRegExp tuneRx("\\*tune:\\s(.*)\\n(.*)");
			if (moodRx.indexIn(postRx.cap(3)) != -1){
				body.appendChild(doc.createElement("br"));
				QDomElement bold = doc.createElement("b");
				bold.appendChild(doc.createTextNode("mood: "));
				body.appendChild(bold);
				QDomElement img = doc.createElement("icon");
				img.setAttribute("name","mood/"+moodRx.cap(1).left(moodRx.cap(1).size()-1).toLower());
				img.setAttribute("text",moodRx.cap(1));
				body.appendChild(img);
				body.appendChild(doc.createTextNode(" "+moodRx.cap(2)));
				msg = " " + moodRx.cap(3) + " ";
			} else if(geoRx.indexIn(postRx.cap(3)) != -1) {
				body.appendChild(doc.createElement("br"));
				QDomElement bold = doc.createElement("b");
				bold.appendChild(doc.createTextNode("geo: "+ geoRx.cap(1) ));
				body.appendChild(bold);
				msg = " " + geoRx.cap(2) + " ";
			} else if(tuneRx.indexIn(postRx.cap(3)) != -1) {
				body.appendChild(doc.createElement("br"));
				QDomElement bold = doc.createElement("b");
				bold.appendChild(doc.createTextNode("tune: "+ tuneRx.cap(1) ));
				body.appendChild(bold);
				msg = " " + tuneRx.cap(2) + " ";
			}
			else{
				msg = " " + postRx.cap(3) + " ";
			}
			if (showAvatars){
				QDomElement table = doc.createElement("table");
				QDomElement tableRow = doc.createElement("tr");
				QDomElement td1 = doc.createElement("td");
				td1.setAttribute("valign","top");
				QDomElement td2 = doc.createElement("td");
				QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick");
				if (dir.exists()){
					QDomElement img = doc.createElement("img");
					img.setAttribute("src", QString(QUrl::fromLocalFile(QString("%1/@%2;").arg(dir.absolutePath()).arg(postRx.cap(1))).toEncoded()));
					td1.appendChild(img);
				}
				elementFromString(&td2, &doc,msg, jidToSend);
				tableRow.appendChild(td1);
				tableRow.appendChild(td2);
				table.appendChild(tableRow);
				body.appendChild(table);
			} else {
				body.appendChild(doc.createElement("br"));
				//обрабатываем текст сообщения
				elementFromString(&body, &doc,msg,  jidToSend);
			}
			//xmpp ссылка на сообщение
			addMessageId(&body, &doc,postRx.cap(4), replyMsgString, "xmpp:%1%3?message;type=chat;body=%2",jidToSend,resLink);
			//ссылка на сообщение
			body.appendChild(doc.createTextNode(" "));
			addPlus(&body, &doc, postRx.cap(4),jidToSend, resLink);
			body.appendChild(doc.createTextNode(" "));
			addSubscribe(&body, &doc, postRx.cap(4),jidToSend,resLink);
			body.appendChild(doc.createTextNode(" "));
			addFavorite(&body, &doc, postRx.cap(4),jidToSend,resLink);
			body.appendChild(doc.createTextNode(" "));
			addHttpLink(&body, &doc, postRx.cap(5));
			msg = "";
		} else if (replyRx.indexIn(msg) != -1){
			//обработка реплеев
			QString resLink("");
			QString replyId(replyRx.cap(4));
			if (idAsResource) {
				resource = replyId.left(replyId.indexOf("/"));
				resLink = "/" + resource;
				resLink.replace("#","%23");
			}
			body.appendChild(doc.createElement("br"));
			addUserLink(&body, &doc, "@" + replyRx.cap(1), altTextUser ,"xmpp:%1?message;type=chat;body=%2+", jidToSend);
			body.appendChild(doc.createTextNode(tr(" replied:")));
			//цитата
			QDomElement blockquote = doc.createElement("blockquote");
			blockquote.setAttribute("style",quoteStyle);
			blockquote.appendChild(doc.createTextNode(replyRx.cap(2)));
			//обрабатываем текст сообщения
			msg = " " + replyRx.cap(3) + " ";
			if (showAvatars){
				QDomElement table = doc.createElement("table");
				QDomElement tableRow = doc.createElement("tr");
				QDomElement td1 = doc.createElement("td");
				td1.setAttribute("valign","top");
				QDomElement td2 = doc.createElement("td");
				QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick");
				if (dir.exists()){
					QDomElement img = doc.createElement("img");
					img.setAttribute("src", QString(QUrl::fromLocalFile(QString("%1/@%2;").arg(dir.absolutePath()).arg(replyRx.cap(1))).toEncoded()));
					td1.appendChild(img);
				}
				td2.appendChild(blockquote);
				elementFromString(&td2, &doc,msg,jidToSend);
				tableRow.appendChild(td1);
				tableRow.appendChild(td2);
				table.appendChild(tableRow);
				body.appendChild(table);
			} else {
				body.appendChild(blockquote);
				elementFromString(&body, &doc,msg,jidToSend);
			}
			//xmpp ссылка на сообщение
			addMessageId(&body, &doc,replyId, replyMsgString, "xmpp:%1%3?message;type=chat;body=%2",jidToSend,resLink);
			//ссылка на сообщение
			body.appendChild(doc.createTextNode(" "));
			QString msgId = replyId.split("/").first();
			addUnsubscribe(&body, &doc, replyId,jidToSend, resLink);
			body.appendChild(doc.createTextNode(" "));
			addPlus(&body, &doc, msgId, jidToSend, resLink);
			body.appendChild(doc.createTextNode(" "));
			addHttpLink(&body, &doc, replyRx.cap(5));
			msg = "";
		} else if (rpostRx.indexIn(msg) != -1) {
			//Reply posted
			QString resLink("");
			if (idAsResource) {
				QString tmp(rpostRx.cap(1));
				resource = tmp.left(tmp.indexOf("/"));
				resLink = "/" + resource;
				resLink.replace("#","%23");
			}
			body.appendChild(doc.createElement("br"));
			body.appendChild(doc.createTextNode(tr("Reply posted.")));
			body.appendChild(doc.createElement("br"));
			//xmpp ссылка на сообщение
			addMessageId(&body, &doc,rpostRx.cap(1), replyMsgString, "xmpp:%1%3?message;type=chat;body=%2",jidToSend,resLink);
			body.appendChild(doc.createTextNode(" "));
			addDelete(&body, &doc,rpostRx.cap(1),jidToSend,resLink);
			body.appendChild(doc.createTextNode(" "));
			//ссылка на сообщение
			body.appendChild(doc.createTextNode(" "));
			addHttpLink(&body, &doc, rpostRx.cap(2));
			msg = "";

		} else if (msgPostRx.indexIn(msg) != -1) {
			//New message posted
			QString resLink("");
			if (idAsResource) {
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
			body.appendChild(doc.createElement("br"));
			body.appendChild(doc.createTextNode(tr("New message posted.")));
			body.appendChild(doc.createElement("br"));
			//xmpp ссылка на сообщение
			addMessageId(&body, &doc,msgPostRx.cap(1), replyMsgString, "xmpp:%1%3?message;type=chat;body=%2",jidToSend);
			body.appendChild(doc.createTextNode(" "));
			addDelete(&body, &doc,msgPostRx.cap(1),jidToSend);
			body.appendChild(doc.createTextNode(" "));
			//ссылка на сообщение
			body.appendChild(doc.createTextNode(" "));
			addHttpLink(&body, &doc, msgPostRx.cap(2));
			msg = "";
		} else if (threadRx.indexIn(msg) != -1) {
			//Show All Messages
			QString resLink("");
			if (idAsResource) {
				resource = threadRx.cap(4);
				resLink = "/" + resource;
				resLink.replace("#","%23");
				res = resLink;
			}
			body.appendChild(doc.createElement("br"));
			addUserLink(&body, &doc, "@" + threadRx.cap(1), altTextUser ,"xmpp:%1?message;type=chat;body=%2+",jidToSend);
			body.appendChild(doc.createTextNode(": "));
			if (!threadRx.cap(2).isEmpty()) {
				//добавляем теги
				foreach (const QString& tag,threadRx.cap(2).trimmed().split(" ")){
					addTagLink(&body, &doc, tag,jidToSend);
				}
			}
			body.appendChild(doc.createElement("br"));
			//обрабатываем текст сообщения
			QString newMsg(" " + threadRx.cap(3) + " ");
			elementFromString(&body, &doc,newMsg,jidToSend);
			//xmpp ссылка на сообщение
			addMessageId(&body, &doc,threadRx.cap(4), replyMsgString, "xmpp:%1%3?message;type=chat;body=%2",jidToSend,resLink);
			//ссылка на сообщение
			body.appendChild(doc.createTextNode(" "));
			addSubscribe(&body, &doc, threadRx.cap(4),jidToSend, resLink);
			body.appendChild(doc.createTextNode(" "));
			addFavorite(&body, &doc, threadRx.cap(4),jidToSend, resLink);
			body.appendChild(doc.createTextNode(" "));
			addHttpLink(&body, &doc, threadRx.cap(5));
			msg = msg.right(msg.size() - threadRx.matchedLength() + threadRx.cap(6).length());
		} else if (singleMsgRx.indexIn(msg) != -1) {
			//просмотр отдельного поста
			body.appendChild(doc.createElement("br"));
			addUserLink(&body, &doc, "@" + singleMsgRx.cap(1), altTextUser ,"xmpp:%1?message;type=chat;body=%2+",jidToSend);
			body.appendChild(doc.createTextNode(": "));
			if (!singleMsgRx.cap(2).isEmpty()) {
				//добавляем теги
				foreach (const QString& tag,singleMsgRx.cap(2).trimmed().split(" ")){
					addTagLink(&body, &doc, tag,jidToSend);
				}
			}
			body.appendChild(doc.createElement("br"));
			//обрабатываем текст сообщения
			QString newMsg = " " + singleMsgRx.cap(3) + " ";
			elementFromString(&body, &doc, newMsg,jidToSend);
			//xmpp ссылка на сообщение
			if (singleMsgRx.cap(5).isEmpty()) {
				messageLinkPattern = "xmpp:%1%3?message;type=chat;body=%2";
				altTextMsg = replyMsgString;
			}
			addMessageId(&body, &doc, singleMsgRx.cap(4), altTextMsg, messageLinkPattern,jidToSend);
			//ссылка на сообщение
			body.appendChild(doc.createTextNode(" "));
			addSubscribe(&body, &doc, singleMsgRx.cap(4),jidToSend);
			body.appendChild(doc.createTextNode(" "));
			addFavorite(&body, &doc, singleMsgRx.cap(4),jidToSend);
			body.appendChild(doc.createTextNode(" "+singleMsgRx.cap(5)));
			addHttpLink(&body, &doc, singleMsgRx.cap(6));
			msg = "";
		} else if (msg.indexOf("Recommended blogs:") != -1) {
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
				   || delReplyRx.indexIn(msg) != -1 ) {
			msg = msg.left(msg.size() - 1);
		}
		if (idAsResource && resource.isEmpty() && (jid != jubo || usernameJ != "jubo%nologin.ru")) {
			QStringList tmp = activeTab->getJid().split('/');
			if (tmp.count() > 1 && jid == tmp.first()){
				resource = tmp.last();
			}
		}
		//обработка по умолчанию
		elementFromString(&body, &doc,msg,jidToSend,res);
		element.appendChild(body);
		e.lastChild().appendChild(element);
		if (!resource.isEmpty()) {
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

void JuickPlugin::clearCache()
{
	QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick");
	foreach(const QString& file, dir.entryList(QDir::Files)) {
		QFile::remove(dir.absolutePath()+"/"+file);
	}
}

void JuickPlugin::setOptionAccessingHost(OptionAccessingHost* host)
{
	psiOptions = host;
}

void JuickPlugin::setActiveTabAccessingHost(ActiveTabAccessingHost* host)
{
	activeTab = host;
}

void JuickPlugin::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host)
{
	applicationInfo = host;
}

/*
  void JuickPlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost *host) {
  accInfo = host;
  }
*/

bool JuickPlugin::incomingStanza(int account, const QDomElement& stanza)
{
	if(!enabled)
		return false;

	Q_UNUSED(account);
	QString jid("");
	QString usernameJ("");
	if (stanza.tagName() == "message" ) {
		jid = stanza.attribute("from").split('/').first();
		usernameJ = jid.split("@").first();
		if (workInGroupChat && jid == "juick@conference.jabber.ru") {
			QString msg = stanza.firstChild().nextSibling().firstChild().nodeValue();
			msg.replace(QRegExp("#(\\d+)"),"http://juick.com/\\1");
			stanza.firstChild().nextSibling().firstChild().setNodeValue(msg);
		}

		if (showAvatars && (jidList_.contains(jid) || usernameJ == "juick%juick.com" || usernameJ == "jubo%nologin.ru")) {
			QDomNodeList childs = stanza.childNodes();
			int size = childs.size();
			for(int i = 0; i< size; ++i) {
				QDomElement element = childs.item(i).toElement();
				if (!element.isNull() && element.tagName() == "juick") {
					QDomElement userElement = element.firstChildElement("user");
					const QString uid = userElement.attribute("uid");
					QString unick = "@" + userElement.attribute("uname");
					QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick");
					if(!dir.exists())
						return false;

					QStringList fileNames = dir.entryList(QStringList(QString(unick + ";*")));
					if (!fileNames.empty()) {
						QFile file(QString("%1/%2").arg(dir.absolutePath()).arg(fileNames.first()));
						if (QFileInfo(file).lastModified().daysTo(QDateTime::currentDateTime()) > avatarsUpdateInterval
								|| file.size() == 0) {
							file.remove();
						}
						else {
							return false;
						}
					}

					QMetaObject::invokeMethod(this, "getAvatar",
								Qt::QueuedConnection,
								Q_ARG(const QString&, uid),
								Q_ARG(const QString&, unick));

					break;
				}
			}
		}
	}
	return false;
}


// Эта функция обновляет чатлоги, чтобы они перезагрузили
// картинки с диска
static void updateWidgets(QList<QWidget*> widgets)
{
	foreach(QWidget *w, widgets) {
		if(w->inherits("QTextEdit"))
			w->update();
		else {
			QWebView *wv = w->findChild<QWebView*>();
			if(wv) {
				wv->update();
			}
		}
	}
}

void JuickPlugin::getAvatar(const QString &uid, const QString& unick)
{
	QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick");
	Http *http = new Http(this);
	http->setHost("i.juick.com");

	Proxy prx = applicationInfo->getProxyFor(constPluginName);
	http->setProxyHostPort(prx.host, prx.port, prx.user, prx.pass, prx.type);

	QByteArray img = http->get("/as/"+uid+".png");
	if(img.isEmpty())
		img = http->get("/a/"+uid+".png");
	http->deleteLater();
	QFile file(QString("%1/%2;").arg(dir.absolutePath()).arg(unick));

	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::warning(0, tr("Warning"),tr("Cannot write to file %1:\n%2.")
				     .arg(file.fileName())
				     .arg(file.errorString()));
	}
	else {
		file.write(img);
		file.close();
	}

	updateWidgets(logs_);
}

void JuickPlugin::getPhoto(const QUrl &url)
{
	Http *http = new Http(this);
	http->setHost(url.host());

	Proxy prx = applicationInfo->getProxyFor(constPluginName);
	http->setProxyHostPort(prx.host, prx.port, prx.user, prx.pass, prx.type);
	http->setProperty("path", url.path().replace("/", "%"));
	connect(http, SIGNAL(dataReady(QByteArray)), SLOT(photoReady(QByteArray)));
	http->get(QString(url.path()).replace("/photos-1024/","/ps/"), false);
}

void JuickPlugin::photoReady(const QByteArray &ba)
{
	Http* http = static_cast<Http*>(sender());
	http->deleteLater();
	QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick/photos");
	if(!dir.exists())
		return;

	QString path = http->property("path").toString();
	QFile file(QString("%1/%2").arg(dir.absolutePath()).arg(path));

	if(!file.open(QIODevice::WriteOnly)){
		QMessageBox::warning(0, tr("Warning"),tr("Cannot write to file %1:\n%2.")
				     .arg(file.fileName())
				     .arg(file.errorString()));
	}
	else {
		file.write(ba);
		file.close();
	}

	updateWidgets(logs_);
}

void JuickPlugin::elementFromString(QDomElement* body,QDomDocument* e, const QString& msg, const QString& jid, const QString& resource)
{
	int new_pos = 0;
	int pos = 0;
	while ((new_pos = regx.indexIn(msg, pos)) != -1) {
		QString before = msg.mid(pos,new_pos-pos+regx.cap(1).length());
		int quoteSize = 0;
		nl2br(body, e, before.right(before.size() - quoteSize));
		QString seg = regx.cap(2);
		switch (seg.at(0).toAscii()) {
		case '#':{
			idRx.indexIn(seg);
			if (!idRx.cap(2).isEmpty()) {
				//для #1234/12 - +ненужен
				messageLinkPattern = "xmpp:%1%3?message;type=chat;body=%2";
				altTextMsg = replyMsgString;
			}
			addMessageId(body, e,idRx.cap(1)+idRx.cap(2), altTextMsg, messageLinkPattern,jid, resource);
			body->appendChild(e->createTextNode(idRx.cap(3)));
			break;}
		case '@':{
			nickRx.indexIn(seg);
			addUserLink(body, e, nickRx.cap(1), altTextUser ,userLinkPattern,jid);
			body->appendChild(e->createTextNode(nickRx.cap(2)));
			//tag
			if (nickRx.cap(2) == ":" && (regx.cap(1) == "\n" || regx.cap(1) == "\n\n")){
				body->appendChild(e->ownerDocument().createTextNode(" "));
				QString tagMsg = msg.right(msg.size()-(new_pos+regx.matchedLength()-regx.cap(3).size()));
				for (int i=0; i < 6; ++i){
					if (tagRx.indexIn(tagMsg, 0) != -1){
						addTagLink(body, e, tagRx.cap(1),jid);
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
			QDomElement bold = e->createElement("b");
			bold.appendChild(e->createTextNode(seg.mid(1,seg.size()-2)));
			body->appendChild(bold);
			break;}
		case '_':{
			QDomElement under = e->createElement("u");
			under.appendChild(e->createTextNode(seg.mid(1,seg.size()-2)));
			body->appendChild(under);
			break;}
		case '/':{
			QDomElement italic = e->createElement("i");
			italic.appendChild(e->createTextNode(seg.mid(1,seg.size()-2)));
			body->appendChild(italic);
			break;}
		case 'h':
		case 'f':{
			QDomElement ahref = e->createElement("a");
			ahref.setAttribute("style","color:" + commonLinkColor + ";");
			ahref.setAttribute("href",seg);
			ahref.appendChild(e->createTextNode(seg));
			body->appendChild(ahref);
			break;}
		default:{}
		}
		pos = new_pos+regx.matchedLength()-regx.cap(3).size();
		new_pos = pos;
	}
	nl2br(body, e , msg.right(msg.size()-pos));
	body->appendChild(e->createElement("br"));
}

void JuickPlugin::nl2br(QDomElement *body,QDomDocument* e, const QString& msg)
{
	foreach (const QString& str, msg.split("\n")) {
		body->appendChild(e->createTextNode(str));
		body->appendChild(e->createElement("br"));
	}
	body->removeChild(body->lastChild());
}

void JuickPlugin::addPlus(QDomElement *body,QDomDocument* e, const QString& msg_, const QString& jid, const QString& resource)
{
	QString msg(msg_);
	QDomElement plus = e->createElement("a");
	plus.setAttribute("style",idStyle);
	plus.setAttribute("title",showAllmsgString);
	plus.setAttribute("href",QString("xmpp:%1%3?message;type=chat;body=%2+").arg(jid).arg(msg.replace("#","%23")).arg(resource));
	plus.appendChild(e->createTextNode("+"));
	body->appendChild(plus);
}

void JuickPlugin::addSubscribe(QDomElement* body,QDomDocument* e, const QString& msg_, const QString& jid, const QString& resource)
{
	QString msg(msg_);
	QDomElement subscribe = e->createElement("a");
	subscribe.setAttribute("style",idStyle);
	subscribe.setAttribute("title",subscribeString);
	subscribe.setAttribute("href",QString("xmpp:%1%3?message;type=chat;body=S %2").arg(jid).arg(msg.replace("#","%23")).arg(resource));
	subscribe.appendChild(e->createTextNode("S"));
	body->appendChild(subscribe);
}

void JuickPlugin::addHttpLink(QDomElement* body,QDomDocument* e, const QString& msg)
{
	QDomElement ahref = e->createElement("a");
	ahref.setAttribute("href",msg);
	ahref.setAttribute("style",linkStyle);
	ahref.appendChild(e->createTextNode(msg));
	body->appendChild(ahref);
}

void JuickPlugin::addTagLink(QDomElement* body,QDomDocument* e, const QString& tag, const QString& jid)
{
	QDomElement taglink = e->createElement("a");
	taglink.setAttribute("style",tagStyle);
	taglink.setAttribute("title",showLastTenString.arg(tag));
	taglink.setAttribute("href",QString("xmpp:%1?message;type=chat;body=%2").arg(jid).arg(tag));
	taglink.appendChild(e->createTextNode( tag));
	body->appendChild(taglink);
	body->appendChild(e->createTextNode(" "));
}

void JuickPlugin::addUserLink(QDomElement* body,QDomDocument* e, const QString& nick, const QString& altText, const QString& pattern, const QString& jid)
{
	QDomElement ahref = e->createElement("a");
	ahref.setAttribute("style", userStyle);
	ahref.setAttribute("title", altText.arg(nick));
	ahref.setAttribute("href", pattern.arg(jid).arg(nick));
	ahref.appendChild(e->createTextNode(nick));
	body->appendChild(ahref);
}

void JuickPlugin::addMessageId(QDomElement* body,QDomDocument* e, const QString& mId_,
			       const QString& altText, const QString& pattern, const QString& jid, const QString& resource)
{
	QString mId(mId_);
	QDomElement ahref = e->createElement("a");
	ahref.setAttribute("style",idStyle);
	ahref.setAttribute("title",altText);
	ahref.setAttribute("href",QString(pattern).arg(jid).arg(mId.replace("#","%23")).arg(resource));
	ahref.appendChild(e->createTextNode(mId.replace("%23","#")));
	body->appendChild(ahref);
}

void JuickPlugin::addUnsubscribe(QDomElement* body,QDomDocument* e, const QString& msg_, const QString& jid, const QString& resource)
{
	QString msg(msg_);
	QDomElement unsubscribe = e->createElement("a");
	unsubscribe.setAttribute("style",idStyle);
	unsubscribe.setAttribute("title",unsubscribeString);
	unsubscribe.setAttribute("href",QString("xmpp:%1%3?message;type=chat;body=U %2").arg(jid).arg(msg.left(msg.indexOf("/")).replace("#","%23")).arg(resource));
	unsubscribe.appendChild(e->createTextNode("U"));
	body->appendChild(unsubscribe);
}

void JuickPlugin::addDelete(QDomElement* body, QDomDocument* e, const QString& msg_, const QString& jid, const QString& resource)
{
	QString msg(msg_);
	QDomElement unsubscribe = e->createElement("a");
	unsubscribe.setAttribute("style",idStyle);
	unsubscribe.setAttribute("title",tr("Delete"));
	unsubscribe.setAttribute("href",QString("xmpp:%1%3?message;type=chat;body=D %2").arg(jid).arg(msg.replace("#","%23")).arg(resource));
	unsubscribe.appendChild(e->createTextNode("D"));
	body->appendChild(unsubscribe);
}

void JuickPlugin::addFavorite(QDomElement* body,QDomDocument* e, const QString& msg_, const QString& jid, const QString& resource)
{
	QString msg(msg_);
	QDomElement unsubscribe = e->createElement("a");
	unsubscribe.setAttribute("style",idStyle);
	unsubscribe.setAttribute("title",tr("Add to favorites"));
	unsubscribe.setAttribute("href",QString("xmpp:%1%3?message;type=chat;body=! %2").arg(jid).arg(msg.replace("#","%23")).arg(resource));
	unsubscribe.appendChild(e->createTextNode("!"));
	body->appendChild(unsubscribe);
}

// На самом деле мы никаких акшенов не добавляем.
// Здесь мы просто ищем и сохраняем список уже открытых
// чатов с juick
QAction* JuickPlugin::getAction(QObject *parent, int /*account*/, const QString &contact)
{
	if(jidList_.contains(contact.split("/").first())) {
		QWidget* log = parent->findChild<QWidget*>("log");
		if(log) {
			logs_.append(log);
			connect(log, SIGNAL(destroyed()), SLOT(removeWidget()));
		}
	}
	return 0;
}

void JuickPlugin::removeWidget()
{
	QWidget* w = static_cast<QWidget*>(sender());
	logs_.removeAll(w);
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
