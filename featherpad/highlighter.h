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

struct BracketInfo
{
    char character; // '[' or ']'
    int position;
};


/* This class is for detection of matching parentheses and
   braces, and also for highlighting of here-documents. */
class TextBlockData : public QTextBlockUserData
{
public:
    TextBlockData() { Highlighted = false; OpenNests = 0; }
    ~TextBlockData();
    QVector<ParenthesisInfo *> parentheses();
    QVector<BraceInfo *> braces();
    QVector<BracketInfo *> brackets();
    QString delimiter();
    bool isHighlighted();
    int openNests();
    void insertInfo (ParenthesisInfo *info);
    void insertInfo (BraceInfo *info);
    void insertInfo (BracketInfo *info);
    void insertInfo (QString str);
    void insertHighlightInfo (bool highlighted);
    void insertNestInfo (int nests);

private:
    QVector<ParenthesisInfo *> allParentheses;
    QVector<BraceInfo *> allBraces;
    QVector<BracketInfo *> allBrackets;
    QString Delimiter; // The delimiter string of a here-doc.
    bool Highlighted; // Is this block completely highlighted?
    /* "Nest" is a generalized bracket. This variable
       is the number of unclosed nests in a block. */
    int OpenNests;
};
/*************************/
/* This is a tricky but effective way for syntax highlighting. */
class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    Highlighter (QTextDocument *parent, QString lang, QTextCursor start, QTextCursor end,
                 bool darkColorScheme, bool showWhiteSpace = false);
    ~Highlighter();

    void setLimit (QTextCursor start, QTextCursor end) {
        startCursor = start;
        endCursor = end;
    }

protected:
    void highlightBlock (const QString &text);

private:
    QStringList keywords (QString& lang);
    QStringList types();
    bool isEscapedQuote (const QString &text, const int pos, bool isStartQuote);
    bool isQuoted (const QString &text, const int index);
    bool isMLCommented (const QString &text, const int index);
    void resetHereDocStates (QTextBlock block);
    bool isHereDocument (const QString &text);
    void pythonMLComment (const QString &text, const int indx);
    void htmlStyleHighlighter (const QString &text);
    void htmlBrackets (const QString &text);
    void htmlJavascript (const QString &text);
    int cssHighlighter (const QString &text);
    void singleLineComment (const QString &text, int start, int end = -1,
                            bool canBeQuoted = false);
    void multiLineComment (const QString &text,
                           int index, int cssIndx,
                           QRegExp commentStartExp, QRegExp commentEndExp,
                           int commState,
                           QTextCharFormat comFormat);
    bool textEndsWithBackSlash (const QString &text);
    void multiLineQuote (const QString &text);
    void xmlQuotes (const QString &text);
    void setFormatWithoutOverwrite (int start,
                                    int count,
                                    const QTextCharFormat &newFormat,
                                    const QTextCharFormat &oldFormat);
    /* SH specific methods: */
    void SH_MultiLineQuote(const QString &text);
    bool SH_SkipQuote (const QString &text, const int pos, bool isStartQuote);
    bool SH_CharIsEscaped (const QString &text, const int pos);
    void SH_CommentsInsideCmnd (const QString &text, int start, int end);
    bool SH_CmndSubstVar (const QString &text,
                          TextBlockData *currentBlockData,
                          int prevOpenNests);
    void debControlFormatting (const QString &text);

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
    QTextCharFormat quoteFormat; // Usually for double quote.
    QTextCharFormat altQuoteFormat; // Usually for single quote.
    QTextCharFormat urlFormat;
    QTextCharFormat blockQuoteFormat;
    QTextCharFormat codeBlockFormat;
    /* Used when there is a need to mark text or undo fomatting. */
    QTextCharFormat neutralFormat;
    QTextCharFormat whiteSpaceFormat; // For whitespaces.

    /* Programming language: */
    QString progLan;

    QRegExp quoteMark;
    QColor Blue, DarkBlue, Red, DarkRed, DarkGreen, DarkGreenAlt, DarkMagenta, Violet, Brown, DarkYellow;

    /* The start and end cursors of the visible text: */
    QTextCursor startCursor, endCursor;

    /* Block states: */
    enum
    {
        commentState = 1,

        /* Next-line commnets (ending back-slash in c/c++): */
        nextLineCommentState,

        /* Quotation marks: */
        doubleQuoteState,
        singleQuoteState,

        SH_DoubleQuoteState,
        SH_SingleQuoteState,

        /* Python comments: */
        pyDoubleQuoteState,
        pySingleQuoteState,

        xmlValueState,

        /* Markdown: */
        markdownBlockQuoteState,
        markdownCodeBlockState,

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

        endState // 24

        /* For here-docs, state >= endState or state < -1. */
    };
};

}

#endif // HIGHLIGHTER_H
