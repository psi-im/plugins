/*
 * gomokugameplugin.h - Gomoku Game plugin
 * Copyright (C) 2011  Aleksey Andreev
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * You can also redistribute and/or modify this program under the
 * terms of the Psi License, specified in the accompanied COPYING
 * file, as published by the Psi Project; either dated January 1st,
 * 2005, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef GOMOKUGAMEPLUGIN_H
#define GOMOKUGAMEPLUGIN_H

#include <QtGui>
#include <QtCore>
#include <QObject>
#include <QDomElement>


#include "psiplugin.h"
#include "plugininfoprovider.h"
#include "optionaccessor.h"
#include "optionaccessinghost.h"
#include "iconfactoryaccessor.h"
#include "iconfactoryaccessinghost.h"
#include "toolbariconaccessor.h"
#include "activetabaccessor.h"
#include "activetabaccessinghost.h"
#include "accountinfoaccessor.h"
#include "accountinfoaccessinghost.h"
#include "contactinfoaccessor.h"
#include "contactinfoaccessinghost.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"
#include "stanzafilter.h"
#include "eventcreator.h"
#include "eventcreatinghost.h"
#include "soundaccessor.h"
#include "soundaccessinghost.h"
#include "menuaccessor.h"
#include "popupaccessor.h"
#include "popupaccessinghost.h"

#include "ui_options.h"


class GomokuGamePlugin : public QObject, public PsiPlugin, public PluginInfoProvider, public OptionAccessor,
public IconFactoryAccessor, public ToolbarIconAccessor, public ActiveTabAccessor, public AccountInfoAccessor,
public ContactInfoAccessor, public StanzaSender, public StanzaFilter, public EventCreator, public SoundAccessor,
public MenuAccessor, public PopupAccessor
{
	Q_OBJECT
	Q_INTERFACES(PsiPlugin PluginInfoProvider OptionAccessor IconFactoryAccessor ToolbarIconAccessor
		     ActiveTabAccessor AccountInfoAccessor ContactInfoAccessor StanzaSender StanzaFilter
		     EventCreator SoundAccessor MenuAccessor PopupAccessor)

public:
	explicit GomokuGamePlugin(QObject *parent = 0);
	// Psiplugin
	virtual QString name() const;
	virtual QString shortName() const;
	virtual QString version() const;
	virtual QWidget* options();
	virtual bool enable();
	virtual bool disable();
	virtual void applyOptions();
	virtual void restoreOptions();
	// Plugin info provider
	virtual QString pluginInfo();
	// Option accessor
	virtual void setOptionAccessingHost(OptionAccessingHost*);
	virtual void optionChanged(const QString&);
	// Iconfactory accessor
	virtual void setIconFactoryAccessingHost(IconFactoryAccessingHost*);
	// Toolbar icon accessor
	virtual QList<QVariantHash> getButtonParam();
	virtual QAction* getAction(QObject* , int , const QString& );
	// Activetab accessor
	virtual void setActiveTabAccessingHost(ActiveTabAccessingHost*);
	// Account info accessor
	virtual void setAccountInfoAccessingHost(AccountInfoAccessingHost*);
	// Contact info accessor
	virtual void setContactInfoAccessingHost(ContactInfoAccessingHost*);
	// Stanza sender
	virtual void setStanzaSendingHost(StanzaSendingHost*);
	// Stanza filter
	virtual bool incomingStanza(int account, const QDomElement& xml);
	virtual bool outgoingStanza(int account, QDomElement& xml);
	// Event creator
	virtual void setEventCreatingHost(EventCreatingHost*);
	// Sound accessor
	virtual void setSoundAccessingHost(SoundAccessingHost*);
	// Menu accessor
	virtual QList<QVariantHash> getAccountMenuParam();
	virtual QList<QVariantHash> getContactMenuParam();
	virtual QAction* getContactAction(QObject*, int, const QString&);
	virtual QAction* getAccountAction(QObject*, int);
	// Popup accessor
	virtual void setPopupAccessingHost(PopupAccessingHost*);

private:
	bool enabled_;
	ActiveTabAccessingHost *psiTab;
	IconFactoryAccessingHost *psiIcon;
	AccountInfoAccessingHost *psiAccInfo;
	ContactInfoAccessingHost *psiContactInfo;
	StanzaSendingHost *psiSender;
	EventCreatingHost *psiEvent;
	SoundAccessingHost *psiSound;
	PopupAccessingHost *psiPopup;
	// --
	Ui::options ui_;

private:
	void invite(int account, QString full_jid);

private slots:
	void toolButtonPressed();
	void menuActivated();
	void doPsiEvent(int, QString, QString, QObject *, const char *);
	void sendGameStanza(int account, const QString stanza);
	void testSound();
	void getSound();
	void doPopup(const QString text);
	void playSound(const QString);

};

#endif // GOMOKUGAMEPLUGIN_H
