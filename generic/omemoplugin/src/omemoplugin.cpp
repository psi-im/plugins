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

#include <QAction>
#include <QMenu>
#include <QScreen>
#include <QtGui>
#include <QtSql>
#include <QtXml>

#include "accountinfoaccessinghost.h"
#include "applicationinfoaccessinghost.h"
#include "configwidget.h"
#include "eventcreatinghost.h"
#include "stanzasendinghost.h"

#include "configwidget.h"
#include "omemoplugin.h"

namespace psiomemo {

QString OMEMOPlugin::name() const { return "OMEMO Plugin"; }

QWidget *OMEMOPlugin::options()
{
    if (!m_enabled)
        return nullptr;

    auto configWidget = new ConfigWidget(m_omemo, m_accountInfo);
    connect(this, &OMEMOPlugin::applyPluginSettings, configWidget, &ConfigWidget::applySettings);
    return configWidget;
}

QStringList OMEMOPlugin::pluginFeatures()
{
    if (!m_enabled)
        return QStringList();
    return QStringList(m_omemo->deviceListNodeName() + "+notify");
}

bool OMEMOPlugin::enable()
{
    if (m_enabled)
        return true;

    if (!(Crypto::isSupported() && m_accountInfo && m_stanzaSender && m_accountController && m_contactInfo
          && m_optionHost)) {
        return false;
    }
    m_omemo = new OMEMO();
    m_omemo->setAccountInfoAccessor(m_accountInfo);
    m_omemo->setStanzaSender(m_stanzaSender);
    m_omemo->setAccountController(m_accountController);
    m_omemo->setContactInfoAccessor(m_contactInfo);

    bool firstCall = (m_optionHost == nullptr);
    if (firstCall)
        optionChanged(QString());

    connect(m_omemo, &OMEMO::saveSettings, this, &OMEMOPlugin::savePluginOptions);

    m_omemo->init(m_applicationInfo->appCurrentProfileDir(ApplicationInfoAccessingHost::DataLocation));

    m_enabled = true;
    return true;
}

bool OMEMOPlugin::disable()
{
    if (!m_enabled)
        return true;

    m_enabled = false;
    m_omemo->deinit();
    delete m_omemo;
    m_omemo = nullptr;

    return true;
}

QPixmap OMEMOPlugin::icon() const { return getIcon(); }

QPixmap OMEMOPlugin::getIcon() const
{
    if (QGuiApplication::primaryScreen()->devicePixelRatio() >= 2)
        return QPixmap(":/omemoplugin/omemo@2x.png");

    return QPixmap(":/omemoplugin/omemo.png");
}

void OMEMOPlugin::applyOptions()
{
    if (!m_enabled)
        return;

    applyPluginSettings();
}

void OMEMOPlugin::restoreOptions() { }

QString OMEMOPlugin::pluginInfo()
{
    QString out;
    out += tr("OMEMO is an end-to-end encryption protocol that leverages the "
              "Double Ratchet encryption scheme to provide multi-end to "
              "multi-end encryption, allowing messages to be synchronized "
              "securely across multiple clients, even if some of them are "
              "offline.")
        + "<br/>";
    out += "<br/>";
    out += tr("In comparison with OTR, the OMEMO protocol offers multi-users "
              "encrypted chat, safe message synchronization between user's "
              "devices, offline messages queuing, forward secrecy and file "
              "transfer.")
        + "<br/>";
    out += "<br/>";
    out += tr("OMEMO provides the following guarantees:");
    out += "<dl>";
    out += "<dt>" + tr("Confidentiality") + "</dt>";
    out += "<dd>"
        + tr("Nobody else except sender and receiver is able to read "
             "the content of a message.")
        + "</dd>";
    out += "<dt>" + tr("Perfect forward secrecy") + "</dt>";
    out += "<dd>"
        + tr("Compromised key material does not compromise previous or "
             "future message exchanges.")
        + "</dd>";
    out += "<dt>" + tr("Authentication") + "</dt>";
    out += "<dd>"
        + tr("Every peer is able to authenticate the sender or "
             "receiver of a message.")
        + "</dd>";
    out += "<dt>" + tr("Immutability") + "</dt>";
    out += "<dd>"
        + tr("Every peer can ensure that a message was not changed by "
             "any intermediate node.")
        + "</dd>";
    out += "<dt>" + tr("Plausible deniability") + "</dt>";
    out += "<dd>" + tr("No participant can prove who created a specific message.") + "</dd>";
    out += "<dt>" + tr("Asynchronicity") + "</dt>";
    out += "<dd>"
        + tr("The usability of the protocol does not depend on the "
             "online status of any participant.")
        + "</dd>";
    out += "</dl>";
    out += "<br/>";
    out += tr("OMEMO is not intended to protect against the following use "
              "cases:");
    out += "<dl>";
    out += "<dt>"
        + tr("* An attacker has permanent access to your device. (Temporary "
             "access is covered by perfect forward secrecy.)")
        + "</dt>";
    out += "<dt>"
        + tr("* You lost your device and an attacker can read messages on your "
             "notification screen.")
        + "</dt>";
    out += "<dt>" + tr("* Any kind of denial-of-service attack.") + "</dt>";
    out += "<dt>" + tr("* etc.");
    out += "</dl>";
    out += "<br/>";
    out += tr("Credits: ") + "<br/>";
    out += "* <a href=\"https://github.com/signalapp/libsignal-protocol-c\">"
           "libsignal-protocol-c</a> -- Open Whisper Systems<br/>";
    out += "* OMEMO logo -- <a href=\"https://github.com/fiaxh\">fiaxh</a><br/>";
    return out;
}

bool OMEMOPlugin::incomingStanza(int account, const QDomElement &xml)
{
    if (!m_enabled) {
        return false;
    }

    const auto ownJid = m_accountInfo->getJid(account).split("/").first();

    if (m_omemo->processBundle(ownJid, account, xml)) {
        return true;
    }

    if (m_omemo->processDeviceList(ownJid, account, xml)) {
        updateAction(account, xml.attribute("from"));
        return true;
    }

    if (xml.nodeName() == "presence") {
        QDomNodeList nodes = xml.childNodes();
        for (int i = 0; i < nodes.size(); ++i) {
            QDomNode node = nodes.item(i);
            if (node.nodeName() == "x" && node.toElement().namespaceURI() == "http://jabber.org/protocol/muc#user") {
                const auto bareJid = xml.attribute("from").split("/").first();
                QTimer::singleShot(0, [=]() { updateAction(account, bareJid); });
                break;
            }
        }
    }

    return false;
}

bool OMEMOPlugin::decryptMessageElement(int account, QDomElement &message)
{
    if (!m_enabled) {
        return false;
    }

    bool decrypted = m_omemo->decryptMessage(account, message);
    if (!decrypted) {
        return false;
    }

    QString jid = m_contactInfo->realJid(account, message.attribute("from")).split("/").first();
    if (!m_omemo->isEnabledForUser(account, jid)) {
        m_omemo->setEnabledForUser(account, jid, true);
        updateAction(account, jid);
    }

    if (message.firstChildElement("body").firstChild().nodeValue().startsWith("aesgcm://")) {
        processEncryptedFile(account, message);
    }

    return true;
}

void OMEMOPlugin::processEncryptedFile(int account, QDomElement &xml)
{
    QDomElement body = xml.firstChildElement("body");
    QUrl        url(body.firstChild().nodeValue().replace("aesgcm://", "https://"));

    QByteArray keyData = QByteArray::fromHex(url.fragment().toLatin1());
    url.setFragment(QString());

    QDir cacheDir(m_applicationInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation) + "/aesgcm_files");
    if (!cacheDir.exists()) {
        cacheDir.mkpath(".");
    }
    QFile   f(cacheDir.filePath(QString::number(qHash(url)) + "_" + url.fileName()));
    QString fileUrl = QUrl::fromLocalFile(f.fileName()).toString();
    if (f.exists()) {
        body.firstChild().setNodeValue(fileUrl);
        return;
    }

