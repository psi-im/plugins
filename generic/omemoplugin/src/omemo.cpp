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

#include <QtXml>
#include "omemo.h"
#include "crypto.h"

#define OMEMO_XMLNS "eu.siacs.conversations.axolotl"

namespace psiomemo {
  void OMEMO::init(const QString &dataPath) {
    m_dataPath = dataPath;
  }

  void OMEMO::deinit() {
    foreach (auto signal, m_accountToSignal.values()) {
      signal->deinit();
    }
  }

  void OMEMO::accountConnected(int account, const QString &ownJid) {
    QString stanzaId = pepRequest(account, ownJid, ownJid, deviceListNodeName());
    m_ownDeviceListRequests.insert(QString::number(account) + "-" + stanzaId);
  }

  void OMEMO::publishOwnBundle(int account) {
    Bundle b = getSignal(account)->collectBundle();
    if (!b.isValid()) return;

    QDomDocument doc;
    QDomElement publish = doc.createElement("publish");
    doc.appendChild(publish);

    QDomElement item = doc.createElement("item");
    publish.appendChild(item);

    QDomElement bundle = doc.createElementNS(OMEMO_XMLNS, "bundle");
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

    foreach (auto preKey, b.preKeys) {
      QDomElement preKeyPublic = doc.createElement("preKeyPublic");
      preKeyPublic.setAttribute("preKeyId", preKey.first);
      setNodeText(preKeyPublic, preKey.second);
      preKeys.appendChild(preKeyPublic);
    }

    pepPublish(account, doc.toString());
  }

