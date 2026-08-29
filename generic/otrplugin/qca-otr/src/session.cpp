#include "qca-otr/session.h"

#include "qca-otr/data.h"
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

} // namespace

struct OtrSession::Private
{
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

    DsaPrivateKey identityKey;
    quint32 localInstance = 0;
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
        return result(SessionStatus::Ignored);
    }

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
            routed.status = SessionStatus::Authenticated;
            break;
        }
        }

        if (!akeResult.outgoingMessage.isEmpty() &&
            !encodeOutgoing(akeResult.outgoingMessage, maxMessageSize, &routed.outgoingMessages)) {
            routed.outgoingMessages.clear();
            routed.status = SessionStatus::Error;
        }
        return routed;
    }

    if (type == DataMessageType) {
        Private::PeerContext *peer = d->findPeer(route.senderInstance);
        if (!peer || !peer->data || !peer->data->isReady())
            return result(SessionStatus::Ignored, route.senderInstance);

        const DataReceiveResult dataResult = peer->data->processIncoming(raw);
        SessionResult routed;
        routed.peerInstance = route.senderInstance;
        routed.flags = dataResult.flags;
        routed.extraKey = dataResult.extraKey;
        switch (dataResult.status) {
        case DataReceiveStatus::Ignored:
            routed.status = SessionStatus::Ignored;
            break;
        case DataReceiveStatus::Error:
            routed.status = SessionStatus::Error;
            break;
        case DataReceiveStatus::Message:
            routed.status = SessionStatus::Message;
            routed.plaintext = dataResult.plaintext;
            break;
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
    if (!transportMessages)
        return false;
    transportMessages->clear();

    Private::PeerContext *peer = d->findPeer(peerInstance);
    if (!peer || !peer->data || !peer->data->isReady())
        return false;

    // Creating a Data Message advances counters and may rotate keys. Preserve
    // the previous state until the transport layer has also accepted the
    // message; an invalid MMS must not consume ratchet state.
    DataSession snapshot = *peer->data;
    QByteArray raw;
    if (!peer->data->sendMessage(plaintext, &raw, flags))
        return false;

    if (!encodeOutgoing(raw, maxMessageSize, transportMessages)) {
        *peer->data = std::move(snapshot);
        transportMessages->clear();
        return false;
    }
    return true;
}

bool OtrSession::isEncrypted(quint32 peerInstance) const
{
    const Private::PeerContext *peer = d->findPeer(peerInstance);
    return peer && peer->data && peer->data->isReady();
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
    if (!peer || !peer->hasEstablished)
        return false;
    *established = peer->established;
    return true;
}

void OtrSession::resetPeer(quint32 peerInstance)
{
    d->peers.erase(peerInstance);
    d->fragments.erase(peerInstance);
}

void OtrSession::reset()
{
    d->masterAke.reset();
    d->peers.clear();
    d->fragments.clear();
}

} // namespace QcaOtr
