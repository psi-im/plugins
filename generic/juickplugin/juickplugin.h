/*
 * juickplugin.h - plugin
 * Copyright (C) 2009-2012  Kravtsov Nikolai, Evgeny Khryukin
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

#ifndef JUICKPLUGIN_H
#define JUICKPLUGIN_H

#include "psiplugin.h"
#include "stanzafilter.h"
#include "optionaccessor.h"
#include "activetabaccessor.h"
#include "plugininfoprovider.h"
#include "chattabaccessor.h"
#include "applicationinfoaccessor.h"
#include "webkitaccessor.h"
#include "ui_settings.h"

class OptionAccessingHost;
class ActiveTabAccessingHost;
class ApplicationInfoAccessingHost;
class JuickDownloader;
class QDomDocument;

class JuickPlugin : public QObject,
        public PsiPlugin,
        public OptionAccessor,
        public ActiveTabAccessor,
		public StanzaFilter,
        public ApplicationInfoAccessor,
        public PluginInfoProvider,
        public ChatTabAccessor,
        public WebkitAccessor
{
	Q_OBJECT
#ifdef HAVE_QT5
	Q_PLUGIN_METADATA(IID "com.psi-plus.JuickPlugin")
#endif
	Q_INTERFACES(PsiPlugin OptionAccessor ActiveTabAccessor StanzaFilter
			ApplicationInfoAccessor PluginInfoProvider ChatTabAccessor WebkitAccessor)

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
	virtual QPixmap icon() const;
	virtual void setOptionAccessingHost(OptionAccessingHost* host);
	virtual void optionChanged(const QString& ) {}
	virtual void setActiveTabAccessingHost(ActiveTabAccessingHost* host);
	virtual void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host);
    virtual void setWebkitAccessingHost(WebkitAccessingHost* host);
	virtual QString pluginInfo();

	// ChatTabAccessor
	void setupChatTab(QWidget* tab, int account, const QString& contact);
	void setupGCTab(QWidget* /*tab*/, int /*account*/, const QString& /*contact*/) { /* do nothing*/ }
	bool appendingChatMessage(int, const QString&, QString&, QDomElement&, bool) { return false; }

	virtual bool incomingStanza(int account, const QDomElement& stanza);
	virtual bool outgoingStanza(int , QDomElement& ) { return false; }

private slots:
	void chooseColor(QWidget *);
	void clearCache();
	void updateJidList(const QStringList& jids);
	void requestJidList();
	void removeWidget();
	void updateWidgets(const QList<QByteArray> &urls);

private:
	void createAvatarsDir();
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
	void addAvatar(QDomElement *body, QDomDocument *doc, const QString &msg, const QString &jidToSend, const QString &ujid, const QString &avatarUrl);

private:
	bool enabled = false;
	OptionAccessingHost* psiOptions = nullptr;
	ActiveTabAccessingHost* activeTab = nullptr;
	ApplicationInfoAccessingHost* applicationInfo = nullptr;
    WebkitAccessingHost *webkit = nullptr;
	QColor userColor = false, tagColor = false, msgColor = false, quoteColor = false, lineColor = false;
	bool userBold = true,tagBold = false,msgBold = false,quoteBold = false,lineBold = false;
	bool userItalic = false,tagItalic = true,msgItalic = false,quoteItalic = false,lineItalic = false;
	bool userUnderline = false,tagUnderline = false,msgUnderline = true,quoteUnderline = false,lineUnderline = true;
	QString idStyle,userStyle,tagStyle,quoteStyle,linkStyle;
	QRegExp tagRx, regx, idRx, nickRx, linkRx;
	QString userLinkPattern,messageLinkPattern,altTextUser,altTextMsg,commonLinkColor;
	bool idAsResource = false,showPhoto = false,showAvatars = true,workInGroupChat = false;
	QStringList jidList_;
	QPointer<QWidget> optionsWid;
	QList<QWidget*> logs_;
	Ui::settings ui_;
	JuickDownloader* downloader_ = nullptr;
};

#endif // JUICKPLUGIN_H
