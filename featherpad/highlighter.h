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
#include <QHash>
#include <QTextCharFormat>

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
    bool isdelimiter();
    void insertInfo (ParenthesisInfo *info);
    void insertInfo (BraceInfo *info);
    void insertInfo (QString str);
    void insertInfo (bool boolean);

private:
    QVector<ParenthesisInfo *> allParentheses;
    QVector<BraceInfo *> allBraces;
    QString Delimiter; // The delimiter string of a here-doc.
    bool isDelimiter; // Is this block a here-doc delimiter?
};
/*************************/
/* This is a tricky but effective way for syntax highlighting. */
class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    Highlighter (QTextDocument *parent, QString lang);

protected:
    void highlightBlock (const QString &text);

private slots:
    void docChanged (int pos, int, int);

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

    /* Needed for re-highlighting of here-docs: */
    int delimState;

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

        /* Here-documents: */
        hereDocBodyState,
        hereDocHeaderState,
        // 8=9-1 is reserved.
        hereDocTempState = 9,

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
        cssValueState
    };
};

}

#endif // HIGHLIGHTER_H
