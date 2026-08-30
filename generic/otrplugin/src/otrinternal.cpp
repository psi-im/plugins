/*
 * otrinternal.cpp - Native qca-otr backend for the Psi OTR plugin
 */

#include "otrinternal.h"

#include "qca-otr/ake.h"

#include <QDebug>
#include <QMessageBox>
#include <QObject>
#include <QStringList>
#include <QtCrypto>

namespace {

const QByteArray PsiOtrProtocolId("prpl-jabber");

QCA::SecureArray secureUtf8(const QString &value)
{
    QByteArray utf8 = value.toUtf8();
    QCA::SecureArray result(utf8);
    utf8.fill('\0');
    return result;
}

QString formatFingerprint(const QByteArray &fingerprint)
{
    if (fingerprint.size() != 20)
        return {};

    const QByteArray hex = fingerprint.toHex().toUpper();
    QStringList groups;
    groups.reserve(10);
    for (int i = 0; i < hex.size(); i += 4)
        groups.append(QString::fromLatin1(hex.mid(i, 4)));
    return groups.join(QLatin1Char(' '));
}

} // namespace

struct OtrInternal::Conversation
{
    QString account;
    QString contact;
    std::unique_ptr<QcaOtr::OtrSession> session;
    quint32 activePeerInstance = 0;
    QByteArray activeFingerprint;
    QByteArray sessionId;
    bool initiated = false;
    bool forcedFinished = false;
    bool smpSucceeded = false;
};

OtrInternal::OtrInternal(psiotr::OtrCallback *callback, psiotr::OtrPolicy &policy) :
    m_callback(callback), m_otrPolicy(policy), m_qca(std::make_unique<QCA::Initializer>())
{
    m_profile = std::make_unique<QcaOtr::ProfileStore>(callback->dataDir(), PsiOtrProtocolId);
    QString error;
    m_profileLoaded = m_profile->load(&error);
    if (!m_profileLoaded)
        qWarning() << "Cannot load OTR profile:" << error;
}

OtrInternal::~OtrInternal() = default;

QcaOtr::SessionPolicy OtrInternal::nativePolicy(psiotr::OtrPolicy policy)
{
    switch (policy) {
    case psiotr::OTR_POLICY_OFF: return QcaOtr::SessionPolicy::Disabled;
    case psiotr::OTR_POLICY_ENABLED: return QcaOtr::SessionPolicy::Manual;
    case psiotr::OTR_POLICY_AUTO: return QcaOtr::SessionPolicy::Opportunistic;
    case psiotr::OTR_POLICY_REQUIRE: return QcaOtr::SessionPolicy::Always;
    }
    return QcaOtr::SessionPolicy::Disabled;
}

QString OtrInternal::conversationKey(const QString &account, const QString &contact)
{
    return account + QChar(0x1f) + contact;
}

bool OtrInternal::ensureAccount(const QString &account, QString *error)
{
    if (!m_profileLoaded) {
        if (error)
            *error = QObject::tr("The OTR profile could not be loaded.");
        return false;
    }

    const QByteArray accountId = account.toUtf8();
    if (!m_profile->ensureIdentity(accountId, error))
        return false;
    return m_profile->ensureInstanceTag(accountId, nullptr, error) >= 0x00000100;
}

OtrInternal::Conversation *OtrInternal::conversation(const QString &account, const QString &contact, bool create)
{
    const QString key = conversationKey(account, contact);
    auto it = m_conversations.find(key);
    if (it != m_conversations.end()) {
        it.value()->session->setPolicy(nativePolicy(m_otrPolicy));
        return it.value().get();
    }
    if (!create)
        return nullptr;

    QString error;
    if (!ensureAccount(account, &error)) {
        displayProtocolError(account,
                             contact,
                             QObject::tr("Cannot initialize OTR for this account: %1").arg(error));
        return nullptr;
    }

    const QByteArray accountId = account.toUtf8();
    const QcaOtr::PrivateKeyRecord *identity = m_profile->identity(accountId);
    bool found = false;
    const quint32 instanceTag = m_profile->instanceTag(accountId, &found);
    if (!identity || !found || instanceTag < 0x00000100)
        return nullptr;

    auto result = std::make_shared<Conversation>();
    result->account = account;
    result->contact = contact;
    result->session = std::make_unique<QcaOtr::OtrSession>(identity->key, instanceTag);
    result->session->setPolicy(nativePolicy(m_otrPolicy));
    m_conversations.insert(key, result);
    return result.get();
}

