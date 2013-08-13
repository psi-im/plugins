/*
 * gmailserviceplugin.h - plugin
 * Copyright (C) 2009-2011 Kravtsov Nikolai, Khryukin Evgeny
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

#ifndef GMAILSERVICEPLUGIN_H
#define GMAILSERVICEPLUGIN_H

class QDomElement;

#include "actionslist.h"

#include "psiplugin.h"
#include "stanzafilter.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"
#include "optionaccessor.h"
#include "optionaccessinghost.h"
#include "plugininfoprovider.h"
#include "accountinfoaccessinghost.h"
#include "accountinfoaccessor.h"
#include "popupaccessinghost.h"
#include "popupaccessor.h"
#include "psiaccountcontroller.h"
#include "psiaccountcontrollinghost.h"
#include "iconfactoryaccessinghost.h"
#include "iconfactoryaccessor.h"
#include "toolbariconaccessor.h"
#include "eventcreatinghost.h"
#include "eventcreator.h"
#include "soundaccessinghost.h"
#include "soundaccessor.h"
#include "menuaccessor.h"

#include "ui_options.h"
#include "accountsettings.h"
#include "viewmaildlg.h"

#define OPTION_LISTS "lists"
#define OPTION_INTERVAL "interval"
#define OPTION_SOUND "sound"
#define OPTION_PROG "program"

#define POPUP_OPTION "Gmail Service Plugin"

#define PLUGIN_VERSION "0.7.5"


class GmailNotifyPlugin : public QObject, public PsiPlugin, public AccountInfoAccessor,
	public StanzaFilter, public StanzaSender, public OptionAccessor, public PluginInfoProvider,
	public PopupAccessor, public PsiAccountController, public IconFactoryAccessor,
	public ToolbarIconAccessor, public EventCreator, public SoundAccessor, public MenuAccessor
{
	Q_OBJECT
	Q_INTERFACES(PsiPlugin StanzaFilter StanzaSender /*EventFilter*/ OptionAccessor PluginInfoProvider
		     AccountInfoAccessor PopupAccessor PsiAccountController IconFactoryAccessor
		     ToolbarIconAccessor EventCreator SoundAccessor MenuAccessor)
public:
	GmailNotifyPlugin();
	virtual QString name() const;
	virtual QString shortName() const;
	virtual QString version() const;
	virtual QWidget* options();
	virtual bool enable();
	virtual bool disable();
	virtual void setOptionAccessingHost(OptionAccessingHost* host);
	virtual void optionChanged(const QString& /*option*/){}
	virtual void applyOptions();
	virtual void restoreOptions();
	virtual bool incomingStanza(int account, const QDomElement& stanza);
	virtual bool outgoingStanza(int account, QDomElement& stanza);
	virtual void logout(int ) {}
	virtual void setStanzaSendingHost(StanzaSendingHost *host);
	virtual void setAccountInfoAccessingHost(AccountInfoAccessingHost* host);
	virtual void setPopupAccessingHost(PopupAccessingHost* host);
	virtual void setPsiAccountControllingHost(PsiAccountControllingHost* host);
	virtual void setIconFactoryAccessingHost(IconFactoryAccessingHost* host);
	virtual void setEventCreatingHost(EventCreatingHost* host);
	virtual void setSoundAccessingHost(SoundAccessingHost* host);
	virtual QList < QVariantHash > getButtonParam();
	virtual QAction* getAction(QObject* parent, int account, const QString& contact);
	virtual QList < QVariantHash > getAccountMenuParam() { return QList < QVariantHash > (); }
	virtual QList < QVariantHash > getContactMenuParam() { return QList < QVariantHash > (); }
	virtual QAction* getContactAction(QObject* parent, int account, const QString& contact);
	virtual QAction* getAccountAction(QObject* /*parent*/, int /*account*/) { return 0; }

	virtual QString pluginInfo();
	virtual QIcon icon() const;

private slots:
	void updateSharedStatus(AccountSettings* as);
	void changeNoSaveState(int account, QString jid, bool val);
	void updateOptions(int index);
	void stopOptionsApply();
	void mailEventActivated();
	void checkSound();
	void getSound();
	void blockActionTriggered(bool);
	void getProg();

private:
	AccountSettings* findAccountSettings(const QString& jid);
	AccountSettings* create(int account, QString jid);

	bool hasAccountSettings(int account);
	bool checkFeatures(int account, const QDomElement& stanza, const QDomElement& query);
	bool checkEmail(int account, const QDomElement& stanza, const QDomElement& query);
	bool checkSettings(int account, const QDomElement& stanza, const QDomElement& query);
	bool checkSharedStatus(int account, const QDomElement& stanza, const QDomElement& query);
	bool checkNoSave(int account, const QDomElement& stanza, const QDomElement& query);
	bool checkAttributes(int account, const QDomElement& stanza, const QDomElement& query);
	void saveLists();
	void loadLists();
	void showPopup(const QString& text);
	void updateActions(AccountSettings* as);
	void incomingMail(int account, const QDomElement& xml);
	void playSound(const QString& file);

private:
	bool enabled;
	bool optionsApplingInProgress_;
	StanzaSendingHost* stanzaSender;
	OptionAccessingHost* psiOptions;
	AccountInfoAccessingHost* accInfo;
	PopupAccessingHost* popup;
	PsiAccountControllingHost* accountController;
	IconFactoryAccessingHost* iconHost;
	EventCreatingHost* psiEvent;
	SoundAccessingHost* sound_;
	QString soundFile;
	ActionsList* actions_;
	QPointer<QWidget> options_;
	QPointer<ViewMailDlg> mailViewer_;
	QList<AccountSettings*> accounts;
	typedef QList<MailItem> MailItemsList;
	QList<MailItemsList> mailItems_;
	QStringList id_;
	int popupId;
	QString program_;
	Ui::Options ui_;
};

#endif
