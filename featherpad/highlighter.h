/*
 * Copyright (C) Pedram Pourang (aka Tsu Jan) 2014 <tsujan2000@gmail.com>
 *
 * FeatherPad is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FeatherPad is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>

namespace FeatherPad {

struct ParenthesisInfo
{
    char character; // '(' or ')'
    int position;
};

struct BraceInfo
{
    char character; // '{' or '}'
    int position;
};

/* This class is for detection of matching parentheses and
   braces, and also for highlighting of here-documents. */
class TextBlockData : public QTextBlockUserData
{
public:
    TextBlockData() {}
    ~TextBlockData();
    QVector<ParenthesisInfo *> parentheses();
    QVector<BraceInfo *> braces();
    QString delimiter();
    void insertInfo (ParenthesisInfo *info);
    void insertInfo (BraceInfo *info);
    void insertInfo (QString str);

private:
    QVector<ParenthesisInfo *> allParentheses;
    QVector<BraceInfo *> allBraces;
    QString Delimiter; // The delimiter string of a here-doc.
};
/*************************/
/* This is a tricky but effective way for syntax highlighting. */
class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    Highlighter (QTextDocument *parent, QString lang, QTextCursor start, QTextCursor end);

    void setStartCursor (QTextCursor cur) {
        startCursor = cur;
    }

    void setEndCursor (QTextCursor cur) {
        endCursor = cur;
    }

    QSet<int> getHighlighted() {
        return highlighted;
    }

    void addHighlighted (int block) {
        highlighted.insert (block);
    }

    void setFirstRun (bool first) {
        firstRun = first;
    }

protected:
    void highlightBlock (const QString &text);

private:
    QStringList keywords (QString& lang);
    QStringList types();
    bool escapedQuote (const QString &text, const int pos, bool canEscapeStart);
    bool isQuoted (const QString &text, const int index);
    bool isMLCommented (const QString &text, const int index);
    void resetHereDocStates (QTextBlock block);
    bool isHereDocument (const QString &text);
    void pythonMLComment (const QString &text, const int indx);
    void htmlStyleHighlighter (const QString &text);
    void htmlBrackets (const QString &text);
    void htmlJavascript (const QString &text);
    int cssHighlighter (const QString &text);
    void singleLineComment (const QString &text, int index);
    void multiLineComment (const QString &text,
                           int index, int cssIndx,
                           QRegExp commentStartExp, QRegExp commentEndExp,
                           int commState);
    void multiLineQuote (const QString &text);

    struct HighlightingRule
    {
        QRegExp pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    /* Multiline comments: */
    QRegExp commentStartExpression;
    QRegExp commentEndExpression;

    QTextCharFormat commentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat urlFormat;

    /* Programming language: */
    QString progLan;

    /* The start and end cursors of the visible text: */
    QTextCursor startCursor, endCursor;
    /* List of all blocks, for which the main formatting is already done: */
    QSet<int> highlighted;
    bool firstRun;

    /* Block states: */
    enum
    {
        commentState = 1,

        /* Quotation marks: */
        doubleQuoteState,
        singleQuoteState,

        /* Python comments: */
        pyDoubleQuoteState,
        pySingleQuoteState,

        /* HTML: */
        htmlStyleState,
        htmlStyleBracketState,
        //htmlStyleAttState,
        htmlStyleSingleQuoteState,
        htmlStyleDoubleQuoteState,
        htmlBracketState,
        //htmlAttState,
        htmlBlockState,
        htmlValueState,
        htmlJavaState,
        htmlJavaCommentState,

        /* CSS: */
        cssBlockState,
        commentInCssState,
        cssValueState,
        endState // 18

        /* For here-docs, state >= endState or state < -1. */
    };
};

}

#endif // HIGHLIGHTER_H
