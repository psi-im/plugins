/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QString>

class QDomDocument;
class QDomElement;

/**
 * Normalizes arbitrary rich-text fragments with Qt's HTML parser and returns
 * a standalone XHTML body suitable for XHTML-IM payloads.
 *
 * The normalizer deliberately does not depend on libtidy. If Qt cannot turn
 * the input into a DOM tree, it falls back to a body containing plain text.
 */
class HtmlNormalizer
{
public:
    explicit HtmlNormalizer(QString html);

    /** Returns the normalized XHTML body serialized as text. */
    QString output() const;

    /** Returns the normalized body as an element owned by @p document. */
    QDomElement output(QDomDocument &document) const;

private:
    QString input_;
};
