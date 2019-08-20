/*
 * rippercc.cpp - plugin
 * Copyright (C) 2016
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef RIPPERCC_H
#define RIPPERCC_H

#include "ripperccoptions.h"

#include <QList>
#include <QNetworkAccessManager>
#include <QStringList>
#include <QTimer>
#include <accountinfoaccessor.h>
#include <applicationinfoaccessinghost.h>
#include <applicationinfoaccessor.h>
#include <contactinfoaccessinghost.h>
#include <contactinfoaccessor.h>
#include <optionaccessor.h>
#include <plugininfoprovider.h>
#include <psiaccountcontroller.h>
#include <psiplugin.h>
#include <stanzafilter.h>
#include <stanzasender.h>

class RipperCC:
        public AccountInfoAccessor,
        public ApplicationInfoAccessor,
        public ContactInfoAccessor,
        public OptionAccessor,
        public PluginInfoProvider,
        public PsiAccountController,
        public PsiPlugin,
        public QObject,
        public StanzaFilter,
        public StanzaSender {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.psi-plus.RipperCC")
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
    QString version() const { return "0.0.3"; }

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
    void updateNameGroup(int account, const QString &jid, const QString &name, const QString &group);

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
