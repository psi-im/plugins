/*
 * syntaxhighlighter.cpp - plugin
 * Copyright (C) 2022-2026  Psi+ Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "syntaxhighlighter.h"

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    keywordFormat.setForeground(Qt::magenta);
    const auto keywordPatterns
        = { QStringLiteral("(?<=\\d)(?:px|pt|em|ex)\\b|%"), QStringLiteral("a?rgba?|hsva?|hsla?|url|(?:x|y)\\d"),
            QStringLiteral("q(?:linear|radial|conical){1}gradient|transparent") };
    for (const auto &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format  = keywordFormat;
        highlightingRules.append(rule);
    }

    typesFormat.setFontWeight(QFont::Bold);
    typesFormat.setForeground(Qt::darkCyan);
    const auto typesPattrens
        = { QStringLiteral("\\b(?:bold|italic|normal|oblique|(?:under|over)?line(?:-through)?)\\b"), // font style
            QStringLiteral("\\b(?:true|false)\\b"),
            QStringLiteral("\\b(?:scroll|fixed)\\b"),
            QStringLiteral("\\b(?:none|dotted|dashed|solid|double|groove|ridge|inset|outset)\\b"),
            QStringLiteral("\\b(?:right|bottom|top|left|center)\\b"),
            QStringLiteral("\\b(?:black|white|red|blue|yellow|magenta|green|grey)\\b"), // colors
            QStringLiteral("\\b(?:disabled|active|selected|on|off)\\b"),
            QStringLiteral("\\b(?:margin|border|padding|content)\\b"),
            QStringLiteral("\\b(?:(?:alternate-)?base|bright-text|button(?:-text)?|dark|highlight(?:ed-text)?)\\b"),
            QStringLiteral("\\b(?:light|link(?:-visited)?|mid(?:light)?|shadow|text|window(?:-text)?)\\b"),
            QStringLiteral("\\b(?:no-)?repeat(?:-[xy])?\\b") };
    for (const auto &pattern : typesPattrens) {
        rule.pattern = QRegularExpression(pattern);
        rule.format  = typesFormat;
        highlightingRules.append(rule);
    }

    classFormat.setFontWeight(QFont::Bold);
    classFormat.setForeground(Qt::darkGreen);

    multiLineCommentFormat.setForeground(Qt::red);

    variablesFormat.setForeground(Qt::blue);
    rule.pattern = QRegularExpression(QStringLiteral("(?:^|[;\\s\\{])([A-Za-z_][A-Za-z0-9_-]*)(?=\\s*:)"));
    rule.format  = variablesFormat;
    highlightingRules.append(rule);

    valuesFormat.setFontWeight(QFont::Bold);
    valuesFormat.setForeground(Qt::darkYellow);
    rule.pattern
        = QRegularExpression(QStringLiteral("(?:^|[^\\w.-])[-+]?(?:\\d+(?:\\.\\d*)?|\\.\\d+)(?:px|pt|em|ex|%)?\\b"));
    rule.format = valuesFormat;
    highlightingRules.append(rule);

    hexNumbersFormat.setFontWeight(QFont::Bold);
    hexNumbersFormat.setForeground(QColor(QStringLiteral("#8B6AAE")));
    rule.pattern = QRegularExpression(
        QStringLiteral("#(?:[A-Fa-f0-9]{3}|[A-Fa-f0-9]{4}|[A-Fa-f0-9]{6}|[A-Fa-f0-9]{8})(?![A-Fa-f0-9])"));
    rule.format = hexNumbersFormat;
    highlightingRules.append(rule);

    QTextCharFormat importantFormat = variablesFormat;
    importantFormat.setFontWeight(QFont::Bold);

    QRegularExpression importantRx(QStringLiteral("\\!important\\b"));
    rule.pattern = importantRx;
    rule.format  = importantFormat;
    highlightingRules.append(rule);

    QRegularExpression pseudoStatesRx(
        QStringLiteral(":(?:hover|pressed|checked|selected|focus|disabled|enabled|active)\\b"));
    rule.pattern = pseudoStatesRx;
    rule.format  = typesFormat;
    highlightingRules.append(rule);

    QRegularExpression qpropertyRx(QStringLiteral("\\bqproperty-[A-Za-z0-9_-]+\\b"));
    rule.pattern = qpropertyRx;
    rule.format  = variablesFormat;
    highlightingRules.append(rule);

    commentStartExpression = QRegularExpression(QStringLiteral("/\\*"));
    commentEndExpression   = QRegularExpression(QStringLiteral("\\*/"));
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : std::as_const(highlightingRules)) {
        auto matches = rule.pattern.globalMatch(text);
        while (matches.hasNext()) {
            const auto match = matches.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    constexpr int InCommentState      = 0x1;
    constexpr int InDeclarationState  = 0x2;
    constexpr int InSingleStringState = 0x4;
    constexpr int InDoubleStringState = 0x8;

    int state = previousBlockState();
    if (state < 0)
        state = 0;

    bool inComment     = (state & InCommentState) != 0;
    bool inDeclaration = (state & InDeclarationState) != 0;
    bool inString      = (state & (InSingleStringState | InDoubleStringState)) != 0;

    QChar quoteChar;
    if (state & InSingleStringState)
        quoteChar = QLatin1Char('\'');
    else if (state & InDoubleStringState)
        quoteChar = QLatin1Char('"');

    int selectorStart = inDeclaration ? -1 : 0;
    int i             = 0;

    while (i < text.size()) {
        if (inComment) {
            const int end = text.indexOf(QStringLiteral("*/"), i);
            if (end < 0) {
                setFormat(i, text.size() - i, multiLineCommentFormat);
                break;
            }

            setFormat(i, end + 2 - i, multiLineCommentFormat);
            i         = end + 2;
            inComment = false;

            if (!inDeclaration)
                selectorStart = i;

            continue;
        }

        const QChar ch = text.at(i);

        if (inString) {
            if (ch == QLatin1Char('\\')) {
                i += (i + 1 < text.size()) ? 2 : 1;
                continue;
            }

            if (ch == quoteChar) {
                inString  = false;
                quoteChar = QChar();
            }

            ++i;
            continue;
        }

        if (ch == QLatin1Char('/') && i + 1 < text.size() && text.at(i + 1) == QLatin1Char('*')) {
            if (selectorStart >= 0 && selectorStart < i)
                setFormat(selectorStart, i - selectorStart, classFormat);

            const int end = text.indexOf(QStringLiteral("*/"), i + 2);
            if (end < 0) {
                setFormat(i, text.size() - i, multiLineCommentFormat);
                inComment = true;
                break;
            }

            setFormat(i, end + 2 - i, multiLineCommentFormat);
            i = end + 2;

            if (!inDeclaration)
                selectorStart = i;

            continue;
        }

        if (ch == QLatin1Char('"') || ch == QLatin1Char('\'')) {
            inString  = true;
            quoteChar = ch;
            ++i;
            continue;
        }

        if (!inDeclaration && ch == QLatin1Char('{')) {
            if (selectorStart >= 0 && selectorStart < i)
                setFormat(selectorStart, i - selectorStart, classFormat);

            inDeclaration = true;
            selectorStart = -1;
            ++i;
            continue;
        }

        if (inDeclaration && ch == QLatin1Char('}')) {
            inDeclaration = false;
            selectorStart = i + 1;
            ++i;
            continue;
        }

        ++i;
    }

    if (!inComment && !inDeclaration && selectorStart >= 0 && selectorStart < text.size()) {
        setFormat(selectorStart, text.size() - selectorStart, classFormat);
    }

    int newState = 0;
    if (inComment)
        newState |= InCommentState;
    if (inDeclaration)
        newState |= InDeclarationState;
    if (inString)
        newState |= (quoteChar == QLatin1Char('\'')) ? InSingleStringState : InDoubleStringState;

    setCurrentBlockState(newState);
}
