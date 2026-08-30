#include "qca-otr/session.h"

#include "qca-otr/data.h"
#include "qca-otr/negotiation.h"
#include "qca-otr/transport.h"

#include <map>
#include <memory>
#include <utility>

namespace QcaOtr {
namespace {

constexpr quint8 DhCommitType = 0x02;
constexpr quint8 DataMessageType = 0x03;
constexpr quint8 DhKeyType = 0x0a;
constexpr quint8 RevealSignatureType = 0x11;
constexpr quint8 SignatureType = 0x12;

const QByteArray UnreadableErrorText("Unreadable encrypted message.");

bool isAkeType(quint8 type)
{
    return type == DhCommitType || type == DhKeyType || type == RevealSignatureType || type == SignatureType;
}

SessionResult result(SessionStatus status, quint32 peerInstance = 0)
{
    SessionResult value;
    value.status = status;
    value.peerInstance = peerInstance;
    return value;
}

bool encodeOutgoing(const QByteArray &raw, int maxMessageSize, QVector<QByteArray> *messages)
{
    if (!messages || raw.isEmpty())
        return false;

    Transport::Route route;
    if (!Transport::routeFromRaw(raw, &route))
        return false;

    return Transport::fragmentMessage(Transport::armor(raw),
                                      maxMessageSize,
                                      route.senderInstance,
                                      route.receiverInstance,
                                      messages);
}

bool startsAkeFromWhitespace(SessionPolicy policy)
{
    return policy == SessionPolicy::Opportunistic || policy == SessionPolicy::Always;
}

bool rawDataFlags(const QByteArray &raw, quint8 *flags)
{
    if (flags)
        *flags = 0;
    if (raw.size() < 12 || static_cast<quint8>(raw.at(0)) != 0x00 ||
        static_cast<quint8>(raw.at(1)) != 0x03 ||
        static_cast<quint8>(raw.at(2)) != DataMessageType) {
        return false;
    }

    if (flags)
        *flags = static_cast<quint8>(raw.at(11));
    return true;
}

bool containsTlv(const QVector<Tlv> &tlvs, TlvType type)
{
    const quint16 wanted = static_cast<quint16>(type);
    for (const Tlv &tlv : tlvs) {
        if (tlv.type == wanted)
            return true;
    }
    return false;
}

bool parseSymmetricKeyTlv(const QVector<Tlv> &tlvs, quint32 *use, QByteArray *useData)
{
    if (use)
        *use = 0;
    if (useData)
        useData->clear();

    const quint16 wanted = static_cast<quint16>(TlvType::SymmetricKey);
    for (const Tlv &tlv : tlvs) {
        if (tlv.type != wanted || tlv.value.size() < 4)
            continue;

        const auto *data = reinterpret_cast<const unsigned char *>(tlv.value.constData());
        const quint32 parsedUse = (static_cast<quint32>(data[0]) << 24) |
            (static_cast<quint32>(data[1]) << 16) |
            (static_cast<quint32>(data[2]) << 8) |
            static_cast<quint32>(data[3]);
        if (use)
            *use = parsedUse;
        if (useData)
            *useData = tlv.value.mid(4);
        return true;
    }
    return false;
}

} // namespace

struct OtrSession::Private
{
    enum class OfferState {
        None,
        Sent,
        Accepted,
        Rejected
    };

    struct PeerContext
    {
        PeerContext(const DsaPrivateKey &identityKey, quint32 localInstance, quint32 peerInstance) :
            ake(identityKey, localInstance, peerInstance)
        {
        }

        explicit PeerContext(AkeSession clonedAke) : ake(std::move(clonedAke)) { }

        AkeSession ake;
        std::unique_ptr<DataSession> data;
        AkeEstablishedSession established;
        bool hasEstablished = false;
        PeerState state = PeerState::Plaintext;
    };

    Private(const DsaPrivateKey &identity, quint32 instance) : identityKey(identity), localInstance(instance) { }

