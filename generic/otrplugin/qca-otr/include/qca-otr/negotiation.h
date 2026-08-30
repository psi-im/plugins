#pragma once

#include <QByteArray>
#include <QtGlobal>

namespace QcaOtr::Negotiation {

using VersionMask = quint8;

constexpr VersionMask Version1 = 0x01;
constexpr VersionMask Version2 = 0x02;
constexpr VersionMask Version3 = 0x04;
constexpr VersionMask NativeVersions = Version3;

// Return the compact query marker used at the start of an OTR Query Message.
// The layout intentionally follows otrl_proto_default_query_msg(): v1 is
// represented by '?' and v2/v3 by a following v...?... group.
QByteArray queryTag(VersionMask versions = NativeVersions);

// libotr-compatible default Query Message, including the human-readable
// fallback text for peers without OTR support.
QByteArray defaultQueryMessage(const QByteArray &ourName,
                               VersionMask versions = NativeVersions);

// Inspect the first ?OTR occurrence exactly as libotr does. isQuery is true
// only when that occurrence has Query Message syntax (?OTR? or ?OTRv...).
VersionMask queryVersions(const QByteArray &message, bool *isQuery = nullptr);
quint8 bestVersion(VersionMask peerVersions,
                   VersionMask localVersions = NativeVersions);
quint8 queryBestVersion(const QByteArray &message,
                        VersionMask localVersions = NativeVersions,
                        bool *isQuery = nullptr);

// OTR Error Messages are text-level protocol messages. As with libotr's
// message classifier, only the first ?OTR occurrence is considered.
QByteArray errorMessage(const QByteArray &text);
bool parseErrorMessage(const QByteArray &message, QByteArray *text = nullptr);

struct WhitespaceTag
{
    bool found = false;
    VersionMask versions = 0;
    int start = -1;
    int end = -1;
};

QByteArray whitespaceTag(VersionMask versions = NativeVersions);
WhitespaceTag detectWhitespaceTag(const QByteArray &message);
quint8 whitespaceBestVersion(const QByteArray &message,
                             VersionMask localVersions = NativeVersions,
                             WhitespaceTag *tag = nullptr);
bool stripWhitespaceTag(QByteArray *message, VersionMask *versions = nullptr);
QByteArray appendWhitespaceTag(const QByteArray &message,
                               VersionMask versions = NativeVersions);

} // namespace QcaOtr::Negotiation