const OtrInternal::Conversation *OtrInternal::conversation(const QString &account, const QString &contact) const
{
    const auto it = m_conversations.constFind(conversationKey(account, contact));
    return it == m_conversations.constEnd() ? nullptr : it.value().get();
}

void OtrInternal::sendTransport(const QString &account,
                                const QString &contact,
                                const QVector<QByteArray> &messages)
{
    for (const QByteArray &message : messages)
        m_callback->sendMessage(account, contact, QString::fromUtf8(message));
}

void OtrInternal::displayProtocolError(const QString &account, const QString &contact, const QString &message)
{
    if (!m_callback->displayOtrMessage(account, contact, message))
        m_callback->notifyUser(account, contact, message, psiotr::OTR_NOTIFY_ERROR);
}

quint32 OtrInternal::activePeer(const Conversation *conversation) const
{
    if (!conversation || !conversation->session)
        return 0;
    if (conversation->activePeerInstance != 0)
        return conversation->activePeerInstance;

    const QVector<quint32> peers = conversation->session->peerInstances();
    for (quint32 peer : peers) {
        if (conversation->session->isEncrypted(peer))
            return peer;
    }
    for (quint32 peer : peers) {
        if (conversation->session->peerState(peer) == QcaOtr::PeerState::Finished)
            return peer;
    }
    return peers.isEmpty() ? 0 : peers.first();
}

bool OtrInternal::activeEstablished(const Conversation *conversation,
                                    quint32 *peerInstance,
                                    QcaOtr::AkeEstablishedSession *established) const
{
    if (!conversation || !established)
        return false;
    const quint32 peer = activePeer(conversation);
    if (peer == 0 || !conversation->session->establishedSession(peer, established))
        return false;
    if (peerInstance)
        *peerInstance = peer;
    return true;
}

bool OtrInternal::rememberFingerprint(Conversation *conversation, quint32 peerInstance, bool *created)
{
    if (created)
        *created = false;
    if (!conversation || peerInstance == 0 || !m_profileLoaded)
        return false;

    QcaOtr::AkeEstablishedSession established;
    if (!conversation->session->establishedSession(peerInstance, &established) || established.peerFingerprint.size() != 20)
        return false;

    const QByteArray account = conversation->account.toUtf8();
    const QByteArray username = conversation->contact.toUtf8();
    QByteArray inheritedTrust;

    const int slash = username.indexOf('/');
    if (slash > 0) {
        const QByteArray bare = username.left(slash);
        const QcaOtr::FingerprintRecord *old = m_profile->fingerprint(bare, account, established.peerFingerprint);
        if (old)
            inheritedTrust = old->trust;
    }

    QString error;
    bool wasCreated = false;
    if (!m_profile->rememberFingerprint(username,
                                        account,
                                        established.peerFingerprint,
                                        inheritedTrust,
                                        &wasCreated,
                                        &error)) {
        displayProtocolError(conversation->account,
                             conversation->contact,
                             QObject::tr("Cannot save OTR fingerprint: %1").arg(error));
        return false;
    }

    conversation->activeFingerprint = established.peerFingerprint;
    conversation->sessionId = established.sessionId;
    conversation->initiated = established.initiated;

    if (wasCreated) {
        const QString message = QObject::tr("You have received a new fingerprint from %1:\n%2")
                                    .arg(m_callback->humanContact(conversation->account, conversation->contact),
                                         formatFingerprint(established.peerFingerprint));
        if (!m_callback->displayOtrMessage(conversation->account, conversation->contact, message))
            m_callback->notifyUser(conversation->account,
                                   conversation->contact,
                                   message,
                                   psiotr::OTR_NOTIFY_INFO);
    }
    if (created)
        *created = wasCreated;
    return true;
}

void OtrInternal::handleAuthenticated(Conversation *conversation, quint32 peerInstance, bool wasSecure)
{
    if (!conversation)
        return;
    conversation->activePeerInstance = peerInstance;
    conversation->forcedFinished = false;
    conversation->smpSucceeded = false;
    rememberFingerprint(conversation, peerInstance);
    m_callback->stateChange(conversation->account,
                            conversation->contact,
                            wasSecure ? psiotr::OTR_STATECHANGE_STILLSECURE : psiotr::OTR_STATECHANGE_GONESECURE);
}

