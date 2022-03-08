/*
 * Copyright (C) Pedram Pourang (aka Tsu Jan) 2022 <tsujan2000@gmail.com>
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
 *
 * @license GPL-3.0+ <https://spdx.org/licenses/GPL-3.0+.html>
 */

#include "highlighter.h"

namespace FeatherPad {

static inline int rustRawLiteral (const QString &text, int index)
{
    /* The start should be b?r#+\" and the number of occurrences
       of "#" determines what the end should be. */
    int N = 0;
    while (index > 0 && text.at (index - 1) == '#')
    {
        ++ N;
        -- index;
    }
    if (N > 0 && index > 0 && text.at (index - 1) == 'r')
    {
        -- index;
        if (index == 0) return N;
        -- index;
        QChar c = text.at (index);
        if (c == 'b')
        {
            if (index == 0) return N;
            -- index;
            c = text.at (index);
        }
        if (c == '\'')
        {
            // -> highlighter.cpp -> rust single quotes
            static const QRegularExpression rustSingleQuote ("'([^'\\\\]{0,1}|\\\\(r|t|n|'|\\\"|\\\\|x[0-9a-fA-F]{2}))\\K'");
            if (text.lastIndexOf (rustSingleQuote, index) == index)
                return N;
        }
        else if (c.isNumber() || !c.isLetterOrNumber())
            return N;
    }
    return 0;
}

static inline bool endsRawLiteral (const QString &text, int endIndex, int N)
{
    if (endIndex > text.length() - 1 - N)
        return false;
    for (int i = 1; i <= N; ++i)
    {
        if (text.at (endIndex + i) != '#')
            return false;
    }
    return true;
}

void Highlighter::multiLineRustQuote (const QString &text)
{
    /* Rust's raw string literals */
    int N = 0;
    TextBlockData *bData = nullptr;
    bData = static_cast<TextBlockData *>(currentBlock().userData());
    QTextBlock prevBlock = currentBlock().previous();
    if (prevBlock.isValid())
    {
        if (TextBlockData *prevData = static_cast<TextBlockData *>(prevBlock.userData()))
            N = prevData->openNests();
    }

    int index = 0;

    /* find the start quote */
    int prevState = previousBlockState();
    if (prevState != doubleQuoteState)
    {
        index = text.indexOf (quoteMark, index);
        /* skip escaped start quotes and all comments */
        while (isEscapedQuote (text, index, true)
               || isMLCommented (text, index, commentState))
        {
            index = text.indexOf (quoteMark, index + 1);
        }
        if (format (index) == commentFormat || format (index) == urlFormat) // single-line comment
            return;
        N = rustRawLiteral (text, index);
    }

    while (index >= 0)
    {
        int endIndex;
        /* if there's no start quote ... */
        if (index == 0 && prevState == doubleQuoteState)
        {
            /* ... search for the end quote from the line start */
            endIndex = text.indexOf (quoteMark, 0);
        }
        else // otherwise, search from the start quote
            endIndex = text.indexOf (quoteMark, index + 1);

        if (N > 0)
        { // check if the quote is inside a rust raw string literal
            while (endIndex > -1 && !endsRawLiteral (text, endIndex, N))
                endIndex = text.indexOf (quoteMark, endIndex + 1);
        }
        else
        { // check if the quote is escaped
            while (isEscapedQuote (text, endIndex, false))
                endIndex = text.indexOf (quoteMark, endIndex + 1);
        }

        int quoteLength;
        if (endIndex == -1)
        {
            setCurrentBlockState (doubleQuoteState);
            quoteLength = text.length() - index;
            if (bData && N > 0)
            {
                bData->insertNestInfo (N);
                setCurrentBlockUserData (bData);
                /* NOTE: The next block will be rehighlighted at highlightBlock()
                         if "openNests()" is changed. */
            }
        }
        else
            quoteLength = endIndex - index + 1 + N; // include #+
        if (N > 0)
        {
            if (index > N)
            { // start quote was found
                if (index > N + 1 && text.at (index - N - 2) == 'b') // include br#+
                    setFormat (index - N - 2, quoteLength + N + 2, quoteFormat);
                else // include r#+
                    setFormat (index - N - 1, quoteLength + N + 1, quoteFormat);
            }
            else
                setFormat (index, quoteLength, quoteFormat);
        }
        else
            setFormat (index, quoteLength, quoteFormat);

#if (QT_VERSION < QT_VERSION_CHECK(6,0,0))
        QString str = text.mid (index, quoteLength);
#else
        QString str = text.sliced (index, quoteLength);
#endif
        int urlIndex = 0;
        QRegularExpressionMatch urlMatch;
        while ((urlIndex = str.indexOf (urlPattern, urlIndex, &urlMatch)) > -1)
        {
                setFormat (urlIndex + index, urlMatch.capturedLength(), urlInsideQuoteFormat);
                urlIndex += urlMatch.capturedLength();
        }

        index = text.indexOf (quoteMark, index + quoteLength);

        while (isEscapedQuote (text, index, true)
               || isMLCommented (text, index, commentState, endIndex + 1))
        {
            index = text.indexOf (quoteMark, index + 1);
        }
        while (format (index) == commentFormat || format (index) == urlFormat)
            index = text.indexOf (quoteMark, index + 1);
        N = rustRawLiteral (text, index);
    }
}

}
