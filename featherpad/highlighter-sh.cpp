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

// multi/single-line quote highlighting for bash.
bool Highlighter::SH_MultiLineQuote (const QString &text)
{
    int index = 0;
    QRegExp quoteExpression = QRegExp ("\"|\'");
    int quote = doubleQuoteState;
    int initialState = currentBlockState();

    /* find the start quote */
    if (previousBlockState() != doubleQuoteState
        && previousBlockState() != singleQuoteState)
    {
        index = quoteExpression.indexIn (text);
        /* skip escaped start quotes and all comments */
        while (isEscapedQuote (text, index, true))
            index = quoteExpression.indexIn (text, index + 1);
        while (format (index) == commentFormat)
            index = quoteExpression.indexIn (text, index + 1);

        /* if the start quote is found... */
        if (index >= 0)
        {
            /* ... distinguish between double and single quotes */
            if (index == quoteMark.indexIn (text, index))
            {
                quoteExpression = quoteMark;
                quote = doubleQuoteState;
            }
            else
            {
                quoteExpression = QRegExp ("\'");
                quote = singleQuoteState;
            }
        }
    }
    else // but if we're inside a quotation...
    {
        /* ... distinguish between the two quote kinds
           by checking the previous line */
        quote = previousBlockState();
        if (quote == doubleQuoteState)
            quoteExpression = quoteMark;
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
            {
                quoteExpression = quoteMark;
                quote = doubleQuoteState;
            }
            else
            {
                quoteExpression = QRegExp ("\'");
                quote = singleQuoteState;
            }
        }

        /* search for the end quote from the start quote */
        int endIndex = quoteExpression.indexIn (text, index + 1);

        /* but if there's no start quote ... */
        if (index == 0
            && (previousBlockState() == doubleQuoteState || previousBlockState() == singleQuoteState))
        {
            /* ... search for the end quote from the line start */
            endIndex = quoteExpression.indexIn (text, 0);
        }

        /* check if the quote is escaped */
        while (isEscapedQuote (text, endIndex, false))
            endIndex = quoteExpression.indexIn (text, endIndex + 1);

        int quoteLength;
        if (endIndex == -1)
        {
            setCurrentBlockState (quote);
            quoteLength = text.length() - index;
        }
        else
            quoteLength = endIndex - index
                          + quoteExpression.matchedLength(); // 1
        if (quoteExpression == quoteMark)
        {
            setFormatWithoutOverwrite (index, quoteLength, quoteFormat, neutralFormat);
            /* also highlight single quotes inside valid commands
               (no double quote highlighting yet) */
            SH_SingleQuoteInsideCommand (text, index, endIndex);
        }
        else
            setFormat (index, quoteLength, altQuoteFormat);

        /* the next quote may be different */
        quoteExpression = QRegExp ("\"|\'");
        index = quoteExpression.indexIn (text, index + quoteLength);

        /* skip escaped start quotes and all comments */
        while (isEscapedQuote (text, index, true))
            index = quoteExpression.indexIn (text, index + 1);
        while (format (index) == commentFormat)
            index = quoteExpression.indexIn (text, index + 1);
    }

    TextBlockData *curData = static_cast<TextBlockData *>(currentBlockUserData());
    if (curData && curData->openNests() != 0
        && initialState != currentBlockState())
    {
        return true;
    }
    return false;
}
/*************************/
// Bash single quote highlgihting inside command substitution variables $(...).
void Highlighter::SH_SingleQuoteInsideCommand (const QString &text, int start, int end)
{
    int index = start;
    QRegExp quoteExpression = QRegExp ("\'");
    int N = 0;
    QTextBlock prevBlock = currentBlock().previous();
    if (prevBlock.isValid())
    {
        if (TextBlockData *prevData = static_cast<TextBlockData *>(prevBlock.userData()))
            N = prevData->openNests();
    }

    if (N < 0)
    { // the unclosed command of the previous block had an unclosed single quote
        index = quoteExpression.indexIn (text, start);
        while (isEscapedQuote (text, index, true))
            index = quoteExpression.indexIn (text, index + 1);
        setFormat (start,
                   (index == -1 ? (end == -1 ? text.length() : end)
                                : (end == -1 ? index + 1
                                             : qMin (end, index) + 1)) - start,
                   altQuoteFormat);
        if (index == -1 || (end != -1 && index >= end))
            return;
        ++ index;
    }

    index = quoteExpression.indexIn (text, index);
    while (isEscapedQuote (text, index, true)
           /* skip it if it's outside codes */
           || (index != -1 && format (index) != neutralFormat))
    {
        index = quoteExpression.indexIn (text, index + 1);
    }
    while (index != -1 && (end == -1 || index < end))
    {
        int endIndex = quoteExpression.indexIn (text, index + 1);
        while (isEscapedQuote (text, endIndex, true)
               || (endIndex != -1 && format (endIndex) != neutralFormat))
        {
            endIndex = quoteExpression.indexIn (text, endIndex + 1);
        }
        setFormat (index,
                   (endIndex == -1 ? (end == -1 ? text.length() : end)
                                   : (end == -1 ? endIndex + 1
                                                : qMin (end, endIndex) + 1)) - index,
                   altQuoteFormat);
        if (endIndex == -1 || (end != -1 && endIndex >= end))
            return;
        index = quoteExpression.indexIn (text, endIndex + 1);
        while (isEscapedQuote (text, index, true)
               || (index != -1 && format (index) != neutralFormat))
        {
            index = quoteExpression.indexIn (text, index + 1);
        }
    }
}
/*************************/
// This function gives a preliminary, neutral format
// to command substitution variables $(...) in order to
// escape double quotes and highlight single quotes inside them.
bool Highlighter::SH_quotedCommands (const QString &text,
                                     TextBlockData *currentBlockData,
                                     int prevOpenNests)
{
    if (progLan != "sh" || !currentBlockData) return false;

    QTextCharFormat functionFormat;
    functionFormat.setFontItalic (true);
    functionFormat.setForeground (Blue);
    QRegExp start = QRegExp ("\\$\\(");
    int N = 0;
    int singleQuotes = 0;
    int p = 0;
    int indx = 0;
    QTextBlock prevBlock = currentBlock().previous();
    if (prevBlock.isValid())
    {
        TextBlockData *prevData = static_cast<TextBlockData *>(prevBlock.userData());
        if (prevData && (N = qAbs (prevData->openNests())) > 0)
        {
            if (prevData->openNests() < 0) // a previously unclosed single quote
                singleQuotes = 1;

            while (N > 0 && indx < text.length())
            {
                if (text.at (indx) == '('
                    && (indx == 0 || text.at (indx - 1) != '\\')) // not escaped
                {
                    ++ p;
                    ++ indx;
                }
                else if (text.at (indx) == ')'
                         && (indx == 0 || text.at (indx - 1) != '\\'))
                {
                    -- p;
                    if (p < 0)
                    {
                        -- N;
                        p = 0;
                    }
                    ++ indx;
                }
                else if (text.mid (indx, 2) == "$(")
                {
                    ++ N;
                    indx += 2;
                }
                else
                {
                    if (text.at (indx) == '\''
                        && (indx == 0 || text.at (indx - 1) != '\\'))
                    {
                        ++ singleQuotes;
                    }
                    ++ indx;
                }
            }
            if (N < 0) N = 0;

            if (indx > 0)
            {
                setFormat (0, indx, neutralFormat);
                /* also highlight all comments appropriately */
                int comment = QRegExp ("^#.*|\\s+#.*").indexIn (text, 0);
                while (comment > -1 && comment < indx)
                {
                    int commentEnd = comment;
                    while (commentEnd < indx && text.at (commentEnd) != ')')
                        ++ commentEnd;
                    singleLineComment (text, comment, commentEnd, true);
                    comment = commentEnd + 1;
                }
            }
        }
    }
    indx = start.indexIn (text, indx);
    while (indx >= 0)
    {
        ++ N;

        int endIndx = indx + 2;
        p = 0;
        while (N > 0 && endIndx < text.length())
        {
            if (text.at (endIndx) == '('
                && (endIndx == 0 || text.at (endIndx - 1) != '\\'))
            {
                ++ p;
                ++ endIndx;
            }
            else if (text.at (endIndx) == ')'
                     && (endIndx == 0 || text.at (endIndx - 1) != '\\'))
            {
                -- p;
                if (p < 0)
                {
                    -- N;
                    p = 0;
                }
                ++ endIndx;
            }
            else if (text.mid (endIndx, 2) == "$(")
            {
                ++ N;
                endIndx += 2;
            }
            else
            {
                if (text.at (endIndx) == '\''
                    && (endIndx == 0 || text.at (endIndx - 1) != '\\'))
                {
                    ++ singleQuotes;
                }
                ++ endIndx;
            };
        }
        if (N < 0) N = 0;

        int length = endIndx - indx;
        setFormat (indx, length, neutralFormat);
        /* highlight all comments */
        int comment = QRegExp ("^#.*|\\s+#.*").indexIn (text, indx);
        while (comment > -1 && comment < endIndx)
        {
            int commentEnd = comment;
            while (commentEnd < endIndx && text.at (commentEnd) != ')')
                ++ commentEnd;
            singleLineComment (text, comment, commentEnd, true);
            comment = commentEnd + 1;
        }

        indx = start.indexIn (text, endIndx);
    }

    /* signal the existence of non-closed single quotes with a negative
       number and consider all quotes closed when commands are closed */
    if (N != 0)
        currentBlockData->insertNestInfo (singleQuotes % 2 == 0 ? N : -N);

    if (N != prevOpenNests)
    {
        /* since SH_MultiLineQuote() will force highlighting of the
           next block ony if the current block has open quotes, next
           block highlighting should also be forced here */
        return true;
    }
    else return false;
}

}
