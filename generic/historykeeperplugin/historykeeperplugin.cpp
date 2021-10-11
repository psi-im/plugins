/*
 * historykeeperplugin.cpp - plugin
 * Copyright (C) 2010-2011 Evgeny Khryukin
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

#include <QAction>
#include <QIcon>
#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>

#include "applicationinfoaccessinghost.h"
#include "applicationinfoaccessor.h"
#include "iconfactoryaccessinghost.h"
#include "iconfactoryaccessor.h"
#include "menuaccessor.h"
#include "optionaccessinghost.h"
#include "optionaccessor.h"
#include "plugininfoprovider.h"
#include "psiplugin.h"

#define constClearHistoryFor "clear-history-for"

class HistoryKeeperPlugin : public QObject,
                            public PsiPlugin,
                            public ApplicationInfoAccessor,
                            public OptionAccessor,
                            public MenuAccessor,
                            public PluginInfoProvider,
                            public IconFactoryAccessor {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.psi-plus.HistoryKeeperPlugin" FILE "psiplugin.json")
    Q_INTERFACES(PsiPlugin OptionAccessor ApplicationInfoAccessor MenuAccessor PluginInfoProvider IconFactoryAccessor)

public:
    HistoryKeeperPlugin();
    virtual QString             name() const;
    virtual QWidget *           options();
    virtual bool                enable();
    virtual bool                disable();
    virtual void                applyOptions();
    virtual void                restoreOptions();
    virtual void                setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host);
    virtual void                setOptionAccessingHost(OptionAccessingHost *host);
    virtual void                setIconFactoryAccessingHost(IconFactoryAccessingHost *host);
    virtual void                optionChanged(const QString & /*option*/) {};
    virtual QList<QVariantHash> getAccountMenuParam();
    virtual QList<QVariantHash> getContactMenuParam();
    virtual QAction *           getContactAction(QObject *p, int acc, const QString &jid);
    virtual QAction *           getAccountAction(QObject *, int) { return nullptr; };
    virtual QString             pluginInfo();

private:
    void           removeHistory();
    static QString nameToFilename(const QString &jid);
    void           addContact(const QString &jid);
    void           removeContact(const QString &jid);

private slots:
    void actionActivated(bool);

private:
    bool                          enabled;
    OptionAccessingHost *         psiOptions;
    ApplicationInfoAccessingHost *appInfo;
    IconFactoryAccessingHost *    icoHost;
    QPointer<QTextEdit>           contactsWidget;
    QStringList                   contacts;
};

HistoryKeeperPlugin::HistoryKeeperPlugin() :
    enabled(false), psiOptions(nullptr), appInfo(nullptr), icoHost(nullptr), contactsWidget(nullptr)
{
}

QString HistoryKeeperPlugin::name() const { return "History Keeper Plugin"; }

bool HistoryKeeperPlugin::enable()
{
    if (psiOptions) {
        enabled  = true;
        contacts = psiOptions->getPluginOption(constClearHistoryFor, QVariant(contacts)).toStringList();
    }
    return enabled;
}

bool HistoryKeeperPlugin::disable()
{
    removeHistory();
    enabled = false;
    return true;
}

void HistoryKeeperPlugin::removeHistory()
{
    if (!enabled)
        return;

    QString historyDir(appInfo->appHistoryDir());
    for (auto jid : contacts) {
        jid              = nameToFilename(jid);
        QString fileName = historyDir + QDir::separator() + jid;
        QFile   file(fileName);
        if (file.open(QIODevice::ReadWrite)) {
            qDebug("Removing file %s", qPrintable(fileName));
            file.remove();
        }
    }
}

