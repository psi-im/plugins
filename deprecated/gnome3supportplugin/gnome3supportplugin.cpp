/*
 * gnome3supportplugin.cpp - plugin
 * Copyright (C) 2011  KukuRuzo
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
#include "psiplugin.h"
#include "plugininfoprovider.h"
#include "optionaccessinghost.h"
#include "optionaccessor.h"
#include "accountinfoaccessinghost.h"
#include "accountinfoaccessor.h"
#include "psiaccountcontroller.h"
#include "psiaccountcontrollinghost.h"

#include <QIcon>
#include <QDBusConnection>
#include <QDBusConnectionInterface>

#define gnome3Service "org.gnome.SessionManager"
#define gnome3Interface "org.gnome.SessionManager.Presence"
#define gnome3Path "/org/gnome/SessionManager/Presence"

static const QStringList statuses = QStringList() << "online" << "invisible" << "dnd" << "away";

#define constVersion "0.0.3"

class Gnome3StatusWatcher : public QObject, public PsiPlugin, public PluginInfoProvider, public OptionAccessor,
		public PsiAccountController, public AccountInfoAccessor
{
	Q_OBJECT
#ifdef HAVE_QT5
	Q_PLUGIN_METADATA(IID "com.psi-plus.Gnome3StatusWatcher")
#endif
	Q_INTERFACES(PsiPlugin PluginInfoProvider OptionAccessor PsiAccountController AccountInfoAccessor)
public:
	Gnome3StatusWatcher();
	virtual QString name() const;
	virtual QString shortName() const;
	virtual QString version() const;
	virtual QWidget* options();
	virtual bool enable();
	virtual bool disable();
	virtual void applyOptions() {};
	virtual void restoreOptions() {};
	virtual QPixmap icon() const;
	virtual void optionChanged(const QString&) {};
	virtual void setOptionAccessingHost(OptionAccessingHost* host);
	virtual void setAccountInfoAccessingHost(AccountInfoAccessingHost* host);
	virtual void setPsiAccountControllingHost(PsiAccountControllingHost* host);
	virtual QString pluginInfo();

private:
	bool enabled;
	OptionAccessingHost* psiOptions;
	AccountInfoAccessingHost* accInfo;
	PsiAccountControllingHost* accControl;
	QString status, statusMessage;
	bool isDBUSConnected;
	void connectToBus(const QString &service_);
	void disconnectFromBus(const QString &service_);
	void setPsiGlobalStatus(const QString &status);

private slots:
	void onGnome3StatusChange(const uint &status);
};

#ifndef HAVE_QT5
Q_EXPORT_PLUGIN(Gnome3StatusWatcher);
#endif

Gnome3StatusWatcher::Gnome3StatusWatcher() {
	enabled = false;
	psiOptions = 0;
	accInfo = 0;
	accControl = 0;
}

QString Gnome3StatusWatcher::name() const {
	return "Gnome 3 Support Plugin";
}

QString Gnome3StatusWatcher::shortName() const {
	return "gnome3support";
}

QString Gnome3StatusWatcher::version() const {
	return constVersion;
}

void Gnome3StatusWatcher::setOptionAccessingHost(OptionAccessingHost* host) {
	psiOptions = host;
}

void Gnome3StatusWatcher::setAccountInfoAccessingHost(AccountInfoAccessingHost* host) {
	accInfo = host;
}

void Gnome3StatusWatcher::setPsiAccountControllingHost(PsiAccountControllingHost* host) {
	accControl = host;
}

bool Gnome3StatusWatcher::enable() {
	if (psiOptions) {
		enabled = true;
		isDBUSConnected = false;
		QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames().value();
		if (services.contains(gnome3Service, Qt::CaseInsensitive)) {
			connectToBus(gnome3Service);
		}
	}
	return enabled;
}

bool Gnome3StatusWatcher::disable(){
	enabled = false;
	if (isDBUSConnected) {
		disconnectFromBus(gnome3Service);
	}
	return true;
}

QPixmap Gnome3StatusWatcher::icon() const
{
	return QPixmap(":/icons/gnome3support.png");
}

QWidget* Gnome3StatusWatcher::options()
{
	return 0;
}

QString Gnome3StatusWatcher::pluginInfo() {
	return tr("Authors: ") +  "KukuRuzo\n\n"
			+ trUtf8("This plugin is designed to add support of GNOME 3 presence status changes");
}

void Gnome3StatusWatcher::connectToBus(const QString &service_)
{
	isDBUSConnected = QDBusConnection::sessionBus().connect(service_,
															QLatin1String(gnome3Path),
															QLatin1String(gnome3Interface),
															QLatin1String("StatusChanged"),
															this,
															SLOT(onGnome3StatusChange(uint)));
}

void Gnome3StatusWatcher::disconnectFromBus(const QString &service_)
{
	QDBusConnection::sessionBus().disconnect(service_,
											QLatin1String(gnome3Path),
											QLatin1String(gnome3Interface),
											QLatin1String("StatusChanged"),
											this,
											SLOT(onGnome3StatusChange(uint)));
}

void Gnome3StatusWatcher::onGnome3StatusChange(const uint &status_)
{
	int i = (int)status_;
	if (i != -1 && i < statuses.length()) {
		setPsiGlobalStatus(statuses[i]);
	}
}

void Gnome3StatusWatcher::setPsiGlobalStatus(const QString &status_)
{
	if (!enabled) return;
	int account = 0;
	while (accInfo->getJid(account) != "-1") {
		QString accStatus = accInfo->getStatus(account);
		if (accStatus != "offline" && accStatus != "invisible" && accStatus != status_) {
			accControl->setStatus(account, status_, "");
		}
		++account;
	}
}
#include "gnome3supportplugin.moc"
