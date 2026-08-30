/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#include "qca-otr/persistence.h"

#include "persistence_p.h"

#include <climits>
#include <cstring>

namespace QcaOtr::Persistence {
namespace {

using Private::hexValue;
using Private::setError;

bool validTextField(const QCA::SecureArray &value)
{
    if (value.isEmpty())
        return false;
    for (int i = 0; i < value.size(); ++i) {
        const char c = value.at(i);
        if (c == '\0' || c == '\r' || c == '\n')
            return false;
    }
    return true;
}

bool equalsAscii(const QCA::SecureArray &value, const char *text)
{
    const size_t length = std::strlen(text);
    return length <= static_cast<size_t>(INT_MAX) && value.size() == static_cast<int>(length) &&
        std::memcmp(value.constData(), text, length) == 0;
}

QCA::SecureArray slice(const QCA::SecureArray &data, int offset, int length)
{
    if (offset < 0 || length < 0 || offset > data.size() || length > data.size() - offset)
        return {};
    QCA::SecureArray result(length);
    if (length > 0)
        std::memcpy(result.data(), data.constData() + offset, static_cast<size_t>(length));
    return result;
}

bool appendByte(QCA::SecureArray *output, char value)
{
    if (!output || output->size() == INT_MAX || !output->resize(output->size() + 1))
        return false;
    output->at(output->size() - 1) = value;
    return true;
}

bool appendBytes(QCA::SecureArray *output, const char *data, int length)
{
    if (!output || length < 0 || (length > 0 && !data) || length > INT_MAX - output->size())
        return false;
    const int oldSize = output->size();
    if (!output->resize(oldSize + length))
        return false;
    if (length > 0)
        std::memcpy(output->data() + oldSize, data, static_cast<size_t>(length));
    return true;
}

bool appendAscii(QCA::SecureArray *output, const char *text)
{
    const size_t length = std::strlen(text);
    return length <= static_cast<size_t>(INT_MAX) && appendBytes(output, text, static_cast<int>(length));
}

char hexDigit(unsigned value)
{
    static constexpr char digits[] = "0123456789ABCDEF";
    return digits[value & 0x0f];
}

bool appendHexAtom(QCA::SecureArray *output, const char *data, int length)
{
    if (!appendByte(output, '#'))
        return false;
    for (int i = 0; i < length; ++i) {
        const unsigned byte = static_cast<unsigned char>(data[i]);
        if (!appendByte(output, hexDigit(byte >> 4)) || !appendByte(output, hexDigit(byte)))
            return false;
    }
    return appendByte(output, '#');
}

bool appendHexAtom(QCA::SecureArray *output, const QCA::SecureArray &value)
{
    return appendHexAtom(output, value.constData(), value.size());
}

bool appendHexAtom(QCA::SecureArray *output, const QByteArray &value)
{
    return appendHexAtom(output, value.constData(), value.size());
}

QCA::BigInteger unsignedInteger(const QCA::SecureArray &bytes)
{
    QCA::SecureArray positive(bytes.size() + 1);
    positive.at(0) = '\0';
    if (!bytes.isEmpty())
        std::memcpy(positive.data() + 1, bytes.constData(), static_cast<size_t>(bytes.size()));
    return QCA::BigInteger(positive);
}

QCA::SecureArray unsignedBytes(const QCA::BigInteger &value, bool *ok)
{
    if (ok)
        *ok = false;
    if (value < QCA::BigInteger(0))
        return {};

    const QCA::SecureArray bytes = value.toArray();
    int offset = 0;
    while (offset + 1 < bytes.size() && bytes.at(offset) == '\0')
        ++offset;
    if (offset + 1 == bytes.size() && bytes.at(offset) == '\0') {
        if (ok)
            *ok = true;
        return {};
    }

    QCA::SecureArray result = offset == 0 ? bytes : slice(bytes, offset, bytes.size() - offset);
    if (ok)
        *ok = true;
    return result;
}

QByteArray metadataBytes(const QCA::SecureArray &value)
{
    return QByteArray(value.constData(), value.size());
}

struct SexpNode
{
    bool list = false;
    QCA::SecureArray atom;
    QList<SexpNode> children;
};

class SexpParser
{
public:
    explicit SexpParser(const QCA::SecureArray &data) : data_(data) {}

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

    bool parseAtom(QCA::SecureArray *atom)
    {
        if (!atom || offset_ >= data_.size())
            return false;

        const char c = data_.at(offset_);
        if (c == '#')
            return parseHexAtom(atom);
        if (c == '"')
            return parseQuotedAtom(atom);
        if (c >= '0' && c <= '9') {
            const int saved = offset_;
            int colon = offset_;
            while (colon < data_.size() && data_.at(colon) >= '0' && data_.at(colon) <= '9')
                ++colon;
            if (colon < data_.size() && data_.at(colon) == ':')
                return parseCanonicalAtom(atom, colon);
            offset_ = saved;
        }
        return parseBareAtom(atom);
    }

