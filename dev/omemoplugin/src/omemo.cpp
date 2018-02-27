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
    m_signal.init(dataPath);
  }

  void OMEMO::deinit() {
    m_signal.deinit();
  }

  void OMEMO::accountConnected(int account, const QString &ownJid) {
    pepRequest(account, ownJid, ownJid, deviceListNodeName());
  }

  void OMEMO::publishOwnBundle(int account) {
    Bundle b = m_signal.collectBundle();
    if (!b.isValid()) return;

    QDomDocument doc;
    QDomElement publish = doc.createElement("publish");
    doc.appendChild(publish);

    QDomElement item = doc.createElement("item");
    publish.appendChild(item);

    QDomElement bundle = doc.createElementNS(OMEMO_XMLNS, "bundle");
    item.appendChild(bundle);

    publish.setAttribute("node", QString("%1.bundles:%2").arg(OMEMO_XMLNS).arg(m_signal.getDeviceId()));

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

  QDomElement OMEMO::decryptMessage(int account, const QDomElement &xml) {
    QDomElement result;

    QDomElement encrypted = xml.firstChildElement("encrypted");
    if (encrypted.isNull() || encrypted.attribute("xmlns") != OMEMO_XMLNS || xml.attribute("type") != "chat") {
      return result;
    }

    QDomElement header = encrypted.firstChildElement("header");

    QDomElement keyElement = header.firstChildElement("key");
    while (!keyElement.isNull() && keyElement.attribute("rid").toUInt() != m_signal.getDeviceId()) {
      keyElement = keyElement.nextSiblingElement("key");
    }
    if (keyElement.isNull()) {
      return result;
    }

    QString preKeyAttr = keyElement.attribute("prekey");
    bool isPreKey = preKeyAttr == "true" || preKeyAttr == "1";
    uint32_t preKeyCount = isPreKey ? m_signal.preKeyCount() : 0;

    QByteArray encryptedKey = QByteArray::fromBase64(keyElement.firstChild().nodeValue().toUtf8());

    uint deviceId = header.attribute("sid").toUInt();
    QString sender = xml.attribute("from").split("/").first();
    QPair<QByteArray, bool> decryptionResult = m_signal.decryptKey(sender, EncryptedKey(deviceId, isPreKey, encryptedKey));
    QByteArray decryptedKey = decryptionResult.first;
    bool buildSessionWithPreKey = decryptionResult.second;
    if (buildSessionWithPreKey) {
      // remote has an invalid session, let's recover by overwriting it with a fresh one
      QDomElement emptyMessage = xml.cloneNode(false).toElement();
      QString to = xml.attribute("to");
      QString from = xml.attribute("from");
      emptyMessage.setAttribute("from", to);
      emptyMessage.setAttribute("to", from);

      buildSessionsFromBundle(QVector<uint32_t>({deviceId}), to.split("/").first(), account, emptyMessage);
      return result;
    }

    QDomElement payloadElement = encrypted.firstChildElement("payload");
    if (!decryptedKey.isNull()) {
      if (isPreKey && m_signal.preKeyCount() < preKeyCount) {
        publishOwnBundle(account);
      }
      if (!payloadElement.isNull()) {
        QByteArray payload(QByteArray::fromBase64(payloadElement.firstChild().nodeValue().toUtf8()));
        QCA::InitializationVector iv(QByteArray::fromBase64(header.firstChildElement("iv").firstChild().nodeValue().toUtf8()));
        QCA::AuthTag tag(OMEMO_AES_GCM_TAG_LENGTH);

        if (decryptedKey.size() > OMEMO_AES_128_KEY_LENGTH) {
          tag = decryptedKey.right(decryptedKey.size() - OMEMO_AES_128_KEY_LENGTH);
          decryptedKey = decryptedKey.left(OMEMO_AES_128_KEY_LENGTH);
        } else {
          tag = payload.right(OMEMO_AES_GCM_TAG_LENGTH);
          payload.chop(OMEMO_AES_GCM_TAG_LENGTH);
        }

        QPair<QByteArray, QCA::AuthTag> decryptedBody = Crypto::aes_gcm(QCA::Decode, iv, decryptedKey, payload, tag);
        if (!decryptedBody.first.isNull() && tag == decryptedBody.second) {
          bool trusted = m_signal.isTrusted(sender, deviceId);
          QDomNode decrypted = xml.cloneNode(true);
          decrypted.removeChild(decrypted.firstChildElement("encrypted"));
          QDomElement body = decrypted.ownerDocument().createElement("body");
          QString text = decryptedBody.first;

          if (!trusted) {
            bool res = m_accountController->appendSysMsg(account, sender, "[OMEMO] The following message is from an untrusted device:");
            if (!res) {
              text = "[UNTRUSTED]: " + text;
            }
          }

          body.appendChild(body.ownerDocument().createTextNode(text));
          decrypted.appendChild(body);

          return decrypted.toElement();
        }
      }
    }
    return result;
  }

  bool OMEMO::encryptMessage(const QString &ownJid, int account, QDomElement &xml, bool buildSessions, const uint32_t *toDeviceId) {
    QString recipient = xml.attribute("to").split("/").first();
    if (!isEnabledForUser(recipient)) {
      return false;
    }

    if (buildSessions) {
      QVector<uint32_t> invalidSessions = m_signal.invalidSessions(recipient);
      if (!invalidSessions.isEmpty()) {
        buildSessionsFromBundle(invalidSessions, ownJid, account, xml);
        xml = QDomElement();
        return true;
      }
    }

    m_signal.processUndecidedDevices(recipient, false);
    m_signal.processUndecidedDevices(ownJid, true);

    QDomElement encrypted = xml.ownerDocument().createElementNS(OMEMO_XMLNS, "encrypted");
    QDomElement header = xml.ownerDocument().createElement("header");
    header.setAttribute("sid", m_signal.getDeviceId());
    encrypted.appendChild(header);
    xml.appendChild(encrypted);

    QCA::InitializationVector iv(OMEMO_AES_GCM_IV_LENGTH);

    QDomElement ivElement = xml.ownerDocument().createElement("iv");
    ivElement.appendChild(xml.ownerDocument().createTextNode(iv.toByteArray().toBase64()));
    header.appendChild(ivElement);

    QCA::SymmetricKey key(OMEMO_AES_128_KEY_LENGTH);
    QDomElement body = xml.firstChildElement("body");
    QPair<QByteArray, QCA::AuthTag> encryptedBody;
    if (!body.isNull()) {
      QString plainText = body.firstChild().nodeValue();
      encryptedBody = Crypto::aes_gcm(QCA::Encode, iv, key, plainText.toUtf8());
      key += encryptedBody.second;
    }
    QList<EncryptedKey> encryptedKeys = m_signal.encryptKey(ownJid, recipient, key);

    if (encryptedKeys.isEmpty()) {
      m_accountController->appendSysMsg(account, recipient, "[OMEMO] Unable to build any sessions, the message was not sent");
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
        xml.removeChild(body);

        QDomElement payload = xml.ownerDocument().createElement("payload");
        payload.appendChild(xml.ownerDocument().createTextNode(encryptedBody.first.toBase64()));
        encrypted.appendChild(payload);
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
    else if (xml.nodeName() != "iq" || xml.attribute("type") != "error" || from != ownJid ||
             xml.firstChildElement("pubsub").firstChildElement("items").attribute("node") != deviceListNodeName()) {
      return false;
    }

    if (ownJid == from) {
      if (!actualIds.contains(m_signal.getDeviceId())) {
        actualIds.insert(m_signal.getDeviceId());

        QDomDocument doc;
        QDomElement publish = doc.createElement("publish");
        doc.appendChild(publish);

        QDomElement item = doc.createElement("item");
        publish.appendChild(item);

        QDomElement list = doc.createElementNS(OMEMO_XMLNS, "list");
        item.appendChild(list);

        publish.setAttribute("node", deviceListNodeName());

        foreach (uint32_t id, actualIds) {
          QDomElement device = doc.createElement("device");
          device.setAttribute("id", id);
          list.appendChild(device);
        }

        pepPublish(account, doc.toString());
        publishOwnBundle(account);
      }
    }

    m_signal.updateDeviceList(from, actualIds);

    return true;
  }

  void OMEMO::buildSessionsFromBundle(const QVector<uint32_t> &invalidSessions, const QString &ownJid, int account,
                                      const QDomElement &messageToResend) {
    QTextStream stream;
    stream.setString(new QString());
    messageToResend.save(stream, 0);

    MessageWaitingForBundles message;
    message.xml = *stream.string();

    QString recipient = messageToResend.attribute("to").split("/").first();

    foreach (uint32_t deviceId, invalidSessions) {
      QString stanza = pepRequest(account, ownJid, recipient, bundleNodeName(deviceId));
      message.sentStanzas.insert(stanza);
      message.pendingBundles.insert(deviceId);
    }
    m_pendingMessages.append(message);
  }

  bool OMEMO::processBundle(const QString &ownJid, int account, const QDomElement &xml) {
    QString stanzaId = xml.attribute("id");
    if (stanzaId.isNull()) return false;

    MessageWaitingForBundles *message = nullptr;
    foreach (MessageWaitingForBundles msg, m_pendingMessages) {
      if (msg.sentStanzas.contains(stanzaId)) {
        message = &msg;
        break;
      }
    }
    if (message == nullptr) {
      return false;
    }

    Bundle bundle;

    QDomElement items = xml.firstChildElement("pubsub").firstChildElement("items");
    uint32_t deviceId = items.attribute("node").split(':').last().toUInt();
    message->pendingBundles.remove(deviceId);

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
        m_signal.processBundle(from, deviceId, bundle);
      }
    }

    if (message->pendingBundles.isEmpty()) {
      QDomDocument xmlDoc;
      xmlDoc.setContent(message->xml);
      QDomElement messageXml = xmlDoc.firstChild().toElement();
      if (!messageXml.hasAttribute("id")) {
        messageXml.setAttribute("id", m_stanzaSender->uniqueId(account));
      }
      encryptMessage(ownJid, account, messageXml, false, messageXml.firstChildElement("body").isNull() ? &deviceId : nullptr);
      m_stanzaSender->sendStanza(account, messageXml);
      m_pendingMessages.removeOne(*message);
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

  const QString OMEMO::bundleNodeName(uint32_t deviceId) const {
    return QString("%1.bundles:%2").arg(OMEMO_XMLNS).arg(deviceId);
  }

  const QString OMEMO::deviceListNodeName() const {
    return QString(OMEMO_XMLNS) + ".devicelist";
  }

  bool OMEMO::isAvailableForUser(const QString &user) {
    return m_signal.isAvailableForUser(user);
  }

  bool OMEMO::isEnabledForUser(const QString &user) {
    return m_signal.isEnabledForUser(user);
  }

  void OMEMO::setEnabledForUser(const QString &user, bool enabled) {
    m_signal.setEnabledForUser(user, enabled);
  }

  uint32_t OMEMO::getDeviceId() {
    return m_signal.getDeviceId();
  }

  QString OMEMO::getOwnFingerprint() {
    return m_signal.getOwnFingerprint();
  }

  QList<Fingerprint> OMEMO::getKnownFingerprints() {
    return m_signal.getKnownFingerprints();
  }

  void OMEMO::confirmDeviceTrust(const QString &user, uint32_t deviceId) {
    m_signal.confirmDeviceTrust(user, deviceId, true);
  }
}