void OtrInternal::handleSmpEvent(Conversation *conversation, const QcaOtr::SessionResult &result)
{
    if (!conversation || result.smpEvent == QcaOtr::SmpEvent::None)
        return;

    switch (result.smpEvent) {
    case QcaOtr::SmpEvent::AskForAnswer:
    case QcaOtr::SmpEvent::AskForSecret:
        conversation->smpSucceeded = false;
        m_callback->receivedSMP(conversation->account,
                                conversation->contact,
                                QString::fromUtf8(result.smpQuestion));
        break;
    case QcaOtr::SmpEvent::InProgress:
        m_callback->updateSMP(conversation->account, conversation->contact, result.smpProgress);
        break;
    case QcaOtr::SmpEvent::Success:
        conversation->smpSucceeded = true;
        m_callback->updateSMP(conversation->account, conversation->contact, 100);
        break;
    case QcaOtr::SmpEvent::Failure:
        conversation->smpSucceeded = false;
        m_callback->updateSMP(conversation->account, conversation->contact, 100);
        break;
    case QcaOtr::SmpEvent::Abort:
        conversation->smpSucceeded = false;
        m_callback->updateSMP(conversation->account, conversation->contact, -1);
        break;
    case QcaOtr::SmpEvent::Cheated:
    case QcaOtr::SmpEvent::Error: {
        conversation->smpSucceeded = false;
        QVector<QByteArray> messages;
        conversation->session->abortSmp(result.peerInstance, &messages);
        sendTransport(conversation->account, conversation->contact, messages);
        m_callback->updateSMP(conversation->account, conversation->contact, -2);
        break;
    }
    case QcaOtr::SmpEvent::None:
        break;
    }
}

QString OtrInternal::encryptMessage(const QString &account, const QString &contact, const QString &message)
{
    Conversation *ctx = conversation(account, contact, true);
    if (!ctx)
        return {};

    const QcaOtr::OutgoingResult outgoing =
        ctx->session->prepareOutgoing(message.toUtf8(), 0, m_callback->humanAccountPublic(account).toUtf8());

    if (outgoing.status == QcaOtr::OutgoingStatus::Finished) {
        displayProtocolError(account,
                             contact,
                             QObject::tr("Your message was not sent. Either end your private conversation, or restart it."));
        return {};
    }
    if (outgoing.status == QcaOtr::OutgoingStatus::Error || outgoing.messages.isEmpty()) {
        displayProtocolError(account,
                             contact,
                             QObject::tr("Encrypting message to %1 failed.\nThe message was not sent.").arg(contact));
        return {};
    }

    if (outgoing.peerInstance != 0)
        ctx->activePeerInstance = outgoing.peerInstance;

    for (int i = 0; i + 1 < outgoing.messages.size(); ++i)
        m_callback->sendMessage(account, contact, QString::fromUtf8(outgoing.messages.at(i)));
    return QString::fromUtf8(outgoing.messages.last());
}

psiotr::OtrMessageType OtrInternal::decryptMessage(const QString &account,
                                                   const QString &contact,
                                                   const QString &message,
                                                   QString &decrypted)
{
    Conversation *ctx = conversation(account, contact, true);
    if (!ctx)
        return psiotr::OTR_MESSAGETYPE_NONE;

    const bool wasSecure = getMessageState(account, contact) == psiotr::OTR_MESSAGESTATE_ENCRYPTED;
    QcaOtr::SessionResult result = ctx->session->processIncoming(message.toUtf8());
    if (result.peerInstance != 0)
        ctx->activePeerInstance = result.peerInstance;
    sendTransport(account, contact, result.outgoingMessages);
    handleSmpEvent(ctx, result);

    switch (result.status) {
    case QcaOtr::SessionStatus::Authenticated:
        handleAuthenticated(ctx, result.peerInstance, wasSecure);
        return psiotr::OTR_MESSAGETYPE_IGNORE;
    case QcaOtr::SessionStatus::Message:
        if (result.plaintext.isEmpty() && !result.tlvs.isEmpty())
            return psiotr::OTR_MESSAGETYPE_IGNORE;
        decrypted = QString::fromUtf8(result.plaintext);
        return psiotr::OTR_MESSAGETYPE_OTR;
    case QcaOtr::SessionStatus::Disconnected:
        ctx->forcedFinished = true;
        m_callback->stateChange(account, contact, psiotr::OTR_STATECHANGE_REMOTECLOSE);
        return psiotr::OTR_MESSAGETYPE_IGNORE;
    case QcaOtr::SessionStatus::Plaintext:
        if (wasSecure) {
            const QString warning = QObject::tr("<b>The following message received from %1 was <i>not</i> encrypted:</b>")
                                        .arg(m_callback->humanContact(account, contact));
            m_callback->displayOtrMessage(account, contact, warning);
        }
        return psiotr::OTR_MESSAGETYPE_NONE;
    case QcaOtr::SessionStatus::Unreadable:
        displayProtocolError(account, contact, QObject::tr("Received message is unreadable."));
        return psiotr::OTR_MESSAGETYPE_IGNORE;
    case QcaOtr::SessionStatus::RemoteError:
        displayProtocolError(account,
                             contact,
                             result.errorText.isEmpty() ? QObject::tr("Remote OTR protocol error.")
                                                        : QString::fromUtf8(result.errorText));
        return psiotr::OTR_MESSAGETYPE_IGNORE;
    case QcaOtr::SessionStatus::Error:
        displayProtocolError(account, contact, QObject::tr("Received message contains malformed OTR data."));
        return psiotr::OTR_MESSAGETYPE_IGNORE;
    case QcaOtr::SessionStatus::Ignored:
    case QcaOtr::SessionStatus::ProtocolMessage:
    case QcaOtr::SessionStatus::FragmentIncomplete:
    case QcaOtr::SessionStatus::Handled:
    case QcaOtr::SessionStatus::ForOtherInstance:
        return psiotr::OTR_MESSAGETYPE_IGNORE;
    }
    return psiotr::OTR_MESSAGETYPE_NONE;
}

