#include "qca-otr/persistence.h"

#include <QFile>
#include <QFileDevice>
#include <QSaveFile>

#include <limits>

namespace QcaOtr::Persistence {
namespace {

constexpr quint32 MinimumInstanceTag = 0x00000100;

void setError(QString *error, const QString &message)
{
    if (error && error->isEmpty())
        *error = message;
}

bool validTextField(const QByteArray &value)
{
    return !value.isEmpty() && !value.contains('\0') && !value.contains('\r') && !value.contains('\n');
}

bool validTabField(const QByteArray &value)
{
    return validTextField(value) && !value.contains('\t');
}

bool isHex(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

QCA::BigInteger unsignedInteger(const QByteArray &bytes)
{
    QCA::SecureArray positive(1, '\0');
    positive.append(QCA::SecureArray(bytes));
    return QCA::BigInteger(positive);
}

QByteArray unsignedBytes(const QCA::BigInteger &value, bool *ok)
{
    if (ok)
        *ok = false;
    if (value < QCA::BigInteger(0))
        return {};

    QByteArray bytes = value.toArray().toByteArray();
    while (bytes.size() > 1 && bytes.front() == '\0')
        bytes.remove(0, 1);
    if (bytes.size() == 1 && bytes.front() == '\0')
        bytes.clear();

    if (ok)
        *ok = true;
    return bytes;
}

QByteArray hexAtom(const QByteArray &value)
{
    return QByteArray("#") + value.toHex().toUpper() + '#';
}

struct SexpNode
{
    bool list = false;
    QByteArray atom;
    QList<SexpNode> children;
};

class SexpParser
{
public:
    explicit SexpParser(const QByteArray &data) : data_(data) {}

    bool parse(SexpNode *node, QString *error)
    {
        error_ = error;
        if (error_)
            error_->clear();
        if (!parseNode(node))
            return false;
        skipSpace();
        if (offset_ != data_.size()) {
            fail(QStringLiteral("Trailing data after private-key S-expression"));
            return false;
        }
        return true;
    }

private:
    void skipSpace()
    {
        while (offset_ < data_.size()) {
            const char c = data_.at(offset_);
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
                break;
            ++offset_;
        }
    }

    void fail(const QString &message)
    {
        setError(error_, message);
    }

    bool parseNode(SexpNode *node)
    {
        if (!node)
            return false;
        skipSpace();
        if (offset_ >= data_.size()) {
            fail(QStringLiteral("Unexpected end of S-expression"));
            return false;
        }

        if (data_.at(offset_) == '(')
            return parseList(node);

        node->list = false;
        return parseAtom(&node->atom);
    }

    bool parseList(SexpNode *node)
    {
        node->list = true;
        node->atom.clear();
        node->children.clear();
        ++offset_;

        for (;;) {
            skipSpace();
            if (offset_ >= data_.size()) {
                fail(QStringLiteral("Unterminated S-expression list"));
                return false;
            }
            if (data_.at(offset_) == ')') {
                ++offset_;
                return true;
            }

            SexpNode child;
            if (!parseNode(&child))
                return false;
            node->children.append(child);
        }
    }

    bool parseAtom(QByteArray *atom)
    {
        if (!atom || offset_ >= data_.size())
            return false;

        const char c = data_.at(offset_);
        if (c == '#')
            return parseHexAtom(atom);
        if (c == '"')
            return parseQuotedAtom(atom);
        if (c >= '0' && c <= '9') {
            const qsizetype saved = offset_;
            qsizetype colon = offset_;
            while (colon < data_.size() && data_.at(colon) >= '0' && data_.at(colon) <= '9')
                ++colon;
            if (colon < data_.size() && data_.at(colon) == ':')
                return parseCanonicalAtom(atom, colon);
            offset_ = saved;
        }
        return parseBareAtom(atom);
    }

    bool parseHexAtom(QByteArray *atom)
    {
        ++offset_;
        const qsizetype begin = offset_;
        while (offset_ < data_.size() && data_.at(offset_) != '#') {
            if (!isHex(data_.at(offset_))) {
                fail(QStringLiteral("Invalid hexadecimal S-expression atom"));
                return false;
            }
            ++offset_;
        }
        if (offset_ >= data_.size()) {
            fail(QStringLiteral("Unterminated hexadecimal S-expression atom"));
            return false;
        }

        const QByteArray hex = data_.mid(begin, offset_ - begin);
        ++offset_;
        if ((hex.size() & 1) != 0) {
            fail(QStringLiteral("Odd-length hexadecimal S-expression atom"));
            return false;
        }
        *atom = QByteArray::fromHex(hex);
        return true;
    }

    bool parseQuotedAtom(QByteArray *atom)
    {
        ++offset_;
        atom->clear();
        while (offset_ < data_.size()) {
            char c = data_.at(offset_++);
            if (c == '"')
                return true;
            if (c != '\\') {
                atom->append(c);
                continue;
            }
            if (offset_ >= data_.size()) {
                fail(QStringLiteral("Unterminated escape in quoted S-expression atom"));
                return false;
            }

            c = data_.at(offset_++);
            switch (c) {
            case 'n': atom->append('\n'); break;
            case 'r': atom->append('\r'); break;
            case 't': atom->append('\t'); break;
            case 'b': atom->append('\b'); break;
            case 'f': atom->append('\f'); break;
            case 'v': atom->append('\v'); break;
            case '\\': atom->append('\\'); break;
            case '"': atom->append('"'); break;
            case '\n': break;
            case 'x': {
                if (offset_ + 1 >= data_.size() || !isHex(data_.at(offset_)) || !isHex(data_.at(offset_ + 1))) {
                    fail(QStringLiteral("Invalid hexadecimal escape in quoted S-expression atom"));
                    return false;
                }
                const QByteArray pair = data_.mid(offset_, 2);
                atom->append(static_cast<char>(pair.toUInt(nullptr, 16)));
                offset_ += 2;
                break;
            }
            default:
                if (c >= '0' && c <= '7') {
                    unsigned value = static_cast<unsigned>(c - '0');
                    int digits = 1;
                    while (digits < 3 && offset_ < data_.size() && data_.at(offset_) >= '0' && data_.at(offset_) <= '7') {
                        value = value * 8 + static_cast<unsigned>(data_.at(offset_) - '0');
                        ++offset_;
                        ++digits;
                    }
                    atom->append(static_cast<char>(value & 0xff));
                } else {
                    atom->append(c);
                }
                break;
            }
        }

        fail(QStringLiteral("Unterminated quoted S-expression atom"));
        return false;
    }

    bool parseCanonicalAtom(QByteArray *atom, qsizetype colon)
    {
        bool lengthOk = false;
        const qulonglong length = data_.mid(offset_, colon - offset_).toULongLong(&lengthOk, 10);
        if (!lengthOk || length > static_cast<qulonglong>(std::numeric_limits<int>::max())) {
            fail(QStringLiteral("Invalid canonical S-expression length"));
            return false;
        }
        offset_ = colon + 1;
        if (length > static_cast<qulonglong>(data_.size() - offset_)) {
            fail(QStringLiteral("Truncated canonical S-expression atom"));
            return false;
        }
        *atom = data_.mid(offset_, static_cast<qsizetype>(length));
        offset_ += static_cast<qsizetype>(length);
        return true;
    }

    bool parseBareAtom(QByteArray *atom)
    {
        const qsizetype begin = offset_;
        while (offset_ < data_.size()) {
            const char c = data_.at(offset_);
            if (c == '(' || c == ')' || c == ' ' || c == '\t' || c == '\r' || c == '\n')
                break;
            ++offset_;
        }
        if (offset_ == begin) {
            fail(QStringLiteral("Invalid S-expression atom"));
            return false;
        }
        *atom = data_.mid(begin, offset_ - begin);
        return true;
    }

    QByteArray data_;
    qsizetype offset_ = 0;
    QString *error_ = nullptr;
};

const SexpNode *listChild(const SexpNode &node, const QByteArray &name)
{
    if (!node.list)
        return nullptr;
    for (const SexpNode &child : node.children) {
        if (!child.list || child.children.isEmpty() || child.children.first().list)
            continue;
        if (child.children.first().atom == name)
            return &child;
    }
    return nullptr;
}

const QByteArray *atomValue(const SexpNode &node, const QByteArray &name)
{
    const SexpNode *child = listChild(node, name);
    if (!child || child->children.size() != 2 || child->children.at(1).list)
        return nullptr;
    return &child->children.at(1).atom;
}

bool parseDsaKey(const SexpNode &accountNode, PrivateKeyRecord *record, QString *error)
{
    const QByteArray *account = atomValue(accountNode, "name");
    const QByteArray *protocol = atomValue(accountNode, "protocol");
    const SexpNode *privateKey = listChild(accountNode, "private-key");
    const SexpNode *dsa = privateKey ? listChild(*privateKey, "dsa") : nullptr;
    if (!account || !protocol || !dsa || !validTextField(*account) || !validTextField(*protocol)) {
        setError(error, QStringLiteral("Invalid libotr private-key account record"));
        return false;
    }

    const QByteArray *p = atomValue(*dsa, "p");
    const QByteArray *q = atomValue(*dsa, "q");
    const QByteArray *g = atomValue(*dsa, "g");
    const QByteArray *y = atomValue(*dsa, "y");
    const QByteArray *x = atomValue(*dsa, "x");
    if (!p || !q || !g || !y || !x || p->isEmpty() || q->isEmpty() || g->isEmpty() || y->isEmpty() || x->isEmpty()) {
        setError(error, QStringLiteral("Incomplete DSA private key in libotr key store"));
        return false;
    }

    DsaPrivateKey key;
    key.domain.p = unsignedInteger(*p);
    key.domain.q = unsignedInteger(*q);
    key.domain.g = unsignedInteger(*g);
    key.x = unsignedInteger(*x);
    const DsaPublicKey publicKey = dsaPublicKey(key);
    if (publicKey.y <= QCA::BigInteger(0) || publicKey.y != unsignedInteger(*y)) {
        setError(error, QStringLiteral("DSA public/private values do not match in libotr key store"));
        return false;
    }

    record->account = *account;
    record->protocol = *protocol;
    record->key = key;
    return true;
}

bool readFile(const QString &path, QByteArray *data, QString *error)
{
    if (!data)
        return false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(error, file.errorString());
        return false;
    }
    *data = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        setError(error, file.errorString());
        return false;
    }
    return true;
}

bool writeFileAtomically(const QString &path, const QByteArray &data, bool privateFile, QString *error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(error, file.errorString());
        return false;
    }
    if (privateFile && !file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
        file.cancelWriting();
        setError(error, QStringLiteral("Failed to set private-key file permissions"));
        return false;
    }
    if (file.write(data) != data.size()) {
        const QString message = file.errorString();
        file.cancelWriting();
        setError(error, message);
        return false;
    }
    if (!file.commit()) {
        setError(error, file.errorString());
        return false;
    }
    return true;
}

} // namespace

