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

#include "omemo.h"
#include "crypto.h"
#include <QtXml>

static const QString k_omemoXmlns("eu.siacs.conversations.axolotl");

namespace psiomemo {
void OMEMO::init(const QString &dataPath)
{
    m_dataPath = dataPath;
    m_accountController->subscribeLogout(this, [this](int account) {
        auto signal = m_accountToSignal.take(account);
        if (signal) {
            signal->deinit();
        }
        // TODO cleanup m_pendingMessages for this account as well. They all have null xml.
    });
}

void OMEMO::deinit()
{
    for (auto signal : m_accountToSignal.values()) {
        signal->deinit();
    }
}

void OMEMO::accountConnected(int account, const QString &ownJid)
{
    QString stanzaId = pepRequest(account, ownJid, ownJid, deviceListNodeName());
    m_ownDeviceListRequests.insert(QString::number(account) + "-" + stanzaId);
}

void OMEMO::askUserDevicesList(int account, const QString &ownJid, const QString &user)
{
    Q_UNUSED(pepRequest(account, ownJid, user, deviceListNodeName()));
}

void OMEMO::publishOwnBundle(int account)
{
    Bundle b = getSignal(account)->collectBundle();
    if (!b.isValid())
        return;

    QDomDocument doc;
    QDomElement  publish = doc.createElement("publish");
    doc.appendChild(publish);

    QDomElement item = doc.createElement("item");
    publish.appendChild(item);

    QDomElement bundle = doc.createElementNS(k_omemoXmlns, "bundle");
    item.appendChild(bundle);

    publish.setAttribute("node", bundleNodeName(getSignal(account)->getDeviceId()));

    QDomElement signedPreKey = doc.createElement("signedPreKeyPublic");
    signedPreKey.setAttribute("signedPreKeyId", b.signedPreKeyId);
    setNodeText(signedPreKey, b.signedPreKeyPublic);
    bundle.appendChild(signedPreKey);

    QDomElement signedPreKeySignature = doc.createElement("signedPreKeySignature");
    setNodeText(signedPreKeySignature, b.signedPreKeySignature);
    bundle.appendChild(signedPreKeySignature);

    QDomElement identityKey = doc.createElement("identityKey");
    setNodeText(identityKey, b.identityKeyPublic);
    bundle.appendChild(identityKey);

    QDomElement preKeys = doc.createElement("prekeys");
    bundle.appendChild(preKeys);

    for (auto preKey : b.preKeys) {
        QDomElement preKeyPublic = doc.createElement("preKeyPublic");
        preKeyPublic.setAttribute("preKeyId", preKey.first);
        setNodeText(preKeyPublic, preKey.second);
        preKeys.appendChild(preKeyPublic);
    }

    pepPublish(account, doc.toString());
}

bool OMEMO::decryptMessage(int account, QDomElement &xml)
{
    std::shared_ptr<Signal> signal = getSignal(account);

    bool        isCarbon = false;
    QDomElement message  = xml;

    if (message.firstChild().toElement().namespaceURI() == "urn:xmpp:carbons:2") {
        message  = message.firstChild().firstChildElement("forwarded").firstChildElement("message");
        isCarbon = true;
    }

    QDomElement encrypted = message.firstChildElement("encrypted");
    if (encrypted.isNull() || encrypted.namespaceURI() != k_omemoXmlns) {
        return false;
    }

    QString messageId = message.attribute("id");
    if (message.attribute("type") == "groupchat" && m_encryptedGroupMessages.contains(messageId)) {
        message.removeChild(encrypted);
        QDomElement body = message.ownerDocument().createElement("body");
        body.appendChild(body.ownerDocument().createTextNode(m_encryptedGroupMessages.value(messageId)));
        m_encryptedGroupMessages.remove(messageId);
        message.appendChild(body);
        return true;
    }

    QDomElement header = encrypted.firstChildElement("header");

    QDomElement keyElement = header.firstChildElement("key");
    while (!keyElement.isNull() && keyElement.attribute("rid").toUInt() != signal->getDeviceId()) {
        keyElement = keyElement.nextSiblingElement("key");
    }
    if (keyElement.isNull()) {
        xml = QDomElement();
        return true;
    }

    QString  preKeyAttr  = keyElement.attribute("prekey");
    bool     isPreKey    = preKeyAttr == "true" || preKeyAttr == "1";
    uint32_t preKeyCount = isPreKey ? signal->preKeyCount() : 0;

    QByteArray encryptedKey = QByteArray::fromBase64(keyElement.firstChild().nodeValue().toUtf8());

    QString from     = message.attribute("from");
    QString sender   = m_contactInfoAccessor->realJid(account, from).split("/").first();
    QString to       = message.attribute("to");
    uint    deviceId = header.attribute("sid").toUInt();
    if (!signal->getDeviceList(sender).contains(deviceId)) {
        const auto ownJid = m_accountInfoAccessor->getJid(account).split("/").first();
        pepRequest(account, ownJid, sender, deviceListNodeName());
    }

    QPair<QByteArray, bool> decryptionResult
        = signal->decryptKey(sender, EncryptedKey(deviceId, isPreKey, encryptedKey));
    QByteArray decryptedKey           = decryptionResult.first;
    bool       buildSessionWithPreKey = decryptionResult.second;
    if (buildSessionWithPreKey) {
        // remote has an invalid session, let's recover by overwriting it with a fresh one
        QDomElement emptyMessage = message.cloneNode(false).toElement();
        emptyMessage.setAttribute("from", to);
        emptyMessage.setAttribute("to", from);

        auto recipientInvalidSessions = QMap<QString, QVector<uint32_t>>({ { to, QVector<uint32_t>({ deviceId }) } });
        buildSessionsFromBundle(recipientInvalidSessions, QVector<uint32_t>(), to.split("/").first(), account,
                                emptyMessage);
        xml = QDomElement();
        return true;
    }

    QDomElement payloadElement = encrypted.firstChildElement("payload");
    if (!decryptedKey.isNull()) {
        if (isPreKey && signal->preKeyCount() < preKeyCount) {
            publishOwnBundle(account);
        }
        if (!payloadElement.isNull()) {
            QByteArray payload(QByteArray::fromBase64(payloadElement.firstChild().nodeValue().toUtf8()));
            QByteArray iv(QByteArray::fromBase64(header.firstChildElement("iv").firstChild().nodeValue().toUtf8()));
            QByteArray tag(OMEMO_AES_GCM_TAG_LENGTH, Qt::Uninitialized);

            if (decryptedKey.size() > OMEMO_AES_128_KEY_LENGTH) {
                tag          = decryptedKey.right(decryptedKey.size() - OMEMO_AES_128_KEY_LENGTH);
                decryptedKey = decryptedKey.left(OMEMO_AES_128_KEY_LENGTH);
            } else {
                tag = payload.right(OMEMO_AES_GCM_TAG_LENGTH);
                payload.chop(OMEMO_AES_GCM_TAG_LENGTH);
            }

            QPair<QByteArray, QByteArray> decryptedBody
                = Crypto::aes_gcm(Crypto::Decode, iv, decryptedKey, payload, tag);
            if (!decryptedBody.first.isNull()) {
                bool trusted = signal->isTrusted(sender, deviceId);
                message.removeChild(encrypted);
                QDomElement body = message.ownerDocument().createElement("body");
                QString     text = decryptedBody.first;

                if (!trusted) {
                    bool res = !isCarbon
                        && appendSysMsg(account, message.attribute("from"),
                                        "[OMEMO] " + tr("The following message is from an untrusted device:"));
                    if (!res) {
                        text = tr("[UNTRUSTED]: ") + text;
                    }
                }

                body.appendChild(body.ownerDocument().createTextNode(text));
                message.removeChild(message.firstChildElement("body"));
                message.appendChild(body);

                // for compatibility with XMPP clients which do not support of XEP-0380
                if (message.elementsByTagNameNS("urn:xmpp:eme:0", "encryption").isEmpty()) {
                    QDomElement encryption = message.ownerDocument().createElementNS("urn:xmpp:eme:0", "encryption");
                    encryption.setAttribute("namespace", k_omemoXmlns);
                    message.appendChild(encryption);
                }

                return true;
            }
        }
    }
    xml = QDomElement();
    return true;
}

bool OMEMO::encryptMessage(const QString &ownJid, int account, QDomElement &xml, bool buildSessions,
                           const uint32_t *toDeviceId)
{
    Q_ASSERT(!xml.isNull());
    std::shared_ptr<Signal> signal    = getSignal(account);
    QString                 recipient = m_contactInfoAccessor->realJid(account, xml.attribute("to")).split("/").first();
    bool                    isGroup   = xml.attribute("type") == "groupchat";
    if (!isEnabledForUser(account, recipient)) {
        return false;
    }

    if (buildSessions) {
        QMap<QString, QVector<uint32_t>> invalidSessions;
        QVector<uint32_t>                invalidSessionsWithOwnDevices;
        if (isGroup) {
            forEachMucParticipant(account, ownJid, recipient, [&](const QString &userJid) {
                QVector<uint32_t> sessions = signal->invalidSessions(userJid);
                if (!sessions.isEmpty()) {
                    invalidSessions.insert(userJid, sessions);
                }
                return true;
            });
        } else {
            QVector<uint32_t> sessions = signal->invalidSessions(recipient);
            if (!sessions.isEmpty()) {
                invalidSessions.insert(recipient, sessions);
            }
        }
        invalidSessionsWithOwnDevices = signal->invalidSessions(ownJid);
        invalidSessionsWithOwnDevices.removeOne(signal->getDeviceId());
        if (!invalidSessions.isEmpty() || !invalidSessionsWithOwnDevices.isEmpty()) {
            buildSessionsFromBundle(invalidSessions, invalidSessionsWithOwnDevices, ownJid, account, xml);
            xml = QDomElement();
            return true;
        }
    }

    processUndecidedDevices(account, ownJid, recipient);

    QDomElement encrypted = xml.ownerDocument().createElementNS(k_omemoXmlns, "encrypted");
    QDomElement header    = xml.ownerDocument().createElement("header");
    header.setAttribute("sid", signal->getDeviceId());
    encrypted.appendChild(header);
    xml.appendChild(encrypted);

    QByteArray iv = Crypto::randomBytes(OMEMO_AES_GCM_IV_LENGTH);

    QDomElement ivElement = xml.ownerDocument().createElement("iv");
    ivElement.appendChild(xml.ownerDocument().createTextNode(iv.toBase64()));
    header.appendChild(ivElement);

    QByteArray                    key  = Crypto::randomBytes(OMEMO_AES_128_KEY_LENGTH);
    QDomElement                   body = xml.firstChildElement("body");
    QPair<QByteArray, QByteArray> encryptedBody;
    if (!body.isNull()) {
        QString plainText = body.firstChild().nodeValue();
        encryptedBody     = Crypto::aes_gcm(Crypto::Encode, iv, key, plainText.toUtf8());
        key += encryptedBody.second;
    }
    QList<EncryptedKey> encryptedKeys;
    if (isGroup) {
        forEachMucParticipant(account, ownJid, recipient, [&](const QString &userJid) {
            encryptedKeys.append(signal->encryptKey(ownJid, userJid, key));
            return true;
        });
    } else {
        encryptedKeys = signal->encryptKey(ownJid, recipient, key);
    }

    if (encryptedKeys.isEmpty()) {
        appendSysMsg(account, xml.attribute("to"),
                     "[OMEMO] " + tr("Unable to build any sessions, the message was not sent"));
        xml = QDomElement();
    } else {
        for (auto encKey : encryptedKeys) {
            if (toDeviceId != nullptr && *toDeviceId != encKey.deviceId) {
                continue;
            }
            QDomElement keyElement = xml.ownerDocument().createElement("key");
            keyElement.setAttribute("rid", encKey.deviceId);
            if (encKey.isPreKey) {
                keyElement.setAttribute("prekey", "true");
            }
            setNodeText(keyElement, encKey.key);
            header.appendChild(keyElement);
        }

        if (!body.isNull()) {
            if (isGroup) {
                m_encryptedGroupMessages.insert(xml.attribute("id"), body.firstChild().nodeValue());
            }
            xml.removeChild(body);

            QDomElement payload = xml.ownerDocument().createElement("payload");
            payload.appendChild(xml.ownerDocument().createTextNode(encryptedBody.first.toBase64()));
            encrypted.appendChild(payload);

            QDomElement html = xml.firstChildElement("html");
            if (!html.isNull()) {
                xml.removeChild(html);
            }
        }

        xml.appendChild(xml.ownerDocument().createElementNS("urn:xmpp:hints", "store"));

        QDomElement encryption = xml.ownerDocument().createElementNS("urn:xmpp:eme:0", "encryption");
        encryption.setAttribute("namespace", k_omemoXmlns);
        xml.appendChild(encryption);

        QDomElement fallbackBody = xml.ownerDocument().createElement("body");
        fallbackBody.appendChild(xml.ownerDocument().createTextNode(
            tr("You received a message encrypted with OMEMO but your client doesn't support OMEMO or its support is currently disabled.")
        ));
        xml.appendChild(fallbackBody);
    }

    return true;
}

bool OMEMO::processDeviceList(const QString &ownJid, int account, const QDomElement &xml)
{
    QString from            = xml.attribute("from");
    bool    expectedOwnList = m_ownDeviceListRequests.remove(QString::number(account) + "-" + xml.attribute("id"));
    if (from.isNull() && expectedOwnList) {
        from = ownJid;
    }
    if (from.isNull()) {
        return false;
    }

    QSet<uint32_t>          actualIds;
    QMap<uint32_t, QString> deviceLabels;

    if (xml.nodeName() == "message" && xml.attribute("type") == "headline") {
        QDomElement event = xml.firstChildElement("event");
        if (event.isNull() || event.namespaceURI() != "http://jabber.org/protocol/pubsub#event") {
            return false;
        }

        QDomElement items = event.firstChildElement("items");
        if (items.isNull() || items.attribute("node") != deviceListNodeName()) {
            return false;
        }

        QDomElement deviceElement
            = items.firstChildElement("item").firstChildElement("list").firstChildElement("device");
        while (!deviceElement.isNull()) {
            const uint32_t id = deviceElement.attribute("id").toUInt();
            actualIds.insert(id);
            if (!deviceElement.attribute("label").isEmpty()) {
                deviceLabels[id] = deviceElement.attribute("label");
            }
            deviceElement = deviceElement.nextSiblingElement("device");
        }
    } else if (xml.nodeName() != "iq" || xml.attribute("type") != "error" || !expectedOwnList) {
        return false;
    }

    std::shared_ptr<Signal> signal = getSignal(account);
    if (ownJid == from) {
        if (!actualIds.contains(signal->getDeviceId())) {
            actualIds.insert(signal->getDeviceId());
            publishDeviceList(account, actualIds);
            publishOwnBundle(account);
        }
    }

    signal->updateDeviceList(from, actualIds, deviceLabels);
    emit deviceListUpdated(account);

    processUnknownDevices(account, ownJid, from);

    return true;
}

void OMEMO::processUndecidedDevices(int account, const QString &ownJid, const QString &user)
{
    std::shared_ptr<Signal> signal = getSignal(account);
    signal->processUndecidedDevices(user, false, m_trustNewContactDevices);
    signal->processUndecidedDevices(ownJid, true, m_trustNewOwnDevices);
}

void OMEMO::processUnknownDevices(int account, const QString &ownJid, const QString &user)
{
    const QSet<uint32_t> devices = getSignal(account)->getUnknownDevices(user);
    if (devices.isEmpty())
        return;

    std::shared_ptr<MessageWaitingForBundles> message(new MessageWaitingForBundles);
    for (auto deviceId : devices) {
        QString stanza = pepRequest(account, ownJid, user, bundleNodeName(deviceId));
        message->sentStanzas.insert(stanza, deviceId);
    }
    m_pendingMessages.append(message);
}

void OMEMO::publishDeviceList(int account, const QSet<uint32_t> &devices) const
{
    QDomDocument doc;
    QDomElement  publish = doc.createElement("publish");
    doc.appendChild(publish);

    QDomElement item = doc.createElement("item");
    publish.appendChild(item);

    QDomElement list = doc.createElementNS(k_omemoXmlns, "list");
    item.appendChild(list);

    publish.setAttribute("node", deviceListNodeName());

    for (auto id : devices) {
        QDomElement device = doc.createElement("device");
        device.setAttribute("id", id);
        list.appendChild(device);
    }

    pepPublish(account, doc.toString());
}

void OMEMO::buildSessionsFromBundle(const QMap<QString, QVector<uint32_t>> &recipientInvalidSessions,
                                    const QVector<uint32_t> &ownInvalidSessions, const QString &ownJid, int account,
                                    const QDomElement &messageToResend)
{
    std::shared_ptr<MessageWaitingForBundles> message(new MessageWaitingForBundles);
    message->xml = messageToResend;

    auto requestBundle = [&](uint32_t deviceId, const QString &recipient) {
        QString stanza = pepRequest(account, ownJid, recipient, bundleNodeName(deviceId));
        message->sentStanzas.insert(stanza, deviceId);
    };

    for (auto recipient : recipientInvalidSessions.keys()) {
        QString bareRecipient = recipient.split("/").first();
        for (auto deviceId : recipientInvalidSessions.value(recipient)) {
            requestBundle(deviceId, bareRecipient);
        }
    }
    for (auto deviceId : ownInvalidSessions) {
        requestBundle(deviceId, ownJid);
    }

    m_pendingMessages.append(message);
}

bool OMEMO::processBundle(const QString &ownJid, int account, const QDomElement &xml)
{
    QString stanzaId = xml.attribute("id");
    if (stanzaId.isNull())
        return false;

    std::shared_ptr<MessageWaitingForBundles> message;
    for (auto msg : m_pendingMessages) {
        if (msg->sentStanzas.contains(stanzaId)) {
            message = msg;
            break;
        }
    }
    if (message == nullptr) {
        return false;
    }

    Bundle bundle;

    QDomElement items    = xml.firstChildElement("pubsub").firstChildElement("items");
    uint32_t    deviceId = message->sentStanzas.value(stanzaId);
    message->sentStanzas.remove(stanzaId);

    if (xml.firstChildElement("error").isNull()) {
        QString     from          = xml.attribute("from");
        QDomElement bundleElement = items.firstChildElement("item").firstChildElement("bundle");

        QDomElement signedPreKeyPublic = bundleElement.firstChildElement("signedPreKeyPublic");
        bundle.signedPreKeyId          = signedPreKeyPublic.attribute("signedPreKeyId").toUInt();
        bundle.signedPreKeyPublic      = QByteArray::fromBase64(signedPreKeyPublic.firstChild().nodeValue().toUtf8());
        bundle.signedPreKeySignature   = QByteArray::fromBase64(
            bundleElement.firstChildElement("signedPreKeySignature").firstChild().nodeValue().toUtf8());
        bundle.identityKeyPublic
            = QByteArray::fromBase64(bundleElement.firstChildElement("identityKey").firstChild().nodeValue().toUtf8());

        QDomElement prekey = bundleElement.firstChildElement("prekeys").firstChildElement("preKeyPublic");
        while (!prekey.isNull()) {
            uint32_t   preKeyId = prekey.attribute("preKeyId").toUInt();
            QByteArray key      = QByteArray::fromBase64(prekey.firstChild().nodeValue().toUtf8());
            bundle.preKeys.append(qMakePair(preKeyId, key));

            prekey = prekey.nextSiblingElement("preKeyPublic");
        }

        if (bundle.isValid()) {
            getSignal(account)->processBundle(from, deviceId, bundle);
        }
    }

    if (message->sentStanzas.isEmpty()) {
        QDomElement messageXml = message->xml;
        if (!messageXml.isNull()) { // it's null is account was disconnected in the middle
            if (!messageXml.hasAttribute("id")) {
                messageXml.setAttribute("id", m_stanzaSender->uniqueId(account));
            }
            encryptMessage(ownJid, account, messageXml, false,
                           messageXml.firstChildElement("body").isNull() ? &deviceId : nullptr);
            m_stanzaSender->sendStanza(account, messageXml);
        }
        m_pendingMessages.removeOne(message);
    }

    return true;
}

QString OMEMO::pepRequest(int account, const QString &ownJid, const QString &recipient, const QString &node) const
{
    const QString &&item     = QString("<items node='%1'/>").arg(node);
    const QString &&stanzaId = m_stanzaSender->uniqueId(account);
    const QString &&stanza   = QString("<iq id='%1' from='%2' to='%3' type='get'>\n"
                                     "<pubsub xmlns='http://jabber.org/protocol/pubsub'>\n"
                                     "%4\n"
                                     "</pubsub>\n"
                                     "</iq>\n")
                                 .arg(stanzaId)
                                 .arg(ownJid)
                                 .arg(recipient)
                                 .arg(item);

    m_stanzaSender->sendStanza(account, stanza);
    return stanzaId;
}

void OMEMO::pepPublish(int account, const QString &dl_xml) const
{
    QString stanza = QString("<iq id='%1' type='set'>\n"
                             "<pubsub xmlns='http://jabber.org/protocol/pubsub'>\n"
                             "%2\n"
                             "</pubsub>\n"
                             "</iq>\n")
                         .arg(m_stanzaSender->uniqueId(account))
                         .arg(dl_xml);

    m_stanzaSender->sendStanza(account, stanza);
}

void OMEMO::pepUnpublish(int account, const QString &node) const
{
    QString stanza = QString("<iq id='%1' type='set'>"
                             "<pubsub xmlns='http://jabber.org/protocol/pubsub#owner'>"
                             "<delete node='%2'/>"
                             "</pubsub>"
                             "</iq>")
                         .arg(m_stanzaSender->uniqueId(account))
                         .arg(node);

    m_stanzaSender->sendStanza(account, stanza);
}

void OMEMO::setNodeText(QDomElement &node, const QByteArray &byteArray) const
{
    QByteArray array = byteArray.toBase64();
    node.appendChild(node.ownerDocument().createTextNode(array));
}

void OMEMO::setStanzaSender(StanzaSendingHost *stanzaSender) { m_stanzaSender = stanzaSender; }

void OMEMO::setAccountController(PsiAccountControllingHost *accountController)
{
    m_accountController = accountController;
}

void OMEMO::setAccountInfoAccessor(AccountInfoAccessingHost *accountInfoAccessor)
{
    m_accountInfoAccessor = accountInfoAccessor;
}

void OMEMO::setContactInfoAccessor(ContactInfoAccessingHost *contactInfoAccessor)
{
    m_contactInfoAccessor = contactInfoAccessor;
}

bool OMEMO::appendSysMsg(int account, const QString &jid, const QString &message)
{
    return m_accountController->appendSysMsg(account, jid, message);
}

const QString OMEMO::bundleNodeName(uint32_t deviceId) const
{
    static const QString substr = k_omemoXmlns + ".bundles:";
    return substr + QString::number(deviceId);
}

const QString OMEMO::deviceListNodeName() const
{
    static const QString out = k_omemoXmlns + ".devicelist";
    return out;
}

bool OMEMO::isAvailableForUser(int account, const QString &user)
{
    return getSignal(account)->isAvailableForUser(user);
}

bool OMEMO::isAvailableForGroup(int account, const QString &ownJid, const QString &bareJid)
{
    bool any    = false;
    bool result = forEachMucParticipant(account, ownJid, bareJid, [&](const QString &userJid) {
        any = true;
        if (!isAvailableForUser(account, userJid)) {
            if (isEnabledForUser(account, bareJid)) {
                QString message = "[OMEMO] "
                    + tr("%1 does not seem to support OMEMO, disabling for the entire group!").arg(userJid);
                appendSysMsg(account, bareJid, message);
                setEnabledForUser(account, bareJid, false);
            }
            return false;
        }
        return true;
    });

    return any && result;
}

bool OMEMO::isEnabledForUser(int account, const QString &user)
{
    if (m_alwaysEnabled)
        return true;

    if (m_enabledByDefault)
        return !getSignal(account)->isDisabledForUser(user);

    return getSignal(account)->isEnabledForUser(user);
}

void OMEMO::setEnabledForUser(int account, const QString &user, bool value)
{
    if (m_enabledByDefault) {
        getSignal(account)->setDisabledForUser(user, !value);
    } else {
        getSignal(account)->setEnabledForUser(user, value);
    }
}

uint32_t OMEMO::getDeviceId(int account) { return getSignal(account)->getDeviceId(); }

QString OMEMO::getOwnFingerprint(int account) { return getSignal(account)->getOwnFingerprint(); }

QMap<uint32_t, QString> OMEMO::getOwnFingerprintsMap(int account)
{
    return getSignal(account)->getFingerprintsMap(m_accountInfoAccessor->getJid(account));
}

QSet<uint32_t> OMEMO::getOwnDevicesList(int account)
{
    return getSignal(account)->getDeviceList(m_accountInfoAccessor->getJid(account));
}

QList<Fingerprint> OMEMO::getKnownFingerprints(int account) { return getSignal(account)->getKnownFingerprints(); }

bool OMEMO::removeDevice(int account, const QString &user, uint32_t deviceId)
{
    return getSignal(account)->removeDevice(user, deviceId);
}

void OMEMO::confirmDeviceTrust(int account, const QString &user, uint32_t deviceId)
{
    getSignal(account)->confirmDeviceTrust(user, deviceId);
}

void OMEMO::revokeDeviceTrust(int account, const QString &user, uint32_t deviceId)
{
    getSignal(account)->revokeDeviceTrust(user, deviceId);
}

void OMEMO::unpublishDevice(int account, uint32_t deviceId)
{
    pepUnpublish(account, bundleNodeName(deviceId));

    QSet<uint32_t> devices = getOwnDevicesList(account);
    devices.remove(deviceId);
    publishDeviceList(account, devices);
}

void OMEMO::deleteCurrentDevice(int account, uint32_t deviceId)
{
    QSet<uint32_t> devices = getOwnDevicesList(account);
    devices.remove(deviceId);

    getSignal(account)->removeCurrentDevice();
    m_accountToSignal.remove(account);

    uint32_t newDeviceId = getSignal(account)->getDeviceId();
    devices.insert(newDeviceId);

    pepUnpublish(account, bundleNodeName(deviceId));
    publishOwnBundle(account);
    publishDeviceList(account, devices);
}

void OMEMO::setAlwaysEnabled(const bool value) { m_alwaysEnabled = value; }

void OMEMO::setEnabledByDefault(const bool value) { m_enabledByDefault = value; }

void OMEMO::setTrustNewOwnDevices(const bool value) { m_trustNewOwnDevices = value; }

void OMEMO::setTrustNewContactDevices(const bool value) { m_trustNewContactDevices = value; }

bool OMEMO::isAlwaysEnabled() const { return m_alwaysEnabled; }

bool OMEMO::isEnabledByDefault() const { return m_enabledByDefault; }

bool OMEMO::trustNewOwnDevices() const { return m_trustNewOwnDevices; }

bool OMEMO::trustNewContactDevices() const { return m_trustNewContactDevices; }

std::shared_ptr<Signal> OMEMO::getSignal(int account)
{
    if (!m_accountToSignal.contains(account)) {
        std::shared_ptr<Signal> signal(new Signal);
        QString                 accountId = m_accountInfoAccessor->getId(account).replace('{', "").replace('}', "");
        signal->init(m_dataPath, accountId);
        m_accountToSignal[account] = signal;
    }
    return m_accountToSignal[account];
}

template <typename T>
bool OMEMO::forEachMucParticipant(int account, const QString &ownJid, const QString &conferenceJid, T &&lambda)
{
    QStringList list;

    for (auto nick : m_contactInfoAccessor->mucNicks(account, conferenceJid)) {
        QString contactMucJid = conferenceJid + "/" + nick;
        QString realJid       = m_contactInfoAccessor->realJid(account, contactMucJid);
        if (realJid == contactMucJid) {
            // a contact does not have a real JID, give up
            return false;
        }
        QString bareJid = realJid.split("/").first();
        if (bareJid != ownJid) {
            list.append(bareJid);
        }
    }

    for (auto jid : list) {
        if (!lambda(jid)) {
            return false;
        }
    }
    return true;
}
}
