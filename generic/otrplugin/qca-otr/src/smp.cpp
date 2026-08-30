#include "qca-otr/smp.h"

#include "qca-otr/ake.h"
#include "qca-otr/codec.h"
#include "qca-otr/crypto.h"

#include <QVector>

namespace QcaOtr {
namespace {

constexpr int SmpModulusBytes = 192;
constexpr int Message1Length = 6;
constexpr int Message2Length = 11;
constexpr int Message3Length = 8;
constexpr int Message4Length = 3;

QCA::BigInteger zero()
{
    return QCA::BigInteger(0);
}

QCA::BigInteger one()
{
    return QCA::BigInteger(1);
}

QCA::BigInteger two()
{
    return QCA::BigInteger(2);
}

QCA::BigInteger modulus()
{
    return dhModulus();
}

QCA::BigInteger generator()
{
    return dhGenerator();
}

QCA::BigInteger order()
{
    QCA::BigInteger result(modulus());
    result -= one();
    result /= two();
    return result;
}

QCA::BigInteger positiveMod(const QCA::BigInteger &value, const QCA::BigInteger &modulusValue)
{
    QCA::BigInteger result(value);
    result %= modulusValue;
    if (result < zero())
        result += modulusValue;
    return result;
}

QCA::BigInteger unsignedInteger(const QByteArray &bytes)
{
    QByteArray positive;
    positive.reserve(bytes.size() + 1);
    positive.append('\0');
    positive.append(bytes);
    return QCA::BigInteger(QCA::SecureArray(positive));
}

QCA::BigInteger unsignedInteger(const QCA::SecureArray &bytes)
{
    QCA::SecureArray positive(1, '\0');
    positive.append(bytes);
    return QCA::BigInteger(positive);
}

QCA::BigInteger randomExponent()
{
    const QCA::SecureArray random = QCA::Random::randomArray(SmpModulusBytes);
    if (random.size() != SmpModulusBytes)
        return QCA::BigInteger(0);
    return unsignedInteger(random);
}

bool validGroupElement(const QCA::BigInteger &value)
{
    QCA::BigInteger upper(modulus());
    upper -= two();
    return value >= two() && value <= upper;
}

bool validExponent(const QCA::BigInteger &value)
{
    return value >= one() && value < order();
}

bool serializeArray(const QVector<QCA::BigInteger> &values, QByteArray *encoded)
{
    if (!encoded)
        return false;

    Wire::Writer writer;
    writer.writeInt(static_cast<quint32>(values.size()));
    for (const QCA::BigInteger &value : values) {
        if (!writer.writeMpi(value))
            return false;
    }
    *encoded = writer.take();
    return true;
}

bool unserializeArray(const QByteArray &encoded, int expectedCount, QVector<QCA::BigInteger> *values)
{
    if (!values || expectedCount < 0)
        return false;

    Wire::Reader reader(encoded);
    quint32 count = 0;
    if (!reader.readInt(&count) || count != static_cast<quint32>(expectedCount))
        return false;

    QVector<QCA::BigInteger> decoded;
    decoded.reserve(expectedCount);
    for (int i = 0; i < expectedCount; ++i) {
        QCA::BigInteger value;
        if (!reader.readMpi(&value))
            return false;
        decoded.append(value);
    }
    if (!reader.atEnd())
        return false;

    *values = decoded;
    return true;
}

bool smpHash(QCA::BigInteger *hash, quint8 version, const QCA::BigInteger &a,
             const QCA::BigInteger *b = nullptr)
{
    if (!hash)
        return false;

    bool ok = false;
    const QByteArray encodedA = Wire::encodeMpi(a, &ok);
    if (!ok)
        return false;

    QByteArray input;
    input.reserve(1 + encodedA.size() + (b ? SmpModulusBytes + 4 : 0));
    input.append(static_cast<char>(version));
    input.append(encodedA);
    if (b) {
        const QByteArray encodedB = Wire::encodeMpi(*b, &ok);
        if (!ok)
            return false;
        input.append(encodedB);
    }

    const QByteArray digest = sha256(input);
    if (digest.size() != 32)
        return false;
    *hash = unsignedInteger(digest);
    return true;
}

bool proofKnowLog(QCA::BigInteger *c, QCA::BigInteger *d,
                  const QCA::BigInteger &g, const QCA::BigInteger &x, quint8 version)
{
    if (!c || !d)
        return false;

    const QCA::BigInteger r = randomExponent();
    if (r == zero())
        return false;

    const QCA::BigInteger temp = QCA::BigIntegerMath::modPow(g, r, modulus());
    if (!smpHash(c, version, temp))
        return false;

    QCA::BigInteger xc(x);
    xc *= *c;
    xc %= order();

    QCA::BigInteger response(r);
    response -= xc;
    *d = positiveMod(response, order());
    return true;
}

bool checkKnowLog(const QCA::BigInteger &c, const QCA::BigInteger &d,
                  const QCA::BigInteger &g, const QCA::BigInteger &x, quint8 version)
{
    QCA::BigInteger gd = QCA::BigIntegerMath::modPow(g, d, modulus());
    QCA::BigInteger xc = QCA::BigIntegerMath::modPow(x, c, modulus());
    gd *= xc;
    gd %= modulus();

    QCA::BigInteger computed;
    return smpHash(&computed, version, gd) && computed == c;
}

struct ProofState
{
    QCA::BigInteger secret;
    QCA::BigInteger x3;
    QCA::BigInteger g1;
    QCA::BigInteger g2;
    QCA::BigInteger g3;
    QCA::BigInteger g3o;
    QCA::BigInteger qab;
};

bool proofEqualCoords(QCA::BigInteger *c, QCA::BigInteger *d1, QCA::BigInteger *d2,
                      const ProofState &state, const QCA::BigInteger &r, quint8 version)
{
    if (!c || !d1 || !d2)
        return false;

    const QCA::BigInteger r1 = randomExponent();
    const QCA::BigInteger r2 = randomExponent();
    if (r1 == zero() || r2 == zero())
        return false;

    QCA::BigInteger temp1 = QCA::BigIntegerMath::modPow(state.g1, r1, modulus());
    QCA::BigInteger temp2 = QCA::BigIntegerMath::modPow(state.g2, r2, modulus());
    temp2 *= temp1;
    temp2 %= modulus();
    temp1 = QCA::BigIntegerMath::modPow(state.g3, r1, modulus());
    if (!smpHash(c, version, temp1, &temp2))
        return false;

    QCA::BigInteger rc(r);
    rc *= *c;
    rc %= order();
    QCA::BigInteger first(r1);
    first -= rc;
    *d1 = positiveMod(first, order());

    QCA::BigInteger sc(state.secret);
    sc *= *c;
    sc %= order();
    QCA::BigInteger second(r2);
    second -= sc;
    *d2 = positiveMod(second, order());
    return true;
}

bool checkEqualCoords(const QCA::BigInteger &c, const QCA::BigInteger &d1,
                      const QCA::BigInteger &d2, const QCA::BigInteger &p,
                      const QCA::BigInteger &qValue, const ProofState &state, quint8 version)
{
    QCA::BigInteger temp1 = QCA::BigIntegerMath::modPow(state.g3, d1, modulus());
    QCA::BigInteger temp3 = QCA::BigIntegerMath::modPow(p, c, modulus());
    temp1 *= temp3;
    temp1 %= modulus();

    QCA::BigInteger temp2 = QCA::BigIntegerMath::modPow(state.g1, d1, modulus());
    temp3 = QCA::BigIntegerMath::modPow(state.g2, d2, modulus());
    temp2 *= temp3;
    temp2 %= modulus();
    temp3 = QCA::BigIntegerMath::modPow(qValue, c, modulus());
    temp2 *= temp3;
    temp2 %= modulus();

    QCA::BigInteger computed;
    return smpHash(&computed, version, temp1, &temp2) && computed == c;
}

bool proofEqualLogs(QCA::BigInteger *c, QCA::BigInteger *d,
                    const ProofState &state, quint8 version)
{
    if (!c || !d)
        return false;

    const QCA::BigInteger r = randomExponent();
    if (r == zero())
        return false;

    const QCA::BigInteger temp1 = QCA::BigIntegerMath::modPow(state.g1, r, modulus());
    const QCA::BigInteger temp2 = QCA::BigIntegerMath::modPow(state.qab, r, modulus());
    if (!smpHash(c, version, temp1, &temp2))
        return false;

    QCA::BigInteger x3c(state.x3);
    x3c *= *c;
    x3c %= order();
    QCA::BigInteger response(r);
    response -= x3c;
    *d = positiveMod(response, order());
    return true;
}

bool checkEqualLogs(const QCA::BigInteger &c, const QCA::BigInteger &d,
                    const QCA::BigInteger &r, const ProofState &state, quint8 version)
{
    QCA::BigInteger temp1 = QCA::BigIntegerMath::modPow(state.g1, d, modulus());
    QCA::BigInteger temp3 = QCA::BigIntegerMath::modPow(state.g3o, c, modulus());
    temp1 *= temp3;
    temp1 %= modulus();

    QCA::BigInteger temp2 = QCA::BigIntegerMath::modPow(state.qab, d, modulus());
    temp3 = QCA::BigIntegerMath::modPow(r, c, modulus());
    temp2 *= temp3;
    temp2 %= modulus();

    QCA::BigInteger computed;
    return smpHash(&computed, version, temp1, &temp2) && computed == c;
}

} // namespace

struct SmpSession::Private
{
    void clear(SmpProgress newProgress = SmpProgress::Ok)
    {
        const QCA::BigInteger empty(0);
        secret = empty;
        x2 = empty;
        x3 = empty;
        g1 = empty;
        g2 = empty;
        g3 = empty;
        g3o = empty;
        p = empty;
        q = empty;
        pab = empty;
        qab = empty;
        expected = SmpExpected::Message1;
        progress = newProgress;
        waitingSecret = false;
        question = false;
        initialized = false;
    }

