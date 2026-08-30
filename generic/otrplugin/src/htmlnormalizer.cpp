/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "htmlnormalizer.h"

#include <QDomDocument>
#include <QDomElement>
#include <QTextDocument>
#include <QTextStream>

#include <utility>

namespace {

constexpr auto XhtmlNamespace = "http://www.w3.org/1999/xhtml";

QDomElement normalizedBody(QDomDocument &target, const QString &input)
{
    QTextDocument richText;
    richText.setHtml(input);

    QDomDocument normalized;
#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
    const bool parsed = normalized.setContent(richText.toHtml());
#else
    const auto parseResult = normalized.setContent(richText.toHtml(), QDomDocument::ParseOption::UseNamespaceProcessing);
    const bool parsed = static_cast<bool>(parseResult);
#endif
    if (!parsed) {
        QDomElement body = target.createElementNS(QString::fromLatin1(XhtmlNamespace), QStringLiteral("body"));
        body.appendChild(target.createTextNode(richText.toPlainText()));
        return body;
    }

    const QDomElement sourceBody = normalized.documentElement().firstChildElement(QStringLiteral("body"));
    QDomElement body = target.createElementNS(QString::fromLatin1(XhtmlNamespace), QStringLiteral("body"));
    if (sourceBody.isNull()) {
        body.appendChild(target.createTextNode(richText.toPlainText()));
        return body;
    }

    for (QDomNode node = sourceBody.firstChild(); !node.isNull(); node = node.nextSibling())
        body.appendChild(target.importNode(node, true));
    return body;
}

} // namespace

HtmlNormalizer::HtmlNormalizer(QString html) : input_(std::move(html)) { }

QString HtmlNormalizer::output() const
{
    QDomDocument document;
    const QDomElement body = normalizedBody(document, input_);
    QString result;
    QTextStream stream(&result);
    body.save(stream, 0);
    return result;
}

QDomElement HtmlNormalizer::output(QDomDocument &document) const
{
    return normalizedBody(document, input_);
}
