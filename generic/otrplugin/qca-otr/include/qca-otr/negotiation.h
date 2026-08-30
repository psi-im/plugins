/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <QByteArray>
#include <QtGlobal>

namespace QcaOtr::Negotiation {

/** Bit mask of OTR protocol versions advertised by negotiation helpers. */
using VersionMask = quint8;

constexpr VersionMask Version1 = 0x01;
constexpr VersionMask Version2 = 0x02;
constexpr VersionMask Version3 = 0x04;

/** Protocol versions implemented by the native engine. */
constexpr VersionMask NativeVersions = Version3;

/**
 * Returns the compact marker at the start of an OTR Query Message.
 *
 * The layout follows libotr: v1 is represented by `?` and v2/v3 by a
 * following `v...?...` group.
 */
QByteArray queryTag(VersionMask versions = NativeVersions);

/**
 * Builds a libotr-compatible Query Message including the human-readable
 * fallback text for peers without OTR support.
 */
QByteArray defaultQueryMessage(const QByteArray &ourName,
                               VersionMask versions = NativeVersions);

/**
 * Inspects the first `?OTR` occurrence in @p message.
 * @param isQuery receives true only for Query Message syntax (`?OTR?` or
 * `?OTRv...`).
 * @return the advertised version mask, or zero when no compatible query is
 * present.
 */
VersionMask queryVersions(const QByteArray &message, bool *isQuery = nullptr);

/** Returns the highest mutually supported OTR version, or zero if none match. */
quint8 bestVersion(VersionMask peerVersions,
                   VersionMask localVersions = NativeVersions);

/** Parses a Query Message and returns the highest mutually supported version. */
quint8 queryBestVersion(const QByteArray &message,
                        VersionMask localVersions = NativeVersions,
                        bool *isQuery = nullptr);

/** Builds a text-level OTR Error Message. */
QByteArray errorMessage(const QByteArray &text);

/**
 * Parses the first text-level OTR Error Message occurrence.
 * @param text optionally receives the remote error text.
 */
bool parseErrorMessage(const QByteArray &message, QByteArray *text = nullptr);

/** Location and advertised versions of an OTR whitespace discovery tag. */
struct WhitespaceTag
{
    bool found = false;
    VersionMask versions = 0;
    int start = -1;
    int end = -1;
};

/** Builds an invisible OTR whitespace discovery tag. */
QByteArray whitespaceTag(VersionMask versions = NativeVersions);

/** Finds the first valid OTR whitespace discovery tag in @p message. */
WhitespaceTag detectWhitespaceTag(const QByteArray &message);

/** Returns the highest mutually supported version advertised by a whitespace tag. */
quint8 whitespaceBestVersion(const QByteArray &message,
                             VersionMask localVersions = NativeVersions,
                             WhitespaceTag *tag = nullptr);

/** Removes the first detected whitespace tag in place. */
bool stripWhitespaceTag(QByteArray *message, VersionMask *versions = nullptr);

/** Appends an OTR whitespace discovery tag unless the caller chooses no versions. */
QByteArray appendWhitespaceTag(const QByteArray &message,
                               VersionMask versions = NativeVersions);

} // namespace QcaOtr::Negotiation
