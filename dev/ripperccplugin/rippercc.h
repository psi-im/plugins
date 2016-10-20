/*
 * rippercc.cpp
 *
 * Copyright (C) 2016
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

#ifndef RIPPERCC_H
#define RIPPERCC_H

#include "ripperccoptions.h"

#include <psiplugin.h>
#include <applicationinfoaccessinghost.h>
#include <plugininfoprovider.h>
#include <stanzafilter.h>
#include <stanzasender.h>
#include <psiaccountcontroller.h>
#include <optionaccessor.h>
#include <accountinfoaccessor.h>
#include <applicationinfoaccessor.h>
#include <applicationinfoaccessinghost.h>
#include <contactinfoaccessinghost.h>
#include <contactinfoaccessor.h>

#include <QStringList>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QList>

class RipperCC : public QObject
			   , public PsiPlugin
			   , public PluginInfoProvider
			   , public StanzaFilter
			   , public PsiAccountController
			   , public OptionAccessor
			   , public StanzaSender
			   , public AccountInfoAccessor
			   , public ApplicationInfoAccessor
			   , public ContactInfoAccessor
{
	Q_OBJECT
#ifdef HAVE_QT5
	Q_PLUGIN_METADATA(IID "com.psi-plus.RipperCC")
#endif
	Q_INTERFACES(PsiPlugin
				 PluginInfoProvider
				 StanzaFilter
				 PsiAccountController
				 OptionAccessor
				 StanzaSender
				 AccountInfoAccessor
				 ApplicationInfoAccessor
				 ContactInfoAccessor)
public:
	RipperCC();
	~RipperCC();

	// from PsiPlugin
	QString name() const { return "RipperCC"; }
	QString shortName() const { return "rippercc"; }
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
	bool outgoingStanza(int account, QDomElement &stanza);

	// from PsiAccountController
	void setPsiAccountControllingHost(PsiAccountControllingHost *host) { _accountHost = host; }

	// from OptionAccessor
	void setOptionAccessingHost(OptionAccessingHost *host) { _optionHost = host; }
	void optionChanged(const QString &/*option*/) { }

	// from AccountInfoAccessor
	void setAccountInfoAccessingHost(AccountInfoAccessingHost* host) { _accountInfo = host; }

	// from ApplicationInfoAccessor
	void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host) { _appInfo = host; }

	// from ContactInfoAccessor
	void setContactInfoAccessingHost(ContactInfoAccessingHost *host) { _contactInfo = host; }

	void handleStanza(int account, const QDomElement &stanza, bool incoming);

public slots:
	void updateRipperDb();
	void parseRipperDb();

private:
	bool _enabled;
	PsiAccountControllingHost *_accountHost;
	OptionAccessingHost *_optionHost;
	StanzaSendingHost *_stanzaSending;
	AccountInfoAccessingHost *_accountInfo;
	ApplicationInfoAccessingHost *_appInfo;
	ContactInfoAccessingHost *_contactInfo;
	QNetworkAccessManager *_nam;
	QTimer *_timer;
	RipperCCOptions *_optionsForm;

	struct Ripper {
		QString jid;
		QString url;
		QDateTime lastAttentionTime;
	};

	QList<Ripper> _rippers;
};

#endif // RIPPERCC_H
