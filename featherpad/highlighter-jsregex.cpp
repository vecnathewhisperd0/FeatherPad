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
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 */

#include "highlighter.h"

namespace FeatherPad {

// This is only for the starting "/".
bool Highlighter::isEscapedJSRegex (const QString &text, const int pos)
{
    if (pos < 0) return false;
    if (progLan != "javascript") return false;

    /* escape "<.../>", "</...>" and the single-line comment sign ("//") */
    if ((text.length() > pos + 1 && (text.at (pos + 1) == '>'
                                     || text.at (pos + 1) == '/'))
        || (pos > 0 && (text.at (pos - 1) == '<'
                        || (text.at (pos - 1) == '/'
                            /* not the end of (a previously formatted) JS regex */
                            && format (pos - 1) != JSRegexFormat))))
    {
        return true;
    }

    static QRegExp keys;

    int i = pos - 1;
    while (i >= 0 && (text.at (i) == ' ' || text.at (i) == '\t'))
        --i;
    if (i == -1)
    { // examine the previous line(s)
        QTextBlock prev = currentBlock().previous();
        if (!prev.isValid()) return false;
        QString txt = prev.text();
        QRegExp nonSpace ("[^\\s]+");
        while (nonSpace.indexIn (txt, 0) == -1)
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
                if (keys.isEmpty())
                    keys = QRegExp (keywords (progLan).join ('|'));
                int len = qMin (12, last + 1);
                QString str = txt.mid (last - len + 1, len);
                int j;
                if ((j = keys.lastIndexIn (str)) > -1 && j + keys.matchedLength() == len)
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
        { // a regex isn't escaped if it follows a JavaScript keyword
            if (keys.isEmpty())
                keys = QRegExp (keywords (progLan).join ('|'));
            int len = qMin (12, i + 1);
            QString str = text.mid (i - len + 1, len);
            int j;
            if ((j = keys.lastIndexIn (str)) > -1 && j + keys.matchedLength() == len)
                return false;
            return true;
        }
    }

    return false;
}
/*************************/
// This should be used with care because it gives correct results only in special places.
bool Highlighter::isInsideJSRegex (const QString &text, const int index)
{
    if (index < 0) return false;
    if (progLan != "javascript") return false;

    QRegExp exp ("/");
    bool res = false;
    int pos = -1;
    int N;

    if (previousBlockState() != JSRegexState)
        N = 0;
    else
    {
        N = 1;
        res = true;
    }

    while ((pos = exp.indexIn (text, pos + 1)) >= 0)
    {
        /* skip formatted comments and quotes */
        if (format (pos) == commentFormat || format (pos) == quoteFormat || format (pos) == altQuoteFormat)
            continue;

        ++N;
        if ((N % 2 == 0 && isEscapedChar (text, pos)) // an escaped end sign
            || (N % 2 != 0 && isEscapedJSRegex (text, pos))) // or an escaped start sign
        {
            --N;
            continue;
        }

        if (index <= pos) // they may be equal, as when "//" is at the end of "/...//"
        {
            if (N % 2 == 0) res = true;
            else res = false;
            break;
        }

        /* "pos" might be negative next time */
        if (N % 2 == 0) res = false;
        else res = true;
    }

    return res;
}
/*************************/
void Highlighter::multiLineJSRegex (const QString &text, const int index)
{
    if (index < 0) return;
    if (progLan != "javascript") return;

    int startIndex = index;
    QRegExp startExp ("/");
    QRegExp endExp ("/[A-Za-z0-9_]*");

    if (previousBlockState() != JSRegexState || startIndex > 0)
    {
        startIndex = startExp.indexIn (text, startIndex);
        /* skip comments and quotations (all formatted to this point) */
        while (isEscapedJSRegex (text, startIndex)
               || format (startIndex) == commentFormat
               || format (startIndex) == quoteFormat
               || format (startIndex) == altQuoteFormat)
        {
            startIndex = startExp.indexIn (text, startIndex + 1);
        }
    }

    while (startIndex >= 0)
    {
        int endIndex;
        /* when the start sign is in the prvious line
           and the search for the end sign has just begun,
           search for the end sign from the line start */
        if (previousBlockState() == JSRegexState && startIndex == 0)
            endIndex = endExp.indexIn (text, 0);
        else
            endIndex = endExp.indexIn (text, startIndex + startExp.matchedLength());

        while (isEscapedChar (text, endIndex))
            endIndex = endExp.indexIn (text, endIndex + 1);

        int len;
        if (endIndex == -1)
        {
            setCurrentBlockState (JSRegexState);
            len = text.length() - startIndex;
        }
        else
        {
            len = endIndex - startIndex
                  + endExp.matchedLength();
        }
        setFormat (startIndex, len, JSRegexFormat);

        startIndex = startExp.indexIn (text, startIndex + len);

        /* skip comments and quotations again */
        while (isEscapedJSRegex (text, startIndex)
               || format (startIndex) == commentFormat
               || format (startIndex) == quoteFormat
               || format (startIndex) == altQuoteFormat)
        {
            startIndex = startExp.indexIn (text, startIndex + 1);
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
