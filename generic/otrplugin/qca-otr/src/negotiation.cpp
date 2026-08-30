/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#include "qca-otr/negotiation.h"

namespace QcaOtr::Negotiation {
namespace {

const QByteArray ErrorTypePrefix("?OTR Error:");
const QByteArray ErrorMessagePrefix("?OTR Error: ");
const QByteArray BaseWhitespaceTag(" \t  \t\t\t\t \t \t \t  ");
const QByteArray V1WhitespaceTag(" \t \t  \t ");
const QByteArray V2WhitespaceTag("  \t\t  \t ");
const QByteArray V3WhitespaceTag("  \t\t  \t\t");

bool isWhitespaceGroup(const QByteArray &message, int offset)
{
    if (offset < 0 || offset + 8 > message.size())
        return false;

    for (int i = 0; i < 8; ++i) {
        const char ch = message.at(offset + i);
        if (ch != ' ' && ch != '\t')
            return false;
    }
    return true;
}

} // namespace

QByteArray queryTag(VersionMask versions)
{
    QByteArray tag("?OTR");
    if (versions & Version1)
        tag.append('?');
    if (versions & (Version2 | Version3)) {
        tag.append('v');
        if (versions & Version2)
            tag.append('2');
        if (versions & Version3)
            tag.append('3');
        tag.append('?');
    }
    return tag;
}

QByteArray defaultQueryMessage(const QByteArray &ourName, VersionMask versions)
{
    QByteArray message = queryTag(versions);
    message.append("\n<b>");
    message.append(ourName);
    message.append("</b> has requested an "
                   "<a href=\"https://otr.cypherpunks.ca/\">Off-the-Record "
                   "private conversation</a>.  However, you do not have a plugin "
                   "to support that.\nSee <a href=\"https://otr.cypherpunks.ca/\">"
                   "https://otr.cypherpunks.ca/</a> for more information.");
    return message;
}

VersionMask queryVersions(const QByteArray &message, bool *isQuery)
{
    if (isQuery)
        *isQuery = false;

    const int start = message.indexOf("?OTR");
    if (start < 0 || start + 4 >= message.size())
        return 0;

    int pos = start + 4;
    const char marker = message.at(pos);
    if (marker != '?' && marker != 'v')
        return 0;

    if (isQuery)
        *isQuery = true;

    VersionMask versions = 0;
    if (message.at(pos) == '?') {
        versions |= Version1;
        ++pos;
    }

    if (pos < message.size() && message.at(pos) == 'v') {
        for (++pos; pos < message.size() && message.at(pos) != '?'; ++pos) {
            if (message.at(pos) == '2')
                versions |= Version2;
            else if (message.at(pos) == '3')
                versions |= Version3;
        }
    }
    return versions;
}

quint8 bestVersion(VersionMask peerVersions, VersionMask localVersions)
{
    const VersionMask common = peerVersions & localVersions;
    if (common & Version3)
        return 3;
    if (common & Version2)
        return 2;
    if (common & Version1)
        return 1;
    return 0;
}

quint8 queryBestVersion(const QByteArray &message,
                        VersionMask localVersions,
                        bool *isQuery)
{
    return bestVersion(queryVersions(message, isQuery), localVersions);
}

QByteArray errorMessage(const QByteArray &text)
{
    QByteArray message = ErrorMessagePrefix;
    message.append(text);
    return message;
}

bool parseErrorMessage(const QByteArray &message, QByteArray *text)
{
    if (text)
        text->clear();

    const int start = message.indexOf("?OTR");
    if (start < 0 || message.size() - start < ErrorTypePrefix.size())
        return false;
    if (message.mid(start, ErrorTypePrefix.size()) != ErrorTypePrefix)
        return false;

    int textStart = start + ErrorTypePrefix.size();
    if (textStart < message.size() && message.at(textStart) == ' ')
        ++textStart;
    if (text)
        *text = message.mid(textStart);
    return true;
}

QByteArray whitespaceTag(VersionMask versions)
{
    QByteArray tag = BaseWhitespaceTag;
    if (versions & Version1)
        tag.append(V1WhitespaceTag);
    if (versions & Version2)
        tag.append(V2WhitespaceTag);
    if (versions & Version3)
        tag.append(V3WhitespaceTag);
    return tag;
}

WhitespaceTag detectWhitespaceTag(const QByteArray &message)
{
    WhitespaceTag result;
    const int start = message.indexOf(BaseWhitespaceTag);
    if (start < 0)
        return result;

    result.found = true;
    result.start = start;
    result.end = start + BaseWhitespaceTag.size();

    while (isWhitespaceGroup(message, result.end)) {
        const QByteArray group = message.mid(result.end, 8);
        if (group == V1WhitespaceTag)
            result.versions |= Version1;
        if (group == V2WhitespaceTag)
            result.versions |= Version2;
        if (group == V3WhitespaceTag)
            result.versions |= Version3;
        result.end += 8;
    }
    return result;
}

quint8 whitespaceBestVersion(const QByteArray &message,
                             VersionMask localVersions,
                             WhitespaceTag *tag)
{
    const WhitespaceTag detected = detectWhitespaceTag(message);
    if (tag)
        *tag = detected;
    return bestVersion(detected.versions, localVersions);
}

bool stripWhitespaceTag(QByteArray *message, VersionMask *versions)
{
    if (versions)
        *versions = 0;
    if (!message)
        return false;

    const WhitespaceTag tag = detectWhitespaceTag(*message);
    if (!tag.found)
        return false;

    if (versions)
        *versions = tag.versions;
    message->remove(tag.start, tag.end - tag.start);
    return true;
}

QByteArray appendWhitespaceTag(const QByteArray &message, VersionMask versions)
{
    QByteArray tagged = message;
    tagged.append(whitespaceTag(versions));
    return tagged;
}

} // namespace QcaOtr::Negotiation
