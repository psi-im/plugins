/*
 * messagefilter.h - plugin main class
 *
 * Copyright (C) 2015  Ivan Romanov <drizt@land.ru>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef MESSAGEFILTER_H
#define MESSAGEFILTER_H

#include "options.h"

#include <psiplugin.h>
#include <applicationinfoaccessinghost.h>
#include <plugininfoprovider.h>
#include <stanzafilter.h>
#include <stanzasender.h>
#include <psiaccountcontroller.h>
#include <optionaccessor.h>
#include <toolbariconaccessor.h>
#include <iconfactoryaccessor.h>
#include <activetabaccessor.h>
#include <accountinfoaccessor.h>

class Options;
class QMenu;

class MessageFilter : public QObject
					, public PsiPlugin
					, public PluginInfoProvider
					, public StanzaFilter
					, public PsiAccountController
					, public OptionAccessor
					, public StanzaSender
					, public AccountInfoAccessor
{
	Q_OBJECT
#ifdef HAVE_QT5
	Q_PLUGIN_METADATA(IID "com.psi-plus.MessageFilter")
#endif
	Q_INTERFACES(PsiPlugin
				 PluginInfoProvider
				 StanzaFilter
				 PsiAccountController
				 OptionAccessor
				 StanzaSender
				 AccountInfoAccessor)

public:
	MessageFilter();
	~MessageFilter();

	// from PsiPlugin
	QString name() const { return "Message Filter Plugin"; }
	QString shortName() const { return "messagefilter"; }
	QString version() const { return "0.0.2"; }

	QWidget *options();
	bool enable();
	bool disable();
	void applyOptions();
	void restoreOptions();
	QPixmap icon() const;

	// from PluginInfoProvider
	QString pluginInfo();

	// from StanzaSender
	void setStanzaSendingHost(StanzaSendingHost *host) { _stanzaSending = host; }

	// from StanzaFilter
	bool incomingStanza(int account, const QDomElement &stanza);
	bool outgoingStanza(int /*account*/, QDomElement &/*stanza*/) { return false; }

	// from PsiAccountController
	void setPsiAccountControllingHost(PsiAccountControllingHost *host) { _accountHost = host; }

	// from OptionAccessor
	void setOptionAccessingHost(OptionAccessingHost *host) { _optionHost = host; }
	void optionChanged(const QString &/*option*/) { }

	// from AccountInfoAccessor
	void setAccountInfoAccessingHost(AccountInfoAccessingHost* host) { _accountInfo = host; }

private:
	void loadRules();
	bool _enabled;
	Options *_optionsForm;
	PsiAccountControllingHost *_accountHost;
	OptionAccessingHost *_optionHost;
	StanzaSendingHost *_stanzaSending;
	AccountInfoAccessingHost *_accountInfo;
	QList<Rule> _rules;
};

#endif // MESSAGEFILTER_H