QString HistoryKeeperPlugin::nameToFilename(const QString &jid)
{
    QString jid2;

    for (int n = 0; n < jid.length(); ++n) {
        if (jid.at(n) == '@') {
            jid2.append("_at_");
        } else if (jid.at(n) == '.') {
            jid2.append('.');
        } else if (!jid.at(n).isLetterOrNumber()) {
            // hex encode
            jid2.append(QString::asprintf("%%%02X", jid.at(n).toLatin1()));
        } else {
            jid2.append(jid.at(n));
        }
    }

    return jid2.toLower() + ".history";
}

QWidget *HistoryKeeperPlugin::options()
{
    if (!enabled) {
        return nullptr;
    }
    QWidget *    options = new QWidget();
    QVBoxLayout *layout  = new QVBoxLayout(options);

    contactsWidget = new QTextEdit();
    QString text;
    for (auto contact : contacts) {
        text += contact + "\n";
    }
    contactsWidget->setText(text);

    QLabel *wikiLink
        = new QLabel(tr("<a href=\"https://psi-plus.com/wiki/en:plugins#history_keeper_plugin\">Wiki (Online)</a>"));
    wikiLink->setOpenExternalLinks(true);

    layout->addWidget(new QLabel(tr("Remove history for contacts:")));
    layout->addWidget(contactsWidget);
    layout->addWidget(new QLabel());
    layout->addWidget(wikiLink);

    return options;
}

void HistoryKeeperPlugin::addContact(const QString &jid)
{
    if (!contacts.contains(jid)) {
        contacts.append(jid);
        psiOptions->setPluginOption(constClearHistoryFor, QVariant(contacts));
        restoreOptions();
    }
}

void HistoryKeeperPlugin::removeContact(const QString &jid)
{
    if (contacts.contains(jid)) {
        contacts.removeAt(contacts.indexOf(jid));
        psiOptions->setPluginOption(constClearHistoryFor, QVariant(contacts));
        restoreOptions();
    }
}

void HistoryKeeperPlugin::actionActivated(bool check)
{
    QString jid = sender()->property("jid").toString();
    if (check)
        addContact(jid);
    else
        removeContact(jid);
}

void HistoryKeeperPlugin::applyOptions()
{
    if (!contactsWidget)
        return;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    contacts = contactsWidget->toPlainText().split(QRegExp("\\s+"), Qt::SkipEmptyParts);
#else
    contacts = contactsWidget->toPlainText().split(QRegExp("\\s+"), QString::SkipEmptyParts);
#endif
    psiOptions->setPluginOption(constClearHistoryFor, QVariant(contacts));
}

void HistoryKeeperPlugin::restoreOptions()
{
    if (!contactsWidget)
        return;

    QString text;
    for (const QString &contact : contacts) {
        text += contact + "\n";
    }
    contactsWidget->setText(text);
}

void HistoryKeeperPlugin::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host) { appInfo = host; }

void HistoryKeeperPlugin::setIconFactoryAccessingHost(IconFactoryAccessingHost *host) { icoHost = host; }

void HistoryKeeperPlugin::setOptionAccessingHost(OptionAccessingHost *host) { psiOptions = host; }

QList<QVariantHash> HistoryKeeperPlugin::getAccountMenuParam() { return QList<QVariantHash>(); }

QList<QVariantHash> HistoryKeeperPlugin::getContactMenuParam() { return QList<QVariantHash>(); }

QAction *HistoryKeeperPlugin::getContactAction(QObject *p, int /*acc*/, const QString &jid)
{
    QAction *act = new QAction(icoHost->getIcon("psi/clearChat"), tr("Clear history on exit"), p);
    act->setCheckable(true);
    act->setChecked(contacts.contains(jid));
    act->setProperty("jid", jid);
    connect(act, &QAction::triggered, this, &HistoryKeeperPlugin::actionActivated);

    return act;
}

QString HistoryKeeperPlugin::pluginInfo()
{
    return tr("This plugin is designed to remove the history of selected contacts when the Psi is closed.\n"
              "You can select or deselect a contact for history removal from the context menu of a contact or via the "
              "plugin options.");
}

#include "historykeeperplugin.moc"
