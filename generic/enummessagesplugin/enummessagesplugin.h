/*
 * Copyright (C) 2016  Evgeny Khryukin
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

#ifndef ENUMMESSAGESPLUGIN_H
#define ENUMMESSAGESPLUGIN_H

#include "activetabaccessor.h"
#include "applicationinfoaccessor.h"
#include "chattabaccessor.h"
#include "optionaccessor.h"
#include "plugininfoprovider.h"
#include "psiaccountcontroller.h"
#include "psiplugin.h"
#include "stanzafilter.h"
#include "toolbariconaccessor.h"
#include "ui_options.h"

#include <QColor>
#include <QPointer>

class ActiveTabAccessingHost;
class ApplicationInfoAccessingHost;
class OptionAccessingHost;
class QDomDocument;

class EnumMessagesPlugin:
        public ActiveTabAccessor,
        public ApplicationInfoAccessor,
        public ChatTabAccessor,
        public OptionAccessor,
        public PluginInfoProvider,
        public PsiAccountController,
        public PsiPlugin,
        public QObject,
        public StanzaFilter,
        public ToolbarIconAccessor {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.psi-plus.EnumMessagesPlugin")
    Q_INTERFACES(PsiPlugin OptionAccessor ActiveTabAccessor StanzaFilter
            ApplicationInfoAccessor PluginInfoProvider ChatTabAccessor
            PsiAccountController ToolbarIconAccessor)

public:
    EnumMessagesPlugin();
    virtual QString name() const;
    virtual QString shortName() const;
    virtual QString version() const;
    virtual QWidget* options();
    virtual bool enable();
    virtual bool disable();
    virtual void applyOptions();
    virtual void restoreOptions();
    virtual QPixmap icon() const;

    virtual void setOptionAccessingHost(OptionAccessingHost* host);
    virtual void optionChanged(const QString& ) {}

    virtual void setActiveTabAccessingHost(ActiveTabAccessingHost* host);
    virtual void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host);
    virtual void setPsiAccountControllingHost(PsiAccountControllingHost* host);

    virtual QString pluginInfo();

    // ChatTabAccessor
    void setupChatTab(QWidget* tab, int account, const QString& contact);
    void setupGCTab(QWidget* /*tab*/, int /*account*/, const QString& /*contact*/) { /* do nothing*/ }
    virtual bool appendingChatMessage(int account, const QString& contact,
                      QString& body, QDomElement& html, bool local);

    //stanza filter
    virtual bool incomingStanza(int account, const QDomElement& stanza);
    virtual bool outgoingStanza(int , QDomElement& );

    //ToolbarIconAccessor
    virtual QList < QVariantHash > getButtonParam() { return QList < QVariantHash >(); }
    virtual QAction* getAction(QObject* parent, int account, const QString& contact);

private slots:
    void removeWidget();
    void getColor();
    void onActionActivated(bool);

private:
    void addMessageNum(QDomDocument* doc, QDomElement* stanza, quint16 num, const QColor& color);
    static QString numToFormatedStr(int number);
    static void nl2br(QDomElement *body,QDomDocument* doc, const QString& msg);
    bool isEnabledFor(int account, const QString& jid) const;

private:
    bool enabled;
    OptionAccessingHost* _psiOptions;
    ActiveTabAccessingHost* _activeTab;
    ApplicationInfoAccessingHost* _applicationInfo;
    PsiAccountControllingHost* _accContrller;

    typedef QMap <QString, quint16> JidEnums;
    QMap <int, JidEnums> _enumsIncomming, _enumsOutgoing;
    QColor _inColor, _outColor;
    bool _defaultAction;
    Ui::Options _ui;
    QPointer<QWidget> _options;
    typedef QMap <QString, bool> JidActions;
    QMap <int, JidActions> _jidActions;
};

#endif // ENUMMESSAGESPLUGIN_H
