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

#include <QDomElement>
#include <QMessageBox>
#include <QColorDialog>
#include <QTextEdit>
#include <QtWebKit/QWebView>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebElement>

#include "psiplugin.h"
//#include "eventfilter.h"
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
#include "ui_settings.h"
#include "juickparser.h"

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

static const QString showAllmsgString(QObject::tr("Show all messages"));
static const QString replyMsgString(QObject::tr("Reply"));
static const QString userInfoString(QObject::tr("Show %1's info and last 10 messages"));
static const QString subscribeString(QObject::tr("Subscribe"));
static const QString showLastTenString(QObject::tr("Show last 10 messages with tag %1"));
static const QString unsubscribeString(QObject::tr("Unsubscribe"));
static const QString juick("juick@juick.com");
static const QString jubo("jubo@nologin.ru");

static const QString chatPlusAction = "xmpp:%1?message;type=chat;body=%2+";
static const QString chatAction = "xmpp:%1%3?message;type=chat;body=%2";

static const int avatarsUpdateInterval = 10;



//static void debugElement(const QDomElement& e)
//{
//	QString out;
//	QTextStream str(&out);
//	e.save(str, 3);
//	qDebug() << out;
//}

static void save(const QString &path, const QByteArray &img)
{
	QFile file(path);

	if(file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		file.write(img);
	}
	else
		QMessageBox::warning(0, QObject::tr("Warning"), QObject::tr("Cannot write to file %1:\n%2.")
				     .arg(file.fileName())
				     .arg(file.errorString()));
}

static void nl2br(QDomElement *body,QDomDocument* e, const QString& msg)
{
	foreach (const QString& str, msg.split("\n")) {
		body->appendChild(e->createTextNode(str));
		body->appendChild(e->createElement("br"));
	}
	body->removeChild(body->lastChild());
}




