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
    return new ConfigWidget(&m_omemo);
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

  bool OMEMOPlugin::stanzaWasEncrypted(const QString &stanzaId) {
    if (m_encryptedStanzaIds.contains(stanzaId)) {
      m_encryptedStanzaIds.remove(stanzaId);
      return true;
    }
    return false;
  }

  bool OMEMOPlugin::incomingStanza(int account, const QDomElement &xml) {
    if (!m_enabled) {
      return false;
    }

    if (m_omemo.processBundle(m_accountInfo->getJid(account), account, xml)) {
      return true;
    }

    if (m_omemo.processDeviceList(m_accountInfo->getJid(account), account, xml)) {
      updateAction(xml.attribute("from"));
      return true;
    }

    if (xml.nodeName() != "message") {
      return false;
    }

    QDomElement decrypted = m_omemo.decryptMessage(account, xml);
    if (decrypted.isNull()) {
      return false;
    }

    QString jid = xml.attribute("from").split("/").first();
    if (!m_omemo.isEnabledForUser(jid)) {
      m_omemo.setEnabledForUser(jid, true);
      updateAction(jid);
    }

    if (decrypted.firstChildElement("body").firstChild().nodeValue().startsWith("aesgcm://")) {
      processEncryptedFile(account, decrypted);
    }

    m_encryptedStanzaIds.insert(xml.attribute("id"));
    m_eventCreator->createNewMessageEvent(account, decrypted);

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
      m_encryptedStanzaIds.insert(xml.attribute("id"));
      m_eventCreator->createNewMessageEvent(reply->property("account").toInt(), xml);
    }
  }

  bool OMEMOPlugin::outgoingStanza(int account, QDomElement &xml) {
    if (!m_enabled) {
      return false;
    }

    if (xml.nodeName() == "presence" && xml.attribute("type").isNull()) {
      m_omemo.accountConnected(account, m_accountInfo->getJid(account));
    }

    if (xml.nodeName() != "message" || xml.firstChildElement("body").isNull() ||
        !xml.firstChildElement("encrypted").isNull() || xml.attribute("type") != "chat") {
      return false;
    }

    QString stanzaId = xml.attribute("id");
    bool encrypted = m_omemo.encryptMessage(m_accountInfo->getJid(account), account, xml);
    if (encrypted) {
      m_encryptedStanzaIds.insert(stanzaId);
    }
    return encrypted;
  }

  void OMEMOPlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost *host) {
    m_accountInfo = host;
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

  void OMEMOPlugin::onEnableOMEMOAction(bool checked) {
    auto action = dynamic_cast<QAction*>(sender());
    QString jid = action->property("jid").toString();
    m_omemo.setEnabledForUser(jid, checked);
    updateAction(jid);
  }

  QList<QVariantHash> OMEMOPlugin::getButtonParam() {
    return QList<QVariantHash>();
  }

  QAction *OMEMOPlugin::getAction(QObject *parent, int account, const QString &contact) {
    Q_UNUSED(account);
    QString bareJid = contact.split("/").first();
    QAction *action = new QAction(getIcon(), "Enable OMEMO", parent);
    action->setCheckable(true);
    connect(action, SIGNAL(triggered(bool)), SLOT(onEnableOMEMOAction(bool)));
    connect(action, SIGNAL(destroyed(QObject*)), SLOT(onActionDestroyed(QObject*)));
    m_actions.insert(bareJid, action);
    updateAction(bareJid);
    return action;
  }

  void OMEMOPlugin::onActionDestroyed(QObject *action) {
    m_actions.remove(action->property("jid").toString());
  }

  void OMEMOPlugin::updateAction(const QString &user) {
    QString bareJid = user.split("/").first();
    QAction *action = m_actions.value(bareJid, nullptr);
    if (action != nullptr) {
      bool available = m_omemo.isAvailableForUser(bareJid);
      bool enabled = available && m_omemo.isEnabledForUser(bareJid);
      action->setEnabled(available);
      action->setChecked(enabled);
      action->setProperty("jid", bareJid);
      action->setText(!available ? "OMEMO is not available for this contact" : enabled ? "OMEMO is enabled" : "Enable OMEMO");
    }
  }
}