    QNetworkReply *reply = m_networkManager.get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::finished, this, &OMEMOPlugin::fileDownloadFinished);
    reply->setProperty("keyData", keyData);
    reply->setProperty("account", account);
    reply->setProperty("filePath", f.fileName());

    QDomElement newXml = xml.cloneNode(true).toElement();
    newXml.firstChildElement("body").firstChild().setNodeValue(fileUrl);
    QString     string;
    QTextStream stream(&string);
    newXml.save(stream, 0);
    reply->setProperty("xml", string);
}

void OMEMOPlugin::showOwnFingerprint(int account, const QString &jid)
{
    const QString &&msg = tr("Fingerprint for account \"%1\": %2")
                              .arg(m_accountInfo->getName(account))
                              .arg(m_omemo->getOwnFingerprint(account));
    m_omemo->appendSysMsg(account, jid, msg);
}

void OMEMOPlugin::fileDownloadFinished()
{
    auto reply = dynamic_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    QByteArray data = reply->readAll();
    QByteArray tag  = data.right(OMEMO_AES_GCM_TAG_LENGTH);
    data.chop(OMEMO_AES_GCM_TAG_LENGTH);

    QByteArray keyData = reply->property("keyData").toByteArray();
    QByteArray iv      = keyData.left(OMEMO_AES_GCM_IV_LENGTH);
    QByteArray key     = keyData.right(keyData.size() - OMEMO_AES_GCM_IV_LENGTH);

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

bool OMEMOPlugin::outgoingStanza(int account, QDomElement &xml)
{
    if (!m_enabled) {
        return false;
    }

    if (xml.nodeName() == "presence" && !xml.hasAttributes()) {
        m_omemo->accountConnected(account, m_accountInfo->getJid(account));
    }

    return false;
}

bool OMEMOPlugin::encryptMessageElement(int account, QDomElement &message)
{
    if (!m_enabled) {
        return false;
    }

    if (message.firstChildElement("body").isNull() || !message.firstChildElement("encrypted").isNull()) {
        return false;
    }

    return m_omemo->encryptMessage(m_accountInfo->getJid(account), account, message);
}

void OMEMOPlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost *host) { m_accountInfo = host; }