    bool parseHexAtom(QCA::SecureArray *atom)
    {
        ++offset_;
        const int begin = offset_;
        while (offset_ < data_.size() && data_.at(offset_) != '#') {
            if (hexValue(data_.at(offset_)) < 0) {
                fail(QStringLiteral("Invalid hexadecimal S-expression atom"));
                return false;
            }
            ++offset_;
        }
        if (offset_ >= data_.size()) {
            fail(QStringLiteral("Unterminated hexadecimal S-expression atom"));
            return false;
        }

        const int hexLength = offset_ - begin;
        ++offset_;
        if ((hexLength & 1) != 0) {
            fail(QStringLiteral("Odd-length hexadecimal S-expression atom"));
            return false;
        }
        if (!atom->resize(hexLength / 2)) {
            fail(QStringLiteral("Cannot allocate hexadecimal S-expression atom"));
            return false;
        }
        for (int i = 0; i < hexLength; i += 2) {
            const int high = hexValue(data_.at(begin + i));
            const int low = hexValue(data_.at(begin + i + 1));
            atom->at(i / 2) = static_cast<char>((high << 4) | low);
        }
        return true;
    }

    bool parseQuotedAtom(QCA::SecureArray *atom)
    {
        ++offset_;
        atom->clear();
        while (offset_ < data_.size()) {
            char c = data_.at(offset_++);
            if (c == '"')
                return true;
            if (c != '\\') {
                if (!appendByte(atom, c))
                    return false;
                continue;
            }
            if (offset_ >= data_.size()) {
                fail(QStringLiteral("Unterminated escape in quoted S-expression atom"));
                return false;
            }

            c = data_.at(offset_++);
            switch (c) {
            case 'n': c = '\n'; break;
            case 'r': c = '\r'; break;
            case 't': c = '\t'; break;
            case 'b': c = '\b'; break;
            case 'f': c = '\f'; break;
            case 'v': c = '\v'; break;
            case '\\': c = '\\'; break;
            case '"': c = '"'; break;
            case '\n': continue;
            case 'x': {
                if (offset_ + 1 >= data_.size()) {
                    fail(QStringLiteral("Invalid hexadecimal escape in quoted S-expression atom"));
                    return false;
                }
                const int high = hexValue(data_.at(offset_));
                const int low = hexValue(data_.at(offset_ + 1));
                if (high < 0 || low < 0) {
                    fail(QStringLiteral("Invalid hexadecimal escape in quoted S-expression atom"));
                    return false;
                }
                c = static_cast<char>((high << 4) | low);
                offset_ += 2;
                break;
            }
            default:
                if (c >= '0' && c <= '7') {
                    unsigned value = static_cast<unsigned>(c - '0');
                    int digits = 1;
                    while (digits < 3 && offset_ < data_.size() && data_.at(offset_) >= '0' &&
                           data_.at(offset_) <= '7') {
                        value = value * 8 + static_cast<unsigned>(data_.at(offset_) - '0');
                        ++offset_;
                        ++digits;
                    }
                    c = static_cast<char>(value & 0xff);
                }
                break;
            }
            if (!appendByte(atom, c))
                return false;
        }

        fail(QStringLiteral("Unterminated quoted S-expression atom"));
        return false;
    }

    bool parseCanonicalAtom(QCA::SecureArray *atom, int colon)
    {
        unsigned long long length = 0;
        for (int i = offset_; i < colon; ++i) {
            const unsigned digit = static_cast<unsigned>(data_.at(i) - '0');
            if (length > (static_cast<unsigned long long>(INT_MAX) - digit) / 10) {
                fail(QStringLiteral("Invalid canonical S-expression length"));
                return false;
            }
            length = length * 10 + digit;
        }
        offset_ = colon + 1;
        if (length > static_cast<unsigned long long>(data_.size() - offset_)) {
            fail(QStringLiteral("Truncated canonical S-expression atom"));
            return false;
        }
        *atom = slice(data_, offset_, static_cast<int>(length));
        offset_ += static_cast<int>(length);
        return true;
    }

    bool parseBareAtom(QCA::SecureArray *atom)
    {
        const int begin = offset_;
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
        *atom = slice(data_, begin, offset_ - begin);
        return true;
    }