  bool OMEMO::decryptMessage(int account, QDomElement &xml) {
    std::shared_ptr<Signal> signal = getSignal(account);

    bool isCarbon = false;
    QDomElement message = xml;

    if (message.firstChild().toElement().attribute("xmlns") == "urn:xmpp:carbons:2") {
      message = message.firstChild().firstChildElement("forwarded").firstChildElement("message");
      isCarbon = true;
    }

    QDomElement encrypted = message.firstChildElement("encrypted");
    if (encrypted.isNull() || encrypted.attribute("xmlns") != OMEMO_XMLNS) {
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

    QString preKeyAttr = keyElement.attribute("prekey");
    bool isPreKey = preKeyAttr == "true" || preKeyAttr == "1";
    uint32_t preKeyCount = isPreKey ? signal->preKeyCount() : 0;

    QByteArray encryptedKey = QByteArray::fromBase64(keyElement.firstChild().nodeValue().toUtf8());

    uint deviceId = header.attribute("sid").toUInt();
    QString from = message.attribute("from");
    QString sender = m_contactInfoAccessor->realJid(account, from).split("/").first();
    QPair<QByteArray, bool> decryptionResult = signal->decryptKey(sender, EncryptedKey(deviceId, isPreKey, encryptedKey));
    QByteArray decryptedKey = decryptionResult.first;
    bool buildSessionWithPreKey = decryptionResult.second;
    if (buildSessionWithPreKey) {
      // remote has an invalid session, let's recover by overwriting it with a fresh one
      QDomElement emptyMessage = message.cloneNode(false).toElement();
      QString to = message.attribute("to");
      emptyMessage.setAttribute("from", to);
      emptyMessage.setAttribute("to", from);

      auto recipientInvalidSessions = QMap<QString, QVector<uint32_t>>({{to, QVector<uint32_t>({deviceId})}});
      buildSessionsFromBundle(recipientInvalidSessions, QVector<uint32_t>(), to.split("/").first(), account, emptyMessage);
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
          tag = decryptedKey.right(decryptedKey.size() - OMEMO_AES_128_KEY_LENGTH);
          decryptedKey = decryptedKey.left(OMEMO_AES_128_KEY_LENGTH);
        } else {
          tag = payload.right(OMEMO_AES_GCM_TAG_LENGTH);
          payload.chop(OMEMO_AES_GCM_TAG_LENGTH);
        }

        QPair<QByteArray, QByteArray> decryptedBody = Crypto::aes_gcm(Crypto::Decode, iv, decryptedKey, payload, tag);
        if (!decryptedBody.first.isNull()) {
          bool trusted = signal->isTrusted(sender, deviceId);
          message.removeChild(encrypted);
          QDomElement body = message.ownerDocument().createElement("body");
          QString text = decryptedBody.first;

          if (!trusted) {
            bool res = !isCarbon && m_accountController->appendSysMsg(account, message.attribute("from"),
                                                                      "[OMEMO] The following message is from an untrusted device:");
            if (!res) {
              text = "[UNTRUSTED]: " + text;
            }
          }

          body.appendChild(body.ownerDocument().createTextNode(text));
          message.removeChild(message.firstChildElement("body"));
          message.appendChild(body);

          return true;
        }
      }
    }
    xml = QDomElement();
    return true;
  }

  bool OMEMO::encryptMessage(const QString &ownJid, int account, QDomElement &xml, bool buildSessions, const uint32_t *toDeviceId) {
    std::shared_ptr<Signal> signal = getSignal(account);
    QString recipient = m_contactInfoAccessor->realJid(account, xml.attribute("to")).split("/").first();
    bool isGroup = xml.attribute("type") == "groupchat";
    if (!isEnabledForUser(account, recipient)) {
      return false;
    }

    if (buildSessions) {
      QMap<QString, QVector<uint32_t>> invalidSessions;
      QVector<uint32_t> invalidSessionsWithOwnDevices;
      if (isGroup) {
        forEachMucParticipant(account, ownJid, recipient, [&](const QString &userJid) {
          QVector<uint32_t> sessions = signal->invalidSessions(userJid);
          if (!sessions.isEmpty()) {
            invalidSessions.insert(userJid, sessions);
          }
          return true;
        });
      }
      else {
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

    signal->processUndecidedDevices(recipient, false);
    signal->processUndecidedDevices(ownJid, true);

    QDomElement encrypted = xml.ownerDocument().createElementNS(OMEMO_XMLNS, "encrypted");
    QDomElement header = xml.ownerDocument().createElement("header");
    header.setAttribute("sid", signal->getDeviceId());
    encrypted.appendChild(header);
    xml.appendChild(encrypted);

    QByteArray iv = Crypto::randomBytes(OMEMO_AES_GCM_IV_LENGTH);

    QDomElement ivElement = xml.ownerDocument().createElement("iv");
    ivElement.appendChild(xml.ownerDocument().createTextNode(iv.toBase64()));
    header.appendChild(ivElement);

    QByteArray key = Crypto::randomBytes(OMEMO_AES_128_KEY_LENGTH);
    QDomElement body = xml.firstChildElement("body");
    QPair<QByteArray, QByteArray> encryptedBody;
    if (!body.isNull()) {
      QString plainText = body.firstChild().nodeValue();
      encryptedBody = Crypto::aes_gcm(Crypto::Encode, iv, key, plainText.toUtf8());
      key += encryptedBody.second;
    }
    QList<EncryptedKey> encryptedKeys;
    if (isGroup) {
      forEachMucParticipant(account, ownJid, recipient, [&](const QString &userJid) {
        encryptedKeys.append(signal->encryptKey(ownJid, userJid, key));
        return true;
      });
    }
    else {
      encryptedKeys = signal->encryptKey(ownJid, recipient, key);
    }

    if (encryptedKeys.isEmpty()) {
      m_accountController->appendSysMsg(account, xml.attribute("to"), "[OMEMO] Unable to build any sessions, the message was not sent");
      xml = QDomElement();
    }
    else {
      foreach (EncryptedKey encKey, encryptedKeys) {
        if (toDeviceId != nullptr && *toDeviceId != encKey.deviceId) {
          continue;
        }
        QDomElement keyElement = xml.ownerDocument().createElement("key");
        keyElement.setAttribute("rid", encKey.deviceId);
        if (encKey.isPreKey) {
          keyElement.setAttribute("prekey", 1);
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
      encryption.setAttribute("namespace", OMEMO_XMLNS);
      encryption.setAttribute("name", "OMEMO");
      xml.appendChild(encryption);
    }

    return true;
  }

  bool OMEMO::processDeviceList(const QString &ownJid, int account, const QDomElement &xml) {
    QString from = xml.attribute("from");
    bool expectedOwnList = m_ownDeviceListRequests.remove(QString::number(account) + "-" + xml.attribute("id"));
    if (from.isNull() && expectedOwnList) {
      from = ownJid;
    }
    if (from.isNull()) {
      return false;
    }

    QSet<uint32_t> actualIds;

    if (xml.nodeName() == "message" && xml.attribute("type") == "headline") {
      QDomElement event = xml.firstChildElement("event");
      if (event.isNull() || event.attribute("xmlns") != "http://jabber.org/protocol/pubsub#event") {
        return false;
      }

      QDomElement items = event.firstChildElement("items");
      if (items.isNull() || items.attribute("node") != deviceListNodeName()) {
        return false;
      }

      QDomElement deviceElement = items.firstChildElement("item").firstChildElement("list").firstChildElement("device");
      while (!deviceElement.isNull()) {
        actualIds.insert(deviceElement.attribute("id").toUInt());
        deviceElement = deviceElement.nextSiblingElement("device");
      }
    }
    else if (xml.nodeName() != "iq" || xml.attribute("type") != "error" || !expectedOwnList) {
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

    signal->updateDeviceList(from, actualIds);
    emit deviceListUpdated(account);

    return true;
  }

  void OMEMO::publishDeviceList(int account, const QSet<uint32_t> &devices) const {
    QDomDocument doc;
    QDomElement publish = doc.createElement("publish");
    doc.appendChild(publish);

    QDomElement item = doc.createElement("item");
    publish.appendChild(item);

    QDomElement list = doc.createElementNS(OMEMO_XMLNS, "list");
    item.appendChild(list);

    publish.setAttribute("node", deviceListNodeName());

    foreach (uint32_t id, devices) {
      QDomElement device = doc.createElement("device");
      device.setAttribute("id", id);
      list.appendChild(device);
    }

    pepPublish(account, doc.toString());
  }

  void OMEMO::buildSessionsFromBundle(const QMap<QString, QVector<uint32_t>> &recipientInvalidSessions,
                                      const QVector<uint32_t> &ownInvalidSessions,
                                      const QString &ownJid, int account,
                                      const QDomElement &messageToResend) {
    std::shared_ptr<MessageWaitingForBundles> message(new MessageWaitingForBundles);
    message->xml = messageToResend;

    auto requestBundle = [&](uint32_t deviceId, const QString &recipient) {
      QString stanza = pepRequest(account, ownJid, recipient, bundleNodeName(deviceId));
      message->sentStanzas.insert(stanza, deviceId);
    };

    foreach (QString recipient, recipientInvalidSessions.keys()) {
      QString bareRecipient = recipient.split("/").first();
      foreach (uint32_t deviceId, recipientInvalidSessions.value(recipient)) {
        requestBundle(deviceId, bareRecipient);
      }
    }
    foreach (uint32_t deviceId, ownInvalidSessions) {
      requestBundle(deviceId, ownJid);
    }

    m_pendingMessages.append(message);
  }

  bool OMEMO::processBundle(const QString &ownJid, int account, const QDomElement &xml) {
    QString stanzaId = xml.attribute("id");
    if (stanzaId.isNull()) return false;

    std::shared_ptr<MessageWaitingForBundles> message;
    foreach (std::shared_ptr<MessageWaitingForBundles> msg, m_pendingMessages) {
      if (msg->sentStanzas.contains(stanzaId)) {
        message = msg;
        break;
      }
    }
    if (message == nullptr) {
      return false;
    }

    Bundle bundle;

    QDomElement items = xml.firstChildElement("pubsub").firstChildElement("items");
    uint32_t deviceId = message->sentStanzas.value(stanzaId);
    message->sentStanzas.remove(stanzaId);

    if (xml.firstChildElement("error").isNull()) {
      QString from = xml.attribute("from");
      QDomElement bundleElement = items.firstChildElement("item").firstChildElement("bundle");

      QDomElement signedPreKeyPublic = bundleElement.firstChildElement("signedPreKeyPublic");
      bundle.signedPreKeyId = signedPreKeyPublic.attribute("signedPreKeyId").toUInt();
      bundle.signedPreKeyPublic = QByteArray::fromBase64(signedPreKeyPublic.firstChild().nodeValue().toUtf8());
      bundle.signedPreKeySignature = QByteArray::fromBase64(
          bundleElement.firstChildElement("signedPreKeySignature").firstChild().nodeValue().toUtf8());
      bundle.identityKeyPublic = QByteArray::fromBase64(
          bundleElement.firstChildElement("identityKey").firstChild().nodeValue().toUtf8());

      QDomElement prekey = bundleElement.firstChildElement("prekeys").firstChildElement("preKeyPublic");
      while (!prekey.isNull()) {
        uint32_t preKeyId = prekey.attribute("preKeyId").toUInt();
        QByteArray key = QByteArray::fromBase64(prekey.firstChild().nodeValue().toUtf8());
        bundle.preKeys.append(qMakePair(preKeyId, key));

        prekey = prekey.nextSiblingElement("preKeyPublic");
      }

      if (bundle.isValid()) {
        getSignal(account)->processBundle(from, deviceId, bundle);
      }
    }

    if (message->sentStanzas.isEmpty()) {
      QDomElement messageXml = message->xml;
      if (!messageXml.hasAttribute("id")) {
        messageXml.setAttribute("id", m_stanzaSender->uniqueId(account));
      }
      encryptMessage(ownJid, account, messageXml, false, messageXml.firstChildElement("body").isNull() ? &deviceId : nullptr);
      m_stanzaSender->sendStanza(account, messageXml);
      m_pendingMessages.removeOne(message);
    }

    return true;
  }

  QString OMEMO::pepRequest(int account, const QString &ownJid, const QString &recipient, const QString &node) const {
    QString stanzaId = m_stanzaSender->uniqueId(account);
    QString stanza = QString("<iq id='%1' from='%2' to='%3' type='get'>"
                             "<pubsub xmlns='http://jabber.org/protocol/pubsub'>"
                             "<items node='%4'/>"
                             "</pubsub>"
                             "</iq>").arg(stanzaId).arg(ownJid).arg(recipient).arg(node);

    m_stanzaSender->sendStanza(account, stanza);
    return stanzaId;
  }

  void OMEMO::pepPublish(int account, const QString &dl_xml) const {
    QString stanza = QString("<iq id='%1' type='set'>"
                             "<pubsub xmlns='http://jabber.org/protocol/pubsub'>"
                             "%2"
                             "</pubsub>"
                             "</iq>").arg(m_stanzaSender->uniqueId(account)).arg(dl_xml);

    m_stanzaSender->sendStanza(account, stanza);
  }

  void OMEMO::pepUnpublish(int account, const QString &node) const {
    QString stanza = QString("<iq id='%1' type='set'>"
                             "<pubsub xmlns='http://jabber.org/protocol/pubsub#owner'>"
                             "<delete node='%2'/>"
                             "</pubsub>"
                             "</iq>").arg(m_stanzaSender->uniqueId(account)).arg(node);

    m_stanzaSender->sendStanza(account, stanza);
  }

  void OMEMO::setNodeText(QDomElement &node, const QByteArray &byteArray) const {
    QByteArray array = byteArray.toBase64();
    node.appendChild(node.ownerDocument().createTextNode(array));
  }

  void OMEMO::setStanzaSender(StanzaSendingHost *stanzaSender) {
    m_stanzaSender = stanzaSender;
  }

  void OMEMO::setAccountController(PsiAccountControllingHost *accountController) {
    m_accountController = accountController;
  }

  void OMEMO::setAccountInfoAccessor(AccountInfoAccessingHost *accountInfoAccessor) {
    m_accountInfoAccessor = accountInfoAccessor;
  }

  void OMEMO::setContactInfoAccessor(ContactInfoAccessingHost *contactInfoAccessor) {
    m_contactInfoAccessor = contactInfoAccessor;
  }

  const QString OMEMO::bundleNodeName(uint32_t deviceId) const {
    return QString("%1.bundles:%2").arg(OMEMO_XMLNS).arg(deviceId);
  }

  const QString OMEMO::deviceListNodeName() const {
    return QString(OMEMO_XMLNS) + ".devicelist";
  }

  bool OMEMO::isAvailableForUser(int account, const QString &user) {
    return getSignal(account)->isAvailableForUser(user);
  }

  bool OMEMO::isAvailableForGroup(int account, const QString &ownJid, const QString &bareJid) {
    bool any = false;
    bool result = forEachMucParticipant(account, ownJid, bareJid, [&](const QString &userJid) {
      any = true;
      if (!isAvailableForUser(account, userJid)) {
        if (isEnabledForUser(account, bareJid)) {
          QString message = QString("[OMEMO] %1 does not seem to support OMEMO, disabling for the entire group!").arg(userJid);
          m_accountController->appendSysMsg(account, bareJid, message);
          setEnabledForUser(account, bareJid, false);
        }
        return false;
      }
      return true;
    });

    return any && result;
  }

  bool OMEMO::isEnabledForUser(int account, const QString &user) {
    return getSignal(account)->isEnabledForUser(user);
  }

  void OMEMO::setEnabledForUser(int account, const QString &user, bool enabled) {
    getSignal(account)->setEnabledForUser(user, enabled);
  }

  uint32_t OMEMO::getDeviceId(int account) {
    return getSignal(account)->getDeviceId();
  }

  QString OMEMO::getOwnFingerprint(int account) {
    return getSignal(account)->getOwnFingerprint();
  }

  QSet<uint32_t> OMEMO::getOwnDeviceList(int account) {
    return getSignal(account)->getDeviceList(m_accountInfoAccessor->getJid(account));
  }

  QList<Fingerprint> OMEMO::getKnownFingerprints(int account) {
    return getSignal(account)->getKnownFingerprints();
  }

  void OMEMO::confirmDeviceTrust(int account, const QString &user, uint32_t deviceId) {
    getSignal(account)->confirmDeviceTrust(user, deviceId, true);
  }

  void OMEMO::unpublishDevice(int account, uint32_t deviceId) {
    pepUnpublish(account, bundleNodeName(deviceId));

    QSet<uint32_t> devices = getOwnDeviceList(account);
    devices.remove(deviceId);
    publishDeviceList(account, devices);
  }

  std::shared_ptr<Signal> OMEMO::getSignal(int account) {
    if (!m_accountToSignal.contains(account)) {
      std::shared_ptr<Signal> signal(new Signal);
      QString accountId = m_accountInfoAccessor->getId(account).replace('{', "").replace('}', "");
      signal->init(m_dataPath, accountId);
      m_accountToSignal[account] = signal;
    }
    return m_accountToSignal[account];
  }

  template<typename T>
  bool OMEMO::forEachMucParticipant(int account, const QString &ownJid, const QString &conferenceJid, T&& lambda) {
    QStringList list;

    foreach (QString nick, m_contactInfoAccessor->mucNicks(account, conferenceJid)) {
      QString contactMucJid = conferenceJid + "/" + nick;
      QString realJid = m_contactInfoAccessor->realJid(account, contactMucJid);
      if (realJid == contactMucJid) {
        // a contact does not have a real JID, give up
        return false;
      }
      QString bareJid = realJid.split("/").first();
      if (bareJid != ownJid) {
        list.append(bareJid);
      }
    }

    foreach (QString jid, list) {
      if (!lambda(jid)) {
        return false;
      }
    }
    return true;
  }
}