void OMEMOPlugin::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host) { m_applicationInfo = host; }

void OMEMOPlugin::setStanzaSendingHost(StanzaSendingHost *host) { m_stanzaSender = host; }

void OMEMOPlugin::setEventCreatingHost(EventCreatingHost *host) { m_eventCreator = host; }

void OMEMOPlugin::setPsiAccountControllingHost(PsiAccountControllingHost *host) { m_accountController = host; }

void OMEMOPlugin::setContactInfoAccessingHost(ContactInfoAccessingHost *host) { m_contactInfo = host; }

void OMEMOPlugin::setOptionAccessingHost(OptionAccessingHost *host) { m_optionHost = host; }

void OMEMOPlugin::optionChanged(const QString &)
{
    if (!m_enabled)
        return;

    if (!m_optionHost)
        return;

    m_omemo->setAlwaysEnabled(m_optionHost->getPluginOption("always-enabled", m_omemo->isAlwaysEnabled()).toBool());
    m_omemo->setEnabledByDefault(
        m_optionHost->getPluginOption("enabled-by-default", m_omemo->isEnabledByDefault()).toBool());
    m_omemo->setTrustNewOwnDevices(
        m_optionHost->getPluginOption("trust-new-own-devices", m_omemo->trustNewOwnDevices()).toBool());
    m_omemo->setTrustNewContactDevices(
        m_optionHost->getPluginOption("trust-new-contact-devices", m_omemo->trustNewContactDevices()).toBool());
}

void OMEMOPlugin::savePluginOptions()
{
    if (!m_enabled)
        return;

    m_optionHost->setPluginOption("always-enabled", QVariant(m_omemo->isAlwaysEnabled()));
    m_optionHost->setPluginOption("enabled-by-default", QVariant(m_omemo->isEnabledByDefault()));
    m_optionHost->setPluginOption("trust-new-own-devices", QVariant(m_omemo->trustNewOwnDevices()));
    m_optionHost->setPluginOption("trust-new-contact-devices", QVariant(m_omemo->trustNewContactDevices()));
}

void OMEMOPlugin::enableOMEMOAction(bool checked)
{
    QAction *action = dynamic_cast<QAction *>(sender());
    if (!action)
        return;

    action->setChecked(!checked);

    QMenu *  menu                  = new QMenu();
    QAction *actEnableOmemo        = new QAction(tr("Enable OMEMO encryption"), this);
    QAction *actDisableOmemo       = new QAction(tr("Disable OMEMO encryption"), this);
    QAction *actManageFingerprints = new QAction(tr("Manage contact fingerprints"), this);
    QAction *actShowOwnFingerprint = new QAction(tr("Show own &fingerprint"), this);

    actEnableOmemo->setVisible(checked);
    actDisableOmemo->setVisible(!checked);

    menu->addAction(actEnableOmemo);
    menu->addAction(actDisableOmemo);

    menu->addSeparator();
    menu->addAction(actManageFingerprints);
    menu->addAction(actShowOwnFingerprint);

    const QString jid     = action->property("jid").toString();
    const int     account = action->property("account").toInt();

    if (!action->property("isGroup").toBool()) {
        const auto ownJid = m_accountInfo->getJid(account).split("/").first();
        m_omemo->processUnknownDevices(account, ownJid, jid);
    }

    QAction *act = menu->exec(QCursor::pos());
    if (act == actEnableOmemo) {
        m_omemo->setEnabledForUser(account, jid, true);
        updateAction(account, jid);
        if (!action->property("isGroup").toBool()) {
            const auto ownJid = m_accountInfo->getJid(account).split("/").first();
            m_omemo->processUndecidedDevices(account, ownJid, jid);
        }
    } else if (act == actDisableOmemo) {
        m_omemo->setEnabledForUser(account, jid, false);
        updateAction(account, jid);
    } else if (act == actManageFingerprints) {
        auto screen = QGuiApplication::primaryScreen();
        auto w      = new KnownFingerprints(account, m_omemo, nullptr);
        w->filterContacts(jid);
        w->setWindowTitle(tr("Manage contact fingerprints"));
        w->resize(1000, 500);
        w->move((screen->geometry().width() / 2) - 500, (screen->geometry().height() / 2) - 250);
        w->show();
        w->raise();
    } else if (act == actShowOwnFingerprint) {
        showOwnFingerprint(account, jid);
    }

    delete menu;
}

