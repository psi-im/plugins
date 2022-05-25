/*
 * OMEMO Plugin for Psi
 * Copyright (C) 2018 Vyacheslav Karpukhin
 * Copyright (C) 2020 Boris Pek
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

#ifndef PSIOMEMO_OMEMO_H
#define PSIOMEMO_OMEMO_H

#include "accountinfoaccessinghost.h"
#include "contactinfoaccessinghost.h"
#include "psiaccountcontrollinghost.h"
#include "signal.h"
#include "stanzasendinghost.h"
#include <QMap>
#include <QSet>
#include <QtXml>
#include <memory>

namespace psiomemo {
class OMEMO : public QObject {
    Q_OBJECT

public:
    void init(const QString &dataPath);
    void setStanzaSender(StanzaSendingHost *stanzaSender);
    void setAccountController(PsiAccountControllingHost *accountController);
    void setAccountInfoAccessor(AccountInfoAccessingHost *accountInfoAccessor);
    void setContactInfoAccessor(ContactInfoAccessingHost *contactInfoAccessor);
    bool appendSysMsg(int account, const QString &jid, const QString &message);

    bool decryptMessage(int account, QDomElement &message);
    bool encryptMessage(const QString &ownJid, int account, QDomElement &xml, bool buildSessions = true,
                        const uint32_t *toDeviceId = nullptr);
    bool processDeviceList(const QString &ownJid, int account, const QDomElement &xml);
    void processUndecidedDevices(int account, const QString &ownJid, const QString &user);
    void processUnknownDevices(int account, const QString &ownJid, const QString &user);
    void deinit();
    bool processBundle(const QString &ownJid, int account, const QDomElement &xml);

    void accountConnected(int account, const QString &ownJid);
    void askUserDevicesList(int account, const QString &ownJid, const QString &user);

    const QString           deviceListNodeName() const;
    bool                    isAvailableForUser(int account, const QString &user);
    bool                    isAvailableForGroup(int account, const QString &ownJid, const QString &bareJid);
    bool                    isEnabledForUser(int account, const QString &user);
    void                    setEnabledForUser(int account, const QString &user, bool value);
    uint32_t                getDeviceId(int account);
    QString                 getOwnFingerprint(int account);
    QList<Fingerprint>      getKnownFingerprints(int account);
    QMap<uint32_t, QString> getOwnFingerprintsMap(int account);
    QSet<uint32_t>          getOwnDevicesList(int account);
    bool                    removeDevice(int account, const QString &user, uint32_t deviceId);
    void                    confirmDeviceTrust(int account, const QString &user, uint32_t deviceId);
    void                    revokeDeviceTrust(int account, const QString &user, uint32_t deviceId);
    void                    unpublishDevice(int account, uint32_t deviceId);
    void                    deleteCurrentDevice(int account, uint32_t deviceId);

    void setAlwaysEnabled(const bool value);
    void setEnabledByDefault(const bool value);
    void setTrustNewOwnDevices(const bool value);
    void setTrustNewContactDevices(const bool value);
    bool isAlwaysEnabled() const;
    bool isEnabledByDefault() const;
    bool trustNewOwnDevices() const;
    bool trustNewContactDevices() const;

signals:
    void deviceListUpdated(int account);
    void saveSettings();

private:
    void pepPublish(int account, const QString &dl_xml) const;
    void pepUnpublish(int account, const QString &dl_xml) const;
    void publishOwnBundle(int account);

    void          setNodeText(QDomElement &node, const QByteArray &byteArray) const;
    void          buildSessionsFromBundle(const QMap<QString, QVector<uint32_t>> &recipientInvalidSessions,
                                          const QVector<uint32_t> &ownInvalidSessions, const QString &ownJid, int account,
                                          const QDomElement &messageToResend);
    QString       pepRequest(int account, const QString &ownJid, const QString &recipient, const QString &node) const;
    void          publishDeviceList(int account, const QSet<uint32_t> &devices) const;
    const QString bundleNodeName(uint32_t deviceId) const;

    template <typename T>
    bool forEachMucParticipant(int account, const QString &ownJid, const QString &conferenceJid, T &&lambda);

    std::shared_ptr<Signal> getSignal(int account);

private:
    class MessageWaitingForBundles {
    public:
        QDomElement              xml;
        QHash<QString, uint32_t> sentStanzas;
    };

    StanzaSendingHost                                 *m_stanzaSender        = nullptr;
    PsiAccountControllingHost                         *m_accountController   = nullptr;
    AccountInfoAccessingHost                          *m_accountInfoAccessor = nullptr;
    ContactInfoAccessingHost                          *m_contactInfoAccessor = nullptr;
    QVector<std::shared_ptr<MessageWaitingForBundles>> m_pendingMessages;
    QString                                            m_dataPath;
    QHash<int, std::shared_ptr<Signal>>                m_accountToSignal;
    QSet<QString>                                      m_ownDeviceListRequests;
    QHash<QString, QString>                            m_encryptedGroupMessages;

    bool m_alwaysEnabled          = false;
    bool m_enabledByDefault       = false;
    bool m_trustNewOwnDevices     = false;
    bool m_trustNewContactDevices = false;
};
}

#endif // PSIOMEMO_OMEMO_H