    PeerContext *findPeer(quint32 peerInstance)
    {
        const auto it = peers.find(peerInstance);
        return it == peers.end() ? nullptr : it->second.get();
    }

    const PeerContext *findPeer(quint32 peerInstance) const
    {
        const auto it = peers.find(peerInstance);
        return it == peers.end() ? nullptr : it->second.get();
    }

    PeerContext *ensurePeer(quint32 peerInstance)
    {
        if (PeerContext *existing = findPeer(peerInstance))
            return existing;

        auto peer = std::make_unique<PeerContext>(identityKey, localInstance, peerInstance);
        PeerContext *result = peer.get();
        peers.emplace(peerInstance, std::move(peer));
        return result;
    }

    PeerContext *cloneMasterToPeer(quint32 peerInstance)
    {
        if (!masterAke || masterAke->state() != AkeState::AwaitingDhKey)
            return nullptr;

        AkeSession clone = *masterAke;
        if (!clone.bindPeerInstance(peerInstance))
            return nullptr;

        if (PeerContext *existing = findPeer(peerInstance)) {
            existing->ake = std::move(clone);
            return existing;
        }

        auto peer = std::make_unique<PeerContext>(std::move(clone));
        PeerContext *result = peer.get();
        peers.emplace(peerInstance, std::move(peer));
        return result;
    }

    void forcePeerState(PeerContext *peer, PeerState state)
    {
        if (!peer)
            return;
        peer->ake.reset();
        peer->data.reset();
        peer->established = AkeEstablishedSession();
        peer->hasEstablished = false;
        peer->state = state;
    }

    quint32 preferredEncryptedPeer(quint32 requested) const
    {
        if (requested != 0) {
            const PeerContext *peer = findPeer(requested);
            return peer && peer->state == PeerState::Encrypted && peer->data && peer->data->isReady()
                ? requested
                : 0;
        }

        const PeerContext *active = findPeer(activePeerInstance);
        if (active && active->state == PeerState::Encrypted && active->data && active->data->isReady())
            return activePeerInstance;

        for (const auto &entry : peers) {
            const PeerContext *peer = entry.second.get();
            if (peer->state == PeerState::Encrypted && peer->data && peer->data->isReady())
                return entry.first;
        }
        return 0;
    }

    quint32 preferredFinishedPeer(quint32 requested) const
    {
        if (requested != 0) {
            const PeerContext *peer = findPeer(requested);
            return peer && peer->state == PeerState::Finished ? requested : 0;
        }

        const PeerContext *active = findPeer(activePeerInstance);
        if (active && active->state == PeerState::Finished)
            return activePeerInstance;

        for (const auto &entry : peers) {
            if (entry.second->state == PeerState::Finished)
                return entry.first;
        }
        return 0;
    }

    void clearPendingRequired()
    {
        pendingRequiredMessage.clear();
        hasPendingRequired = false;
        pendingRequiredFlags = 0;
        pendingRequiredMms = 0;
    }

