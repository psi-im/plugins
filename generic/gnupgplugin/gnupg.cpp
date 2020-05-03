/*
 * gnupg.cpp - plugin main class
 *
 * Copyright (C) 2013  Ivan Romanov <drizt@land.ru>
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

#include "gnupg.h"
#include "accountinfoaccessinghost.h"
#include "activetabaccessinghost.h"
#include "gpgprocess.h"
#include "iconfactoryaccessinghost.h"
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

GnuPG::GnuPG() :
    _enabled(false), _optionsForm(nullptr), _accountHost(nullptr), _optionHost(nullptr), _iconFactory(nullptr),
    _menu(nullptr), _stanzaSending(nullptr), _activeTab(nullptr), _accountInfo(nullptr)
{
}

GnuPG::~GnuPG() { }

QWidget *GnuPG::options()
{
    if (!_enabled) {
        return nullptr;
    }

    _optionsForm = new Options();
    _optionsForm->setOptionAccessingHost(_optionHost);
    _optionsForm->loadSettings();
    return qobject_cast<QWidget *>(_optionsForm);
}

bool GnuPG::enable()
{
    QFile file(":/icons/key.png");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray image = file.readAll();
        _iconFactory->addIcon("gnupg/icon", image);
        file.close();
        _enabled = true;
    } else {
        _enabled = false;
    }
    return _enabled;
}

bool GnuPG::disable()
{
    _enabled = false;
    return true;
}

void GnuPG::applyOptions() { _optionsForm->saveSettings(); }

void GnuPG::restoreOptions() { }

QPixmap GnuPG::icon() const { return QPixmap(":/icons/gnupg.png"); }

QString GnuPG::pluginInfo()
{
    return name() + "\n\n" + tr("Author: ") + "Ivan Romanov\n" + tr("Email: ") + "drizt@land.ru\n\n"
        + tr("GnuPG Key Manager can create, remove, export and import GnuPG keys. "
             "It can do only the base operations but I hope it will be enough for your needs.");
}

bool GnuPG::incomingStanza(int account, const QDomElement &stanza)
{
    if (!_enabled) {
        return false;
    }

    if (!_optionHost->getPluginOption("auto-import", true).toBool()) {
        return false;
    }

    if (stanza.tagName() != "message" && stanza.attribute("type") != "chat") {
        return false;
    }

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
    QStringList arguments;
    arguments << "--batch"
              << "--import";
    gpg.start(arguments);
    gpg.waitForStarted();
    gpg.write(key.toUtf8());
    gpg.closeWriteChannel();
    gpg.waitForFinished();

    QString from = stanza.attribute("from");
    // Cut trash from gpg command output
    QString res = QString::fromUtf8(gpg.readAllStandardError());
    res         = _stanzaSending->escape(res.mid(0, res.indexOf('\n')));
    res.replace("&quot;", "\"");
    res.replace("&lt;", "<");
    res.replace("&gt;", ">");
    _accountHost->appendSysMsg(account, from, res);

    // Don't hide message if an error occurred
    if (gpg.exitCode()) {
        return false;
    }

    if (!_optionHost->getPluginOption("hide-key-message", true).toBool()) {
        return false;
    } else {
        return true;
    }
}

QList<QVariantHash> GnuPG::getButtonParam()
{
    QList<QVariantHash> l;

    QVariantHash hash;
    hash["tooltip"] = QVariant(tr("Send GnuPG Public Key"));
    hash["icon"]    = QVariant(QString("gnupg/icon"));
    hash["reciver"] = qVariantFromValue(qobject_cast<QObject *>(this));
    hash["slot"]    = QVariant(SLOT(actionActivated()));
    l << hash;
    return l;
}

void GnuPG::actionActivated()
{
    if (_menu) {
        delete _menu;
    }

    _menu = new QMenu();

    Model *model = new Model(_menu);
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

        QAction *action = _menu->addAction(str);
        action->setData(model->item(i, Model::Fingerprint)->text());
        connect(action, SIGNAL(triggered()), SLOT(sendPublicKey()));
    }

    _menu->popup(QCursor::pos());
}

void GnuPG::sendPublicKey()
{
    QAction *action      = qobject_cast<QAction *>(sender());
    QString  fingerprint = "0x" + action->data().toString();

    GpgProcess  gpg;
    QStringList arguments;
    arguments << "--armor"
              << "--export" << fingerprint;

    gpg.start(arguments);
    gpg.waitForFinished();

    // do nothing if error is occurred
    if (gpg.exitCode()) {
        return;
    }

    QString key = QString::fromUtf8(gpg.readAllStandardOutput());

    QString jid       = _activeTab->getYourJid();
    QString jidToSend = _activeTab->getJid();
    int     account   = 0;
    QString tmpJid;
    while (jid != (tmpJid = _accountInfo->getJid(account))) {
        ++account;
        if (tmpJid == "-1") {
            return;
        }
    }

    _stanzaSending->sendMessage(account, jidToSend, key, "", "chat");

    QString res = tr("Public key \"%1\" sent").arg(action->text());
    res         = _stanzaSending->escape(res);
    res.replace("&quot;", "\"");
    res.replace("&lt;", "<");
    res.replace("&gt;", ">");
    _accountHost->appendSysMsg(account, jidToSend, res);
}
