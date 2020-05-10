/*
 * openpgp.cpp - plugin main class
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

#include "openpgp.h"
#include "accountinfoaccessinghost.h"
#include "activetabaccessinghost.h"
#include "gpgprocess.h"
#include "model.h"
#include "optionaccessinghost.h"
#include "options.h"
#include "psiaccountcontrollinghost.h"
#include "stanzasendinghost.h"
#include <QCursor>
#include <QDomElement>
#include <QFile>
#include <QMenu>
#include <QMessageBox>

OpenPGP::OpenPGP() :
    m_optionsForm(nullptr), m_accountHost(nullptr), m_optionHost(nullptr),
    m_menu(nullptr), m_stanzaSending(nullptr), m_activeTab(nullptr), m_accountInfo(nullptr)
{
}

OpenPGP::~OpenPGP() { }

QWidget *OpenPGP::options()
{
    m_optionsForm = new Options();
    m_optionsForm->setOptionAccessingHost(m_optionHost);
    m_optionsForm->loadSettings();
    return qobject_cast<QWidget *>(m_optionsForm);
}

bool OpenPGP::enable()
{
    m_optionHost->setGlobalOption("options.pgp.enabled", true);
    return true;
}

bool OpenPGP::disable()
{
    m_optionHost->setGlobalOption("options.pgp.enabled", false);
    return true;
}

void OpenPGP::applyOptions()
{
    m_optionsForm->saveSettings();
}

void OpenPGP::restoreOptions() { }

QPixmap OpenPGP::icon() const
{
    return QPixmap(":/icons/openpgp.png");
}

QString OpenPGP::pluginInfo()
{
    return name() + "\n\n"
        + tr("Authors: ")
        + "Boris Pek, Ivan Romanov\n\n"
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

bool OpenPGP::incomingStanza(int account, const QDomElement &stanza)
{
    if (!isEnabled())
        return false;

    if (!m_optionHost->getPluginOption("auto-import", true).toBool())
        return false;

    if (stanza.tagName() != "message" && stanza.attribute("type") != "chat")
        return false;

    QString body = stanza.firstChildElement("body").text();

    int start = body.indexOf("-----BEGIN PGP PUBLIC KEY BLOCK-----");
    if (start == -1) {
        return false;
    }

    int end = body.indexOf("-----END PGP PUBLIC KEY BLOCK-----", start);
    if (end == -1) {
        return false;
    }

    QString key = body.mid(start, end - start);

    GpgProcess  gpg;
    const QStringList arguments { "--batch", "--import" };
    gpg.start(arguments);
    gpg.waitForStarted();
    gpg.write(key.toUtf8());
    gpg.closeWriteChannel();
    gpg.waitForFinished();

    QString from = stanza.attribute("from");
    // Cut trash from gpg command output
    QString res = QString::fromUtf8(gpg.readAllStandardError());
    res         = m_stanzaSending->escape(res.mid(0, res.indexOf('\n')));
    res.replace("&quot;", "\"");
    res.replace("&lt;", "<");
    res.replace("&gt;", ">");
    m_accountHost->appendSysMsg(account, from, res);

    // Don't hide message if an error occurred
    if (gpg.exitCode()) {
        return false;
    }

    if (!m_optionHost->getPluginOption("hide-key-message", true).toBool()) {
        return false;
    } else {
        return true;
    }
}

QList<QVariantHash> OpenPGP::getButtonParam()
{
    return QList<QVariantHash>();
}

QAction *OpenPGP::getAction(QObject *parent, int, const QString &)
{
    m_action = new QAction(QPixmap(":/icons/key.png"), tr("Send GnuPG Public Key"), parent);
    m_action->setCheckable(false);
    connect(m_action, &QAction::triggered, this, &OpenPGP::actionActivated);
    return m_action;
}

void OpenPGP::actionActivated()
{
    if (m_menu) {
        delete m_menu;
    }

    m_menu = new QMenu();

    Model *model = new Model(m_menu);
    model->listKeys();

    for (int i = 0; i < model->rowCount(); i++) {
        if (model->item(i, Model::Type)->text() != "sec") {
            continue;
        }

        QString str;
        // User name
        if (!model->item(i, Model::Name)->text().isEmpty()) {
            str += model->item(i, Model::Name)->text();
        }

        // Comment
        if (!model->item(i, Model::Comment)->text().isEmpty()) {
            if (!str.isEmpty()) {
                str += " ";
            }
            str += QString("(%1)").arg(model->item(i, Model::Comment)->text());
        }

        // Email
        if (!model->item(i, Model::Email)->text().isEmpty()) {
            if (!str.isEmpty()) {
                str += " ";
            }
            str += QString("<%1>").arg(model->item(i, Model::Email)->text());
        }

        // Short ID
        if (!str.isEmpty()) {
            str += " ";
        }
        str += model->item(i, Model::ShortId)->text();

        QAction *action = m_menu->addAction(str);
        action->setData(model->item(i, Model::Fingerprint)->text());
        connect(action, &QAction::triggered, this, &OpenPGP::sendPublicKey);
    }

    m_menu->popup(QCursor::pos());
}

void OpenPGP::sendPublicKey()
{
    QAction *action      = qobject_cast<QAction *>(sender());
    QString  fingerprint = "0x" + action->data().toString();

    const QStringList arguments { "--armor", "--export", fingerprint };

    GpgProcess gpg;
    gpg.start(arguments);
    gpg.waitForFinished();

    // do nothing if error is occurred
    if (gpg.exitCode()) {
        return;
    }

    QString key = QString::fromUtf8(gpg.readAllStandardOutput());

    QString jid       = m_activeTab->getYourJid();
    QString jidToSend = m_activeTab->getJid();
    int     account   = 0;
    QString tmpJid;
    while (jid != (tmpJid = m_accountInfo->getJid(account))) {
        ++account;
        if (tmpJid == "-1") {
            return;
        }
    }

    m_stanzaSending->sendMessage(account, jidToSend, key, "", "chat");

    QString res = tr("Public key \"%1\" sent").arg(action->text());
    res         = m_stanzaSending->escape(res);
    res.replace("&quot;", "\"");
    res.replace("&lt;", "<");
    res.replace("&gt;", ">");
    m_accountHost->appendSysMsg(account, jidToSend, res);
}

bool OpenPGP::isEnabled() const
{
    if (!m_action)
        return false;

    return m_action->isChecked();
}