QList<QVariantHash> OMEMOPlugin::getButtonParam() { return QList<QVariantHash>(); /* check m_enabled if implemented */ }

QAction *OMEMOPlugin::getAction(QObject *parent, int account, const QString &contact)
{
    if (!m_enabled)
        return nullptr;

    return createAction(parent, account, contact, false);
}

QAction *OMEMOPlugin::createAction(QObject *parent, int account, const QString &contact, bool isGroup)
{
    QString  bareJid = m_contactInfo->realJid(account, contact).split("/").first();
    QAction *action  = new QAction(getIcon(), tr("OMEMO encryption"), parent);
    action->setCheckable(true);
    action->setProperty("isGroup", QVariant(isGroup));
    connect(action, &QAction::triggered, this, &OMEMOPlugin::enableOMEMOAction);
    connect(action, &QAction::destroyed, this, &OMEMOPlugin::actionDestroyed);
    m_actions.insert(bareJid, action);
    updateAction(account, bareJid);
    if (!isGroup) {
        const auto ownJid = m_accountInfo->getJid(account).split("/").first();
        m_omemo->askUserDevicesList(account, ownJid, bareJid);
    }
    return action;
}

QList<QVariantHash> OMEMOPlugin::getGCButtonParam() { return getButtonParam(); }

QAction *OMEMOPlugin::getGCAction(QObject *parent, int account, const QString &contact)
{
    if (!m_enabled)
        return nullptr;

    return createAction(parent, account, contact, true);
}

void OMEMOPlugin::actionDestroyed(QObject *action)
{
    m_actions.remove(action->property("jid").toString(), reinterpret_cast<QAction *>(action));
}

void OMEMOPlugin::updateAction(int account, const QString &user)
{
    QString bareJid = m_contactInfo->realJid(account, user).split("/").first();
    for (QAction *action : m_actions.values(bareJid)) {
        const auto ownJid    = m_accountInfo->getJid(account).split("/").first();
        bool       isGroup   = action->property("isGroup").toBool();
        bool       available = isGroup ? m_omemo->isAvailableForGroup(account, ownJid, bareJid)
                                 : m_omemo->isAvailableForUser(account, bareJid);
        bool enabled = available && m_omemo->isEnabledForUser(account, bareJid);
        action->setEnabled(available);
        action->setChecked(enabled);
        action->setProperty("jid", bareJid);
        action->setProperty("account", account);
        if (!available) {
            if (isGroup)
                action->setText(tr("OMEMO encryption is not available for this group"));
            else
                action->setText(tr("OMEMO encryption is not available for this contact"));
        } else {
            action->setText(tr("OMEMO encryption"));
        }
    }
}

bool OMEMOPlugin::execute(int account, const QHash<QString, QVariant> &args, QHash<QString, QVariant> *result)
{
    if (!m_enabled) {
        return false;
    }

    if (args.contains("is_enabled_for")) {
        return m_omemo->isEnabledForUser(
            account, m_contactInfo->realJid(account, args["is_enabled_for"].toString()).split("/").first());
    } else if (args.contains("encrypt_data")) {
        QByteArray data = args["encrypt_data"].toByteArray();
        QByteArray iv   = Crypto::randomBytes(OMEMO_AES_GCM_IV_LENGTH);
        QByteArray key  = Crypto::randomBytes(32);

        QPair<QByteArray, QByteArray> encResult = Crypto::aes_gcm(Crypto::Encode, iv, key, data);
        result->insert("data", encResult.first + encResult.second);
        result->insert("anchor", iv + key);

        return true;
    } else if (args.contains("encrypt_message")) {
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