    void initialize()
    {
        clear();
        g1 = generator();
        initialized = true;
    }

    ProofState proofState() const
    {
        return ProofState {secret, x3, g1, g2, g3, g3o, qab};
    }

    SmpStepResult unexpected() const
    {
        SmpStepResult result;
        result.status = SmpStepStatus::Unexpected;
        result.progress = progress;
        return result;
    }

    SmpStepResult invalid()
    {
        clear(SmpProgress::Cheated);
        SmpStepResult result;
        result.status = SmpStepStatus::Invalid;
        result.progress = SmpProgress::Cheated;
        return result;
    }

    QCA::BigInteger secret;
    QCA::BigInteger x2;
    QCA::BigInteger x3;
    QCA::BigInteger g1;
    QCA::BigInteger g2;
    QCA::BigInteger g3;
    QCA::BigInteger g3o;
    QCA::BigInteger p;
    QCA::BigInteger q;
    QCA::BigInteger pab;
    QCA::BigInteger qab;
    SmpExpected expected = SmpExpected::Message1;
    SmpProgress progress = SmpProgress::Ok;
    bool waitingSecret = false;
    bool question = false;
    bool initialized = false;
};

SmpSession::SmpSession() : d(std::make_unique<Private>()) { }
SmpSession::~SmpSession() = default;
SmpSession::SmpSession(SmpSession &&) noexcept = default;
SmpSession &SmpSession::operator=(SmpSession &&) noexcept = default;

SmpExpected SmpSession::expected() const
{
    return d->expected;
}

SmpProgress SmpSession::progress() const
{
    return d->progress;
}

bool SmpSession::awaitingSecret() const
{
    return d->waitingSecret;
}

bool SmpSession::receivedQuestion() const
{
    return d->question;
}

SmpStepResult SmpSession::initiate(const QCA::SecureArray &secret)
{
    d->initialize();
    d->secret = unsignedInteger(secret);
    d->x2 = randomExponent();
    d->x3 = randomExponent();
    if (d->x2 == zero() || d->x3 == zero())
        return d->invalid();

    QVector<QCA::BigInteger> message(Message1Length);
    message[0] = QCA::BigIntegerMath::modPow(d->g1, d->x2, modulus());
    if (!proofKnowLog(&message[1], &message[2], d->g1, d->x2, 1))
        return d->invalid();
    message[3] = QCA::BigIntegerMath::modPow(d->g1, d->x3, modulus());
    if (!proofKnowLog(&message[4], &message[5], d->g1, d->x3, 2))
        return d->invalid();

    SmpStepResult result;
    if (!serializeArray(message, &result.outgoing))
        return d->invalid();

    d->expected = SmpExpected::Message2;
    d->progress = SmpProgress::Ok;
    result.status = SmpStepStatus::Ok;
    result.progress = SmpProgress::Ok;
    return result;
}

SmpStepResult SmpSession::receiveMessage1(const QByteArray &message, bool receivedQuestion)
{
    if (d->expected != SmpExpected::Message1 || d->waitingSecret)
        return d->unexpected();

    d->initialize();
    d->question = receivedQuestion;

    QVector<QCA::BigInteger> values;
    if (!unserializeArray(message, Message1Length, &values) || !validGroupElement(values[0]) ||
        !validExponent(values[2]) || !validGroupElement(values[3]) || !validExponent(values[5])) {
        return d->invalid();
    }

    d->g3o = values[3];
    if (!checkKnowLog(values[1], values[2], d->g1, values[0], 1) ||
        !checkKnowLog(values[4], values[5], d->g1, values[3], 2)) {
        return d->invalid();
    }

    d->x2 = randomExponent();
    d->x3 = randomExponent();
    if (d->x2 == zero() || d->x3 == zero())
        return d->invalid();

    d->g2 = QCA::BigIntegerMath::modPow(values[0], d->x2, modulus());
    d->g3 = QCA::BigIntegerMath::modPow(values[3], d->x3, modulus());
    d->waitingSecret = true;
    d->progress = SmpProgress::Ok;

    SmpStepResult result;
    result.status = SmpStepStatus::Ok;
    result.progress = SmpProgress::Ok;
    return result;
}

SmpStepResult SmpSession::respond(const QCA::SecureArray &secret)
{
    if (!d->initialized || !d->waitingSecret || d->expected != SmpExpected::Message1)
        return d->unexpected();

    d->secret = unsignedInteger(secret);
    QVector<QCA::BigInteger> message(Message2Length);

    message[0] = QCA::BigIntegerMath::modPow(d->g1, d->x2, modulus());
    if (!proofKnowLog(&message[1], &message[2], d->g1, d->x2, 3))
        return d->invalid();
    message[3] = QCA::BigIntegerMath::modPow(d->g1, d->x3, modulus());
    if (!proofKnowLog(&message[4], &message[5], d->g1, d->x3, 4))
        return d->invalid();

    const QCA::BigInteger r = randomExponent();
    if (r == zero())
        return d->invalid();

    d->p = QCA::BigIntegerMath::modPow(d->g3, r, modulus());
    message[6] = d->p;

    QCA::BigInteger q1 = QCA::BigIntegerMath::modPow(d->g1, r, modulus());
    QCA::BigInteger q2 = QCA::BigIntegerMath::modPow(d->g2, d->secret, modulus());
    q1 *= q2;
    q1 %= modulus();
    d->q = q1;
    message[7] = d->q;

    if (!proofEqualCoords(&message[8], &message[9], &message[10], d->proofState(), r, 5))
        return d->invalid();

    SmpStepResult result;
    if (!serializeArray(message, &result.outgoing))
        return d->invalid();

    d->waitingSecret = false;
    d->expected = SmpExpected::Message3;
    d->progress = SmpProgress::Ok;
    result.status = SmpStepStatus::Ok;
    result.progress = SmpProgress::Ok;
    return result;
}

SmpStepResult SmpSession::receiveMessage2(const QByteArray &message)
{
    if (!d->initialized || d->expected != SmpExpected::Message2)
        return d->unexpected();

    QVector<QCA::BigInteger> values;
    if (!unserializeArray(message, Message2Length, &values) || !validGroupElement(values[0]) ||
        !validGroupElement(values[3]) || !validGroupElement(values[6]) || !validGroupElement(values[7]) ||
        !validExponent(values[2]) || !validExponent(values[5]) || !validExponent(values[9]) ||
        !validExponent(values[10])) {
        return d->invalid();
    }

    d->g3o = values[3];
    if (!checkKnowLog(values[1], values[2], d->g1, values[0], 3) ||
        !checkKnowLog(values[4], values[5], d->g1, values[3], 4)) {
        return d->invalid();
    }

    d->g2 = QCA::BigIntegerMath::modPow(values[0], d->x2, modulus());
    d->g3 = QCA::BigIntegerMath::modPow(values[3], d->x3, modulus());
    if (!checkEqualCoords(values[8], values[9], values[10], values[6], values[7], d->proofState(), 5))
        return d->invalid();

    const QCA::BigInteger r = randomExponent();
    if (r == zero())
        return d->invalid();

    QVector<QCA::BigInteger> response(Message3Length);
    d->p = QCA::BigIntegerMath::modPow(d->g3, r, modulus());
    response[0] = d->p;

    QCA::BigInteger q1 = QCA::BigIntegerMath::modPow(d->g1, r, modulus());
    QCA::BigInteger q2 = QCA::BigIntegerMath::modPow(d->g2, d->secret, modulus());
    q1 *= q2;
    q1 %= modulus();
    d->q = q1;
    response[1] = d->q;

    if (!proofEqualCoords(&response[2], &response[3], &response[4], d->proofState(), r, 6))
        return d->invalid();

    QCA::BigInteger inverse;
    if (!QCA::BigIntegerMath::modInverse(values[6], modulus(), &inverse))
        return d->invalid();
    d->pab = d->p;
    d->pab *= inverse;
    d->pab %= modulus();

    if (!QCA::BigIntegerMath::modInverse(values[7], modulus(), &inverse))
        return d->invalid();
    d->qab = d->q;
    d->qab *= inverse;
    d->qab %= modulus();

    response[5] = QCA::BigIntegerMath::modPow(d->qab, d->x3, modulus());
    if (!proofEqualLogs(&response[6], &response[7], d->proofState(), 7))
        return d->invalid();

    SmpStepResult result;
    if (!serializeArray(response, &result.outgoing))
        return d->invalid();

    d->expected = SmpExpected::Message4;
    d->progress = SmpProgress::Ok;
    result.status = SmpStepStatus::Ok;
    result.progress = SmpProgress::Ok;
    return result;
}

SmpStepResult SmpSession::receiveMessage3(const QByteArray &message)
{
    if (!d->initialized || d->expected != SmpExpected::Message3)
        return d->unexpected();

    QVector<QCA::BigInteger> values;
    if (!unserializeArray(message, Message3Length, &values) || !validGroupElement(values[0]) ||
        !validGroupElement(values[1]) || !validGroupElement(values[5]) || !validExponent(values[3]) ||
        !validExponent(values[4]) || !validExponent(values[7])) {
        return d->invalid();
    }

    if (!checkEqualCoords(values[2], values[3], values[4], values[0], values[1], d->proofState(), 6))
        return d->invalid();

    QCA::BigInteger inverse;
    if (!QCA::BigIntegerMath::modInverse(d->p, modulus(), &inverse))
        return d->invalid();
    d->pab = values[0];
    d->pab *= inverse;
    d->pab %= modulus();

    if (!QCA::BigIntegerMath::modInverse(d->q, modulus(), &inverse))
        return d->invalid();
    d->qab = values[1];
    d->qab *= inverse;
    d->qab %= modulus();

    if (!checkEqualLogs(values[6], values[7], values[5], d->proofState(), 7))
        return d->invalid();

    QVector<QCA::BigInteger> response(Message4Length);
    response[0] = QCA::BigIntegerMath::modPow(d->qab, d->x3, modulus());
    if (!proofEqualLogs(&response[1], &response[2], d->proofState(), 8))
        return d->invalid();

    SmpStepResult result;
    if (!serializeArray(response, &result.outgoing))
        return d->invalid();

    const QCA::BigInteger rab = QCA::BigIntegerMath::modPow(values[5], d->x3, modulus());
    const SmpProgress finalProgress = rab == d->pab ? SmpProgress::Succeeded : SmpProgress::Failed;
    d->clear(finalProgress);

    result.status = SmpStepStatus::Ok;
    result.progress = finalProgress;
    return result;
}

SmpStepResult SmpSession::receiveMessage4(const QByteArray &message)
{
    if (!d->initialized || d->expected != SmpExpected::Message4)
        return d->unexpected();

    QVector<QCA::BigInteger> values;
    if (!unserializeArray(message, Message4Length, &values) || !validGroupElement(values[0]) ||
        !validExponent(values[2])) {
        return d->invalid();
    }

    if (!checkEqualLogs(values[1], values[2], values[0], d->proofState(), 8))
        return d->invalid();

    const QCA::BigInteger rab = QCA::BigIntegerMath::modPow(values[0], d->x3, modulus());
    const SmpProgress finalProgress = rab == d->pab ? SmpProgress::Succeeded : SmpProgress::Failed;
    d->clear(finalProgress);

    SmpStepResult result;
    result.status = SmpStepStatus::Ok;
    result.progress = finalProgress;
    return result;
}

void SmpSession::abort()
{
    d->clear();
}

void SmpSession::reset()
{
    d->clear();
}

} // namespace QcaOtr
