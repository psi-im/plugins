#pragma once

#include <QByteArray>
#include <QtCrypto>

#include <memory>

namespace QcaOtr {

enum class SmpExpected {
    Message1,
    Message2,
    Message3,
    Message4,
    Message5
};

enum class SmpProgress {
    Ok,
    Cheated,
    Failed,
    Succeeded
};

enum class SmpStepStatus {
    Ok,
    Unexpected,
    Invalid
};

struct SmpStepResult
{
    SmpStepStatus status = SmpStepStatus::Unexpected;
    SmpProgress progress = SmpProgress::Ok;
    QByteArray outgoing;
};

// OTRv3 Socialist Millionaires' Protocol primitive state machine.
//
// The secret passed here is the 32-byte combined SMP secret defined by libotr,
// not the user-entered answer. OtrSession is responsible for combining the
// initiator/responder fingerprints, secure session id and user secret.
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
    bool awaitingSecret() const;
    bool receivedQuestion() const;

    SmpStepResult initiate(const QCA::SecureArray &secret);
    SmpStepResult receiveMessage1(const QByteArray &message, bool receivedQuestion);
    SmpStepResult respond(const QCA::SecureArray &secret);
    SmpStepResult receiveMessage2(const QByteArray &message);
    SmpStepResult receiveMessage3(const QByteArray &message);
    SmpStepResult receiveMessage4(const QByteArray &message);

    void abort();
    void reset();

private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace QcaOtr
