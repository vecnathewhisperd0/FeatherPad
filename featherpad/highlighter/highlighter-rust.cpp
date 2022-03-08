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

static inline bool isRustRawLiteral (const QString &text, int index)
{
    bool res = false;
    if (index > 1
        && text.at (index - 1) == '#' && text.at (index - 2) == 'r')
    {
        if (index == 2)
            res = true;
        else
        {
            QChar c = text.at (index - 3);
            if (c == '\'')
            {
                // -> highlighter.cpp -> rust single quotes
                static const QRegularExpression rustSingleQuote ("'([^'\\\\]{0,1}|\\\\(r|t|n|'|\\\"|\\\\|x[0-9a-fA-F]{2}))\\K'");
                if (text.lastIndexOf (rustSingleQuote, index - 3) == index - 3)
                    res = true;
            }
            else if (c.isNumber() || !c.isLetterOrNumber())
                res = true;
        }
    }
    return res;
}

void Highlighter::multiLineRustQuote (const QString &text)
{
    /* Rust's raw string literals */
    bool rustLiteral = false;
    TextBlockData *bData = nullptr;
    bData = static_cast<TextBlockData *>(currentBlock().userData());
    QTextBlock prevBlock = currentBlock().previous();
    if (prevBlock.isValid())
    {
        if (TextBlockData *prevData = static_cast<TextBlockData *>(prevBlock.userData()))
            rustLiteral = prevData->getProperty();
    }

    int index = 0;
    QRegularExpressionMatch quoteMatch;

    /* find the start quote */
    int prevState = previousBlockState();
    if (prevState != doubleQuoteState || index > 0)
    {
        index = text.indexOf (quoteMark, index);
        /* skip escaped start quotes and all comments */
        while (isEscapedQuote (text, index, true)
               || isMLCommented (text, index, commentState))
        {
            index = text.indexOf (quoteMark, index + 1);
        }
        while (format (index) == commentFormat || format (index) == urlFormat) // single-line
            index = text.indexOf (quoteMark, index + 1);
        rustLiteral = isRustRawLiteral (text, index);
    }

    while (index >= 0)
    {
        int endIndex;
        /* if there's no start quote ... */
        if (index == 0 && prevState == doubleQuoteState)
        {
            /* ... search for the end quote from the line start */
            endIndex = text.indexOf (quoteMark, 0, &quoteMatch);
        }
        else // otherwise, search from the start quote
            endIndex = text.indexOf (quoteMark, index + 1, &quoteMatch);

        if (rustLiteral)
        { // check if the quote is inside a rust raw string literal
            while (endIndex > -1
                   && (endIndex == text.length() - 1 || text.at (endIndex + 1) != '#'))
            {
                endIndex = text.indexOf (quoteMark, endIndex + 1, &quoteMatch);
            }
        }
        else
        { // check if the quote is escaped
            while (isEscapedQuote (text, endIndex, false))
                endIndex = text.indexOf (quoteMark, endIndex + 1, &quoteMatch);
        }

        int quoteLength;
        if (endIndex == -1)
        {
            setCurrentBlockState (doubleQuoteState);
            quoteLength = text.length() - index;

            /* set the delimiter string for C++11 */
            if (bData && rustLiteral)
            {
                bData->setProperty (true);
                setCurrentBlockUserData (bData);
                /* NOTE: The next block will be rehighlighted at highlightBlock()
                         if the property is changed. */
            }
        }
        else
        {
            quoteLength = endIndex - index
                          + quoteMatch.capturedLength();
            if (rustLiteral)
                quoteLength += 1;
        }
        if (rustLiteral)
        {
            if (index > 1)
                setFormat (index - 2, quoteLength + 2, quoteFormat);
            else
                setFormat (index, quoteLength, quoteFormat);
        }
        else
            setFormat (index, quoteLength, quoteMark == quoteMark ? quoteFormat
                                                                        : altQuoteFormat);

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
        rustLiteral = isRustRawLiteral (text, index);
    }
}

}
