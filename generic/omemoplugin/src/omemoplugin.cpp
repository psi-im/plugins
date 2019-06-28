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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <QtGui>
#include <QtXml>
#include <QtSql>
#include <QAction>

#include "accountinfoaccessinghost.h"
#include "applicationinfoaccessinghost.h"
#include "stanzasendinghost.h"
#include "eventcreatinghost.h"

#include "omemoplugin.h"
#include "configwidget.h"

namespace psiomemo {
  QString OMEMOPlugin::name() const {
    return "OMEMO Plugin";
  }

  QString OMEMOPlugin::shortName() const {
    return "omemo";
  }

  QString OMEMOPlugin::version() const {
    return "0.1";
  }

  QWidget *OMEMOPlugin::options() {
    return new ConfigWidget(&m_omemo, m_accountInfo);
  }

  QStringList OMEMOPlugin::pluginFeatures() {
    return QStringList(m_omemo.deviceListNodeName() + "+notify");
  }

  bool OMEMOPlugin::enable() {
    if (!Crypto::isSupported()) {
      return false;
    }

    m_omemo.init(m_applicationInfo->appCurrentProfileDir(ApplicationInfoAccessingHost::DataLocation));

    m_enabled = true;
    return true;
  }

  bool OMEMOPlugin::disable() {
    m_enabled = false;
    m_omemo.deinit();

    return true;
  }

  QPixmap OMEMOPlugin::icon() const {
    return getIcon();
  }

  QPixmap OMEMOPlugin::getIcon() const {
    return QPixmap(QGuiApplication::primaryScreen()->devicePixelRatio() >= 2 ? ":/omemoplugin/omemo@2x.png" : ":/omemoplugin/omemo.png");
  }

  void OMEMOPlugin::applyOptions() {

  }

  void OMEMOPlugin::restoreOptions() {

  }

  QString OMEMOPlugin::pluginInfo() {
    return QString("<strong>OMEMO Multi-End Message and Object Encryption</strong><br/>"
                   "Author: Vyacheslav Karpukhin (vyacheslav@karpukhin.com)<br/><br/>"
                   "Credits:<br/>"
                   "<dl>"
                   "<dt>libsignal-protocol-c</dt><dd>Copyright 2015-2016 Open Whisper Systems</dd>"
                   "<dt>OMEMO logo</dt><dd>fiaxh (https://github.com/fiaxh)</dd>"
                   "</dl>"
                   );
  }

  bool OMEMOPlugin::incomingStanza(int account, const QDomElement &xml) {
    if (!m_enabled) {
      return false;
    }

    if (m_omemo.processBundle(m_accountInfo->getJid(account), account, xml)) {
      return true;
    }

    if (m_omemo.processDeviceList(m_accountInfo->getJid(account), account, xml)) {
      updateAction(account, xml.attribute("from"));
      return true;
    }

    if (xml.nodeName() == "presence") {
      QDomNodeList nodes = xml.childNodes();
      for (int i = 0; i < nodes.length(); i++) {
        QDomNode node = nodes.item(i);
        if (node.nodeName() == "x" && node.toElement().namespaceURI() == "http://jabber.org/protocol/muc#user") {
          QString bareJid = xml.attribute("from").split("/").first();
          QTimer::singleShot(0, [=]() { updateAction(account, bareJid); });
          break;
        }
      }
    }

    return false;
  }

  bool OMEMOPlugin::decryptMessageElement(int account, QDomElement &message) {
    if (!m_enabled) {
      return false;
    }

    bool decrypted = m_omemo.decryptMessage(account, message);
    if (!decrypted) {
      return false;
    }

    QString jid = m_contactInfo->realJid(account, message.attribute("from")).split("/").first();
    if (!m_omemo.isEnabledForUser(account, jid)) {
      m_omemo.setEnabledForUser(account, jid, true);
      updateAction(account, jid);
    }

    if (message.firstChildElement("body").firstChild().nodeValue().startsWith("aesgcm://")) {
      processEncryptedFile(account, message);
    }

    return true;
  }

