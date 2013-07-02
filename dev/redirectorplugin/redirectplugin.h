/*
 * redirectplugin.h - plugin
 * Copyright (C) 2013  Il'inykh Sergey
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

#ifndef REDIRECTPLUGIN_H
#define REDIRECTPLUGIN_H

#include "psiplugin.h"
#include "optionaccessor.h"
#include "stanzafilter.h"
#include "stanzasender.h"
#include "accountinfoaccessor.h"
#include "applicationinfoaccessor.h"
#include "plugininfoprovider.h"
#include "contactinfoaccessor.h"

class QDomElement;

class OptionAccessingHost;
class StanzaSendingHost;
class AccountInfoAccessingHost;
class ApplicationInfoAccessingHost;
class ContactInfoAccessingHost;

#include "ui_options.h"

class Redirector: public QObject, public PsiPlugin, public OptionAccessor, public StanzaSender,  public StanzaFilter,
public AccountInfoAccessor, public ApplicationInfoAccessor,
public PluginInfoProvider, public ContactInfoAccessor
{
	Q_OBJECT
	Q_INTERFACES(PsiPlugin OptionAccessor StanzaSender StanzaFilter AccountInfoAccessor ApplicationInfoAccessor
				 PluginInfoProvider ContactInfoAccessor)

public:
	inline Redirector() : enabled(false)
	  , psiOptions(0)
	  , stanzaHost(0)
	  , accInfoHost(0)
	  , appInfoHost(0)
	  , contactInfo(0) {}
	QString name() const { return "Redirect Plugin"; }
	QString shortName() const { return "redirect"; }
	QString version() const { return "0.0.1"; }
	//PsiPlugin::Priority priority() {return PriorityNormal;}
	QWidget* options();
	bool enable();
	bool disable();
	void applyOptions();
	void restoreOptions();
	void setOptionAccessingHost(OptionAccessingHost* host) { psiOptions = host; }
	void optionChanged(const QString& ) {}
	void setStanzaSendingHost(StanzaSendingHost *host) { stanzaHost = host; }
	bool incomingStanza(int account, const QDomElement& xml);
	bool outgoingStanza(int account, QDomElement& xml);
	void setAccountInfoAccessingHost(AccountInfoAccessingHost* host) { accInfoHost = host; }
	void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host) { appInfoHost = host; }
	void setContactInfoAccessingHost(ContactInfoAccessingHost* host) { contactInfo = host; }
	QString pluginInfo();

private slots:

private:
	QString targetJid;
	QHash<QString, int> contactIdMap;
	int nextContactId;
	QWidget *options_;

	bool enabled;
	OptionAccessingHost* psiOptions;
	StanzaSendingHost* stanzaHost;
	AccountInfoAccessingHost *accInfoHost;
	ApplicationInfoAccessingHost *appInfoHost;
	ContactInfoAccessingHost* contactInfo;

	Ui::Options ui_;
};

#endif