QList<psiotr::Fingerprint> OtrInternal::getFingerprints()
{
    QList<psiotr::Fingerprint> result;
    if (!m_profileLoaded)
        return result;

    for (const QcaOtr::FingerprintRecord &record : m_profile->fingerprints()) {
        if (record.fingerprint.size() != 20)
            continue;
        result.append(psiotr::Fingerprint(record.fingerprint,
                                          QString::fromUtf8(record.account),
                                          QString::fromUtf8(record.username),
                                          QString::fromUtf8(record.trust)));
    }
    return result;
}

bool OtrInternal::activeFingerprintMatches(const psiotr::Fingerprint &fingerprint, Conversation **matched) const
{
    if (matched)
        *matched = nullptr;
    if (!fingerprint.isValid())
        return false;
    const auto it = m_conversations.constFind(conversationKey(fingerprint.account, fingerprint.username));
    if (it == m_conversations.constEnd() || it.value()->activeFingerprint != fingerprint.value)
        return false;
    if (matched)
        *matched = it.value().get();
    return true;
}

void OtrInternal::verifyFingerprint(const psiotr::Fingerprint &fingerprint, bool verified)
{
    if (!m_profileLoaded || !fingerprint.isValid())
        return;
    QString error;
    if (!m_profile->setFingerprintTrust(fingerprint.username.toUtf8(),
                                        fingerprint.account.toUtf8(),
                                        fingerprint.value,
                                        verified ? QObject::tr("verified").toUtf8() : QByteArray(),
                                        &error)) {
        displayProtocolError(fingerprint.account,
                             fingerprint.username,
                             QObject::tr("Cannot update OTR fingerprint trust: %1").arg(error));
        return;
    }

    Conversation *ctx = nullptr;
    if (activeFingerprintMatches(fingerprint, &ctx) && ctx)
        m_callback->stateChange(ctx->account, ctx->contact, psiotr::OTR_STATECHANGE_TRUST);
}

void OtrInternal::deleteFingerprint(const psiotr::Fingerprint &fingerprint)
{
    if (!m_profileLoaded || !fingerprint.isValid())
        return;

    Conversation *ctx = nullptr;
    const bool active = activeFingerprintMatches(fingerprint, &ctx);
    QString error;
    if (!m_profile->removeFingerprint(fingerprint.username.toUtf8(),
                                      fingerprint.account.toUtf8(),
                                      fingerprint.value,
                                      &error)) {
        displayProtocolError(fingerprint.account,
                             fingerprint.username,
                             QObject::tr("Cannot delete OTR fingerprint: %1").arg(error));
        return;
    }
    if (active && ctx) {
        const quint32 peer = activePeer(ctx);
        if (peer != 0)
            ctx->session->resetPeer(peer);
        ctx->forcedFinished = true;
    }
}

QHash<QString, QString> OtrInternal::getPrivateKeys()
{
    QHash<QString, QString> result;
    if (!m_profileLoaded)
        return result;

    for (const QcaOtr::PrivateKeyRecord &record : m_profile->identities()) {
        result.insert(QString::fromUtf8(record.account),
                      formatFingerprint(QcaOtr::dsaPublicKeyFingerprint(QcaOtr::dsaPublicKey(record.key))));
    }
    return result;
}

