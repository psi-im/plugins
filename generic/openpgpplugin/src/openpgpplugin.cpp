/*
 * openpgpplugin.cpp - plugin main class
 *
 * Copyright (C) 2013  Ivan Romanov <drizt@land.ru>
 * Copyright (C) 2020  Boris Pek <tehnick-8@yandex.ru>
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
 */

#include "openpgpplugin.h"
#include "accountinfoaccessinghost.h"
#include "activetabaccessinghost.h"
#include "gpgprocess.h"
#include "model.h"
#include "openpgpmessaging.h"
#include "optionaccessinghost.h"
#include "options.h"
#include "psiaccountcontrollinghost.h"
#include "stanzasendinghost.h"
#include <QDomElement>

using OpenPgpPluginNamespace::GpgProcess;

OpenPgpPlugin::OpenPgpPlugin() : m_pgpMessaging(new OpenPgpMessaging())
{
}

OpenPgpPlugin::~OpenPgpPlugin()
{
    delete m_pgpMessaging;
    m_pgpMessaging = nullptr;
}

QWidget *OpenPgpPlugin::options()
{
    m_optionsDialog = new Options();
    m_optionsDialog->setOptionAccessingHost(m_optionHost);
    m_optionsDialog->setAccountInfoAccessingHost(m_accountInfo);
    m_optionsDialog->setPsiAccountControllingHost(m_accountHost);
    m_optionsDialog->loadSettings();
    connect(m_optionsDialog, &Options::destroyed, this, &OpenPgpPlugin::optionsDestroyed);
    return qobject_cast<QWidget *>(m_optionsDialog);
}

bool OpenPgpPlugin::enable() { return true; }

bool OpenPgpPlugin::disable() { return true; }

void OpenPgpPlugin::applyOptions() { m_optionsDialog->saveSettings(); }

void OpenPgpPlugin::restoreOptions() { }

QPixmap OpenPgpPlugin::icon() const { return QPixmap(":/icons/openpgp.png"); }

QString OpenPgpPlugin::pluginInfo()
{
    return name() + "\n\n" + tr("Authors: ") + "Boris Pek, Ivan Romanov\n\n"
        + tr("OpenPGP is the most widely used encryption standard. "
             "It is extremely simple in usage:\n"
             "* Generate a key pair (public key + secret key) or "
             "choose existing one and set it in program settings.\n"
             "* Protect your secret key with a strong password and never "
             "give it to anyone.\n"
             "* Share your public key with buddies and get their public "
             "keys using any communication channel which you trust "
             "(xmpp, email, PGP keys server).\n"
             "* Enable PGP encryption in chat with you buddy and have fun "
             "the protected conversation.")
        + "\n\n"
        + tr("This plugin uses standard command-line tool GnuPG, so "
             "attentively check that you properly installed and configured "
             "gpg and gpg-agent.")
        + "\n\n"
        + tr("Embedded Keys Manager can do only basic operations like "
             "creating, removing, exporting and importing PGP keys. "
             "This should be enough to most of users needs. For more "
             "complicated cases use special software.");
}

void OpenPgpPlugin::setStanzaSendingHost(StanzaSendingHost *host)
{
     m_pgpMessaging->setStanzaSendingHost(host);
}

bool OpenPgpPlugin::incomingStanza(int account, const QDomElement &stanza)
{
    return m_pgpMessaging->incomingStanza(account, stanza);
}

bool OpenPgpPlugin::outgoingStanza(int account, QDomElement &stanza)
{
    return m_pgpMessaging->outgoingStanza(account, stanza);
}

void OpenPgpPlugin::setPsiAccountControllingHost(PsiAccountControllingHost *host)
{
    m_accountHost = host;
    m_pgpMessaging->setPsiAccountControllingHost(host);
}

void OpenPgpPlugin::setOptionAccessingHost(OptionAccessingHost *host)
{
    m_optionHost = host;
    m_pgpMessaging->setOptionAccessingHost(host);
}

void OpenPgpPlugin::optionChanged(const QString &) { }

QList<QVariantHash> OpenPgpPlugin::getButtonParam() { return QList<QVariantHash>(); }

QAction *OpenPgpPlugin::getAction(QObject *parent, int, const QString &)
{
    return nullptr; // This is temporary, until code is moved from Psi core

    // tr("OpenPGP key is not set in your account settings!")
    m_action = new QAction(icon(), tr("OpenPGP encryption"), parent);
    m_action->setCheckable(false);
    connect(m_action, &QAction::triggered, this, &OpenPgpPlugin::actionActivated);
    connect(m_action, &QAction::destroyed, this, &OpenPgpPlugin::actionDestroyed);
    return m_action;
}

void OpenPgpPlugin::setActiveTabAccessingHost(ActiveTabAccessingHost *host) { m_activeTab = host; }

void OpenPgpPlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost *host) { m_accountInfo = host; }

void OpenPgpPlugin::actionActivated()
{
    ; // This is temporary, until code is moved from Psi core
}

void OpenPgpPlugin::sendPublicKey()
{
    return; // This is temporary, until code is moved from Psi core

    QAction *action = qobject_cast<QAction *>(sender());

    const QString &&jid       = m_activeTab->getYourJid();
    const QString &&jidToSend = m_activeTab->getJid();

    int     account   = 0;
    QString tmpJid;
    while (jid != (tmpJid = m_accountInfo->getJid(account))) {
        ++account;
        if (tmpJid == "-1") {
            return;
        }
    }

    const QString &&keyId = action->data().toString();
    const QString &&userId = action->text();

    m_pgpMessaging->sendPublicKey(account, jidToSend, keyId, userId);
}

void OpenPgpPlugin::actionDestroyed(QObject *) { m_action = nullptr; }

void OpenPgpPlugin::optionsDestroyed(QObject *) { m_optionsDialog = nullptr; }

bool OpenPgpPlugin::isEnabled() const
{
    if (!m_action)
        return false;

    return m_action->isChecked();
}
