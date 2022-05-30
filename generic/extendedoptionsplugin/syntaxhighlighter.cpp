/*
 * syntaxhighlighter.cpp - plugin
 * Copyright (C) 2022  Psi+ Project
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
        = { QStringLiteral("\\bbold|italic|normal|oblique|(?:under|over)?line(?:-through)?\\b"), // font style
            QStringLiteral("\\btrue|false\\b"),
            QStringLiteral("\\bscroll|fixed\\b"),
            QStringLiteral(
                "\\b(?:(?:dot-){1,2})?dash(?:ed)?|dotted|double|groove|(?:in|out|up)set|ridge|solid|none|stretch\\b"),
            QStringLiteral("\\bright|bottom|top|left|center\\b"),
            QStringLiteral("\\bblack|white|red|blue|yellow|magenta|green|grey\\b"), // colors
            QStringLiteral("\\b(?:disabled|active|selected|on|off)\\b"),
            QStringLiteral("\\bmargin|border|padding|content\\b"),
            QStringLiteral("\\b(?:alternate-)?base|bright-text|button(?:-text)?|dark|highligh(?:ted-text)?\\b"),
            QStringLiteral("\\blight|link(?:-visited)?|mid(?:light)?|shadow|text|window(?:-text)?\\b"),
            QStringLiteral("\\b(?:no-)?repeat(?:-[xy])?\\b")

          };
    for (const auto &pattern : typesPattrens) {
        rule.pattern = QRegularExpression(pattern);
        rule.format  = typesFormat;
        highlightingRules.append(rule);
    }

    classFormat.setFontWeight(QFont::Bold);
    classFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegularExpression(QStringLiteral("\\b.+(?=\\s+{)"));
    rule.format  = classFormat;
    highlightingRules.append(rule);

    singleLineCommentFormat.setForeground(Qt::red);
    rule.pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
    rule.format  = singleLineCommentFormat;
    highlightingRules.append(rule);

    multiLineCommentFormat.setForeground(Qt::red);

    variablesFormat.setForeground(Qt::blue);
    rule.pattern = QRegularExpression(QStringLiteral("(?<=\\s)[a-z-]+:(?=\\s)"));
    rule.format  = variablesFormat;
    highlightingRules.append(rule);

    valuesFormat.setFontWeight(QFont::Bold);
    valuesFormat.setForeground(Qt::darkYellow);
    rule.pattern
        = QRegularExpression(QStringLiteral("(?:\\s|\\b|(?<=,|\\())[-]?\\d+(?:\\.\\d+)?|(?<=url\\()[^\\)]*(?=\\))"));
    rule.format = valuesFormat;
    highlightingRules.append(rule);

    hexNumbersFormat.setFontWeight(QFont::Bold);
    hexNumbersFormat.setForeground(Qt::green);
    rule.pattern = QRegularExpression(QStringLiteral("#[A-Fa-f0-9]{3,6}"));
    rule.format  = hexNumbersFormat;
    highlightingRules.append(rule);

    commentStartExpression = QRegularExpression(QStringLiteral("/\\*"));
    commentEndExpression   = QRegularExpression(QStringLiteral("\\*/"));
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : qAsConst(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(commentStartExpression);

    while (startIndex >= 0) {
        QRegularExpressionMatch match         = commentEndExpression.match(text, startIndex);
        int                     endIndex      = match.capturedStart();
        int                     commentLength = 0;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + match.capturedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }
}
