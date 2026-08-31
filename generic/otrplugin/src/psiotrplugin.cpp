/*
 * psiotrplugin.cpp
 *
 * Off-the-Record Messaging plugin for Psi
 * Copyright (C) 2007-2011  Timo Engel (timo-e@freenet.de)
 *                    2011  Florian Fieber
 *
 * This program was originally written as part of a diplom thesis
 * advised by Prof. Dr. Ruediger Weis (PST Labor)
 * at the Technical University of Applied Sciences Berlin.
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

#include "psiotrplugin.h"
#include "accountinfoaccessinghost.h"
#include "applicationinfoaccessinghost.h"
#include "contactinfoaccessinghost.h"
#include "encryptionmethodaccessinghost.h"
#include "htmlnormalizer.h"
#include "iconfactoryaccessinghost.h"
#include "psiaccountcontrollinghost.h"
#include "psiotrclosure.h"
#include "psiotrconfig.h"

#include <QAction>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QTextDocument>

namespace psiotr {

namespace {

const QString &otrWhitespaceTag()
{
    static const QString tag = QString::fromLatin1(QByteArray::fromHex("20092020090909092009200920092020"));
    return tag;
}

const QString &otrWhitespaceV1()
{
    static const QString tag = QString::fromLatin1(QByteArray::fromHex("2009200920200920"));
    return tag;
}

const QString &otrWhitespaceV2()
{
    static const QString tag = QString::fromLatin1(QByteArray::fromHex("2020090920200920"));
    return tag;
}

const QString &otrWhitespaceV3()
{
    static const QString tag = QString::fromLatin1(QByteArray::fromHex("2020090920200909"));
    return tag;
}

bool hasOtrV3WhitespaceTag(const QString &text)
{
    auto searchFrom = 0;
    while (true) {
        const auto tagPos = text.indexOf(otrWhitespaceTag(), searchFrom);
        if (tagPos < 0)
            return false;

        auto pos = tagPos + otrWhitespaceTag().size();
        while (pos + 8 <= text.size()) {
            const QString versionTag = text.mid(pos, 8);
            if (versionTag == otrWhitespaceV3())
                return true;
            if (versionTag != otrWhitespaceV1() && versionTag != otrWhitespaceV2())
                break;
            pos += 8;
        }

        searchFrom = tagPos + otrWhitespaceTag().size();
    }
}

QString endpointKey(const QString &account, const QString &contact)
{
    return account + QLatin1Char('\n') + contact;
}

QString unescapeLegacyEntities(const QString &escaped)
{
    QString plain(escaped);
    plain.replace(QStringLiteral("&lt;"), QStringLiteral("<"))
        .replace(QStringLiteral("&gt;"), QStringLiteral(">"))
        .replace(QStringLiteral("&quot;"), QStringLiteral("\""))
        .replace(QStringLiteral("&#39;"), QStringLiteral("'"))
        .replace(QStringLiteral("&amp;"), QStringLiteral("&"));
    return plain;
}

bool hasDirectChild(const QDomElement &parent, const QString &ns, const QString &name)
{
    for (auto child = parent.firstChildElement(); !child.isNull(); child = child.nextSiblingElement()) {
        if (child.namespaceURI() == ns && child.localName() == name)
            return true;
    }
    return false;
}

QDomElement appendChildIfMissing(QDomElement &parent, const QString &ns, const QString &name)
{
    if (hasDirectChild(parent, ns, name))
        return {};

    auto child = parent.ownerDocument().createElementNS(ns, name);
    parent.appendChild(child);
    return child;
}

void addOtrProcessingHints(QDomElement &message)
{
    appendChildIfMissing(message, QStringLiteral("urn:xmpp:hints"), QStringLiteral("no-copy"));
    appendChildIfMissing(message, QStringLiteral("urn:xmpp:hints"), QStringLiteral("no-permanent-store"));
    appendChildIfMissing(message, QStringLiteral("urn:xmpp:carbons:2"), QStringLiteral("private"));
}

bool hasOtrEme(const QDomElement &message)
{
    for (auto child = message.firstChildElement(); !child.isNull(); child = child.nextSiblingElement()) {
        if (child.namespaceURI() == QLatin1String("urn:xmpp:eme:0")
            && child.localName() == QLatin1String("encryption")
            && child.attribute(QStringLiteral("namespace")) == QLatin1String("urn:xmpp:otr:0")) {
            return true;
        }
    }
    return false;
}

void addOtrEme(QDomElement &message)
{
    if (hasOtrEme(message))
        return;

    auto eme = message.ownerDocument().createElementNS(QStringLiteral("urn:xmpp:eme:0"),
                                                       QStringLiteral("encryption"));
    eme.setAttribute(QStringLiteral("namespace"), QStringLiteral("urn:xmpp:otr:0"));
    message.appendChild(eme);
}

void removeOtrEme(QDomElement &message)
{
    for (auto child = message.firstChildElement(); !child.isNull();) {
        const auto next = child.nextSiblingElement();
        if (child.namespaceURI() == QLatin1String("urn:xmpp:eme:0")
            && child.localName() == QLatin1String("encryption")
            && child.attribute(QStringLiteral("namespace")) == QLatin1String("urn:xmpp:otr:0")) {
            message.removeChild(child);
        }
        child = next;
    }
}

void clearChildren(QDomElement &element)
{
    while (!element.firstChild().isNull())
        element.removeChild(element.firstChild());
}

bool isEncodedOtrMessage(const QString &message)
{
    return message.startsWith(QLatin1String("?OTR:")) || message.startsWith(QLatin1String("?OTR|"));
}

} // namespace

class OtrEncryptionProvider final : public EncryptionMethodProvider {
public:
    static constexpr auto OtrNamespace = "urn:xmpp:otr:0";

    class Session final : public EncryptionMethodProvider::Session {
    public:
        Session(OtrEncryptionProvider *provider, int account, const Context &context) :
            provider_(provider), account_(account),
            contact_(context.recipients.isEmpty() ? QString() : context.recipients.constFirst()),
            carbon_(context.options.value(QStringLiteral("forwardedCarbon")).toBool())
        {
            if (!contact_.isEmpty() && provider_ && provider_->plugin_ && provider_->plugin_->m_otrConnection
                && provider_->plugin_->m_accountInfo) {
                const QString accountId = provider_->plugin_->m_accountInfo->getId(account_);
                if (provider_->plugin_->m_otrConnection->getMessageState(accountId, contact_)
                    != OTR_MESSAGESTATE_ENCRYPTED) {
                    provider_->plugin_->m_otrConnection->startSession(accountId, contact_);
                }
            }
        }

        void encryptStanza(const QDomElement &stanza, QObject *context, Completion completion) override
        {
            Result result;
            if (!provider_ || !provider_->plugin_ || !context) {
                result.error = Error::Cancelled;
                result.errorString = QObject::tr("OTR encryption session was cancelled");
                completion(std::move(result));
                return;
            }
            if (contact_.isEmpty()) {
                result.error = Error::NoRecipients;
                result.errorString = QObject::tr("OTR requires a recipient");
                completion(std::move(result));
                return;
            }

            const QString accountId = provider_->plugin_->m_accountInfo->getId(account_);
            if (provider_->plugin_->m_otrConnection->getMessageState(accountId, contact_)
                != OTR_MESSAGESTATE_ENCRYPTED) {
                result.error = Error::NoSession;
                result.errorString = QObject::tr("The OTR session with %1 is not established yet").arg(contact_);
                completion(std::move(result));
                return;
            }

            QDomDocument document;
            auto message = document.importNode(stanza, true).toElement();
            document.appendChild(message);
            message.setAttribute(QStringLiteral("to"), contact_);
            if (!provider_->plugin_->encryptMessageElement(account_, message, contact_) || message.isNull()) {
                result.error = Error::CryptoError;
                result.errorString = QObject::tr("Could not encrypt the message with OTR");
                completion(std::move(result));
                return;
            }

            result.success = true;
            result.stanza = message;
            completion(std::move(result));
        }

        void decryptStanza(const QDomElement &stanza, QObject *context, Completion completion) override
        {
            Result result;
            if (!provider_ || !provider_->plugin_ || !context) {
                result.error = Error::Cancelled;
                result.errorString = QObject::tr("OTR decryption session was cancelled");
                completion(std::move(result));
                return;
            }

            QDomDocument document;
            auto message = document.importNode(stanza, true).toElement();
            document.appendChild(message);
            const QString contact = message.attribute(QStringLiteral("from"));

            // XEP-0364: OTR protocol traffic received through Message Carbons
            // belongs to another endpoint and must be ignored silently.
            if (carbon_) {
                result.success = true;
                result.stanza = message;
                result.metadata.sender = contact;
                result.metadata.protocolOnly = true;
                completion(std::move(result));
                return;
            }

            const bool decrypted = provider_->plugin_->decryptMessageElement(account_, message, contact);
            if (message.isNull()) {
                result.success = true;
                result.metadata.sender = contact;
                result.metadata.protocolOnly = true;
                completion(std::move(result));
                return;
            }
            if (!decrypted) {
                result.error = Error::ProtocolError;
                result.errorString = QObject::tr("The stanza was not a valid OTR message");
                completion(std::move(result));
                return;
            }

            result.success = true;
            result.stanza = message;
            result.metadata.sender = contact;
            completion(std::move(result));
        }

    private:
        OtrEncryptionProvider *provider_ = nullptr;
        int account_ = -1;
        QString contact_;
        bool carbon_ = false;
    };

    explicit OtrEncryptionProvider(PsiOtrPlugin *plugin) : plugin_(plugin) { }

    QString id() const override { return QStringLiteral("otr"); }
    QString name() const override { return QObject::tr("OTR"); }
    QIcon icon() const override { return QIcon(QStringLiteral(":/otrplugin/otr_yes.png")); }
    Capabilities capabilities() const override { return XmppStanza; }
    QStringList features() const override { return { QString::fromLatin1(OtrNamespace) }; }
    UiCapabilities uiCapabilities() const override { return Settings; }

    bool canDecrypt(int, const QDomElement &stanza) const override
    {
        if (stanza.tagName() != QLatin1String("message"))
            return false;

        const auto body = stanza.firstChildElement(QStringLiteral("body"));
        if (!body.isNull() && body.text().startsWith(QLatin1String("?OTR")))
            return true;

        return hasOtrEme(stanza);
    }

    Availability availability(int account, const QString &endpointJid) const override
    {
        if (!plugin_ || !plugin_->m_enabled || !plugin_->m_contactInfo || !plugin_->m_accountInfo
            || !plugin_->m_otrConnection || !plugin_->m_optionHost || endpointJid.isEmpty()
            || plugin_->m_optionHost->getPluginOption(OPTION_POLICY, DEFAULT_POLICY).toInt() == OTR_POLICY_OFF) {
            return Availability::Unavailable;
        }

        if (plugin_->isXep0364Peer(account, endpointJid))
            return Availability::Available;

        const QString accountId = plugin_->m_accountInfo->getId(account);
        if (plugin_->m_otrDiscoveredEndpoints.contains(endpointKey(accountId, endpointJid))
            || plugin_->m_otrConnection->getMessageState(accountId, endpointJid) == OTR_MESSAGESTATE_ENCRYPTED) {
            return Availability::Available;
        }

        // Older OTR endpoints can advertise only with the XEP-0364 whitespace
        // tag (or not advertise at all), so lack of XEP-0378 caps is unknown,
        // not proof that OTR is unsupported.
        return Availability::Unknown;
    }

    Session *startSession(int account, const Context &sessionContext) override
    {
        if (!plugin_ || !plugin_->m_enabled || !plugin_->m_otrConnection || !plugin_->m_optionHost
            || plugin_->m_optionHost->getPluginOption(OPTION_POLICY, DEFAULT_POLICY).toInt() == OTR_POLICY_OFF
            || sessionContext.recipients.size() > 1) {
            return nullptr;
        }

        // The incoming-decryption path intentionally uses zero recipients.
        // For an outgoing session keep the exact endpoint JID supplied by Psi;
        // XMPP transports are allowed to expose online legacy contacts as bare JIDs.
        if (sessionContext.recipients.size() == 1 && sessionContext.recipients.constFirst().isEmpty())
            return nullptr;

        return new Session(this, account, sessionContext);
    }

    QWidget *createSettingsWidget(int, QWidget *parent) override
    {
        if (!plugin_ || !plugin_->m_otrConnection)
            return nullptr;
        return new ConfigDialog(plugin_->m_otrConnection, plugin_->m_optionHost, plugin_->m_accountInfo, parent);
    }

private:
    PsiOtrPlugin *plugin_ = nullptr;
};

PsiOtrPlugin::PsiOtrPlugin() :
    m_enabled(false), m_otrConnection(nullptr), m_onlineUsers(), m_optionHost(nullptr), m_senderHost(nullptr),
    m_applicationInfo(nullptr), m_accountHost(nullptr), m_accountInfo(nullptr), m_contactInfo(nullptr),
    m_iconHost(nullptr), m_psiEvent(nullptr), m_encryptionHost(nullptr), m_encryptionProvider(nullptr),
    m_messageBoxList()
{
}

PsiOtrPlugin::~PsiOtrPlugin() { }

QString PsiOtrPlugin::name() const { return "OTR Plugin"; }

QWidget *PsiOtrPlugin::options()
{
    // OTR settings are provided by the encryption method and embedded into
    // Psi's Security page.
    return nullptr;
}

bool PsiOtrPlugin::enable()
{
    const QVariant policyOption = m_optionHost->getPluginOption(OPTION_POLICY, DEFAULT_POLICY);
    m_otrConnection = new OtrMessaging(this, static_cast<OtrPolicy>(policyOption.toInt()));
    m_enabled = true;

    QFile f(":/otrplugin/otr_yes.png");
    if (f.open(QIODevice::ReadOnly)) {
        m_iconHost->addIcon("otrplugin/otr_yes", f.readAll());
        f.close();
    } else {
        qWarning("failed to open %s", qPrintable(f.errorString()));
    }

    f.setFileName(":/otrplugin/otr_no.png");
    if (f.open(QIODevice::ReadOnly)) {
        m_iconHost->addIcon("otrplugin/otr_no", f.readAll());
        f.close();
    } else {
        qWarning("failed to open %s", qPrintable(f.errorString()));
    }

    f.setFileName(":/otrplugin/otr_unverified.png");
    if (f.open(QIODevice::ReadOnly)) {
        m_iconHost->addIcon("otrplugin/otr_unverified", f.readAll());
        f.close();
    } else {
        qWarning("failed to open %s", qPrintable(f.errorString()));
    }

    if (!m_encryptionHost) {
        delete m_otrConnection;
        m_otrConnection = nullptr;
        m_enabled = false;
        return false;
    }

    m_encryptionProvider = new OtrEncryptionProvider(this);
    if (!m_encryptionHost->registerEncryptionMethod(m_encryptionProvider)) {
        delete m_encryptionProvider;
        m_encryptionProvider = nullptr;
        delete m_otrConnection;
        m_otrConnection = nullptr;
        m_enabled = false;
        return false;
    }

    return true;
}

bool PsiOtrPlugin::disable()
{
    if (m_encryptionHost && m_encryptionProvider)
        m_encryptionHost->unregisterEncryptionMethod(m_encryptionProvider);
    delete m_encryptionProvider;
    m_encryptionProvider = nullptr;

    const QStringList accounts = m_onlineUsers.keys();
    for (const QString &account : accounts) {
        const QStringList contacts = m_onlineUsers.value(account).keys();
        for (const QString &contact : contacts) {
            if (m_onlineUsers[account][contact]->encrypted())
                m_otrConnection->endSession(account, contact);
            m_onlineUsers[account][contact]->disable();
            delete m_onlineUsers[account][contact];
        }
        m_onlineUsers[account].clear();
    }
    m_onlineUsers.clear();
    m_otrDiscoveredEndpoints.clear();
    m_xep0364Endpoints.clear();

    while (!m_messageBoxList.empty()) {
        qDeleteAll(m_messageBoxList.begin(), m_messageBoxList.end());
        m_messageBoxList.clear();
    }

    delete m_otrConnection;
    m_otrConnection = nullptr;
    m_enabled = false;
    return true;
}

void PsiOtrPlugin::applyOptions() { }
void PsiOtrPlugin::restoreOptions() { }

QString PsiOtrPlugin::pluginInfo()
{
    QString out;
    out += tr("Off-the-Record Messaging (OTR) is a cryptographic protocol "
              "that provides encryption for instant messaging conversations. "
              "In addition to authentication and encryption, OTR provides "
              "forward secrecy and malleable encryption.")
        + "<br/>";
    out += "<br/>";
    out += tr("In comparison with OpenPGP and OMEMO, the OTR protocol does "
              "not depend on XMPP specific structures which allows one to use it "
              "for protecting conversations via XMPP transports (to Telegram, "
              "Skype, VK, QQ and other networks).")
        + "<br/>";
    out += "<br/>";
    out += tr("OTR features:") + "<br/>";
    out += tr("* Fast and easy update of encryption keys.") + "<br/>";
    out += tr("* Simple and convenient authentication of interlocutor "
              "without necessity of comparing public key fingerprints "
              "through an outside communication channel.")
        + "<br/>";
    out += "<br/>";
    out += tr("OTR limitations:") + "<br/>";
    out += tr("* No support of offline messages.") + "<br/>";
    out += tr("* No support of carbon copies to other XMPP resources.") + "<br/>";
    out += tr("* No support of multi-user chats.") + "<br/>";
    out += tr("* No support of file transfer.") + "<br/>";
    out += "<br/>";
    out += tr("OTR provides the following guarantees:");
    out += "<dl>";
    out += "<dt>" + tr("Encryption") + "</dt>";
    out += "<dd>" + tr("No one else can read your instant messages.") + "</dd>";
    out += "<dt>" + tr("Authentication") + "</dt>";
    out += "<dd>" + tr("You are assured the correspondent is who you think it is.") + "</dd>";
    out += "<dt>" + tr("Deniability") + "</dt>";
    out += "<dd>"
        + tr("The messages you send do not have digital signatures that "
             "are checkable by a third party. Anyone can forge messages "
             "after a conversation to make them look like they came from "
             "you. However, during a conversation, your correspondent is "
             "assured the messages (s)he sees are authentic and unmodified.")
        + "</dd>";
    out += "<dt>" + tr("Perfect forward secrecy") + "</dt>";
    out += "<dd>"
        + tr("If you lose control of your private keys, no previous "
             "conversation is compromised.")
        + "</dd>";
    out += "</dl>";
    out += tr("For further information, see "
              "&lt;<a href=\"https://otr.cypherpunks.ca/\">"
              "https://otr.cypherpunks.ca/</a>&gt;.");
    return out;
}

bool PsiOtrPlugin::isXep0364Peer(int accountIndex, const QString &contact)
{
    if (contact.isEmpty() || !m_accountInfo)
        return false;

    const QString account = m_accountInfo->getId(accountIndex);
    if (m_xep0364Endpoints.contains(endpointKey(account, contact)))
        return true;

    if (m_contactInfo
        && m_contactInfo->hasCaps(accountIndex, contact, { QStringLiteral("urn:xmpp:otr:0") })) {
        markXep0364Peer(account, contact);
        return true;
    }

    return false;
}

void PsiOtrPlugin::markXep0364Peer(const QString &account, const QString &contact)
{
    if (contact.isEmpty())
        return;

    const QString key = endpointKey(account, contact);
    m_xep0364Endpoints.insert(key);
    m_otrDiscoveredEndpoints.insert(key);
}

bool PsiOtrPlugin::decryptMessageElement(int accountIndex, QDomElement &messageElement, const QString &contactOverride)
{
    if (!m_enabled || messageElement.isNull()
        || messageElement.attribute(QStringLiteral("type")) == QLatin1String("error")
        || messageElement.attribute(QStringLiteral("type")) == QLatin1String("groupchat")) {
        return false;
    }

    const QString contact = contactOverride.isEmpty() ? messageElement.attribute(QStringLiteral("from"))
                                                       : contactOverride;
    if (contact.isEmpty())
        return false;

    auto plainBody = messageElement.firstChildElement(QStringLiteral("body"));
    if (plainBody.isNull())
        return false;

    const QString account = m_accountInfo->getId(accountIndex);

    // EME and XEP-0378 caps are strong signals that the remote endpoint follows
    // XEP-0364 payload semantics. Once observed, keep that knowledge for the
    // lifetime of this exact endpoint; whitespace-only discovery remains legacy.
    if (hasOtrEme(messageElement))
        markXep0364Peer(account, contact);
    const bool xep0364Peer = isXep0364Peer(accountIndex, contact);

    QString decrypted;
    const OtrMessageType messageType =
        m_otrConnection->decryptMessage(account, contact, plainBody.text(), decrypted);

    if (messageType == OTR_MESSAGETYPE_NONE)
        return false;

    m_otrDiscoveredEndpoints.insert(endpointKey(account, contact));
    if (m_encryptionHost && m_encryptionProvider)
        m_encryptionHost->encryptionMethodStateChanged(m_encryptionProvider);

    if (messageType == OTR_MESSAGETYPE_IGNORE) {
        messageElement = QDomElement();
        return false;
    }

    // Never trust the original stanza's XHTML-IM sibling: it was outside OTR.
    // Legacy rich text, when applicable, is reconstructed only from the
    // authenticated decrypted OTR payload.
    auto htmlElement = messageElement.firstChildElement(QStringLiteral("html"));
    if (!htmlElement.isNull())
        messageElement.removeChild(htmlElement);

    QString bodyText = decrypted;
    if (!xep0364Peer) {
        if (Qt::mightBeRichText(decrypted)) {
            QDomDocument document = messageElement.ownerDocument();
            HtmlNormalizer normalizer(decrypted);
            QDomElement normalizedBody = normalizer.output(document);

            htmlElement = document.createElementNS(QStringLiteral("http://jabber.org/protocol/xhtml-im"),
                                                   QStringLiteral("html"));
            htmlElement.appendChild(normalizedBody);
            messageElement.appendChild(htmlElement);

            QTextDocument plainText;
            plainText.setHtml(decrypted);
            bodyText = plainText.toPlainText();
        } else {
            // Legacy OTR clients commonly treat the OTR payload as HTML even
            // without an XHTML-IM wrapper, so undo the entity escaping used on
            // our legacy outbound path.
            bodyText = unescapeLegacyEntities(decrypted);
        }
    }

    // XEP-0364 §4.1 requires XML-looking OTR plaintext to remain literal text.
    // For legacy endpoints bodyText above is the safe plain-text fallback for
    // the normalized authenticated HTML payload.
    clearChildren(plainBody);
    plainBody.appendChild(messageElement.ownerDocument().createTextNode(bodyText));

    // Keep the peer's original EME, if present, as wire evidence. The native
    // EncryptionManager already carries successful OTR decryption as metadata,
    // so do not synthesize an EME marker for legacy peers after decryption.
    return true;
}

bool PsiOtrPlugin::encryptMessageElement(int accountIndex, QDomElement &message, const QString &contactOverride)
{
    if (!m_enabled || message.isNull()
        || message.attribute(QStringLiteral("type")) == QLatin1String("groupchat")
        || message.attribute(QStringLiteral("type")) == QLatin1String("error")) {
        return false;
    }

    const QString contact = contactOverride.isEmpty() ? message.attribute(QStringLiteral("to"))
                                                       : contactOverride;
    if (contact.isEmpty())
        return false;

    auto bodyElement = message.firstChildElement(QStringLiteral("body"));
    if (bodyElement.isNull())
        return false;

    const QString account = m_accountInfo->getId(accountIndex);
    const bool xep0364Peer = isXep0364Peer(accountIndex, contact);
    const QString plaintext = xep0364Peer ? bodyElement.text() : bodyElement.text().toHtmlEscaped();
    const QString encrypted = m_otrConnection->encryptMessage(account, contact, plaintext);

    // Fail closed: never let an encryption failure fall through to sending the
    // original plaintext stanza.
    if (encrypted.isEmpty()) {
        message = QDomElement();
        return false;
    }

    clearChildren(bodyElement);
    bodyElement.appendChild(message.ownerDocument().createTextNode(encrypted));

    // XHTML-IM is not authenticated separately by OTR and must not survive as
    // a plaintext side channel next to the encrypted body.
    auto htmlElement = message.firstChildElement(QStringLiteral("html"));
    if (!htmlElement.isNull())
        message.removeChild(htmlElement);

    // EME also declares the XEP-0364 payload semantics to the remote endpoint.
    // Do not emit it while using legacy HTML-compatible OTR plaintext encoding.
    if (xep0364Peer)
        addOtrEme(message);
    else
        removeOtrEme(message);
    addOtrProcessingHints(message);
    return true;
}

void PsiOtrPlugin::setOptionAccessingHost(OptionAccessingHost *host) { m_optionHost = host; }

void PsiOtrPlugin::optionChanged(const QString &)
{
    const QVariant policyOption = m_optionHost->getPluginOption(OPTION_POLICY, DEFAULT_POLICY);
    m_otrConnection->setPolicy(static_cast<OtrPolicy>(policyOption.toInt()));
    if (m_encryptionHost && m_encryptionProvider)
        m_encryptionHost->encryptionMethodStateChanged(m_encryptionProvider);
}

void PsiOtrPlugin::setStanzaSendingHost(StanzaSendingHost *host) { m_senderHost = host; }

void PsiOtrPlugin::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host) { m_applicationInfo = host; }

void PsiOtrPlugin::setPsiAccountControllingHost(PsiAccountControllingHost *host)
{
    m_accountHost = host;
    host->subscribeLogout(this, [this](int accountIndex) {
        if (!m_enabled)
            return;

        const QString account = m_accountInfo->getId(accountIndex);
        if (m_onlineUsers.contains(account)) {
            const QStringList contacts = m_onlineUsers.value(account).keys();
            for (const QString &contact : contacts) {
                m_otrConnection->endSession(account, contact);
                m_onlineUsers[account][contact]->updateMessageState();
            }
        }

        const QString prefix = account + QLatin1Char('\n');
        const auto clearAccountState = [&prefix](QSet<QString> &set) {
            for (auto it = set.begin(); it != set.end();) {
                if ((*it).startsWith(prefix))
                    it = set.erase(it);
                else
                    ++it;
            }
        };
        clearAccountState(m_otrDiscoveredEndpoints);
        clearAccountState(m_xep0364Endpoints);

        if (m_encryptionHost && m_encryptionProvider)
            m_encryptionHost->encryptionMethodStateChanged(m_encryptionProvider);
    });
}

void PsiOtrPlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost *host) { m_accountInfo = host; }
void PsiOtrPlugin::setContactInfoAccessingHost(ContactInfoAccessingHost *host) { m_contactInfo = host; }
void PsiOtrPlugin::setIconFactoryAccessingHost(IconFactoryAccessingHost *host) { m_iconHost = host; }
void PsiOtrPlugin::setEncryptionMethodAccessingHost(EncryptionMethodAccessingHost *host) { m_encryptionHost = host; }
void PsiOtrPlugin::setEventCreatingHost(EventCreatingHost *host) { m_psiEvent = host; }

bool PsiOtrPlugin::incomingStanza(int accountIndex, const QDomElement &xml)
{
    if (!m_enabled)
        return false;

    const QString account = m_accountInfo->getId(accountIndex);

    if (xml.nodeName() == QLatin1String("message")) {
        if (xml.attribute(QStringLiteral("type")) == QLatin1String("groupchat")
            || xml.attribute(QStringLiteral("type")) == QLatin1String("error")) {
            return false;
        }

        // XEP-0364 whitespace discovery is also an active negotiation signal:
        // qca-otr starts AKE from it for Opportunistic/Always policies. Keep the
        // original plaintext stanza visible, but feed the exact body to qca-otr.
        const auto body = xml.firstChildElement(QStringLiteral("body"));
        const QString contact = xml.attribute(QStringLiteral("from"));
        if (!body.isNull() && !contact.isEmpty() && !body.text().startsWith(QLatin1String("?OTR"))
            && body.text().contains(otrWhitespaceTag())) {
            QString ignored;
            m_otrConnection->decryptMessage(account, contact, body.text(), ignored);

            if (hasOtrV3WhitespaceTag(body.text())) {
                m_otrDiscoveredEndpoints.insert(endpointKey(account, contact));
                if (m_encryptionHost && m_encryptionProvider)
                    m_encryptionHost->encryptionMethodStateChanged(m_encryptionProvider);
            }
        }
        return false;
    }

    if (xml.nodeName() != QLatin1String("presence"))
        return false;

    const QString contact = xml.attribute(QStringLiteral("from"));
    const QString type = xml.attribute(QStringLiteral("type"), QStringLiteral("available"));
    if (type != QLatin1String("unavailable") || contact.isEmpty())
        return false;

    // Track the exact endpoint provided by XMPP routing. Native XMPP peers are
    // normally full JIDs, while XEP-0100 transports may legitimately expose an
    // online legacy-network contact as a bare JID.
    if (m_optionHost->getPluginOption(OPTION_END_WHEN_OFFLINE, DEFAULT_END_WHEN_OFFLINE).toBool())
        m_otrConnection->expireSession(account, contact);

    if (m_onlineUsers.contains(account) && m_onlineUsers.value(account).contains(contact))
        m_onlineUsers[account][contact]->updateMessageState();

    const QString key = endpointKey(account, contact);
    m_otrDiscoveredEndpoints.remove(key);
    m_xep0364Endpoints.remove(key);
    if (m_encryptionHost && m_encryptionProvider)
        m_encryptionHost->encryptionMethodStateChanged(m_encryptionProvider);

    return false;
}

bool PsiOtrPlugin::outgoingStanza(int accountIndex, QDomElement &xml)
{
    Q_UNUSED(accountIndex);

    if (!m_enabled || xml.nodeName() != QLatin1String("message")
        || xml.attribute(QStringLiteral("type")) == QLatin1String("groupchat")
        || xml.attribute(QStringLiteral("type")) == QLatin1String("error")
        || m_optionHost->getPluginOption(OPTION_POLICY, DEFAULT_POLICY).toInt() == OTR_POLICY_OFF) {
        return false;
    }

    auto body = xml.firstChildElement(QStringLiteral("body"));
    if (body.isNull() || body.text().startsWith(QLatin1String("?OTR")))
        return false;

    // Do not advertise OTR inside another encrypted method's payload/fallback.
    if (!xml.elementsByTagNameNS(QStringLiteral("urn:xmpp:eme:0"), QStringLiteral("encryption")).isEmpty())
        return false;
    for (auto child = xml.firstChildElement(); !child.isNull(); child = child.nextSiblingElement()) {
        if (child.tagName() == QLatin1String("encrypted")
            || (child.tagName() == QLatin1String("x")
                && child.namespaceURI() == QLatin1String("jabber:x:encrypted"))) {
            return false;
        }
    }

    if (!hasOtrV3WhitespaceTag(body.text())) {
        const QString discovery = otrWhitespaceTag() + otrWhitespaceV3();
        auto textNode = body.firstChild();
        if (textNode.isText())
            textNode.setNodeValue(textNode.nodeValue() + discovery);
        else
            body.appendChild(xml.ownerDocument().createTextNode(discovery));
    }

    return false;
}

QList<QVariantHash> PsiOtrPlugin::getButtonParam() { return {}; }

QAction *PsiOtrPlugin::getAction(QObject *parent, int accountIndex, const QString &contactJid)
{
    if (!m_enabled)
        return nullptr;

    const QString account = m_accountInfo->getId(accountIndex);

    // Keep exactly the endpoint JID supplied by Psi. Do not collapse a native
    // full JID to bare, and do not reject a transport endpoint that is bare.
    if (!m_onlineUsers.value(account).contains(contactJid))
        m_onlineUsers[account][contactJid] = new PsiOtrClosure(account, contactJid, m_otrConnection);

    return m_onlineUsers[account][contactJid]->getChatDlgMenu(parent);
}

QString PsiOtrPlugin::dataDir()
{
    return m_applicationInfo->appCurrentProfileDir(ApplicationInfoAccessingHost::DataLocation);
}

void PsiOtrPlugin::sendMessage(const QString &account, const QString &contact, const QString &message)
{
    const int accountIndex = getAccountIndexById(account);
    if (accountIndex == -1 || !m_senderHost || contact.isEmpty())
        return;

    QDomDocument document;
    auto stanza = document.createElement(QStringLiteral("message"));
    stanza.setAttribute(QStringLiteral("to"), contact);
    stanza.setAttribute(QStringLiteral("type"), QStringLiteral("chat"));

    auto body = document.createElement(QStringLiteral("body"));
    body.appendChild(document.createTextNode(message));
    stanza.appendChild(body);

    // Processing hints are useful for all OTR traffic regardless of the peer's
    // payload convention: do not archive/copy negotiation, fragments, or data.
    addOtrProcessingHints(stanza);

    // EME declares XEP-0364 semantics. Emit it only when that endpoint has
    // advertised XEP-0378 or sent us an OTR EME marker itself.
    if (isEncodedOtrMessage(message) && isXep0364Peer(accountIndex, contact))
        addOtrEme(stanza);

    document.appendChild(stanza);
    m_senderHost->sendStanza(accountIndex, stanza);
}

void PsiOtrPlugin::notifyUser(const QString &account,
                              const QString &contact,
                              const QString &message,
                              const OtrNotifyType &type)
{
    QMessageBox::Icon messageBoxIcon;
    if (type == OTR_NOTIFY_ERROR)
        messageBoxIcon = QMessageBox::Critical;
    else if (type == OTR_NOTIFY_WARNING)
        messageBoxIcon = QMessageBox::Warning;
    else
        messageBoxIcon = QMessageBox::Information;

    auto *messageBox = new QMessageBox(messageBoxIcon,
                                       tr("Confirm action"),
                                       message,
                                       QMessageBox::Ok,
                                       nullptr,
                                       Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    m_messageBoxList.enqueue(messageBox);

    m_psiEvent->createNewEvent(getAccountIndexById(account),
                               contact,
                               tr("OTR Plugin: event from %1").arg(contact),
                               this,
                               SLOT(eventActivated()));
}

void PsiOtrPlugin::eventActivated()
{
    if (!m_messageBoxList.empty()) {
        QMessageBox *messageBox = m_messageBoxList.dequeue();
        if (messageBox) {
            messageBox->exec();
            delete messageBox;
        }
    }
}

bool PsiOtrPlugin::displayOtrMessage(const QString &account, const QString &contact, const QString &message)
{
    return appendSysMsg(account, contact, message);
}

void PsiOtrPlugin::stateChange(const QString &account, const QString &contact, OtrStateChange change)
{
    if (!contact.isEmpty()) {
        m_otrDiscoveredEndpoints.insert(endpointKey(account, contact));
        if (m_encryptionHost && m_encryptionProvider)
            m_encryptionHost->encryptionMethodStateChanged(m_encryptionProvider);
    }

    if (!m_onlineUsers.value(account).contains(contact))
        m_onlineUsers[account][contact] = new PsiOtrClosure(account, contact, m_otrConnection);

    m_onlineUsers[account][contact]->updateMessageState();

    const bool verified = m_otrConnection->isVerified(account, contact);
    const bool encrypted = m_onlineUsers[account][contact]->encrypted();
    QString msg;
    QString icon;

    switch (change) {
    case OTR_STATECHANGE_GOINGSECURE:
        msg = encrypted ? tr("Attempting to refresh the private conversation")
                        : tr("Attempting to start a private conversation");
        break;
    case OTR_STATECHANGE_GONESECURE:
        msg = verified ? tr("Private conversation started") : tr("Unverified conversation started");
        icon = verified ? "otrplugin/otr_yes" : "otrplugin/otr_unverified";
        break;
    case OTR_STATECHANGE_GONEINSECURE:
        msg = tr("Private conversation lost");
        icon = "otrplugin/otr_no";
        break;
    case OTR_STATECHANGE_CLOSE:
        msg = tr("Private conversation closed");
        icon = "otrplugin/otr_no";
        break;
    case OTR_STATECHANGE_REMOTECLOSE:
        msg = tr("%1 has ended the private conversation with you; you should do the same.")
                  .arg(humanContact(account, contact));
        icon = "otrplugin/otr_no";
        break;
    case OTR_STATECHANGE_STILLSECURE:
        msg = verified ? tr("Private conversation refreshed") : tr("Unverified conversation refreshed");
        icon = verified ? "otrplugin/otr_yes" : "otrplugin/otr_unverified";
        break;
    case OTR_STATECHANGE_TRUST:
        msg = verified ? tr("Contact authenticated") : tr("Contact not authenticated");
        icon = verified ? "otrplugin/otr_yes" : "otrplugin/otr_unverified";
        break;
    }

    appendSysMsg(account, contact, msg, icon);
}

void PsiOtrPlugin::receivedSMP(const QString &account, const QString &contact, const QString &question)
{
    if (m_onlineUsers.contains(account) && m_onlineUsers.value(account).contains(contact))
        m_onlineUsers[account][contact]->receivedSMP(question);
}

void PsiOtrPlugin::updateSMP(const QString &account, const QString &contact, int progress)
{
    if (m_onlineUsers.contains(account) && m_onlineUsers.value(account).contains(contact))
        m_onlineUsers[account][contact]->updateSMP(progress);
}

QString PsiOtrPlugin::humanAccount(const QString &accountId)
{
    const QString human = getAccountNameById(accountId);
    return human.isEmpty() ? accountId : human;
}

QString PsiOtrPlugin::humanAccountPublic(const QString &accountId)
{
    return getAccountJidById(accountId);
}

QString PsiOtrPlugin::humanContact(const QString &accountId, const QString &contact)
{
    return m_contactInfo->name(getAccountIndexById(accountId), contact);
}

bool PsiOtrPlugin::appendSysMsg(const QString &account,
                                const QString &contact,
                                const QString &message,
                                const QString &icon)
{
    QString iconTag;
    if (!icon.isEmpty())
        iconTag = QString("<icon name=\"%1\"> ").arg(icon);
    return m_accountHost->appendSysHtmlMsg(getAccountIndexById(account), contact, iconTag + message);
}

int PsiOtrPlugin::getAccountIndexById(const QString &accountId)
{
    QString id;
    int accountIndex = 0;
    while (((id = m_accountInfo->getId(accountIndex)) != "-1") && (id != accountId))
        ++accountIndex;
    return (id == "-1") ? -1 : accountIndex;
}

QString PsiOtrPlugin::getAccountNameById(const QString &accountId)
{
    return m_accountInfo->getName(getAccountIndexById(accountId));
}

QString PsiOtrPlugin::getAccountJidById(const QString &accountId)
{
    return m_accountInfo->getJid(getAccountIndexById(accountId));
}

} // namespace psiotr
