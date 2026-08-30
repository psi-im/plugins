/*
 * htmltidy.cpp - Normalize rich text with Qt
 */

#include "htmltidy.h"

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
    if (!normalized.setContent(richText.toHtml())) {
#else
    if (!normalized.setContent(richText.toHtml())) {
#endif
        QDomElement body = target.createElementNS(QString::fromLatin1(XhtmlNamespace), QStringLiteral("body"));
        body.appendChild(target.createTextNode(richText.toPlainText()));
        return body;
    }

    const QDomElement sourceBody = normalized.documentElement().firstChildElement(QStringLiteral("body"));
    QDomElement body = target.createElementNS(QString::fromLatin1(XhtmlNamespace), QStringLiteral("body"));
    for (QDomNode node = sourceBody.firstChild(); !node.isNull(); node = node.nextSibling())
        body.appendChild(target.importNode(node, true));
    return body;
}

} // namespace

HtmlTidy::HtmlTidy(QString html) : m_input(std::move(html)) { }

QString HtmlTidy::output() const
{
    QDomDocument document;
    const QDomElement body = normalizedBody(document, m_input);
    QString result;
    QTextStream stream(&result);
    body.save(stream, 0);
    return result;
}

QDomElement HtmlTidy::output(QDomDocument &document) const
{
    return normalizedBody(document, m_input);
}
