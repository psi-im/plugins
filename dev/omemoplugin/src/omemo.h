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

#ifndef PSIOMEMO_OMEMO_H
#define PSIOMEMO_OMEMO_H

#include <QtXml>
#include "psiaccountcontrollinghost.h"
#include "stanzasendinghost.h"
#include "signal.h"

namespace psiomemo {
  class OMEMO {
  public:
    void init(const QString &dataPath);
    void setStanzaSender(StanzaSendingHost *stanzaSender);
    void setAccountController(PsiAccountControllingHost *accountController);

    QDomElement decryptMessage(int account, const QDomElement &xml);
    bool encryptMessage(const QString &ownJid, int account, QDomElement &xml, bool buildSessions = true, const uint32_t *toDeviceId = nullptr);
    bool processDeviceList(const QString &ownJid, int account, const QDomElement &xml);
    void deinit();
    bool processBundle(const QString &ownJid, int account, const QDomElement &xml);

    void accountConnected(int account, const QString &ownJid);

    const QString deviceListNodeName() const;
    bool isAvailableForUser(const QString &user);
    bool isEnabledForUser(const QString &user);
    void setEnabledForUser(const QString &user, bool enabled);
    uint32_t getDeviceId();
    QString getOwnFingerprint();
    QList<Fingerprint> getKnownFingerprints();
    void confirmDeviceTrust(const QString &user, uint32_t deviceId);

  private:
    class MessageWaitingForBundles {
    public:
      QDomElement xml;
      QSet<QString> sentStanzas;
      QSet<uint32_t> pendingBundles;
      friend bool operator==(const MessageWaitingForBundles &rhs, const MessageWaitingForBundles &lhs) {
        return rhs.xml == lhs.xml;
      }
    };

    StanzaSendingHost *m_stanzaSender = nullptr;
    PsiAccountControllingHost *m_accountController = nullptr;
    QVector<MessageWaitingForBundles> m_pendingMessages;
    Signal m_signal;
    void pepPublish(int account, const QString &dl_xml) const;
    void publishOwnBundle(int account);

    void setNodeText(QDomElement &node, const QByteArray &byteArray) const;
    void buildSessionsFromBundle(const QVector<uint32_t> &invalidSessions, const QString &ownJid, int account,
                                 const QDomElement &messageToResend);
    QString pepRequest(int account, const QString &ownJid, const QString &recipient, const QString &node) const;
    const QString bundleNodeName(uint32_t deviceId) const;
  };
}

#endif //PSIOMEMO_OMEMO_H
