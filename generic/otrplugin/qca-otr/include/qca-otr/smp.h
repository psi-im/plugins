/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <QByteArray>
#include <QtCrypto>

#include <memory>

namespace QcaOtr {

/** Next SMP message expected by the primitive state machine. */
enum class SmpExpected {
    Message1,
    Message2,
    Message3,
    Message4,
    Message5
};

/** Authentication outcome/progress tracked by the SMP primitive. */
enum class SmpProgress {
    Ok,
    Cheated,
    Failed,
    Succeeded
};

/** Result category for one SMP protocol step. */
enum class SmpStepStatus {
    Ok,
    Unexpected,
    Invalid
};

/** Result of processing or generating one SMP step. */
struct SmpStepResult
{
    SmpStepStatus status = SmpStepStatus::Unexpected;
    SmpProgress progress = SmpProgress::Ok;
    QByteArray outgoing;
};

/**
 * OTRv3 Socialist Millionaires' Protocol primitive state machine.
 *
 * The secret accepted by this class is the 32-byte combined SMP secret, not
 * the user's textual answer. `OtrSession` combines the ordered fingerprints,
 * secure session id and user-entered secret before calling this layer.
 */
class SmpSession
{
public:
    SmpSession();
    ~SmpSession();

    SmpSession(const SmpSession &) = delete;
    SmpSession &operator=(const SmpSession &) = delete;
    SmpSession(SmpSession &&) noexcept;
    SmpSession &operator=(SmpSession &&) noexcept;

    SmpExpected expected() const;
    SmpProgress progress() const;

    /** Returns true after message 1 is received and a responder secret is required. */
    bool awaitingSecret() const;

    /** Returns whether the incoming SMP1 carried a human-readable question. */
    bool receivedQuestion() const;

    /** Starts SMP as initiator and returns the first outgoing SMP payload. */
    SmpStepResult initiate(const QCA::SecureArray &secret);

    /** Accepts SMP message 1; the secret is supplied later through @ref respond. */
    SmpStepResult receiveMessage1(const QByteArray &message, bool receivedQuestion);

    /** Supplies the responder's combined secret and emits SMP message 2. */
    SmpStepResult respond(const QCA::SecureArray &secret);

    SmpStepResult receiveMessage2(const QByteArray &message);
    SmpStepResult receiveMessage3(const QByteArray &message);
    SmpStepResult receiveMessage4(const QByteArray &message);

    /** Aborts the current exchange and records an aborted/failed state. */
    void abort();

    /** Returns the primitive to its initial state and forgets pending material. */
    void reset();

private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace QcaOtr
