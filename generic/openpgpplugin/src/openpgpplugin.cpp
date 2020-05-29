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
#include "pgputil.h"
#include "psiaccountcontrollinghost.h"
#include "stanzasendinghost.h"
#include <QDir>
#include <QDomElement>

using OpenPgpPluginNamespace::GpgProcess;

OpenPgpPlugin::OpenPgpPlugin() : m_pgpMessaging(new OpenPgpMessaging())
{
    // Create default gpg-agent config if it does not exist yet:
    if (!QDir().exists(GpgProcess().gpgAgentConfig())) {
        PGPUtil::saveGpgAgentConfig(PGPUtil::readGpgAgentConfig(true));
    }
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
    QString out = tr("OpenPGP is the most widely used encryption standard. "
                     "It is extremely simple in usage:<br/>"
                     "* Generate a key pair (public key + secret key) or "
                     "choose existing one and set it in program settings.<br/>"
                     "* Protect your secret key with a strong password and never "
                     "give it to anyone.<br/>"
                     "* Share your public key with buddies and get their public "
                     "keys using any communication channel which you trust "
                     "(xmpp, email, PGP keys server).<br/>"
                     "* Enable PGP encryption in chat with you buddy and have fun "
                     "the protected conversation.");
    out += "<br/><br/>";
    out += tr("OpenPGP features:<br/>"
              "* Offline messages.<br/>"
              "* File transfer. (Not supported by plugin yet.)");
    out += "<br/><br/>";
    out += tr("OpenPGP limitations:<br/>"
              "* No support of message copies to multiple devices.<br/>"
              "* No support of multi-user chats.");
    out += "<br/><br/>";
    out += tr("In comparison with OTR and OMEMO, OpenPGP allows one to keep "
              "encrypted messages history on server side but lucks support "
              "of forward secrecy (they are mutually exclusive).");
    out += "<br/><br/>";
    out += tr("Embedded Keys Manager can do only basic operations like "
              "creating, removing, exporting and importing PGP keys. "
              "This should be enough to most of users needs. For more "
              "complicated cases use special software.");
    out += "<br/><br/>";
    out += tr("OpenPGP plugin uses standard command-line tool GnuPG, so "
              "attentively check that you properly installed and configured "
              "gpg and gpg-agent. For example, in your system:")
        + "<br/>";
#if defined(Q_OS_WIN)
    out += tr("1) Download and install \"%1\" from official website:").arg("Simple installer for the current GnuPG")
        + " ";
    out += QString("<a href=\"https://gnupg.org/download/#binary\">"
                   "https://gnupg.org/download/#binary</a><br/>");
#elif defined(Q_OS_MAC)
    out += tr("1) Install gpg and gpg-agent using Homebrew:") + "<br/><br/>";
    out += QString("brew install gnupg pinentry-mac") + "<br/><br/>";
#else
    out += tr("1) Install gpg and gpg-agent using system packaging tool.") + "<br/>";
#endif
    out += tr("2) Edit configuration file %1 if necessary.")
#if defined(Q_OS_WIN)
               .arg(QDir::toNativeSeparators(GpgProcess().gpgAgentConfig()));
#else
               .arg(GpgProcess().gpgAgentConfig());
#endif

    return out;
}

void OpenPgpPlugin::setStanzaSendingHost(StanzaSendingHost *host) { m_pgpMessaging->setStanzaSendingHost(host); }

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

void OpenPgpPlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost *host)
{
    m_accountInfo = host;
    m_pgpMessaging->setAccountInfoAccessingHost(host);
}

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

    int     account = 0;
    QString tmpJid;
    while (jid != (tmpJid = m_accountInfo->getJid(account))) {
        ++account;
        if (tmpJid == "-1") {
            return;
        }
    }

    const QString &&keyId  = action->data().toString();
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
