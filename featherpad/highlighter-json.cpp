/*
 * Copyright (C) Pedram Pourang (aka Tsu Jan) 2021 <tsujan2000@gmail.com>
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

void Highlighter::jsonKey (const QString &text, const int start,
                           int &K, int &V, int &B,
                           bool &insideValue, QString &braces)
{
    static const QRegularExpression jsonKeyExp ("{|\\}|:|\"");
    int index = text.indexOf (jsonKeyExp, start);
    if (index >= 0)
    {
        if (text.at (index) == '{')
        {
            ++K;
            braces += "{";
            insideValue = false;
            jsonKey (text, index + 1,
                     K, V, B,
                     insideValue, braces);
        }
        else if (text.at (index) == '}')
        {
            braces.chop (1);
            if (K > 0)
            {
                --K;
                if (K == 0) V = 0;
                insideValue = V > 0;
            }
            if (K == 0)
            { // search for the starting brace
                int indx = text.indexOf (index, '{');
                ++K;
                braces += "{";
                if (indx > -1)
                {
                    jsonKey (text, indx + 1,
                             K, V, B,
                             insideValue, braces);
                }
                else
                { // accept a value if no start brace is found
                    ++V;
                    insideValue = true;
                    jsonValue (text, index + 1,
                               K, V, B,
                               insideValue, braces);
                }
            }
            else if (insideValue)
            {
                jsonValue (text, index + 1,
                           K, V, B,
                           insideValue, braces);
            }
            else
            {
                jsonKey (text, index + 1,
                         K, V, B,
                         insideValue, braces);
            }
        }
        else if (text.at (index) == ':')
        {
            ++V;
            insideValue = true;
            jsonValue (text, index + 1,
                       K, V, B,
                       insideValue, braces);
        }
        else // double quote
        {
            int end = text.indexOf (quoteMark, index + 1);
            while (isEscapedChar (text, end))
                end = text.indexOf (quoteMark, end + 1);
            if (end >= 0)
            {
                setFormat (index, end + 1 - index, quoteFormat);
                jsonKey (text, end + 1,
                         K, V, B,
                         insideValue, braces);
            }
            else
            {
                setFormat (index, text.length() - index, quoteFormat);
                setCurrentBlockState (doubleQuoteState);
            }
        }
    }
}
/*************************/
void Highlighter::jsonValue (const QString &text, const int start,
                             int &K, int &V, int &B,
                             bool &insideValue, QString &braces)
{
    static const QRegularExpression jasonNumExp ("(?<=^|[^\\w\\d\\.])(\\+|-)?((\\d*\\.?\\d+|\\d+\\.)((e|E)(\\+|-)?\\d+)?)(?=[^\\w\\d\\.]|$)");
    static const QRegularExpression jsonValueExp ("{|\\}|,|\"|\\[|\\]|\\b(true|false|null)\\b|" + jasonNumExp.pattern());
    QRegularExpressionMatch match;
    int index = text.indexOf (jsonValueExp, start, &match);
    if (index >= 0)
    {
        if (text.at (index) == '{')
        {
            ++K;
            braces += "{";
            insideValue = false;
            jsonKey (text, index + 1,
                     K, V, B,
                     insideValue, braces);
        }
        else if (text.at (index) == '}')
        {
            braces.chop (1);
            if (K > 0)
            {
                --K;
                if (K == 0) V = 0;
                insideValue = V > 0;
            }
            if (K == 0)
            { // search for the starting brace
                int indx = text.indexOf (index, '{');
                ++K;
                braces += "{";
                if (indx > -1)
                {
                    jsonKey (text, indx + 1,
                             K, V, B,
                             insideValue, braces);
                }
                else
                { // accept a value if no start brace is found
                    ++V;
                    insideValue = true;
                    jsonValue (text, index + 1,
                               K, V, B,
                               insideValue, braces);
                }
            }
            else if (insideValue)
            {
                jsonValue (text, index + 1,
                           K, V, B,
                           insideValue, braces);
            }
            else
            {
                jsonKey (text, index + 1,
                         K, V, B,
                         insideValue, braces);
            }
        }
        else if (text.at (index) == '[')
        {
            braces += "[";
            ++B;
            jsonValue (text, index + 1,
                       K, V, B,
                       insideValue, braces);
        }
        else if (text.at (index) == ']')
        {
            braces.chop (1);
            if (B > 0) --B;
            jsonValue (text, index + 1,
                       K, V, B,
                       insideValue, braces);
        }
        else if (text.at (index) == ',')
        {
            if (V > 0) // inside a value and...
            {
                if (B == 0
                    || (!braces.isEmpty() && braces.at (braces.size() - 1) == '{'))
                { // ... either outside all brackets or immediately inside a pair of braces
                    --V;
                    insideValue = false;
                    jsonKey (text, index + 1,
                             K, V, B,
                             insideValue, braces);
                }
                else
                {
                    jsonValue (text, index + 1,
                               K, V, B,
                               insideValue, braces);
                }
            }
            else
            {
                jsonKey (text, index + 1,
                         K, V, B,
                         insideValue, braces);
            }
        }
        else if ((text.at (index) == '.' || text.at (index) == '-' || text.at (index) == '+' || text.at (index).isNumber())
                 && text.indexOf (jasonNumExp, index) == index)
        {
            QTextCharFormat numFormat;
            numFormat.setForeground (Brown);
            numFormat.setFontItalic (true);
            setFormat (index, match.capturedLength(), numFormat);
            jsonValue (text, index + match.capturedLength(),
                       K, V, B,
                       insideValue, braces);
        }
        else if (match.capturedLength() > 1) // keywords
        {
            QTextCharFormat keywordFormat;
            keywordFormat.setForeground (DarkBlue);
            keywordFormat.setFontWeight (QFont::Bold);
            setFormat (index, match.capturedLength(), keywordFormat);
            jsonValue (text, index + match.capturedLength(),
                       K, V, B,
                       insideValue, braces);
        }
        else // double quote
        {
            int end = text.indexOf (quoteMark, index + 1);
            while (isEscapedChar (text, end))
                end = text.indexOf (quoteMark, end + 1);
            if (end >= 0)
            {
                setFormat (index, end + 1 - index, regexFormat);
                jsonValue (text, end + 1,
                           K, V, B,
                           insideValue, braces);
            }
            else
            {
                setFormat (index, text.length() - index, regexFormat);
                setCurrentBlockState (regexState);
            }
        }
    }
}
/*************************/
void Highlighter::highlightJsonBlock (const QString &text)
{
    /* NOTE:
       We try to use the available variables of TextBlockData for Json.
       The meanings of the used variables change as follows:

       TextBlockData::OpenNests          -> Nested keys
       TextBlockData::LastFormattedRegex -> Nested values
       TextBlockData::lastFormattedQuote -> Nested brackets (inside values)
       TextBlockData::Property           -> Locally inside a value (not a key inside a value)
       TextBlockData::label              -> Keeps track of the order of open braces and brackets
    */

    int index = 0;
    bool rehighlightNextBlock = false;

    int oldK = 0, oldV = 0, oldB = 0;
    bool oldInsideValue = false;
    QString oldBraces;
    if (TextBlockData *oldData = static_cast<TextBlockData *>(currentBlockUserData()))
    {
        oldK = oldData->openNests();
        if (oldK > 0)
        {
            oldV = oldData->lastFormattedRegex();
            if (oldV > 0)
            {
                oldInsideValue = oldData->getProperty();
                oldB = oldData->lastFormattedQuote();
                oldBraces = oldData->labelInfo();
            }
        }
    }

    TextBlockData *data = new TextBlockData;
    data->setLastState (currentBlockState());
    setCurrentBlockUserData (data);
    setCurrentBlockState (0);

    int K = 0, V = 0, B = 0;
    bool insideValue = false;
    QString braces;

    QTextBlock prevBlock = currentBlock().previous();
    if (prevBlock.isValid())
    {
        if (TextBlockData *prevData = static_cast<TextBlockData *>(prevBlock.userData()))
        {
            K = prevData->openNests();
            if (K > 0)
            {
                V = prevData->lastFormattedRegex();
                if (V > 0)
                {
                    insideValue = prevData->getProperty();
                    B = prevData->lastFormattedQuote();
                    braces = prevData-> labelInfo();
                }
            }
        }
    }

    if (K > 0)
    {
        if (insideValue) // the value continues from the previous line
        {
            if (previousBlockState() == regexState)
            {
                index = text.indexOf (quoteMark);
                while (isEscapedChar (text, index))
                    index = text.indexOf (quoteMark, index + 1);
                if (index < 0)
                {
                    setFormat (0, text.length(), regexFormat);
                    setCurrentBlockState (regexState);
                }
                else
                {
                    ++ index;
                    setFormat (0, index, regexFormat);
                }
            }
            if (index > -1)
            {
                jsonValue (text, index,
                           K, V, B,
                           insideValue, braces);
            }
        }
        else // the key continues from the previous line
        {
            if (previousBlockState() == doubleQuoteState)
            {
                index = text.indexOf (quoteMark);
                while (isEscapedChar (text, index))
                    index = text.indexOf (quoteMark, index + 1);
                if (index < 0)
                {
                    setFormat (0, text.length(), quoteFormat);
                    setCurrentBlockState (doubleQuoteState);
                }
                else
                {
                    ++ index;
                    setFormat (0, index, quoteFormat);
                }
            }
            if (index > -1)
            {
                jsonKey (text, index,
                         K, V, B,
                         insideValue, braces);
            }
        }
    }
    else
    {
        index = text.indexOf ('{');
        ++K;
        braces += "{";
        if (index > -1)
        {
            jsonKey (text, index + 1,
                     K, V, B,
                     insideValue, braces);
        }
        else
        { // this can't be useful but Qt has a test Json file
            ++V;
            insideValue = true;
            jsonValue (text, 0,
                       K, V, B,
                       insideValue, braces);
        }
    }

    data->insertNestInfo (K); // open keys
    data->insertLastFormattedRegex (V); // open values
    data->insertLastFormattedQuote (B); // open brackets (inside values)
    data->setProperty (insideValue); // locally inside a value
    data->insertInfo (braces); // the order of open braces and brackets

    if (currentBlockState() == data->lastState()
        && (K != oldK || V != oldV || B != oldB
            || insideValue != oldInsideValue || braces != oldBraces))
    {
        rehighlightNextBlock = true;
    }

    /* also, format whitespaces */
    int bn = currentBlock().blockNumber();
    if (bn >= startCursor.blockNumber() && bn <= endCursor.blockNumber())
    {
        data->setHighlighted();
        for (const HighlightingRule &rule : qAsConst (highlightingRules))
        {
            if (rule.format == whiteSpaceFormat)
            {
                QRegularExpressionMatch match;
                index = text.indexOf (rule.pattern, 0, &match);
                while (index >= 0)
                {
                    setFormat (index, match.capturedLength(), rule.format);
                    index = text.indexOf (rule.pattern, index + match.capturedLength(), &match);
                }
                break;
            }
        }
    }

    QTextCharFormat fi;

    /* left brace */
    index = text.indexOf ('{');
    fi = format (index);
    while (index >= 0 && (fi == quoteFormat || fi == regexFormat))
    {
        index = text.indexOf ('{', index + 1);
        fi = format (index);
    }
    while (index >= 0)
    {
        BraceInfo *info = new BraceInfo;
        info->character = '{';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf ('{', index + 1);
        fi = format (index);
        while (index >= 0 && (fi == quoteFormat || fi == regexFormat))
        {
            index = text.indexOf ('{', index + 1);
            fi = format (index);
        }
    }

    /* right brace */
    index = text.indexOf ('}');
    fi = format (index);
    while (index >= 0 && (fi == quoteFormat || fi == regexFormat))
    {
        index = text.indexOf ('}', index + 1);
        fi = format (index);
    }
    while (index >= 0)
    {
        BraceInfo *info = new BraceInfo;
        info->character = '}';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf ('}', index +1);
        fi = format (index);
        while (index >= 0 && (fi == quoteFormat || fi == regexFormat))
        {
            index = text.indexOf ('}', index + 1);
            fi = format (index);
        }
    }

    /* left bracket */
    index = text.indexOf ('[');
    fi = format (index);
    while (index >= 0 && (fi == quoteFormat || fi == regexFormat))
    {
        index = text.indexOf ('[', index + 1);
        fi = format (index);
    }
    while (index >= 0)
    {
        BracketInfo *info = new BracketInfo;
        info->character = '[';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf ('[', index + 1);
        fi = format (index);
        while (index >= 0 && (fi == quoteFormat || fi == regexFormat))
        {
            index = text.indexOf ('[', index + 1);
            fi = format (index);
        }
    }

    /* right bracket */
    index = text.indexOf (']');
    fi = format (index);
    while (index >= 0 && (fi == quoteFormat || fi == regexFormat))
    {
        index = text.indexOf (']', index + 1);
        fi = format (index);
    }
    while (index >= 0)
    {
        BracketInfo *info = new BracketInfo;
        info->character = ']';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf (']', index +1);
        fi = format (index);
        while (index >= 0 && (fi == quoteFormat || fi == regexFormat))
        {
            index = text.indexOf (']', index + 1);
            fi = format (index);
        }
    }

    setCurrentBlockUserData (data);

    if (rehighlightNextBlock)
    {
        QTextBlock nextBlock = currentBlock().next();
        if (nextBlock.isValid())
            QMetaObject::invokeMethod (this, "rehighlightBlock", Qt::QueuedConnection, Q_ARG (QTextBlock, nextBlock));
    }
}

}
