#include "qca-otr/persistence.h"

#include "persistence_p.h"

#include <limits>

namespace QcaOtr::Persistence {
namespace {

constexpr quint32 MinimumInstanceTag = 0x00000100;

using Private::isHex;
using Private::readFile;
using Private::setError;
using Private::validTabField;
using Private::writeFileAtomically;

} // namespace

bool parseFingerprints(const QByteArray &data, QList<FingerprintRecord> *records, QString *error)
{
    if (!records)
        return false;
    records->clear();
    if (error)
        error->clear();

    QList<FingerprintRecord> parsed;
    const QList<QByteArray> lines = data.split('\n');
    for (QByteArray line : lines) {
        if (line.endsWith('\r'))
            line.chop(1);
        if (line.isEmpty())
            continue;

        int positions[4] = {-1, -1, -1, -1};
        int from = 0;
        for (int i = 0; i < 4; ++i) {
            positions[i] = line.indexOf('\t', from);
            if (positions[i] < 0) {
                if (i == 3)
                    break;
                setError(error, QStringLiteral("Malformed libotr fingerprint line"));
                return false;
            }
            from = positions[i] + 1;
        }
        if (positions[0] < 0 || positions[1] < 0 || positions[2] < 0) {
            setError(error, QStringLiteral("Malformed libotr fingerprint line"));
            return false;
        }

        FingerprintRecord record;
        record.username = line.left(positions[0]);
        record.account = line.mid(positions[0] + 1, positions[1] - positions[0] - 1);
        record.protocol = line.mid(positions[1] + 1, positions[2] - positions[1] - 1);
        QByteArray hex;
        if (positions[3] >= 0) {
            hex = line.mid(positions[2] + 1, positions[3] - positions[2] - 1);
            record.trust = line.mid(positions[3] + 1);
        } else {
            hex = line.mid(positions[2] + 1);
        }

        if (!validTabField(record.username) || !validTabField(record.account) || !validTabField(record.protocol) ||
            hex.size() != 40) {
            setError(error, QStringLiteral("Invalid libotr fingerprint record"));
            return false;
        }
        for (char c : hex) {
            if (!isHex(c)) {
                setError(error, QStringLiteral("Invalid hexadecimal fingerprint"));
                return false;
            }
        }
        if (record.trust.contains('\r') || record.trust.contains('\n') || record.trust.contains('\0')) {
            setError(error, QStringLiteral("Invalid libotr fingerprint trust string"));
            return false;
        }
        record.fingerprint = QByteArray::fromHex(hex);
        if (record.fingerprint.size() != 20)
            return false;
        parsed.append(record);
    }

    *records = parsed;
    return true;
}

QByteArray serializeFingerprints(const QList<FingerprintRecord> &records, bool *ok)
{
    if (ok)
        *ok = false;
    QByteArray output;
    for (const FingerprintRecord &record : records) {
        if (!validTabField(record.username) || !validTabField(record.account) || !validTabField(record.protocol) ||
            record.fingerprint.size() != 20 || record.trust.contains('\r') || record.trust.contains('\n') ||
            record.trust.contains('\0')) {
            return {};
        }
        output += record.username + '\t' + record.account + '\t' + record.protocol + '\t' +
            record.fingerprint.toHex() + '\t' + record.trust + '\n';
    }
    if (ok)
        *ok = true;
    return output;
}

bool parseInstanceTags(const QByteArray &data, QList<InstanceTagRecord> *records, QString *error)
{
    if (!records)
        return false;
    records->clear();
    if (error)
        error->clear();

    QList<InstanceTagRecord> parsed;
    const QList<QByteArray> lines = data.split('\n');
    for (QByteArray line : lines) {
        if (line.endsWith('\r'))
            line.chop(1);
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        const int first = line.indexOf('\t');
        const int second = first >= 0 ? line.indexOf('\t', first + 1) : -1;
        if (first < 0 || second < 0 || line.indexOf('\t', second + 1) >= 0) {
            setError(error, QStringLiteral("Malformed libotr instance-tag line"));
            return false;
        }

        InstanceTagRecord record;
        record.account = line.left(first);
        record.protocol = line.mid(first + 1, second - first - 1);
        const QByteArray hex = line.mid(second + 1);
        if (!validTabField(record.account) || !validTabField(record.protocol) || hex.size() != 8) {
            setError(error, QStringLiteral("Invalid libotr instance-tag record"));
            return false;
        }
        for (char c : hex) {
            if (!isHex(c)) {
                setError(error, QStringLiteral("Invalid hexadecimal instance tag"));
                return false;
            }
        }
        bool valueOk = false;
        const qulonglong value = hex.toULongLong(&valueOk, 16);
        if (!valueOk || value > std::numeric_limits<quint32>::max() || value < MinimumInstanceTag) {
            setError(error, QStringLiteral("Reserved or invalid OTR instance tag"));
            return false;
        }
        record.instanceTag = static_cast<quint32>(value);
        parsed.append(record);
    }

    *records = parsed;
    return true;
}

QByteArray serializeInstanceTags(const QList<InstanceTagRecord> &records, bool *ok)
{
    if (ok)
        *ok = false;

    QByteArray output("# WARNING! You shouldn't copy this file to another computer. It is unnecessary and can cause problems.\n");
    for (const InstanceTagRecord &record : records) {
        if (!validTabField(record.account) || !validTabField(record.protocol) || record.instanceTag < MinimumInstanceTag)
            return {};
        output += record.account + '\t' + record.protocol + '\t' +
            QByteArray::number(record.instanceTag, 16).rightJustified(8, '0') + '\n';
    }
    if (ok)
        *ok = true;
    return output;
}

bool readFingerprintsFile(const QString &path, QList<FingerprintRecord> *records, QString *error)
{
    QByteArray data;
    return readFile(path, &data, error) && parseFingerprints(data, records, error);
}

bool writeFingerprintsFile(const QString &path, const QList<FingerprintRecord> &records, QString *error)
{
    bool ok = false;
    const QByteArray data = serializeFingerprints(records, &ok);
    if (!ok) {
        setError(error, QStringLiteral("Cannot serialize libotr fingerprint store"));
        return false;
    }
    return writeFileAtomically(path, data, error);
}

bool readInstanceTagsFile(const QString &path, QList<InstanceTagRecord> *records, QString *error)
{
    QByteArray data;
    return readFile(path, &data, error) && parseInstanceTags(data, records, error);
}

bool writeInstanceTagsFile(const QString &path, const QList<InstanceTagRecord> &records, QString *error)
{
    bool ok = false;
    const QByteArray data = serializeInstanceTags(records, &ok);
    if (!ok) {
        setError(error, QStringLiteral("Cannot serialize libotr instance-tag store"));
        return false;
    }
    return writeFileAtomically(path, data, error);
}

} // namespace QcaOtr::Persistence
