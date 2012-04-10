/*
 * jabberdiskplugin.h - plugin
 * Copyright (C) 2011  Khryukin Evgeny
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

#ifndef JABBERDISKPLUGIN_H
#define JABBERDISKPLUGIN_H

class QAction;
class QDomElement;

#include "psiplugin.h"
#include "iconfactoryaccessor.h"
#include "iconfactoryaccessinghost.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"
#include "accountinfoaccessinghost.h"
#include "accountinfoaccessor.h"
#include "stanzafilter.h"
//#include "popupaccessor.h"
//#include "popupaccessinghost.h"
#include "menuaccessor.h"
#include "plugininfoprovider.h"
#include "optionaccessinghost.h"
#include "optionaccessor.h"

#include "ui_options.h"

#define constVersion "0.0.3"

class JabberDiskPlugin : public QObject, public PsiPlugin, public StanzaSender, public IconFactoryAccessor,
	public PluginInfoProvider, public StanzaFilter, public MenuAccessor, public AccountInfoAccessor, public OptionAccessor
	/*, public PopupAccessor,*/
{
        Q_OBJECT
	Q_INTERFACES(PsiPlugin StanzaFilter StanzaSender IconFactoryAccessor AccountInfoAccessor//  PopupAccessor
		     MenuAccessor PluginInfoProvider OptionAccessor)
public:
	JabberDiskPlugin();
        virtual QString name() const;
        virtual QString shortName() const;
        virtual QString version() const;
        virtual QWidget* options();
        virtual bool enable();
        virtual bool disable();

	virtual Priority priority() { return PsiPlugin::PriorityHigh; }

	virtual void applyOptions();
	virtual void restoreOptions();
	virtual void setOptionAccessingHost(OptionAccessingHost* host);
	virtual void optionChanged(const QString& ) {};
	virtual void setIconFactoryAccessingHost(IconFactoryAccessingHost* host);
	virtual void setStanzaSendingHost(StanzaSendingHost *host);
	virtual void setAccountInfoAccessingHost(AccountInfoAccessingHost* host);
	virtual bool incomingStanza(int account, const QDomElement& xml);
	virtual bool outgoingStanza(int account, QDomElement& xml);
	//virtual void setPopupAccessingHost(PopupAccessingHost* host);
	virtual QList < QVariantHash > getAccountMenuParam();
	virtual QList < QVariantHash > getContactMenuParam();
	virtual QAction* getContactAction(QObject* , int , const QString& );
	virtual QAction* getAccountAction(QObject* , int ) { return 0; };
	virtual QString pluginInfo();


//signals:

private slots:
	void addJid();
	void removeJid();

private:
	void hack();


private:
	bool enabled;
	QPointer<QWidget> options_;
	Ui::Options ui_;
	OptionAccessingHost* psiOptions;
	QStringList jids_;
	IconFactoryAccessingHost* iconHost;
//
//        PopupAccessingHost* popup;
};

#endif


