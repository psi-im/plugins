/*
 * OMEMO Plugin for Psi
 * Copyright (C) 2018 Vyacheslav Karpukhin
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

#ifndef PSIOMEMO_OMEMOPLUGIN_H
#define PSIOMEMO_OMEMOPLUGIN_H

#include <QObject>

#include "psiplugin.h"
#include "stanzafilter.h"
#include "stanzasender.h"
#include "eventcreator.h"
#include "accountinfoaccessor.h"
#include "applicationinfoaccessor.h"
#include "psiaccountcontroller.h"
#include "menuaccessor.h"
#include "plugininfoprovider.h"
#include "toolbariconaccessor.h"
#include "storage.h"
#include "crypto.h"
#include "omemo.h"

namespace psiomemo {
  class OMEMOPlugin : public QObject,
                      public PsiPlugin,
                      public StanzaFilter,
                      public StanzaSender,
                      public EventCreator,
                      public AccountInfoAccessor,
                      public ApplicationInfoAccessor,
                      public PsiAccountController,
                      public MenuAccessor,
                      public PluginInfoProvider,
                      public ToolbarIconAccessor {
  Q_OBJECT
  Q_PLUGIN_METADATA(IID
                        "com.psi.OmemoPlugin")
  Q_INTERFACES(PsiPlugin
               StanzaFilter
               StanzaSender
               EventCreator
               AccountInfoAccessor
               ApplicationInfoAccessor
               PsiAccountController
               MenuAccessor
               PluginInfoProvider
               ToolbarIconAccessor)
  public:
    QString name() const override;
    QString shortName() const override;
    QString version() const override;
    QWidget *options() override;
    bool enable() override;
    bool disable() override;
    QPixmap icon() const override;
    void applyOptions() override;
    void restoreOptions() override;

    QString pluginInfo() override;

    bool incomingStanza(int account, const QDomElement &xml) override;
    bool outgoingStanza(int account, QDomElement &xml) override;
    bool stanzaWasEncrypted(const QString &stanzaId) override;

    void setAccountInfoAccessingHost(AccountInfoAccessingHost *host) override;
    void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host) override;
    void setStanzaSendingHost(StanzaSendingHost *host) override;
    void setEventCreatingHost(EventCreatingHost *host) override;
    void setPsiAccountControllingHost(PsiAccountControllingHost *host) override;
    QStringList pluginFeatures() override;

    QList<QVariantHash> getAccountMenuParam() override;
    QList<QVariantHash> getContactMenuParam() override;
    QAction *getContactAction(QObject *parent, int account, const QString &contact) override;
    QAction *getAccountAction(QObject *parent, int account) override;

    QList<QVariantHash> getButtonParam() override;
    QAction *getAction(QObject *parent, int __unused account, const QString &contact) override;

  private:
    bool m_enabled;
    QSet<QString> m_encryptedStanzaIds;
    QMultiMap<QString, QAction*> m_actions;
    OMEMO m_omemo;

    AccountInfoAccessingHost *m_accountInfo;
    ApplicationInfoAccessingHost *m_applicationInfo;
    EventCreatingHost *m_eventCreator;

    QPixmap getIcon() const;
    void updateActions(const QString &user);
    QAction *createAction(QObject *parent, const QString &contact);
  private slots:
    void onEnableOMEMOAction(bool);
  };
}
#endif //PSIOMEMO_OMEMOPLUGIN_H
