/*
 * redirectplugin.h - plugin
 * Copyright (C) 2013  Sergey Ilinykh
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef REDIRECTORPLUGIN_H
#define REDIRECTORPLUGIN_H

#include "accountinfoaccessor.h"
#include "applicationinfoaccessor.h"
#include "contactinfoaccessor.h"
#include "optionaccessor.h"
#include "plugininfoprovider.h"
#include "psiplugin.h"
#include "stanzafilter.h"
#include "stanzasender.h"
#include <QPixmap>

class QDomElement;

class OptionAccessingHost;
class StanzaSendingHost;
class AccountInfoAccessingHost;
class ApplicationInfoAccessingHost;
class ContactInfoAccessingHost;

class Redirector : public QObject,
                   public PsiPlugin,
                   public OptionAccessor,
                   public StanzaSender,
                   public StanzaFilter,
                   public AccountInfoAccessor,
                   public ApplicationInfoAccessor,
                   public PluginInfoProvider,
                   public ContactInfoAccessor {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.psi-plus.Redirector"  FILE "psiplugin.json" )
    Q_INTERFACES(PsiPlugin OptionAccessor StanzaSender StanzaFilter AccountInfoAccessor ApplicationInfoAccessor
                     PluginInfoProvider ContactInfoAccessor)

public:
    Redirector() = default;
    QString name() const { return "Redirector Plugin"; }
    QString shortName() const { return "redirector"; }
    QString version() const { return "0.0.2"; }
    // PsiPlugin::Priority priority() {return PriorityNormal;}
    QWidget *options();
    bool     enable();
    bool     disable();
    void     applyOptions();
    void     restoreOptions();
    QPixmap  icon() const { return QPixmap(); }
    void     setOptionAccessingHost(OptionAccessingHost *host) { psiOptions = host; }
    void     optionChanged(const QString &) { }
    void     setStanzaSendingHost(StanzaSendingHost *host) { stanzaHost = host; }
    bool     incomingStanza(int account, const QDomElement &xml);
    bool     outgoingStanza(int account, QDomElement &xml);
    void     setAccountInfoAccessingHost(AccountInfoAccessingHost *host) { accInfoHost = host; }
    void     setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host) { appInfoHost = host; }
    void     setContactInfoAccessingHost(ContactInfoAccessingHost *host) { contactInfo = host; }
    QString  pluginInfo();

private slots:

private:
    QString             targetJid;
    QHash<QString, int> contactIdMap;
    int                 nextContactId = 0;
    QWidget            *options_      = nullptr;

    bool                          enabled     = false;
    OptionAccessingHost          *psiOptions  = nullptr;
    StanzaSendingHost            *stanzaHost  = nullptr;
    AccountInfoAccessingHost     *accInfoHost = nullptr;
    ApplicationInfoAccessingHost *appInfoHost = nullptr;
    ContactInfoAccessingHost     *contactInfo = nullptr;

    Ui::Options ui_;
};

#endif
