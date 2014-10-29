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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QDomElement>
#include <QMessageBox>
#include <QFile>
#include <QCursor>
#include <QMenu>
#include "options.h"
#include "gnupg.h"
#include "gpgprocess.h"
#include "psiaccountcontrollinghost.h"
#include "optionaccessinghost.h"
#include "iconfactoryaccessinghost.h"
#include "model.h"
#include "activetabaccessinghost.h"
#include "accountinfoaccessinghost.h"
#include "stanzasendinghost.h"

#ifndef HAVE_QT5
Q_EXPORT_PLUGIN(GnuPG);
#endif

GnuPG::GnuPG()
	: _enabled(false)
	, _optionsForm(0)
	, _accountHost(0)
	, _optionHost(0)
	, _iconFactory(0)
	, _menu(0)
	, _stanzaSending(0)
	, _activeTab(0)
	, _accountInfo(0)
{
}


GnuPG::~GnuPG()
{
}

QWidget *GnuPG::options()
{
	if (!_enabled) {
		return 0;
	}

	_optionsForm = new Options();
	_optionsForm->setOptionAccessingHost(_optionHost);
	_optionsForm->loadSettings();
	return qobject_cast<QWidget*>(_optionsForm);
}

bool GnuPG::enable()
{
	QFile file(":/icons/key.png");
	if ( file.open(QIODevice::ReadOnly) ) {
		QByteArray image = file.readAll();
		_iconFactory->addIcon("gnupg/icon",image);
		file.close();
		_enabled = true;
	}
	else {
		_enabled = false;
	}
	return _enabled;
}

bool GnuPG::disable()
{
	_enabled = false;
	return true;
}

void GnuPG::applyOptions()
{
	_optionsForm->saveSettings();
}

void GnuPG::restoreOptions()
{
}

QPixmap GnuPG::icon() const
{
	return QPixmap(":/icons/gnupg.png");
}

QString GnuPG::pluginInfo()
{
	return tr("Author: ") +	 "Ivan Romanov\n"
		   + tr("e-mail: ") + "drizt@land.ru\n\n"
		   + tr("GnuPG Key Manager can create, remove, export and import GnuPG keys. "
				"It can do only the base operations but I hope it will be enough for your needs.");
}

bool GnuPG::incomingStanza(int account, const QDomElement& stanza)
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

	GpgProcess gpg;
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
	res = _stanzaSending->escape(res.mid(0, res.indexOf('\n')));
	_accountHost->appendSysMsg(account, from, res);

	// Don't hide message if an error occured
	if (gpg.exitCode()) {
		return false;
	}

	if (!_optionHost->getPluginOption("hide-key-message", true).toBool()) {
		return false;
	}
	else {
		return true;
	}
}

QList<QVariantHash> GnuPG::getButtonParam()
{
	QList<QVariantHash> l;

	QVariantHash hash;
	hash["tooltip"] = QVariant(tr("Send GnuPG Public Key"));
	hash["icon"] = QVariant(QString("gnupg/icon"));
	hash["reciver"] = qVariantFromValue(qobject_cast<QObject *>(this));
	hash["slot"] = QVariant(SLOT(actionActivated()));
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
		if (model->item(i)->text() != "sec") {
			continue;
		}

		QString str;
		// User name
		if (!model->item(i, 1)->text().isEmpty()) {
			str += model->item(i, 1)->text();
		}

		// Comment
		if (!model->item(i, 5)->text().isEmpty()) {
			if (!str.isEmpty()) {
				str += " ";
			}
			str += QString("(%1)").arg(model->item(i, 5)->text());
		}

		// Password
		if (!model->item(i, 2)->text().isEmpty()) {
			if (!str.isEmpty()) {
				str += " ";
			}
			str += QString("<%1>").arg(model->item(i, 2)->text());
		}

		// Short ID
		if (!str.isEmpty()) {
			str += " ";
		}
		str += model->item(i, 7)->text();

		QAction *action = _menu->addAction(str);
		action->setData(model->item(i, 8)->text());
		connect(action, SIGNAL(triggered()), SLOT(sendPublicKey()));
	}

	_menu->popup(QCursor::pos());
}

void GnuPG::sendPublicKey()
{
	QAction *action = qobject_cast<QAction*>(sender());
	QString fingerprint = "0x" + action->data().toString();

	GpgProcess gpg;
	QStringList arguments;
	arguments << "--armor"
			  << "--export"
			  << fingerprint;

	gpg.start(arguments);
	gpg.waitForFinished();

	// do nothing if error is occured
	if (gpg.exitCode()) {
		return;
	}

	QString key = QString::fromUtf8(gpg.readAllStandardOutput());

	QString jid = _activeTab->getYourJid();
	QString jidToSend = _activeTab->getJid();
	int account = 0;
	QString tmpJid;
	while (jid != (tmpJid = _accountInfo->getJid(account))) {
		++account;
		if (tmpJid == "-1") {
			return;
		}
	}

	_stanzaSending->sendMessage(account, jidToSend, key, "", "chat");
	_accountHost->appendSysMsg(account, jidToSend, _stanzaSending->escape(QString(tr("Public key %1 sent")).arg(action->text())));
}
