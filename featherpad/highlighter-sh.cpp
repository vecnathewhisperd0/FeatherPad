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

#include "highlighter.h"

namespace FeatherPad {

/*******************************************
  FIXME: THIS APPROACH IS VERY INEFFICIENT
 AND SHOULD BE REPLACED BY A REAL SOLUTION.
********************************************/

// multi/single-line quote highlighting for bash.
void Highlighter::SH_MultiLineQuote (const QString &text)
{
    int index = 0;
    QRegExp quoteExpression = QRegExp ("\"|\'");
    int initialState = currentBlockState();

    /* find the start quote */
    if (previousBlockState() != doubleQuoteState
        && previousBlockState() != SH_SingleQuoteState
        && previousBlockState() != SH_DoubleQuoteState
        && previousBlockState() != singleQuoteState)
    {
        index = quoteExpression.indexIn (text);
        /* skip escaped start quotes and all comments */
        while (SH_SkipQuote (text, index, true))
            index = quoteExpression.indexIn (text, index + 1);

        /* if the start quote is found... */
        if (index >= 0)
        {
            /* ... distinguish between double and single quotes */
            if (index == quoteMark.indexIn (text, index))
                quoteExpression = quoteMark;
            else
                quoteExpression = QRegExp ("\'");
        }
    }
    else // but if we're inside a quotation...
    {
        /* ... distinguish between the two quote kinds
           by checking the previous line */
        if (previousBlockState() == doubleQuoteState
            || previousBlockState() == SH_SingleQuoteState
            || previousBlockState() == SH_DoubleQuoteState)
        {
            quoteExpression = quoteMark;
        }
        else
            quoteExpression = QRegExp ("\'");
    }

    while (index >= 0)
    {
        /* if the search is continued... */
        if (quoteExpression == QRegExp ("\"|\'"))
        {
            /* ... distinguish between double and single quotes
               again because the quote mark may have changed */
            if (index == quoteMark.indexIn (text, index))
                quoteExpression = quoteMark;
            else
                quoteExpression = QRegExp ("\'");
        }

        /* search for the end quote from the start quote */
        int endIndex = quoteExpression.indexIn (text, index + 1);

        /* but if there's no start quote ... */
        if (index == 0
            && (previousBlockState() == doubleQuoteState
                || previousBlockState() == SH_SingleQuoteState
                || previousBlockState() == SH_DoubleQuoteState
                || previousBlockState() == singleQuoteState))
        {
            /* ... search for the end quote from the line start */
            endIndex = quoteExpression.indexIn (text, 0);
        }

        /* check if the quote is escaped */
        while (SH_SkipQuote (text, endIndex, false))
            endIndex = quoteExpression.indexIn (text, endIndex + 1);

        int quoteLength;
        if (endIndex == -1)
        {
            setCurrentBlockState (quoteExpression == quoteMark ? ((initialState == SH_DoubleQuoteState
                                                                   || initialState == SH_SingleQuoteState)
                                                                       ? initialState
                                                                       : doubleQuoteState)
                                                               : singleQuoteState);
            quoteLength = text.length() - index;
        }
        else
            quoteLength = endIndex - index
                          + quoteExpression.matchedLength(); // 1
        if (quoteExpression == quoteMark)
            setFormatWithoutOverwrite (index, quoteLength, quoteFormat, neutralFormat);
        else
            setFormat (index, quoteLength, altQuoteFormat);

        /* the next quote may be different */
        quoteExpression = QRegExp ("\"|\'");
        index = quoteExpression.indexIn (text, index + quoteLength);

        /* skip escaped start quotes and all comments */
        while (SH_SkipQuote (text, index, true))
            index = quoteExpression.indexIn (text, index + 1);
    }
}
/*************************/
// The bash quotes that should be skipped while multiline quotes are being highlighted.
bool Highlighter::SH_SkipQuote (const QString &text, const int pos, bool isStartQuote)
{
    return (isEscapedQuote (text, pos, isStartQuote)
            || format (pos) == neutralFormat // not needed
            || format (pos) == commentFormat
            || format (pos) == quoteFormat
            || format (pos) == altQuoteFormat);
}
/*************************/
// Should be used only with characters that can be escaped.
bool Highlighter::SH_CharIsEscaped (const QString &text, const int pos)
{
    if (pos < 1) return false;
    int i = 0;
    while (pos - i >= 1
           && pos - i - 1 == QRegExp ("\\\\").indexIn (text, pos - i - 1))
    {
        ++i;
    }
    if (i % 2 != 0)
        return true;
    return false;
}
/*************************/
// Highlight all comments inside command substitution variables appropriately.
void Highlighter::SH_CommentsInsideCmnd (const QString &text, int start, int end)
{
    QRegExp exp ("^#.*|\\s+#.*");
    int comment = exp.indexIn (text, start);
    while (comment != -1
           /* skip quoted comments */
           && (format (comment) == quoteFormat
               || format (comment) == altQuoteFormat))
    {
        comment = exp.indexIn (text, comment + 1);
    }
    while (comment != -1 && (end == -1 ? comment < text.length() : comment < end))
    {
        int commentEnd = comment;
        while ((end == -1 ? commentEnd <= text.length() : commentEnd < end)
               && (commentEnd == text.length()
                   || text.at (commentEnd) != ')' || SH_CharIsEscaped (text, commentEnd)))
        {
            ++ commentEnd;
        }
        singleLineComment (text, comment, commentEnd, true);
        comment = commentEnd + 1;
        while (comment != -1 && (format (comment) == quoteFormat
                                 || format (comment) == altQuoteFormat))
        {
            comment = exp.indexIn (text, comment + 1);
        }
    }
}
/*************************/
// This function highlights command substitution variables $(...).
// It highlights code comments only inside their blocks (unlike Kate).
bool Highlighter::SH_CmndSubstVar (const QString &text,
                                   TextBlockData *currentBlockData,
                                   int prevOpenNests)
{
    if (progLan != "sh" || !currentBlockData) return false;
    if (currentBlockState() < -1 || currentBlockState() >= endState)
        return (prevOpenNests != 0);

    QRegExp exp = QRegExp ("\\$\\(|\\)|\\(|\"|\'");
    QRegExp commetExp ("^#.*|\\s+#.*");
    QRegExp quoteExpression;
    int N = 0;
    int p = 0;
    int indx = 0; int start = 0; int end = 0;
    bool quoteFound = false;

    QTextBlock prevBlock = currentBlock().previous();
    if (prevBlock.isValid())
    {
        if (TextBlockData *prevData = static_cast<TextBlockData *>(prevBlock.userData()))
            N = prevData->openNests();
    }

    if (N > 0 && (previousBlockState() == SH_SingleQuoteState
                  || previousBlockState() == SH_DoubleQuoteState))
    { // there was an unclosed quote in the previous block
        if (previousBlockState() == SH_SingleQuoteState)
            quoteExpression = QRegExp ("\'");
        else
            quoteExpression = quoteMark;

        end = quoteExpression.indexIn (text);
        while (isEscapedQuote (text, end, false))
            end = quoteExpression.indexIn (text, end + 1);
        if (end == -1)
        {
            if (quoteExpression == quoteMark)
            {
                setFormat (0, text.length(), quoteFormat);
                setCurrentBlockState (SH_DoubleQuoteState);
            }
            else
            {
                setFormat (0, text.length(), altQuoteFormat);
                setCurrentBlockState (SH_SingleQuoteState);
            }
            if (N != 0)
                currentBlockData->insertNestInfo (N);
            return (N != prevOpenNests);
        }
        else
        {
            setFormat (0, end + 1, quoteExpression == quoteMark ? quoteFormat
                                                                : altQuoteFormat);
            start = indx = end + 1;
        }
    }
    else if (N == 0) // a new search for code blocks
    {
        start = QRegExp ("\\$\\(").indexIn (text);
        if (start == -1 || format (start) == commentFormat)
            return (prevOpenNests != 0);
        N = 1;
        indx = start + 2;
    }

    while (indx < text.length()) // up to here, always N > 0
    {
        while (N > 0 && (indx = exp.indexIn (text, indx)) != -1)
        {
            while (format (indx) == commentFormat)
                ++ indx;
            if (indx == text.length())
                break;
            if (text.at (indx) == '\''
                || text.at (indx) == '\"')
            {
                if (isEscapedQuote (text, indx, true))
                {
                    ++ indx;
                    continue;
                }
                quoteFound = true;
                break;
            }
            if (text.mid (indx, 2) == "$(")
            {
                ++ N;
                indx += 2;
            }
            else if (text.at (indx) == '(')
            {
                if (!SH_CharIsEscaped (text, indx))
                    ++ p;
                ++ indx;
            }
            else if (text.at (indx) == ')')
            {
                if (!SH_CharIsEscaped (text, indx))
                {
                    -- p;
                    if (p < 0)
                    {
                        -- N;
                        p = 0;
                    }
                }
                ++ indx;
            }
            else ++ indx;
        }
        if (N < 0) N = 0;
        if (N == 0) -- indx;

        if (!quoteFound)
        {
            setFormat (start, (indx == -1 ? text.length() : indx + 1) - start , neutralFormat);
            /* also highlight all comments appropriately */
            SH_CommentsInsideCmnd (text, start, indx);
            if (indx == -1) // N > 0
            {
                currentBlockData->insertNestInfo (N);
                return (N != prevOpenNests);
            }
            start = QRegExp ("\\$\\(").indexIn (text, indx + 1);
            if (start == -1 || format (start) == commentFormat)
                return (prevOpenNests != 0);
            indx = start + 2;
            N = 1;
            continue;
        }

        /* a quote is found */
        quoteFound = false;
        setFormat (start, indx - start, neutralFormat);
        /* see if this quote mark is commented out
           (unfortunately, a backward search should be done) */
        int lastComment = commetExp.lastIndexIn (text, indx + 1 - text.length());
        if (lastComment > 0
            && format (lastComment) != quoteFormat
            && format (lastComment) != altQuoteFormat
            && lastComment + commetExp.matchedLength() > indx)
        {
            indx = indx + 1;
            continue;
        }
        if (text.at (indx) == '\'')
            quoteExpression = QRegExp ("\'");
        else
            quoteExpression = quoteMark;
        end = quoteExpression.indexIn (text, indx + 1);
        while (isEscapedQuote (text, end, false))
            end = quoteExpression.indexIn (text, end + 1);
        if (end == -1)
        {
            if (quoteExpression == quoteMark)
            {
                setFormat (indx, text.length() - indx, quoteFormat);
                setCurrentBlockState (SH_DoubleQuoteState);
            }
            else
            {
                setFormat (indx, text.length() - indx, altQuoteFormat);
                setCurrentBlockState (SH_SingleQuoteState);
            }
            if (N != 0)
                currentBlockData->insertNestInfo (N);
            return (N != prevOpenNests);
        }
        else
        {
            setFormat (indx, end + 1 - indx, quoteExpression == quoteMark ? quoteFormat
                                                                          : altQuoteFormat);
            start = indx = end + 1;
        }
    }

    if (N != 0)
        currentBlockData->insertNestInfo (N);
    if (N != prevOpenNests)
    { // forced highlighting of the next block is necessary
        return true;
    }
    else return false;
}

}