bool parsePrivateKeys(const QByteArray &data, QList<PrivateKeyRecord> *records, QString *error)
{
    if (!records)
        return false;
    records->clear();
    if (error)
        error->clear();

    SexpNode root;
    SexpParser parser(data);
    if (!parser.parse(&root, error) || !root.list || root.children.isEmpty() || root.children.first().list ||
        root.children.first().atom != "privkeys") {
        setError(error, QStringLiteral("Not a libotr private-key store"));
        return false;
    }

    QList<PrivateKeyRecord> parsed;
    for (int i = 1; i < root.children.size(); ++i) {
        const SexpNode &child = root.children.at(i);
        if (!child.list || child.children.isEmpty() || child.children.first().list || child.children.first().atom != "account") {
            setError(error, QStringLiteral("Unexpected entry in libotr private-key store"));
            return false;
        }
        PrivateKeyRecord record;
        if (!parseDsaKey(child, &record, error))
            return false;
        parsed.append(record);
    }

    *records = parsed;
    return true;
}

QByteArray serializePrivateKeys(const QList<PrivateKeyRecord> &records, bool *ok)
{
    if (ok)
        *ok = false;

    QByteArray output("(privkeys\n");
    for (const PrivateKeyRecord &record : records) {
        if (!validTextField(record.account) || !validTextField(record.protocol))
            return {};

        const DsaPublicKey publicKey = dsaPublicKey(record.key);
        if (publicKey.y <= QCA::BigInteger(0))
            return {};

        bool pOk = false;
        bool qOk = false;
        bool gOk = false;
        bool yOk = false;
        bool xOk = false;
        const QByteArray p = unsignedBytes(record.key.domain.p, &pOk);
        const QByteArray q = unsignedBytes(record.key.domain.q, &qOk);
        const QByteArray g = unsignedBytes(record.key.domain.g, &gOk);
        const QByteArray y = unsignedBytes(publicKey.y, &yOk);
        const QByteArray x = unsignedBytes(record.key.x, &xOk);
        if (!pOk || !qOk || !gOk || !yOk || !xOk || p.isEmpty() || q.isEmpty() || g.isEmpty() || y.isEmpty() || x.isEmpty())
            return {};

        output += " (account\n";
        output += "  (name " + hexAtom(record.account) + ")\n";
        output += "  (protocol " + hexAtom(record.protocol) + ")\n";
        output += "  (private-key\n";
        output += "   (dsa\n";
        output += "    (p " + hexAtom(p) + ")\n";
        output += "    (q " + hexAtom(q) + ")\n";
        output += "    (g " + hexAtom(g) + ")\n";
        output += "    (y " + hexAtom(y) + ")\n";
        output += "    (x " + hexAtom(x) + ")\n";
        output += "   )\n";
        output += "  )\n";
        output += " )\n";
    }
    output += ")\n";

    if (ok)
        *ok = true;
    return output;
}

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

        if (!validTabField(record.username) || !validTabField(record.account) || !validTabField(record.protocol) || hex.size() != 40) {
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
            record.fingerprint.size() != 20 || record.trust.contains('\r') || record.trust.contains('\n') || record.trust.contains('\0')) {
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

bool readPrivateKeysFile(const QString &path, QList<PrivateKeyRecord> *records, QString *error)
{
    QByteArray data;
    return readFile(path, &data, error) && parsePrivateKeys(data, records, error);
}

bool writePrivateKeysFile(const QString &path, const QList<PrivateKeyRecord> &records, QString *error)
{
    bool ok = false;
    const QByteArray data = serializePrivateKeys(records, &ok);
    if (!ok) {
        setError(error, QStringLiteral("Cannot serialize libotr private-key store"));
        return false;
    }
    return writeFileAtomically(path, data, true, error);
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
    return writeFileAtomically(path, data, false, error);
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
    return writeFileAtomically(path, data, false, error);
}

} // namespace QcaOtr::Persistence