//-----------------------------
//------JuickPlugin------------
//-----------------------------
class JuickPlugin : public QObject, public PsiPlugin, /*public EventFilter,*/ public OptionAccessor, public ActiveTabAccessor,
			public StanzaFilter, public ApplicationInfoAccessor, public PluginInfoProvider, public ToolbarIconAccessor
{
	Q_OBJECT
	Q_INTERFACES(PsiPlugin /*EventFilter*/ OptionAccessor ActiveTabAccessor StanzaFilter
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

//	//event filter
//	virtual bool processEvent(int /*account*/, QDomElement& /*e*/) { return false; }
//	virtual bool processMessage(int , const QString& , const QString& , const QString& ) { return false; }
//	virtual bool processOutgoingMessage(int , const QString& , QString& , const QString& , QString& ) { return false; }
//	virtual void logout(int ) {}

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

private slots:
	void chooseColor(QWidget *);
	void clearCache();
	void updateJidList(const QStringList& jids);
	void requestJidList();
	void photoReady(const QByteArray& ba);
	void removeWidget();
	void updateWidgets();

private:
	void createAvatarsDir();
	void getAvatar(const QString& link, const QString &unick);
	void getPhoto(const QUrl &url);
	Http* newHttp(const QString &path);
	void setStyles();

	void elementFromString(QDomElement* body, QDomDocument* e, const QString &msg, const QString &jid, const QString &resource = "");
	void addPlus(QDomElement* body, QDomDocument* e, const QString &msg, const QString &jid, const QString &resource = "");
	void addSubscribe(QDomElement* body, QDomDocument* e, const QString &msg, const QString &jid, const QString &resource = "");
	void addHttpLink(QDomElement* body, QDomDocument* e, const QString &msg);
	void addTagLink(QDomElement* body, QDomDocument* e, const QString &tag, const QString &jid);
	void addUserLink(QDomElement* body, QDomDocument* e, const QString& nick, const QString& altText, const QString& pattern, const QString& jid);
	void addMessageId(QDomElement* body, QDomDocument* e, const QString& mId, const QString& altText, const QString& pattern, const QString& jid, const QString& resource = "");
	void addUnsubscribe(QDomElement* body, QDomDocument* e, const QString &msg, const QString &jid, const QString &resource = "");
	void addDelete(QDomElement* body ,QDomDocument* e, const QString& msg, const QString& jid, const QString& resource = "");
	void addFavorite(QDomElement* body, QDomDocument* e, const QString &msg, const QString &jid, const QString &resource = "");
	void addAvatar(QDomElement *body, QDomDocument *doc, const QString &msg, const QString &jidToSend, const QString &ujid);

private:
	bool enabled;
	OptionAccessingHost* psiOptions;
	ActiveTabAccessingHost* activeTab;
	ApplicationInfoAccessingHost* applicationInfo;
	QColor userColor, tagColor, msgColor, quoteColor, lineColor;
	bool userBold,tagBold,msgBold,quoteBold,lineBold;
	bool userItalic,tagItalic,msgItalic,quoteItalic,lineItalic;
	bool userUnderline,tagUnderline,msgUnderline,quoteUnderline,lineUnderline;
	QString idStyle,userStyle,tagStyle,quoteStyle,linkStyle;
	QRegExp tagRx, pmRx, regx, userRx, delMsgRx, delReplyRx, idRx, nickRx;
	QString userLinkPattern,messageLinkPattern,altTextUser,altTextMsg,commonLinkColor;
	bool idAsResource,showPhoto,showAvatars,workInGroupChat;
	QStringList jidList_;
	QPointer<QWidget> optionsWid;
	QList<QWidget*> logs_;
	QList<QByteArray> urls_;
	Ui::settings ui_;
	QTimer updateTimer_;
};

Q_EXPORT_PLUGIN(JuickPlugin)

JuickPlugin::JuickPlugin()
	: enabled(false)
	, psiOptions(0), activeTab(0), applicationInfo(0)
	, userColor(0, 85, 255), tagColor(131, 145, 145), msgColor(87, 165, 87), quoteColor(187, 187, 187), lineColor(0, 0, 255)
	, userBold(true), tagBold(false), msgBold(false), quoteBold(false), lineBold(false)
	, userItalic(false), tagItalic(true), msgItalic(false), quoteItalic(false), lineItalic(false)
	, userUnderline(false), tagUnderline(false), msgUnderline(true), quoteUnderline(false), lineUnderline(true)
	, tagRx		("^\\s*(?!\\*\\S+\\*)(\\*\\S+)")
	, pmRx		("^\\nPrivate message from (@.+):(.*)$")
	, regx		("(\\s+)(#\\d+(?:\\S+)|#\\d+/\\d+(?:\\S+)|@\\S+|_[^\\n]+_|\\*[^\\n]+\\*|/[^\\n]+/|http://\\S+|ftp://\\S+|https://\\S+){1}(\\s+)")
	, userRx	("^\\nBlog: http://.*")
	, delMsgRx	("^\\nMessage #\\d+ deleted.\\n$")
	, delReplyRx	("^\\nReply #\\d+/\\d+ deleted.\\n$")
	, idRx		("(#\\d+)(/\\d+){0,1}(\\S+){0,1}")
	, nickRx	("(@[\\w\\-\\.@\\|]*)(\\b.*)")
	, idAsResource(false), showPhoto(false), showAvatars(true), workInGroupChat(false)

{
	pmRx.setMinimal(true);
	regx.setMinimal(true);
	jidList_ = QStringList() << juick << jubo;
	updateTimer_.setSingleShot(true);
	updateTimer_.setInterval(1000);
	connect(&updateTimer_, SIGNAL(timeout()), SLOT(updateWidgets()));
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
	ui_.setupUi(optionsWid);

	QSignalMapper *sm = new QSignalMapper(optionsWid);
	QList<QToolButton*> list = QList<QToolButton*>() << ui_.tb_link
				<< ui_.tb_message << ui_.tb_name << ui_.tb_quote << ui_.tb_tag;
	foreach(QToolButton* b, list) {
		sm->setMapping(b, b);
		connect(b, SIGNAL(clicked()), sm, SLOT(map()));
	}

	restoreOptions();

	connect(sm, SIGNAL(mapped(QWidget*)), SLOT(chooseColor(QWidget*)));
	connect(ui_.pb_clearCache, SIGNAL(released()), SLOT(clearCache()));
	connect(ui_.pb_editJids, SIGNAL(released()), SLOT(requestJidList()));

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
	setStyles();

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
	JuickParser::reset();
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

	userColor = ui_.tb_name->property("psi_color").value<QColor>();
	tagColor = ui_.tb_tag->property("psi_color").value<QColor>();
	msgColor = ui_.tb_message->property("psi_color").value<QColor>();
	quoteColor = ui_.tb_quote->property("psi_color").value<QColor>();
	lineColor = ui_.tb_link->property("psi_color").value<QColor>();
	psiOptions->setPluginOption(constuserColor, userColor);
	psiOptions->setPluginOption(consttagColor, tagColor);
	psiOptions->setPluginOption(constmsgColor, msgColor);
	psiOptions->setPluginOption(constQcolor, quoteColor);
	psiOptions->setPluginOption(constLcolor, lineColor);

	//bold
	userBold = ui_.cb_nameBold->isChecked();
	tagBold = ui_.cb_tagBold->isChecked();
	msgBold = ui_.cb_messageBold->isChecked();
	quoteBold = ui_.cb_quoteBold->isChecked();
	lineBold = ui_.cb_linkBold->isChecked();
	psiOptions->setPluginOption(constUbold, userBold);
	psiOptions->setPluginOption(constTbold, tagBold);
	psiOptions->setPluginOption(constMbold, msgBold);
	psiOptions->setPluginOption(constQbold, quoteBold);
	psiOptions->setPluginOption(constLbold, lineBold);

	//italic
	userItalic = ui_.cb_nameItalic->isChecked();
	tagItalic = ui_.cb_tagItalic->isChecked();
	msgItalic = ui_.cb_messageItalic->isChecked();
	quoteItalic = ui_.cb_quoteItalic->isChecked();
	lineItalic = ui_.cb_linkItalic->isChecked();
	psiOptions->setPluginOption(constUitalic, userItalic);
	psiOptions->setPluginOption(constTitalic, tagItalic);
	psiOptions->setPluginOption(constMitalic, msgItalic);
	psiOptions->setPluginOption(constQitalic, quoteItalic);
	psiOptions->setPluginOption(constLitalic, lineItalic);

	//underline
	userUnderline = ui_.cb_nameUnderline->isChecked();
	tagUnderline = ui_.cb_tagUnderline->isChecked();
	msgUnderline = ui_.cb_messageUnderline->isChecked();
	quoteUnderline = ui_.cb_quoteUnderline->isChecked();
	lineUnderline = ui_.cb_linkUnderline->isChecked();
	psiOptions->setPluginOption(constUunderline, userUnderline);
	psiOptions->setPluginOption(constTunderline, tagUnderline);
	psiOptions->setPluginOption(constMunderline, msgUnderline);
	psiOptions->setPluginOption(constQunderline, quoteUnderline);
	psiOptions->setPluginOption(constLunderline, lineUnderline);

	//asResource
	idAsResource = ui_.cb_idAsResource->isChecked();
	psiOptions->setPluginOption(constIdAsResource, idAsResource);
	showPhoto = ui_.cb_showPhoto->isChecked();
	psiOptions->setPluginOption(constShowPhoto, showPhoto);
	showAvatars = ui_.cb_showAvatar->isChecked();
	if (showAvatars)
		createAvatarsDir();
	psiOptions->setPluginOption(constShowAvatars, showAvatars);
	workInGroupChat = ui_.cb_conference->isChecked();
	psiOptions->setPluginOption(constWorkInGroupchat, workInGroupChat);
	psiOptions->setPluginOption("constJidList",QVariant(jidList_));

	setStyles();
}

void JuickPlugin::restoreOptions()
{
	if (!optionsWid)
		return;

	ui_.tb_name->setStyleSheet(QString("background-color: %1;").arg(userColor.name()));
	ui_.tb_tag->setStyleSheet(QString("background-color: %1;").arg(tagColor.name()));
	ui_.tb_message->setStyleSheet(QString("background-color: %1;").arg(msgColor.name()));
	ui_.tb_quote->setStyleSheet(QString("background-color: %1;").arg(quoteColor.name()));
	ui_.tb_link->setStyleSheet(QString("background-color: %1;").arg(lineColor.name()));
	ui_.tb_name->setProperty("psi_color",userColor);
	ui_.tb_tag->setProperty("psi_color",tagColor);
	ui_.tb_message->setProperty("psi_color",msgColor);
	ui_.tb_quote->setProperty("psi_color",quoteColor);
	ui_.tb_link->setProperty("psi_color",lineColor);

	//bold
	ui_.cb_nameBold->setChecked(userBold);
	ui_.cb_tagBold->setChecked(tagBold);
	ui_.cb_messageBold->setChecked(msgBold);
	ui_.cb_quoteBold->setChecked(quoteBold);
	ui_.cb_linkBold->setChecked(lineBold);

	//italic
	ui_.cb_nameItalic->setChecked(userItalic);
	ui_.cb_tagItalic->setChecked(tagItalic);
	ui_.cb_messageItalic->setChecked(msgItalic);
	ui_.cb_quoteItalic->setChecked(quoteItalic);
	ui_.cb_linkItalic->setChecked(lineItalic);

	//underline
	ui_.cb_nameUnderline->setChecked(userUnderline);
	ui_.cb_tagUnderline->setChecked(tagUnderline);
	ui_.cb_messageUnderline->setChecked(msgUnderline);
	ui_.cb_quoteUnderline->setChecked(quoteUnderline);
	ui_.cb_linkUnderline->setChecked(lineUnderline);

	ui_.cb_idAsResource->setChecked(idAsResource);
	ui_.cb_showPhoto->setChecked(showPhoto);
	ui_.cb_showAvatar->setChecked(showAvatars);
	ui_.cb_conference->setChecked(workInGroupChat);
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
		ui_.cb_idAsResource->toggle();
		ui_.cb_idAsResource->toggle();
	}
}

