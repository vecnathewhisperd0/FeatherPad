/*
 * Copyright (C) Pedram Pourang (aka Tsu Jan) 2018 <tsujan2000@gmail.com>
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

// This is only for the starting "/".
bool Highlighter::isEscapedJSRegex (const QString &text, const int pos)
{
    if (pos < 0) return false;
    if (progLan != "javascript" && progLan != "qml")
        return false;

    if (format (pos) == quoteFormat || format (pos) == altQuoteFormat
        || format (pos) == commentFormat || format (pos) == urlFormat)
    {
        return true;
    }

    if (isMLCommented (text, pos, commentState)
        || isMLCommented (text, pos, htmlJavaCommentState))
    {
        return true;
    }

    /* escape "<.../>", "</...>" and the single-line comment sign ("//") */
    if ((text.length() > pos + 1 && ((progLan == "javascript" && text.at (pos + 1) == '>')
                                     || text.at (pos + 1) == '/'))
            || (pos > 0 && progLan == "javascript" && text.at (pos - 1) == '<'))
    {
        return true;
    }

    QRegularExpressionMatch keyMatch;
    static QRegularExpression keys;

    int i = pos - 1;
    while (i >= 0 && (text.at (i) == ' ' || text.at (i) == '\t'))
        --i;
    if (i == -1)
    { // examine the previous line(s)
        QTextBlock prev = currentBlock().previous();
        if (!prev.isValid()) return false;
        QString txt = prev.text();
        QRegularExpression nonSpace ("[^\\s]+");
        while (txt.indexOf (nonSpace, 0) == -1)
        {
            prev.setUserState (updateState); // update the next line if this one changes
            prev = prev.previous();
            if (!prev.isValid()) return false;
            txt = prev.text();
        }
        int last = txt.length() - 1;
        QChar ch = txt.at (last);
        while (ch == ' ' || ch == '\t')
        {
            --last;
            ch = txt.at (last);
        }
        if (prev.userState() == JSRegexEndState)
            return false; // a regex isn't escaped if it follows another one
        else
        {
            prev.setUserState (updateState); // update the next line if this one changes
            if (ch.isLetterOrNumber() || ch == '_'
                || ch == ')' || ch == ']') // as with Kate
            { // a regex isn't escaped if it follows a JavaScript keyword
                if (keys.pattern().isEmpty())
                    keys.setPattern (keywords (progLan).join ('|'));
                int len = qMin (12, last + 1);
                QString str = txt.mid (last - len + 1, len);
                int j;
                if ((j = str.lastIndexOf (keys, -1, &keyMatch)) > -1 && j + keyMatch.capturedLength() == len)
                    return false;
                return true;
            }
        }
    }
    else
    {
        QChar ch = text.at (i);
        if (format (i) != JSRegexFormat && (ch.isLetterOrNumber() || ch == '_'
                                            || ch == ')' || ch == ']')) // as with Kate
        { // a regex isn't escaped if it follows another one or a JavaScript keyword
            int j;
            if ((j = text.lastIndexOf (QRegularExpression("/\\w+"), i + 1, &keyMatch)) > -1 && j + keyMatch.capturedLength() == i + 1)
                return false;
            if (keys.pattern().isEmpty())
                keys.setPattern (keywords (progLan).join ('|'));
            int len = qMin (12, i + 1);
            QString str = text.mid (i - len + 1, len);
            if ((j = str.lastIndexOf (keys, -1, &keyMatch)) > -1 && j + keyMatch.capturedLength() == len)
                return false;
            return true;
        }
    }

    return false;
}
/*************************/
// For faster processing with very long lines, this function also highlights regex patterns.
// (It should be used with care because it gives correct results only in special places.)
bool Highlighter::isInsideJSRegex (const QString &text, const int index)
{
    if (index < 0) return false;
    if (progLan != "javascript" && progLan != "qml")
        return false;

    int pos = -1;

    if (format (index) == JSRegexFormat)
        return true;
    if (TextBlockData *data = static_cast<TextBlockData *>(currentBlock().userData()))
    {
        pos = data->lastFormattedRegex() - 1;
        if (index <= pos) return false;
    }

    QRegularExpressionMatch match;
    QRegularExpression exp;
    bool res = false;
    int N;

    if (pos == -1)
    {
        if (previousBlockState() != JSRegexState)
        {
            exp.setPattern ("/");
            N = 0;
        }
        else
        {
            exp.setPattern ("/[A-Za-z0-9_]*");
            N = 1;
            res = true;
        }
    }
    else // a new search from the last position
    {
        exp.setPattern ("/");
        N = 0;
    }

    int nxtPos;
    while ((nxtPos = text.indexOf (exp, pos + 1, &match)) >= 0)
    {
        /* skip formatted comments and quotes */
        QTextCharFormat fi = format (nxtPos);
        if (N % 2 == 0
            && (fi == commentFormat || fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat))
        {
            pos = nxtPos;
            continue;
        }

        ++N;
        if ((N % 2 == 0 && isEscapedChar (text, nxtPos)) // an escaped end sign
            || (N % 2 != 0 && isEscapedJSRegex (text, nxtPos))) // or an escaped start sign
        {
            --N;
            pos = nxtPos;
            continue;
        }

        if (N % 2 == 0)
        {
            if (TextBlockData *data = static_cast<TextBlockData *>(currentBlock().userData()))
                data->insertLastFormattedRegex (nxtPos + match.capturedLength());
            pos = qMax (pos, 0);
            setFormat (pos, nxtPos - pos + match.capturedLength(), JSRegexFormat);
        }

        if (index <= nxtPos) // they may be equal, as when "//" is at the end of "/...//"
        {
            if (N % 2 == 0) res = true;
            else res = false;
            break;
        }

        if (N % 2 == 0)
            exp.setPattern("/");
        else
            exp.setPattern("/[A-Za-z0-9_]*");

        /* "pos" might be negative next time */
        if (N % 2 == 0) res = false;
        else res = true;

        pos = nxtPos;
    }

    return res;
}
/*************************/
void Highlighter::multiLineJSRegex (const QString &text, const int index)
{
    if (index < 0) return;
    if (progLan != "javascript" && progLan != "qml")
        return;

    int startIndex = index;
    QRegularExpressionMatch startMatch;
    QRegularExpression startExp ("/");
    QRegularExpressionMatch endMatch;
    QRegularExpression endExp ("/[A-Za-z0-9_]*");
    QTextCharFormat fi;

    int prevState = previousBlockState();
    if (prevState != JSRegexState || startIndex > 0)
    {
        startIndex = text.indexOf (startExp, startIndex, &startMatch);
        /* skip comments and quotations (all formatted to this point) */
        fi = format (startIndex);
        while (startIndex >= 0
               && (isEscapedJSRegex (text, startIndex)
                   || fi == commentFormat
                   || fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat))
        {
            startIndex = text.indexOf (startExp, startIndex + 1, &startMatch);
            fi = format (startIndex);
        }
    }

    while (startIndex >= 0)
    {
        int endIndex;
        /* when the start sign is in the prvious line
           and the search for the end sign has just begun,
           search for the end sign from the line start */
        if (prevState == JSRegexState && startIndex == 0)
            endIndex = text.indexOf (endExp, 0, &endMatch);
        else
            endIndex = text.indexOf (endExp, startIndex + startMatch.capturedLength(), &endMatch);

        while (isEscapedChar (text, endIndex))
            endIndex = text.indexOf (endExp, endIndex + 1, &endMatch);

        int len;
        if (endIndex == -1)
        {
            setCurrentBlockState (JSRegexState);
            len = text.length() - startIndex;
        }
        else
        {
            len = endIndex - startIndex
                  + endMatch.capturedLength();
        }
        setFormat (startIndex, len, JSRegexFormat);

        startIndex = text.indexOf (startExp, startIndex + len, &startMatch);

        /* skip comments and quotations again */
        fi = format (startIndex);
        while (startIndex >= 0
               && (isEscapedJSRegex (text, startIndex)
                   || fi == commentFormat
                   || fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat))
        {
            startIndex = text.indexOf (startExp, startIndex + 1, &startMatch);
            fi = format (startIndex);
        }
    }

    /* If this line ends with a regex plus spaces, give it a special
       state to decide about the probable regex of its following line
       and also for that line to be updated when this state is toggled. */
    if (currentBlockState() == 0 && !text.isEmpty())
    {
        int last = text.length() - 1;
        QChar ch = text.at (last);
        while (ch == ' ' || ch == '\t')
        {
            --last;
            if (last < 0) return;
            ch = text.at (last);
        }
        if (format (last) == JSRegexFormat)
            setCurrentBlockState (JSRegexEndState);
    }
}

}