void OtrInternal::deleteKey(const QString &account)
{
    if (!m_profileLoaded)
        return;
    QString error;
    if (!m_profile->removeIdentity(account.toUtf8(), &error)) {
        displayProtocolError(account, {}, QObject::tr("Cannot delete OTR private key: %1").arg(error));
        return;
    }

    for (auto it = m_conversations.begin(); it != m_conversations.end();) {
        if (it.value()->account == account)
            it = m_conversations.erase(it);
        else
            ++it;
    }
}

void OtrInternal::generateKey(const QString &account)
{
    if (!m_profileLoaded)
        return;

    QString error;
    if (!m_profile->regenerateIdentity(account.toUtf8(), &error)) {
        QMessageBox failMb(QMessageBox::Critical,
                           QObject::tr("Confirm action"),
                           QObject::tr("Failed to generate keys for account \"%1\".\nThe OTR Plugin will not work.")
                               .arg(m_callback->humanAccount(account)),
                           QMessageBox::Ok);
        failMb.exec();
        return;
    }

    for (auto it = m_conversations.begin(); it != m_conversations.end();) {
        if (it.value()->account == account)
            it = m_conversations.erase(it);
        else
            ++it;
    }

    const QString human = formatFingerprint(m_profile->identityFingerprint(account.toUtf8()));
    QMessageBox infoMb(QMessageBox::Information,
                       QObject::tr("Confirm action"),
                       QObject::tr("Keys have been generated. Fingerprint for account \"%1\":\n%2\n\nThanks for your patience.")
                           .arg(m_callback->humanAccount(account), human));
    infoMb.exec();
}

void OtrInternal::startSession(const QString &account, const QString &contact)
{
    Conversation *ctx = conversation(account, contact, true);
    if (!ctx)
        return;

    ctx->forcedFinished = false;
    ctx->smpSucceeded = false;
    m_callback->stateChange(account, contact, psiotr::OTR_STATECHANGE_GOINGSECURE);
    const QcaOtr::OutgoingResult outgoing =
        ctx->session->startNegotiation(m_callback->humanAccountPublic(account).toUtf8());
    if (outgoing.status == QcaOtr::OutgoingStatus::Error || outgoing.messages.isEmpty()) {
        displayProtocolError(account, contact, QObject::tr("Could not start an OTR session."));
        return;
    }
    sendTransport(account, contact, outgoing.messages);
}

void OtrInternal::endSession(const QString &account, const QString &contact)
{
    Conversation *ctx = conversation(account, contact, false);
    if (!ctx)
        return;

    bool hadPrivateState = false;
    QVector<QByteArray> outgoing;
    const QVector<quint32> peers = ctx->session->peerInstances();
    for (quint32 peer : peers) {
        if (!ctx->session->isEncrypted(peer))
            continue;
        hadPrivateState = true;
        QVector<QByteArray> messages;
        if (ctx->session->disconnect(peer, &messages))
            outgoing += messages;
    }
    if (hadPrivateState)
        m_callback->stateChange(account, contact, psiotr::OTR_STATECHANGE_CLOSE);
    sendTransport(account, contact, outgoing);
    ctx->forcedFinished = false;
}

void OtrInternal::expireSession(const QString &account, const QString &contact)
{
    Conversation *ctx = conversation(account, contact, false);
    if (!ctx || getMessageState(account, contact) != psiotr::OTR_MESSAGESTATE_ENCRYPTED)
        return;
    const quint32 peer = activePeer(ctx);
    if (peer != 0)
        ctx->session->resetPeer(peer);
    ctx->forcedFinished = true;
    m_callback->stateChange(account, contact, psiotr::OTR_STATECHANGE_GONEINSECURE);
}

void OtrInternal::startSMP(const QString &account,
                           const QString &contact,
                           const QString &question,
                           const QString &secret)
{
    Conversation *ctx = conversation(account, contact, false);
    if (!ctx)
        return;
    QVector<QByteArray> messages;
    const QCA::SecureArray secure = secureUtf8(secret);
    const bool ok = question.isEmpty() ? ctx->session->startSmp(0, secure, &messages)
                                       : ctx->session->startSmp(0, question.toUtf8(), secure, &messages);
    if (!ok) {
        m_callback->updateSMP(account, contact, -2);
        return;
    }
    ctx->smpSucceeded = false;
    sendTransport(account, contact, messages);
}