    DsaPrivateKey identityKey;
    quint32 localInstance = 0;
    SessionPolicy policy = SessionPolicy::Manual;
    OfferState offer = OfferState::None;
    quint32 activePeerInstance = 0;
    QByteArray queryName;
    QByteArray pendingRequiredMessage;
    bool hasPendingRequired = false;
    quint8 pendingRequiredFlags = 0;
    int pendingRequiredMms = 0;
    std::unique_ptr<AkeSession> masterAke;
    std::map<quint32, std::unique_ptr<PeerContext>> peers;
    std::map<quint32, Transport::FragmentAccumulator> fragments;
};

OtrSession::OtrSession(const DsaPrivateKey &identityKey, quint32 localInstance) :
    d(std::make_unique<Private>(identityKey, localInstance))
{
}

OtrSession::~OtrSession() = default;
OtrSession::OtrSession(OtrSession &&) noexcept = default;
OtrSession &OtrSession::operator=(OtrSession &&) noexcept = default;

quint32 OtrSession::localInstance() const
{
    return d->localInstance;
}

void OtrSession::setPolicy(SessionPolicy policy)
{
    d->policy = policy;
    if (policy != SessionPolicy::Always)
        d->clearPendingRequired();
    if (policy != SessionPolicy::Opportunistic)
        d->offer = Private::OfferState::None;
}

SessionPolicy OtrSession::policy() const
{
    return d->policy;
}

OutgoingResult OtrSession::startNegotiation(const QByteArray &ourName)
{
    OutgoingResult outgoing;
    if (d->policy == SessionPolicy::Disabled)
        return outgoing;

    d->queryName = ourName;
    outgoing.status = OutgoingStatus::Negotiation;
    outgoing.messages.append(Negotiation::defaultQueryMessage(ourName));
    if (d->policy == SessionPolicy::Opportunistic)
        d->offer = Private::OfferState::Sent;
    return outgoing;
}

OutgoingResult OtrSession::prepareOutgoing(const QByteArray &plaintext,
                                           quint32 peerInstance,
                                           const QByteArray &ourName,
                                           int maxMessageSize,
                                           quint8 flags)
{
    OutgoingResult outgoing;
    if (!ourName.isEmpty())
        d->queryName = ourName;

    const quint32 encryptedPeer = d->preferredEncryptedPeer(peerInstance);
    if (encryptedPeer != 0) {
        if (!sendMessage(encryptedPeer, plaintext, &outgoing.messages, maxMessageSize, flags))
            return outgoing;
        outgoing.status = OutgoingStatus::Encrypted;
        outgoing.peerInstance = encryptedPeer;
        d->activePeerInstance = encryptedPeer;
        return outgoing;
    }

    const quint32 finishedPeer = d->preferredFinishedPeer(peerInstance);
    if (finishedPeer != 0) {
        outgoing.status = OutgoingStatus::Finished;
        outgoing.peerInstance = finishedPeer;
        return outgoing;
    }

    switch (d->policy) {
    case SessionPolicy::Disabled:
    case SessionPolicy::Manual:
        outgoing.status = OutgoingStatus::Plaintext;
        outgoing.messages.append(plaintext);
        return outgoing;
    case SessionPolicy::Opportunistic:
        outgoing.status = OutgoingStatus::Plaintext;
        if (d->offer == Private::OfferState::Rejected) {
            outgoing.messages.append(plaintext);
        } else {
            outgoing.messages.append(Negotiation::appendWhitespaceTag(plaintext));
            d->offer = Private::OfferState::Sent;
        }
        return outgoing;
    case SessionPolicy::Always:
        d->pendingRequiredMessage = plaintext;
        d->hasPendingRequired = true;
        d->pendingRequiredFlags = flags;
        d->pendingRequiredMms = maxMessageSize;
        outgoing.status = OutgoingStatus::Negotiation;
        outgoing.messages.append(Negotiation::defaultQueryMessage(d->queryName));
        return outgoing;
    }

    return outgoing;
}

QVector<QByteArray> OtrSession::start(quint32 peerInstance, int maxMessageSize, bool *ok)
{
    if (ok)
        *ok = false;

    AkeSession *ake = nullptr;
    if (peerInstance == 0) {
        d->masterAke = std::make_unique<AkeSession>(d->identityKey, d->localInstance, 0);
        ake = d->masterAke.get();
    } else {
        if (peerInstance < Transport::MinimumInstanceTag)
            return {};
        ake = &d->ensurePeer(peerInstance)->ake;
    }

    bool started = false;
    const QByteArray raw = ake->start(&started);
    if (!started || raw.isEmpty())
        return {};

    QVector<QByteArray> messages;
    if (!encodeOutgoing(raw, maxMessageSize, &messages)) {
        ake->reset();
        if (peerInstance == 0)
            d->masterAke.reset();
        return {};
    }

    if (ok)
        *ok = true;
    return messages;
}

SessionResult OtrSession::processIncoming(const QByteArray &transportMessage, int maxMessageSize)
{
    bool isQuery = false;
    const quint8 queryVersion = Negotiation::queryBestVersion(transportMessage,
                                                               Negotiation::NativeVersions,
                                                               &isQuery);
    if (isQuery) {
        if (d->policy == SessionPolicy::Opportunistic)
            d->offer = Private::OfferState::Accepted;

        SessionResult routed = result(SessionStatus::ProtocolMessage);
        if (d->policy != SessionPolicy::Disabled && queryVersion == 3) {
            bool ok = false;
            routed.outgoingMessages = start(0, maxMessageSize, &ok);
            if (!ok)
                routed.status = SessionStatus::Error;
        }
        return routed;
    }

    QByteArray remoteErrorText;
    if (Negotiation::parseErrorMessage(transportMessage, &remoteErrorText)) {
        SessionResult routed = result(SessionStatus::RemoteError);
        routed.errorText = remoteErrorText;
        if (d->policy == SessionPolicy::Opportunistic || d->policy == SessionPolicy::Always)
            routed.outgoingMessages.append(Negotiation::defaultQueryMessage(d->queryName));
        return routed;
    }

    const Negotiation::WhitespaceTag whitespace = Negotiation::detectWhitespaceTag(transportMessage);
    if (whitespace.found) {
        QByteArray stripped = transportMessage;
        Negotiation::VersionMask versions = 0;
        Negotiation::stripWhitespaceTag(&stripped, &versions);

        SessionResult routed = result(SessionStatus::Plaintext);
        routed.plaintext = stripped;
        if (d->policy == SessionPolicy::Opportunistic)
            d->offer = Private::OfferState::Accepted;

        if (startsAkeFromWhitespace(d->policy) &&
            Negotiation::bestVersion(versions, Negotiation::NativeVersions) == 3) {
            bool ok = false;
            routed.outgoingMessages = start(0, maxMessageSize, &ok);
            if (!ok)
                routed.status = SessionStatus::Error;
        }
        return routed;
    }

    QByteArray completeTransport;
    Transport::Route fragmentRoute;
    bool hadFragmentRoute = false;

    Transport::Fragment fragment;
    const Transport::FragmentParseStatus fragmentStatus = Transport::parseFragment(transportMessage, &fragment);
    if (fragmentStatus == Transport::FragmentParseStatus::Malformed) {
        d->fragments.clear();
        return result(SessionStatus::Error);
    }

    if (fragmentStatus == Transport::FragmentParseStatus::Fragment) {
        if (fragment.route.receiverInstance != 0 && fragment.route.receiverInstance != d->localInstance)
            return result(SessionStatus::ForOtherInstance, fragment.route.senderInstance);

        hadFragmentRoute = true;
        fragmentRoute = fragment.route;
        auto [it, inserted] = d->fragments.try_emplace(fragment.route.senderInstance);
        Q_UNUSED(inserted);
        const Transport::FragmentResult accumulated = it->second.accumulate(transportMessage, &completeTransport);
        if (accumulated == Transport::FragmentResult::Incomplete)
            return result(SessionStatus::FragmentIncomplete, fragment.route.senderInstance);
        if (accumulated != Transport::FragmentResult::Complete) {
            d->fragments.erase(it);
            return result(SessionStatus::Error, fragment.route.senderInstance);
        }
        d->fragments.erase(it);
    } else {
        completeTransport = transportMessage;
    }

    QByteArray raw;
    if (!Transport::dearmor(completeTransport, &raw)) {
        if (completeTransport.contains("?OTR:") || hadFragmentRoute)
            return result(SessionStatus::Error, fragmentRoute.senderInstance);
        if (completeTransport.contains("?OTR"))
            return result(SessionStatus::ProtocolMessage);

        if (d->policy == SessionPolicy::Opportunistic && d->offer == Private::OfferState::Sent)
            d->offer = Private::OfferState::Rejected;
        SessionResult plaintextResult = result(SessionStatus::Plaintext);
        plaintextResult.plaintext = completeTransport;
        return plaintextResult;
    }

    if (d->policy == SessionPolicy::Opportunistic)
        d->offer = Private::OfferState::Accepted;

    Transport::Route route;
    if (!Transport::routeFromRaw(raw, &route))
        return result(SessionStatus::Error);

    if (hadFragmentRoute &&
        (fragmentRoute.senderInstance != route.senderInstance ||
         fragmentRoute.receiverInstance != route.receiverInstance)) {
        return result(SessionStatus::Error, route.senderInstance);
    }

    if (!hadFragmentRoute)
        d->fragments.erase(route.senderInstance);

    if (route.receiverInstance != 0 && route.receiverInstance != d->localInstance)
        return result(SessionStatus::ForOtherInstance, route.senderInstance);
    if (raw.size() < 3)
        return result(SessionStatus::Error, route.senderInstance);

    const quint8 type = static_cast<quint8>(raw.at(2));
    if (route.receiverInstance == 0 && type != DhCommitType)
        return result(SessionStatus::Error, route.senderInstance);

    if (isAkeType(type)) {
        Private::PeerContext *peer = d->findPeer(route.senderInstance);

        // libotr keeps a master AKE for a broadcast Commit and copies that
        // state into an instance child when a D-H Key arrives. Do the same so
        // several remote instances can answer the same Commit independently.
        if (type == DhKeyType && d->masterAke && d->masterAke->state() == AkeState::AwaitingDhKey)
            peer = d->cloneMasterToPeer(route.senderInstance);
        else if (type == DhCommitType && d->masterAke && d->masterAke->state() == AkeState::AwaitingDhKey)
            peer = d->cloneMasterToPeer(route.senderInstance);
        else if (!peer && type == DhCommitType)
            peer = d->ensurePeer(route.senderInstance);

        if (!peer)
            return result(SessionStatus::Ignored, route.senderInstance);

        const AkeHandleResult akeResult = peer->ake.processIncoming(raw);
        SessionResult routed;
        routed.peerInstance = route.senderInstance;

        switch (akeResult.status) {
        case AkeHandleStatus::Ignored:
            routed.status = SessionStatus::Ignored;
            return routed;
        case AkeHandleStatus::Error:
            routed.status = SessionStatus::Error;
            return routed;
        case AkeHandleStatus::Handled:
            routed.status = SessionStatus::Handled;
            break;
        case AkeHandleStatus::Authenticated: {
            auto data = std::make_unique<DataSession>(peer->ake.established(),
                                                      d->localInstance,
                                                      route.senderInstance);
            if (!data->isReady()) {
                routed.status = SessionStatus::Error;
                return routed;
            }
            peer->established = peer->ake.established();
            peer->hasEstablished = true;
            peer->data = std::move(data);
            peer->state = PeerState::Encrypted;
            d->activePeerInstance = route.senderInstance;
            routed.status = SessionStatus::Authenticated;
            break;
        }
        }

        if (!akeResult.outgoingMessage.isEmpty() &&
            !encodeOutgoing(akeResult.outgoingMessage, maxMessageSize, &routed.outgoingMessages)) {
            routed.outgoingMessages.clear();
            routed.status = SessionStatus::Error;
            return routed;
        }

        if (routed.status == SessionStatus::Authenticated && d->hasPendingRequired) {
            QVector<QByteArray> pendingMessages;
            if (!sendMessage(route.senderInstance,
                             d->pendingRequiredMessage,
                             &pendingMessages,
                             d->pendingRequiredMms,
                             d->pendingRequiredFlags)) {
                routed.status = SessionStatus::Error;
                return routed;
            }
            routed.outgoingMessages += pendingMessages;
            d->clearPendingRequired();
        }
        return routed;
    }

    if (type == DataMessageType) {
        quint8 flags = 0;
        const bool haveFlags = rawDataFlags(raw, &flags);
        Private::PeerContext *peer = d->findPeer(route.senderInstance);
        if (!peer || peer->state != PeerState::Encrypted || !peer->data || !peer->data->isReady()) {
            if (haveFlags && (flags & DataFlagIgnoreUnreadable))
                return result(SessionStatus::Ignored, route.senderInstance);

            SessionResult routed = result(SessionStatus::Unreadable, route.senderInstance);
            routed.flags = flags;
            routed.errorText = UnreadableErrorText;
            return routed;
        }

        const DataReceiveResult dataResult = peer->data->processIncoming(raw);
        SessionResult routed;
        routed.peerInstance = route.senderInstance;
        routed.flags = haveFlags ? flags : dataResult.flags;
        routed.extraKey = dataResult.extraKey;
        routed.tlvs = dataResult.tlvs;

        switch (dataResult.status) {
        case DataReceiveStatus::Ignored:
            routed.status = SessionStatus::Ignored;
            break;
        case DataReceiveStatus::Error:
            if (haveFlags && (flags & DataFlagIgnoreUnreadable)) {
                routed.status = SessionStatus::Ignored;
            } else {
                routed.status = SessionStatus::Unreadable;
                routed.errorText = UnreadableErrorText;
                routed.outgoingMessages.append(Negotiation::errorMessage(UnreadableErrorText));
            }
            break;
        case DataReceiveStatus::Message: {
            routed.plaintext = dataResult.plaintext;
            d->activePeerInstance = route.senderInstance;

            quint32 use = 0;
            QByteArray useData;
            if (parseSymmetricKeyTlv(dataResult.tlvs, &use, &useData)) {
                routed.hasSymmetricKey = true;
                routed.symmetricKeyUse = use;
                routed.symmetricKeyData = useData;
                routed.symmetricKey = dataResult.extraKey;
            }

            if (containsTlv(dataResult.tlvs, TlvType::Disconnected)) {
                d->forcePeerState(peer, PeerState::Finished);
                routed.status = SessionStatus::Disconnected;
            } else {
                routed.status = SessionStatus::Message;
            }
            break;
        }
        }
        return routed;
    }

    return result(SessionStatus::Ignored, route.senderInstance);
}

bool OtrSession::sendMessage(quint32 peerInstance,
                             const QByteArray &plaintext,
                             QVector<QByteArray> *transportMessages,
                             int maxMessageSize,
                             quint8 flags)
{
    return sendMessage(peerInstance, plaintext, {}, transportMessages, maxMessageSize, flags);
}

bool OtrSession::sendMessage(quint32 peerInstance,
                             const QByteArray &plaintext,
                             const QVector<Tlv> &tlvs,
                             QVector<QByteArray> *transportMessages,
                             int maxMessageSize,
                             quint8 flags)
{
    if (!transportMessages)
        return false;
    transportMessages->clear();

    Private::PeerContext *peer = d->findPeer(peerInstance);
    if (!peer || peer->state != PeerState::Encrypted || !peer->data || !peer->data->isReady())
        return false;

    // Creating a Data Message advances counters and may rotate keys. Preserve
    // the previous state until the transport layer has also accepted the
    // message; an invalid MMS must not consume ratchet state.
    DataSession snapshot = *peer->data;
    QByteArray raw;
    if (!peer->data->sendMessage(plaintext, tlvs, &raw, flags))
        return false;

    if (!encodeOutgoing(raw, maxMessageSize, transportMessages)) {
        *peer->data = std::move(snapshot);
        transportMessages->clear();
        return false;
    }
    d->activePeerInstance = peerInstance;
    return true;
}

bool OtrSession::disconnect(quint32 peerInstance,
                            QVector<QByteArray> *transportMessages,
                            int maxMessageSize)
{
    if (!transportMessages)
        return false;
    transportMessages->clear();

    if (peerInstance < Transport::MinimumInstanceTag)
        return false;

    Private::PeerContext *peer = d->findPeer(peerInstance);
    if (!peer)
        return true;

    if (peer->state == PeerState::Encrypted && peer->data && peer->data->isReady()) {
        QVector<Tlv> tlvs;
        tlvs.append(Tlv {static_cast<quint16>(TlvType::Disconnected), QByteArray()});
        if (!sendMessage(peerInstance,
                         QByteArray(),
                         tlvs,
                         transportMessages,
                         maxMessageSize,
                         DataFlagIgnoreUnreadable)) {
            return false;
        }
    }

    d->forcePeerState(peer, PeerState::Plaintext);
    if (d->activePeerInstance == peerInstance)
        d->activePeerInstance = 0;
    return true;
}

bool OtrSession::sendSymmetricKey(quint32 peerInstance,
                                  quint32 use,
                                  const QByteArray &useData,
                                  QCA::SecureArray *symmetricKey,
                                  QVector<QByteArray> *transportMessages,
                                  int maxMessageSize)
{
    if (!symmetricKey || !transportMessages || useData.size() > 65531)
        return false;
    *symmetricKey = QCA::SecureArray();
    transportMessages->clear();

    Private::PeerContext *peer = d->findPeer(peerInstance);
    if (!peer || peer->state != PeerState::Encrypted || !peer->data || !peer->data->isReady())
        return false;

    const QCA::SecureArray extraKey = peer->data->currentSendExtraKey();
    if (extraKey.size() != 32)
        return false;

    QByteArray value(4, '\0');
    value[0] = static_cast<char>((use >> 24) & 0xff);
    value[1] = static_cast<char>((use >> 16) & 0xff);
    value[2] = static_cast<char>((use >> 8) & 0xff);
    value[3] = static_cast<char>(use & 0xff);
    value.append(useData);

    QVector<Tlv> tlvs;
    tlvs.append(Tlv {static_cast<quint16>(TlvType::SymmetricKey), value});
    if (!sendMessage(peerInstance,
                     QByteArray(),
                     tlvs,
                     transportMessages,
                     maxMessageSize,
                     DataFlagIgnoreUnreadable)) {
        return false;
    }

    *symmetricKey = extraKey;
    return true;
}

PeerState OtrSession::peerState(quint32 peerInstance) const
{
    const Private::PeerContext *peer = d->findPeer(peerInstance);
    return peer ? peer->state : PeerState::Plaintext;
}

bool OtrSession::isEncrypted(quint32 peerInstance) const
{
    const Private::PeerContext *peer = d->findPeer(peerInstance);
    return peer && peer->state == PeerState::Encrypted && peer->data && peer->data->isReady();
}

QVector<quint32> OtrSession::peerInstances() const
{
    QVector<quint32> result;
    result.reserve(static_cast<int>(d->peers.size()));
    for (const auto &entry : d->peers)
        result.append(entry.first);
    return result;
}

bool OtrSession::establishedSession(quint32 peerInstance, AkeEstablishedSession *established) const
{
    if (!established)
        return false;
    const Private::PeerContext *peer = d->findPeer(peerInstance);
    if (!peer || peer->state != PeerState::Encrypted || !peer->hasEstablished)
        return false;
    *established = peer->established;
    return true;
}

void OtrSession::resetPeer(quint32 peerInstance)
{
    d->peers.erase(peerInstance);
    d->fragments.erase(peerInstance);
    if (d->activePeerInstance == peerInstance)
        d->activePeerInstance = 0;
}

void OtrSession::reset()
{
    d->masterAke.reset();
    d->peers.clear();
    d->fragments.clear();
    d->activePeerInstance = 0;
    d->offer = Private::OfferState::None;
    d->clearPendingRequired();
}

} // namespace QcaOtr
