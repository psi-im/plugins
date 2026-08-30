/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <QByteArray>
#include <QVector>
#include <QtGlobal>

namespace QcaOtr::Transport {

/** Lowest valid non-zero OTRv3 instance tag. */
constexpr quint32 MinimumInstanceTag = 0x00000100;

/** Sender/receiver instance routing information carried by OTRv3 messages. */
struct Route
{
    quint32 senderInstance = 0;
    quint32 receiverInstance = 0;
};

/** Parsed OTRv3 fragment. */
struct Fragment
{
    Route route;
    quint16 index = 0;
    quint16 count = 0;
    QByteArray payload;
};

/** Result of syntactically classifying one transport fragment. */
enum class FragmentParseStatus {
    NotFragment,
    Fragment,
    Malformed
};

/** Result of feeding a transport message into a fragment accumulator. */
enum class FragmentResult {
    Unfragmented,
    Incomplete,
    Complete,
    Malformed
};

/** ASCII-armors a raw binary OTR protocol message as `?OTR:...`. */
QByteArray armor(const QByteArray &raw);

/** Decodes one complete armored OTR message into raw protocol bytes. */
bool dearmor(const QByteArray &message, QByteArray *raw);

/**
 * Extracts OTRv3 instance tags from a raw protocol message.
 *
 * A receiver instance of zero is valid for the initial broadcast D-H Commit.
 */
bool routeFromRaw(const QByteArray &raw, Route *route);

/** Extracts OTRv3 routing information from an armored transport message. */
bool routeFromArmored(const QByteArray &message, Route *route);

/** Parses one libotr-compatible OTRv3 transport fragment. */
FragmentParseStatus parseFragment(const QByteArray &message, Fragment *fragment);

/**
 * Splits an already-armored message using the libotr 4.1.1 OTRv3 fragment format.
 * If @p message already fits @p maxMessageSize, @p fragments receives the
 * original message as its only element.
 */
bool fragmentMessage(const QByteArray &message,
                     int maxMessageSize,
                     quint32 senderInstance,
                     quint32 receiverInstance,
                     QVector<QByteArray> *fragments);

/** Stateful, size-bounded reassembler for ordered OTRv3 fragments. */
class FragmentAccumulator
{
public:
    /**
     * @param maxBufferedBytes hard upper bound for buffered fragment payloads.
     */
    explicit FragmentAccumulator(qsizetype maxBufferedBytes = 16 * 1024 * 1024);

    /**
     * Consumes one transport message.
     * @param completeMessage receives the original unfragmented message or a
     * fully reassembled armored OTR message for Complete/Unfragmented results.
     */
    FragmentResult accumulate(const QByteArray &message, QByteArray *completeMessage = nullptr);

    /** Drops any incomplete fragment sequence. */
    void reset();

private:
    qsizetype maxBufferedBytes_ = 0;
    Route route_;
    quint16 count_ = 0;
    quint16 index_ = 0;
    QByteArray data_;
};

} // namespace QcaOtr::Transport