void OtrInternal::continueSMP(const QString &account, const QString &contact, const QString &secret)
{
    Conversation *ctx = conversation(account, contact, false);
    if (!ctx)
        return;
    QVector<QByteArray> messages;
    const QCA::SecureArray secure = secureUtf8(secret);
    if (!ctx->session->respondSmp(0, secure, &messages)) {
        m_callback->updateSMP(account, contact, -2);
        return;
    }
    sendTransport(account, contact, messages);
}

void OtrInternal::abortSMP(const QString &account, const QString &contact)
{
    Conversation *ctx = conversation(account, contact, false);
    if (!ctx)
        return;
    QVector<QByteArray> messages;
    if (ctx->session->abortSmp(0, &messages))
        sendTransport(account, contact, messages);
    ctx->smpSucceeded = false;
}

psiotr::OtrMessageState OtrInternal::getMessageState(const QString &account, const QString &contact)
{
    Conversation *ctx = conversation(account, contact, false);
    if (!ctx)
        return psiotr::OTR_MESSAGESTATE_UNKNOWN;

    ctx->session->setPolicy(nativePolicy(m_otrPolicy));
    const quint32 peer = activePeer(ctx);
    if (peer != 0) {
        const QcaOtr::PeerState state = ctx->session->peerState(peer);
        if (state == QcaOtr::PeerState::Encrypted)
            return psiotr::OTR_MESSAGESTATE_ENCRYPTED;
        if (state == QcaOtr::PeerState::Finished)
            return psiotr::OTR_MESSAGESTATE_FINISHED;
    }
    if (ctx->forcedFinished)
        return psiotr::OTR_MESSAGESTATE_FINISHED;

    for (quint32 other : ctx->session->peerInstances()) {
        if (ctx->session->isEncrypted(other))
            return psiotr::OTR_MESSAGESTATE_ENCRYPTED;
        if (ctx->session->peerState(other) == QcaOtr::PeerState::Finished)
            return psiotr::OTR_MESSAGESTATE_FINISHED;
    }
    return psiotr::OTR_MESSAGESTATE_PLAINTEXT;
}

QString OtrInternal::getMessageStateString(const QString &account, const QString &contact)
{
    switch (getMessageState(account, contact)) {
    case psiotr::OTR_MESSAGESTATE_PLAINTEXT: return QObject::tr("plaintext");
    case psiotr::OTR_MESSAGESTATE_ENCRYPTED: return QObject::tr("encrypted");
    case psiotr::OTR_MESSAGESTATE_FINISHED: return QObject::tr("finished");
    case psiotr::OTR_MESSAGESTATE_UNKNOWN: break;
    }
    return QObject::tr("unknown");
}

QString OtrInternal::getSessionId(const QString &account, const QString &contact)
{
    const Conversation *ctx = conversation(account, contact);
    if (!ctx || ctx->sessionId.isEmpty())
        return {};

    const int half = ctx->sessionId.size() / 2;
    const QString first = QString::fromLatin1(ctx->sessionId.left(half).toHex());
    const QString second = QString::fromLatin1(ctx->sessionId.mid(half).toHex());
    return ctx->initiated ? QStringLiteral("<b>%1</b> %2").arg(first, second)
                          : QStringLiteral("%1 <b>%2</b>").arg(first, second);
}

psiotr::Fingerprint OtrInternal::getActiveFingerprint(const QString &account, const QString &contact)
{
    Conversation *ctx = conversation(account, contact, false);
    if (!ctx || ctx->activeFingerprint.size() != 20)
        return {};

    const QcaOtr::FingerprintRecord *record =
        m_profile->fingerprint(contact.toUtf8(), account.toUtf8(), ctx->activeFingerprint);
    return psiotr::Fingerprint(ctx->activeFingerprint,
                               account,
                               contact,
                               record ? QString::fromUtf8(record->trust) : QString());
}

bool OtrInternal::isVerified(const QString &account, const QString &contact)
{
    const Conversation *ctx = conversation(account, contact);
    if (!ctx || ctx->activeFingerprint.size() != 20 || !m_profileLoaded)
        return false;
    const QcaOtr::FingerprintRecord *record =
        m_profile->fingerprint(contact.toUtf8(), account.toUtf8(), ctx->activeFingerprint);
    return record && !record->trust.isEmpty();
}

bool OtrInternal::smpSucceeded(const QString &account, const QString &contact)
{
    const Conversation *ctx = conversation(account, contact);
    return ctx && ctx->smpSucceeded;
}