  void OMEMOPlugin::processEncryptedFile(int account, QDomElement &xml) {
    QDomElement body = xml.firstChildElement("body");
    QUrl url(body.firstChild().nodeValue().replace("aesgcm://", "https://"));

    QByteArray keyData = QByteArray::fromHex(url.fragment().toLatin1());
    url.setFragment(QString());

    QDir cacheDir(m_applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation) + "/aesgcm_files");
    if (!cacheDir.exists()) {
      cacheDir.mkpath(".");
    }
    QFile f(cacheDir.filePath(QString::number(qHash(url)) + "_" + url.fileName()));
    QString fileUrl = QUrl::fromLocalFile(f.fileName()).toString();
    if (f.exists()) {
      body.firstChild().setNodeValue(fileUrl);
      return;
    }

    QNetworkReply *reply = m_networkManager.get(QNetworkRequest(url));

    connect(reply, SIGNAL(finished()), SLOT(onFileDownloadFinished()));
    reply->setProperty("keyData", keyData);
    reply->setProperty("account", account);
    reply->setProperty("filePath", f.fileName());

    QDomElement newXml = xml.cloneNode(true).toElement();
    newXml.firstChildElement("body").firstChild().setNodeValue(fileUrl);
    QString string;
    QTextStream stream(&string);
    newXml.save(stream, 0);
    reply->setProperty("xml", string);
  }

  void OMEMOPlugin::onFileDownloadFinished() {
    auto reply = dynamic_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    QByteArray data = reply->readAll();
    QByteArray tag = data.right(OMEMO_AES_GCM_TAG_LENGTH);
    data.chop(OMEMO_AES_GCM_TAG_LENGTH);

    QByteArray keyData = reply->property("keyData").toByteArray();
    QByteArray iv = keyData.left(OMEMO_AES_GCM_IV_LENGTH);
    QByteArray key = keyData.right(keyData.size() - OMEMO_AES_GCM_IV_LENGTH);

    QByteArray decrypted = Crypto::aes_gcm(Crypto::Decode, iv, key, data, tag).first;
    if (!decrypted.isNull()) {
      QFile f(reply->property("filePath").toString());
      f.open(QIODevice::WriteOnly);
      f.write(decrypted);
      f.close();

      QDomDocument doc;
      doc.setContent(reply->property("xml").toString());
      QDomElement xml = doc.firstChild().toElement();
      m_eventCreator->createNewMessageEvent(reply->property("account").toInt(), xml);
    }
  }

  bool OMEMOPlugin::outgoingStanza(int account, QDomElement &xml) {
    if (!m_enabled) {
      return false;
    }

    if (xml.nodeName() == "presence" && !xml.hasAttributes()) {
      m_omemo.accountConnected(account, m_accountInfo->getJid(account));
    }

    return false;
  }

  bool OMEMOPlugin::encryptMessageElement(int account, QDomElement &message) {
    if (!m_enabled) {
      return false;
    }

    if (message.firstChildElement("body").isNull() || !message.firstChildElement("encrypted").isNull()) {
      return false;
    }

    return m_omemo.encryptMessage(m_accountInfo->getJid(account), account, message);
  }

  void OMEMOPlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost *host) {
    m_accountInfo = host;
    m_omemo.setAccountInfoAccessor(host);
  }

  void OMEMOPlugin::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host) {
    m_applicationInfo = host;
  }

  void OMEMOPlugin::setStanzaSendingHost(StanzaSendingHost *host) {
    m_omemo.setStanzaSender(host);
  }

  void OMEMOPlugin::setEventCreatingHost(EventCreatingHost *host) {
    m_eventCreator = host;
  }

  void OMEMOPlugin::setPsiAccountControllingHost(PsiAccountControllingHost *host) {
    m_omemo.setAccountController(host);
  }

  void OMEMOPlugin::setContactInfoAccessingHost(ContactInfoAccessingHost *host) {
    m_contactInfo = host;
    m_omemo.setContactInfoAccessor(host);
  }

  void OMEMOPlugin::onEnableOMEMOAction(bool checked) {
    auto action = dynamic_cast<QAction*>(sender());
    QString jid = action->property("jid").toString();
    int account = action->property("account").toInt();
    m_omemo.setEnabledForUser(account, jid, checked);
    updateAction(account, jid);
  }

  QList<QVariantHash> OMEMOPlugin::getButtonParam() {
    return QList<QVariantHash>();
  }

  QAction *OMEMOPlugin::getAction(QObject *parent, int account, const QString &contact) {
    return createAction(parent, account, contact, false);
  }

  QAction *OMEMOPlugin::createAction(QObject *parent, int account, const QString &contact, bool isGroup) {
    QString bareJid = m_contactInfo->realJid(account, contact).split("/").first();
    QAction *action = new QAction(getIcon(), "Enable OMEMO", parent);
    action->setCheckable(true);
    action->setProperty("isGroup", isGroup);
    connect(action, SIGNAL(triggered(bool)), SLOT(onEnableOMEMOAction(bool)));
    connect(action, SIGNAL(destroyed(QObject*)), SLOT(onActionDestroyed(QObject*)));
    m_actions.insert(bareJid, action);
    updateAction(account, bareJid);
    return action;
  }

  QList<QVariantHash> OMEMOPlugin::getGCButtonParam() {
    return getButtonParam();
  }

  QAction *OMEMOPlugin::getGCAction(QObject *parent, int account, const QString &contact) {
    return createAction(parent, account, contact, true);
  }

  void OMEMOPlugin::onActionDestroyed(QObject *action) {
    m_actions.remove(action->property("jid").toString(), reinterpret_cast<QAction*>(action));
  }

  void OMEMOPlugin::updateAction(int account, const QString &user) {
    QString bareJid = m_contactInfo->realJid(account, user).split("/").first();
    foreach (QAction *action, m_actions.values(bareJid)) {
      bool isGroup = action->property("isGroup").toBool();
      bool available = isGroup ?
              m_omemo.isAvailableForGroup(account, m_accountInfo->getJid(account).split("/").first(), bareJid) :
              m_omemo.isAvailableForUser(account, bareJid);
      bool enabled = available && m_omemo.isEnabledForUser(account, bareJid);
      action->setEnabled(available);
      action->setChecked(enabled);
      action->setProperty("jid", bareJid);
      action->setProperty("account", account);
      action->setText(!available ? (QString("OMEMO is not available for this ") + (isGroup ? "group" : "contact")) :
                      enabled ? "OMEMO is enabled" : "Enable OMEMO");
    }
  }

    bool OMEMOPlugin::execute(int account, const QHash<QString, QVariant> &args, QHash<QString, QVariant> *result) {
      if (!m_enabled) {
        return false;
      }

      if (args.contains("is_enabled_for")) {
        return m_omemo.isEnabledForUser(account, m_contactInfo->realJid(account, args["is_enabled_for"].toString()).split("/").first());
      }
      else if (args.contains("encrypt_data")) {
        QByteArray data = args["encrypt_data"].toByteArray();
        QByteArray iv = Crypto::randomBytes(OMEMO_AES_GCM_IV_LENGTH);
        QByteArray key = Crypto::randomBytes(32);

        QPair<QByteArray, QByteArray> encResult = Crypto::aes_gcm(Crypto::Encode, iv, key, data);
        result->insert("data", encResult.first + encResult.second);
        result->insert("anchor", iv + key);

        return true;
      }
      else if (args.contains("encrypt_message")) {
        QString str = args["encrypt_message"].toString();

        QDomDocument doc;
        doc.setContent(str);
        QDomElement xml = doc.firstChild().toElement();

        if (!encryptMessageElement(account, xml)) {
          return false;
        }
        if (xml.isNull()) {
          return true;
        }

        str.clear();
        QTextStream stream(&str);
        xml.save(stream, 0);

        result->insert("message", str);
        
        return true;
      }

      return false;
    }
}