    const QCA::SecureArray &data_;
    int offset_ = 0;
    QString *error_ = nullptr;
};

const SexpNode *listChild(const SexpNode &node, const char *name)
{
    if (!node.list)
        return nullptr;
    for (const SexpNode &child : node.children) {
        if (!child.list || child.children.isEmpty() || child.children.first().list)
            continue;
        if (equalsAscii(child.children.first().atom, name))
            return &child;
    }
    return nullptr;
}

const QCA::SecureArray *atomValue(const SexpNode &node, const char *name)
{
    const SexpNode *child = listChild(node, name);
    if (!child || child->children.size() != 2 || child->children.at(1).list)
        return nullptr;
    return &child->children.at(1).atom;
}

bool parseDsaKey(const SexpNode &accountNode, PrivateKeyRecord *record, QString *error)
{
    const QCA::SecureArray *account = atomValue(accountNode, "name");
    const QCA::SecureArray *protocol = atomValue(accountNode, "protocol");
    const SexpNode *privateKey = listChild(accountNode, "private-key");
    const SexpNode *dsa = privateKey ? listChild(*privateKey, "dsa") : nullptr;
    if (!account || !protocol || !dsa || !validTextField(*account) || !validTextField(*protocol)) {
        setError(error, QStringLiteral("Invalid libotr private-key account record"));
        return false;
    }

    const QCA::SecureArray *p = atomValue(*dsa, "p");
    const QCA::SecureArray *q = atomValue(*dsa, "q");
    const QCA::SecureArray *g = atomValue(*dsa, "g");
    const QCA::SecureArray *y = atomValue(*dsa, "y");
    const QCA::SecureArray *x = atomValue(*dsa, "x");
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

    record->account = metadataBytes(*account);
    record->protocol = metadataBytes(*protocol);
    record->key = key;
    return true;
}

} // namespace

bool parsePrivateKeys(const QCA::SecureArray &data, QList<PrivateKeyRecord> *records, QString *error)
{
    if (!records)
        return false;
    records->clear();
    if (error)
        error->clear();

    SexpNode root;
    SexpParser parser(data);
    if (!parser.parse(&root, error) || !root.list || root.children.isEmpty() || root.children.first().list ||
        !equalsAscii(root.children.first().atom, "privkeys")) {
        setError(error, QStringLiteral("Not a libotr private-key store"));
        return false;
    }

    QList<PrivateKeyRecord> parsed;
    for (int i = 1; i < root.children.size(); ++i) {
        const SexpNode &child = root.children.at(i);
        if (!child.list || child.children.isEmpty() || child.children.first().list ||
            !equalsAscii(child.children.first().atom, "account")) {
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

QCA::SecureArray serializePrivateKeys(const QList<PrivateKeyRecord> &records, bool *ok)
{
    if (ok)
        *ok = false;

    QCA::SecureArray output("(privkeys\n");
    for (const PrivateKeyRecord &record : records) {
        if (!Private::validTextField(record.account) || !Private::validTextField(record.protocol))
            return {};

        const DsaPublicKey publicKey = dsaPublicKey(record.key);
        if (publicKey.y <= QCA::BigInteger(0))
            return {};

        bool pOk = false;
        bool qOk = false;
        bool gOk = false;
        bool yOk = false;
        bool xOk = false;
        const QCA::SecureArray p = unsignedBytes(record.key.domain.p, &pOk);
        const QCA::SecureArray q = unsignedBytes(record.key.domain.q, &qOk);
        const QCA::SecureArray g = unsignedBytes(record.key.domain.g, &gOk);
        const QCA::SecureArray y = unsignedBytes(publicKey.y, &yOk);
        const QCA::SecureArray x = unsignedBytes(record.key.x, &xOk);
        if (!pOk || !qOk || !gOk || !yOk || !xOk || p.isEmpty() || q.isEmpty() || g.isEmpty() || y.isEmpty() ||
            x.isEmpty()) {
            return {};
        }

        if (!appendAscii(&output, " (account\n  (name ") || !appendHexAtom(&output, record.account) ||
            !appendAscii(&output, ")\n  (protocol ") || !appendHexAtom(&output, record.protocol) ||
            !appendAscii(&output, ")\n  (private-key\n   (dsa\n    (p ") || !appendHexAtom(&output, p) ||
            !appendAscii(&output, ")\n    (q ") || !appendHexAtom(&output, q) || !appendAscii(&output, ")\n    (g ") ||
            !appendHexAtom(&output, g) || !appendAscii(&output, ")\n    (y ") || !appendHexAtom(&output, y) ||
            !appendAscii(&output, ")\n    (x ") || !appendHexAtom(&output, x) ||
            !appendAscii(&output, ")\n   )\n  )\n )\n")) {
            return {};
        }
    }
    if (!appendAscii(&output, ")\n"))
        return {};

    if (ok)
        *ok = true;
    return output;
}

bool readPrivateKeysFile(const QString &path, QList<PrivateKeyRecord> *records, QString *error)
{
    QCA::SecureFile file(path);
    const QCA::SecureArray data = file.read();
    if (file.error() != QCA::SecureFile::NoError) {
        setError(error, file.errorString());
        return false;
    }
    return parsePrivateKeys(data, records, error);
}

bool writePrivateKeysFile(const QString &path, const QList<PrivateKeyRecord> &records, QString *error)
{
    bool ok = false;
    const QCA::SecureArray data = serializePrivateKeys(records, &ok);
    if (!ok) {
        setError(error, QStringLiteral("Cannot serialize libotr private-key store"));
        return false;
    }

    QCA::SecureFile file(path);
    if (!file.write(data)) {
        setError(error, file.errorString());
        return false;
    }
    return true;
}

} // namespace QcaOtr::Persistence