void JuickPlugin::setStyles()
{
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
}

void JuickPlugin::chooseColor(QWidget* w)
{
	QToolButton *button = static_cast<QToolButton*>(w);
	QColor c(button->property("psi_color").value<QColor>());
	c = QColorDialog::getColor(c);
	if(c.isValid()) {
		button->setProperty("psi_color", c);
		button->setStyleSheet(QString("background-color: %1").arg(c.name()));
		//HACK
		ui_.cb_idAsResource->toggle();
		ui_.cb_idAsResource->toggle();

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

bool JuickPlugin::incomingStanza(int /*account*/, const QDomElement& stanza)
{
	if(!enabled)
		return false;

	if (stanza.tagName() == "message" ) {
		const QString jid(stanza.attribute("from").split('/').first());
		const QString usernameJ(jid.split("@").first());

		if (workInGroupChat && jid == "juick@conference.jabber.ru") {
			QString msg = stanza.firstChild().nextSibling().firstChild().nodeValue();
			msg.replace(QRegExp("#(\\d+)"),"http://juick.com/\\1");
			stanza.firstChild().nextSibling().firstChild().setNodeValue(msg);
		}

		if(jidList_.contains(jid) || usernameJ == "juick%juick.com" || usernameJ == "jubo%nologin.ru")
		{
//			qDebug() << "BEFORE";
//			debugElement(stanza);

			QDomDocument doc = stanza.ownerDocument();
			QDomElement nonConstStanza = const_cast<QDomElement&>(stanza);
			JuickParser jp(&nonConstStanza);

			QString resource("");
			QString res("");

			QString jidToSend(juick);
			if (usernameJ == "juick%juick.com") {
				jidToSend = jid;
			}
			if (usernameJ == "jubo%nologin.ru") {
				jidToSend = "juick%juick.com@"+jid.split("@").last();
			}

			userLinkPattern = chatPlusAction;
			altTextUser = userInfoString;
			if ( jid == jubo ) {
				messageLinkPattern = chatAction;
				altTextMsg = replyMsgString;
			} else {
				messageLinkPattern = "xmpp:%1%3?message;type=chat;body=%2+";
				altTextMsg = showAllmsgString;
			}

			if (showAvatars) {
				const QString ava = jp.avatarLink();
				if(!ava.isEmpty()) {
					const QString unick("@" + jp.nick());
					bool getAv = true;
					QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick");
					if(!dir.exists()) {
						getAv = false;
					}
					else {
						QStringList fileNames = dir.entryList(QStringList(QString(unick + ";*")));

						if (!fileNames.empty()) {
							QFile file(QString("%1/%2").arg(dir.absolutePath()).arg(fileNames.first()));
							if (QFileInfo(file).lastModified().daysTo(QDateTime::currentDateTime()) > avatarsUpdateInterval
									|| file.size() == 0) {
								file.remove();
							}
							else {
								getAv = false;
							}
						}
					}

					if(getAv)
						getAvatar(ava, unick);
				}
			}

			//добавляем перевод строки для обработки ссылок и номеров сообщений в конце сообщения
			QString msg = "\n" + jp.originMessage() + "\n";
			msg.replace("&gt;",">");
			msg.replace("&lt;","<");

			//Создаем xhtml-im элемент
			QDomElement element =  doc.createElement("html");
			element.setAttribute("xmlns","http://jabber.org/protocol/xhtml-im");
			QDomElement body = doc.createElement("body");
			body.setAttribute("xmlns","http://www.w3.org/1999/xhtml");

			//HELP
			if (msg.indexOf("\nNICK mynickname - Set a nickname\n\n") != -1) {
				nl2br(&body, &doc, msg);
				if (idAsResource) {
					QStringList tmp = activeTab->getJid().split('/');
					if (tmp.count() > 1 && jid == tmp.first()) {
						resource = tmp.last();
					}
				}
				msg =  "";
			}

			const QString photo = jp.photoLink();
			JuickMessages jm = jp.getMessages();
			const JuickParser::JMType type = jp.type();
			switch (type) {
			case JuickParser::JM_10_Messages:
			{
				body.appendChild(doc.createTextNode(jp.infoText()));
				foreach(const JuickMessage& m, jm) {
					body.appendChild(doc.createElement("br"));
					addUserLink(&body, &doc, "@" + m.unick, altTextUser, chatPlusAction, jidToSend);
					body.appendChild(doc.createTextNode(": "));
					//добавляем теги
					foreach (const QString& tag, m.tags){
						addTagLink(&body, &doc, tag, jidToSend);
					}
					body.appendChild(doc.createElement("br"));
					//обрабатываем текст сообщения
					QString newMsg = " " + m.body + " ";
					elementFromString(&body, &doc, newMsg, jidToSend);
					//xmpp ссылка на сообщение
					addMessageId(&body, &doc, m.messageId, replyMsgString, chatAction, jidToSend);
					body.appendChild(doc.createTextNode(" "));
					addPlus(&body, &doc, m.messageId,jidToSend);
					//ссылка на сообщение
					body.appendChild(doc.createTextNode(" "));
					addSubscribe(&body, &doc, m.messageId, jidToSend);
					body.appendChild(doc.createTextNode(" "));
					addFavorite(&body, &doc, m.messageId, jidToSend);
					body.appendChild(doc.createTextNode(" "+m.infoText));
					addHttpLink(&body, &doc, m.link);
					body.appendChild(doc.createElement("br"));
				}
				body.removeChild(body.lastChild());
				msg.clear();
				break;
			}
			case JuickParser::JM_Tags_Top:
			{
				body.appendChild(doc.createTextNode(jp.infoText()));
				body.appendChild(doc.createElement("br"));
				JuickMessage m = jm.first();
				foreach(const QString& tag, m.tags) {
					addTagLink(&body, &doc, tag, jidToSend);
					body.appendChild(doc.createElement("br"));
				}
				break;
			}
			case JuickParser::JM_Recomendation:
			case JuickParser::JM_Message:
			case JuickParser::JM_All_Messages:
			case JuickParser::JM_Post_View:
			case JuickParser::JM_Jubo:
			{
				JuickMessage m = jm.first();
				QString resLink("");
				if (idAsResource && type != JuickParser::JM_Post_View  && type != JuickParser::JM_Jubo) {
					resource = m.messageId;
					resLink = "/" + resource;
					resLink.replace("#","%23");
				}
				if(type == JuickParser::JM_Jubo) {
					body.appendChild(doc.createTextNode(jp.infoText()));
				}
				body.appendChild(doc.createElement("br"));
				if(!jp.timeStamp().isEmpty()) {
					body.appendChild(doc.createTextNode(tr("Time stamp: %1").arg(jp.timeStamp())));
					body.appendChild(doc.createElement("br"));
				}

				if(type == JuickParser::JM_Recomendation) {
					QStringList tmp = jp.infoText().split("@");
					body.appendChild(doc.createTextNode(tmp.first()));
					addUserLink(&body, &doc, "@" + tmp.last(), altTextUser, chatPlusAction, jidToSend);
					body.appendChild(doc.createTextNode(":"));
					body.appendChild(doc.createElement("br"));
				}

				addUserLink(&body, &doc, "@" + m.unick, altTextUser, chatPlusAction, jidToSend);
				body.appendChild(doc.createTextNode(": "));
				//добавляем теги
				foreach (const QString& tag, m.tags) {
					addTagLink(&body, &doc, tag, jidToSend);
				}
				msg = " " + m.body + " ";

				if (showAvatars) {
					addAvatar(&body, &doc, msg, jidToSend, m.unick);
				} else {
					body.appendChild(doc.createElement("br"));
					//обрабатываем текст сообщения
					elementFromString(&body, &doc, msg, jidToSend);
				}

				if (type == JuickParser::JM_Post_View && m.infoText.isEmpty()) {
					messageLinkPattern = chatAction;
					altTextMsg = replyMsgString;
				}
				//xmpp ссылка на сообщение
				addMessageId(&body, &doc, m.messageId, replyMsgString, chatAction, jidToSend, resLink);
				//ссылка на сообщение
				body.appendChild(doc.createTextNode(" "));
				if(type != JuickParser::JM_All_Messages && type != JuickParser::JM_Post_View) {
					addPlus(&body, &doc, m.messageId, jidToSend, resLink);
					body.appendChild(doc.createTextNode(" "));
				}
				addSubscribe(&body, &doc, m.messageId, jidToSend, resLink);
				body.appendChild(doc.createTextNode(" "));
				addFavorite(&body, &doc, m.messageId, jidToSend, resLink);
				body.appendChild(doc.createTextNode(" "));
				if(!m.infoText.isEmpty() && type != JuickParser::JM_All_Messages) {
					body.appendChild(doc.createTextNode(m.infoText));
					body.appendChild(doc.createTextNode(" "));
				}
				addHttpLink(&body, &doc, m.link);
				if(type == JuickParser::JM_All_Messages)
					msg = m.infoText;
				else
					msg.clear();
				break;
			}
			case JuickParser::JM_Reply:
			{
				JuickMessage m = jm.first();
				QString resLink("");
				QString replyId(m.messageId);
				if (idAsResource) {
					resource = replyId.left(replyId.indexOf("/"));
					resLink = "/" + resource;
					resLink.replace("#","%23");
				}
				body.appendChild(doc.createElement("br"));
				addUserLink(&body, &doc, "@" + m.unick, altTextUser ,chatPlusAction, jidToSend);
				body.appendChild(doc.createTextNode(tr(" replied:")));
				//цитата
				QDomElement blockquote = doc.createElement("blockquote");
				blockquote.setAttribute("style",quoteStyle);
				blockquote.appendChild(doc.createTextNode(jp.infoText()));
				//обрабатываем текст сообщения
				msg = " " + m.body + " ";
				body.appendChild(blockquote);
				if (showAvatars) {
					addAvatar(&body, &doc, msg, jidToSend, m.unick);
					//td2.appendChild(blockquote);
				} else {
					//body.appendChild(blockquote);
					elementFromString(&body, &doc,msg,jidToSend);
				}
				//xmpp ссылка на сообщение
				addMessageId(&body, &doc,m.messageId, replyMsgString, chatAction,jidToSend,resLink);
				//ссылка на сообщение
				body.appendChild(doc.createTextNode(" "));
				QString msgId = m.messageId.split("/").first();
				addUnsubscribe(&body, &doc, m.messageId,jidToSend, resLink);
				body.appendChild(doc.createTextNode(" "));
				addPlus(&body, &doc, msgId, jidToSend, resLink);
				body.appendChild(doc.createTextNode(" "));
				addHttpLink(&body, &doc, m.link);
				msg.clear();
				break;
			}
			case JuickParser::JM_Reply_Posted:
			case JuickParser::JM_Message_Posted:
			{
				JuickMessage m = jm.first();
				QString resLink("");
				if (idAsResource) {
					if(type == JuickParser::JM_Reply_Posted) {
						resource = m.messageId.split("/").first();
						resLink = "/" + resource;
					}
					else {
						QStringList tmp = activeTab->getJid().split('/');
						if (tmp.count() > 1 && jid == tmp.first()) {
							resource = tmp.last();
							resLink = "/" + resource;
						}
						else {
							resLink = "/" + m.messageId.split("/").first();
						}
					}
					resLink.replace("#","%23");
				}
				body.appendChild(doc.createElement("br"));
				body.appendChild(doc.createTextNode(jp.infoText()));
				body.appendChild(doc.createElement("br"));
				//xmpp ссылка на сообщение
				addMessageId(&body, &doc, m.messageId, replyMsgString, chatAction,jidToSend,resLink);
				body.appendChild(doc.createTextNode(" "));
				if(type == JuickParser::JM_Message_Posted) {
					addPlus(&body, &doc, m.messageId, jidToSend, resLink);
					body.appendChild(doc.createTextNode(" "));
				}
				addDelete(&body, &doc, m.messageId, jidToSend, resLink);
				body.appendChild(doc.createTextNode(" "));
				//ссылка на сообщение
				body.appendChild(doc.createTextNode(" "));
				addHttpLink(&body, &doc, m.link);
				msg.clear();
				break;
			}
			default:
			{
				if (msg.indexOf("Recommended blogs:") != -1) {
					//если команда @
					userLinkPattern = "xmpp:%1?message;type=chat;body=S %2";
					altTextUser = tr("Subscribe to %1's blog");
				}
				else if (pmRx.indexIn(msg) != -1) {
					//Если PM
					userLinkPattern = "xmpp:%1?message;type=chat;body=PM %2";
					altTextUser = tr("Send personal message to %1");
				}
				else if (userRx.indexIn(msg) != -1) {
					//Если информация о пользователе
					userLinkPattern = "xmpp:%1?message;type=chat;body=S %2";
					altTextUser = tr("Subscribe to %1's blog");
				}
				else if (msg == "\nPONG\n"
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
				break;
			}
			}

			if (idAsResource && resource.isEmpty() && (jid != jubo || usernameJ != "jubo%nologin.ru")) {
				QStringList tmp = activeTab->getJid().split('/');
				if (tmp.count() > 1 && jid == tmp.first()){
					resource = tmp.last();
				}
			}

			if (!photo.isEmpty()) {
				if(showPhoto) {
					//photo post
					QUrl photoUrl(photo);
					getPhoto(photoUrl);
					QDomElement link = doc.createElement("a");
					link.setAttribute("href", photo);
					QDomElement img = doc.createElement("img");
					QString imgdata = photoUrl.path().replace("/", "%");
					QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick/photos");
					img.setAttribute("src", QString(QUrl::fromLocalFile(QString("%1/%2").arg(dir.absolutePath()).arg(imgdata)).toEncoded()));
					link.appendChild(img);
					link.appendChild(doc.createElement("br"));
					body.insertAfter(link, body.lastChildElement("table"));
				}
				//удаление вложения, пока шлётся ссылка в сообщении
				nonConstStanza.removeChild(jp.findElement("x", "jabber:x:oob"));
			}

			//обработка по умолчанию
			elementFromString(&body, &doc, msg, jidToSend, res);
			element.appendChild(body);
			nonConstStanza.appendChild(element);
			if (!resource.isEmpty()) {
				QString from = stanza.attribute("from");
				from.replace(QRegExp("(.*)/.*"),"\\1/"+resource);
				nonConstStanza.setAttribute("from",from);
			}

//			qDebug() << "AFTER";
//			debugElement(stanza);
		}
	}

	return false;
}

Http* JuickPlugin::newHttp(const QString& path)
{
	Http *http = new Http(this);
	Proxy prx = applicationInfo->getProxyFor(constPluginName);
	http->setProxyHostPort(prx.host, prx.port, prx.user, prx.pass, prx.type);
	http->setProperty("path", path);
	connect(http, SIGNAL(dataReady(QByteArray)), SLOT(photoReady(QByteArray)));
	return http;
}

void JuickPlugin::getAvatar(const QString &link, const QString& unick)
{
	QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick");
	const QString path(QString("%1/%2;")
		     .arg(dir.absolutePath())
		     .arg(unick));

	Http *http = newHttp(path);
	http->setHost("i.juick.com");
	http->get(link);
//	if(img.isEmpty())
//		img = http->get("/a/"+uid+".png");
}

void JuickPlugin::getPhoto(const QUrl &url)
{
	QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick/photos");
	const QString path(QString("%1/%2")
		     .arg(dir.absolutePath())
		     .arg(url.path().replace("/", "%")));

	Http *http = newHttp(path);
	http->setHost(url.host());	
	http->get(QString(url.path()).replace("/photos-1024/","/ps/"));
}

void JuickPlugin::photoReady(const QByteArray &ba)
{
	Http* http = static_cast<Http*>(sender());
	http->deleteLater();

	if(ba.isEmpty())
		return;

	const QString path = http->property("path").toString();
	save(path, ba);

	urls_.append(QUrl::fromLocalFile(path).toEncoded());
	updateTimer_.start();
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
				messageLinkPattern = chatAction;
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

void JuickPlugin::addAvatar(QDomElement* body, QDomDocument* doc, const QString& msg, const QString& jidToSend, const QString& ujid)
{
	QDomElement table = doc->createElement("table");
	QDomElement tableRow = doc->createElement("tr");
	QDomElement td1 = doc->createElement("td");
	td1.setAttribute("valign","top");
	QDomElement td2 = doc->createElement("td");
	QDir dir(applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation)+"/avatars/juick");
	if (dir.exists()) {
		QDomElement img = doc->createElement("img");
		img.setAttribute("src", QString(QUrl::fromLocalFile(QString("%1/@%2;").arg(dir.absolutePath()).arg(ujid)).toEncoded()));
		td1.appendChild(img);
	}
//	td2.appendChild(blockquote);
	elementFromString(&td2, doc, msg, jidToSend);
	tableRow.appendChild(td1);
	tableRow.appendChild(td2);
	table.appendChild(tableRow);
	body->appendChild(table);
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
	const QString jid = contact.split("/").first();
	const QString usernameJ = jid.split("@").first();
	if(jidList_.contains(jid) || usernameJ == "juick%juick.com"|| usernameJ == "jubo%nologin.ru") {
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

// Этот слот обновляет чатлоги, чтобы они перезагрузили
// картинки с диска.
void JuickPlugin::updateWidgets()
{
	foreach(QWidget *w, logs_) {
		QTextEdit* te = qobject_cast<QTextEdit*>(w);
		if(te) {
			QTextDocument* td = te->document();
			foreach(const QByteArray& url, urls_) {
				QUrl u(url);
				td->addResource(QTextDocument::ImageResource, u, QPixmap(u.toLocalFile()));
			}
			td->adjustSize();
		}
		else {
			QWebView *wv = w->findChild<QWebView*>();
			if(wv) {
				int t = QTime::currentTime().msec();
				QWebFrame* wf = wv->page()->mainFrame();
				foreach(const QByteArray& url, urls_) {
					QUrl u(url);
					QWebElement elem = wf->findFirstElement(QString("img[src=\"%1\"]").arg(u.toString()));
					if(!elem.isNull()) {
						elem.setAttribute("src", u.toString() + "?" + QString::number(qrand()%t));
					}
				}
			}
		}
	}
	urls_.clear();
}

QString JuickPlugin::pluginInfo()
{
	return tr("Authors: ") + "VampiRUS, Dealer_WeARE\n\n"
	+ trUtf8("This plugin is designed to work efficiently and comfortably with the Juick microblogging service.\n"
			 "Currently, the plugin is able to: \n"
			 "* Coloring @nick, *tag and #message_id in messages from the juick@juick.com bot\n"
			 "* Detect >quotes in messages\n"
			 "* Enable clickable @nick, *tag, #message_id and other control elements to insert them into the typing area\n\n"
			 "Note: To work correctly, the option options.html.chat.render	must be set to true. ");
}

#include "juickplugin.moc"
