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
 *
 * @license GPL-3.0+ <https://spdx.org/licenses/GPL-3.0+.html>
 */

#include "highlighter.h"

namespace FeatherPad {

// This should be called before "htmlCSSHighlighter()" and "htmlJavascript()".
void Highlighter::htmlBrackets (const QString &text, const int start)
{
    if (progLan != "html") return;

    /*****************************
     * (Multiline) HTML Brackets *
     *****************************/

    int braIndex = start;
    int indx = 0;
    QRegExp braStartExp = QRegExp ("<(?!\\!)/{0,1}[A-Za-z0-9_\\-]+");
    QRegExp braEndExp = QRegExp (">");
    QRegExp styleExp = QRegExp ("<(style|STYLE)$|<(style|STYLE)\\s+[^>]*");
    bool isStyle (false);
    QTextCharFormat htmlBraFormat;
    htmlBraFormat.setFontWeight (QFont::Bold);
    htmlBraFormat.setForeground (Violet);

    int prevState = previousBlockState();
    if (braIndex > 0
        || (prevState != singleQuoteState && prevState != doubleQuoteState
            && (prevState < htmlBracketState || prevState > htmlStyleSingleQuoteState)))
    {
        braIndex = braStartExp.indexIn (text, start);
        while (format (braIndex) == commentFormat || format (braIndex) == urlFormat)
            braIndex = braStartExp.indexIn (text, braIndex + 1);
        if (braIndex > -1)
        {
            indx = styleExp.indexIn (text, start);
            while (format (indx) == commentFormat || format (indx) == urlFormat)
                indx = styleExp.indexIn (text, indx + 1);
            isStyle = indx > -1 && braIndex == indx;
        }
    }
    else if (braIndex == 0 && (prevState == htmlStyleState
                               /* these quote states are set only with styles */
                               || prevState == htmlStyleSingleQuoteState
                               || prevState == htmlStyleDoubleQuoteState))
    {
        isStyle = true;
    }

    int bn = currentBlock().blockNumber();
    bool mainFormatting (bn >= startCursor.blockNumber() && bn <= endCursor.blockNumber());
    bool hugeLine (text.length() > 50000);
    int firstBraIndex = braIndex; // to check progress in the following loop
    while (braIndex >= 0)
    {
        if (hugeLine)
        {
            setFormat (braIndex, text.length() - braIndex, translucentFormat);
            break;
        }
        int braEndIndex;

        int matched = 0;
        if (braIndex == 0
            && (prevState == singleQuoteState || prevState == doubleQuoteState
                || (prevState >= htmlBracketState && prevState <= htmlStyleSingleQuoteState)))
        {
            braEndIndex = braEndExp.indexIn (text, 0);
        }
        else
        {
            matched = braStartExp.matchedLength();
            braEndIndex = braEndExp.indexIn (text,
                                             braIndex + matched);
        }

        int len;
        if (braEndIndex == -1)
        {
            len = text.length() - braIndex;
            if (isStyle)
                setCurrentBlockState (htmlStyleState);
            else
                setCurrentBlockState (htmlBracketState);
        }
        else
        {
            len = braEndIndex - braIndex
                  + braEndExp.matchedLength();
        }

        if (matched > 0)
            setFormat (braIndex, matched, htmlBraFormat);
        if (braEndIndex > -1)
            setFormat (braEndIndex, braEndExp.matchedLength(), htmlBraFormat);


        int endLimit;
        if (braEndIndex == -1)
            endLimit = text.length();
        else
            endLimit = braEndIndex;

        /***************************
         * (Multiline) HTML Quotes *
         ***************************/

        int quoteIndex = braIndex;
        QRegExp quoteExpression = QRegExp ("\"|\'");
        int quote = doubleQuoteState;

        /* find the start quote */
        if (braIndex > firstBraIndex // when we're in another bracket
            || (prevState != doubleQuoteState
                && prevState != singleQuoteState
                && prevState != htmlStyleSingleQuoteState
                && prevState != htmlStyleDoubleQuoteState))
        {
            quoteIndex = quoteExpression.indexIn (text, braIndex);

            /* if the start quote is found... */
            if (quoteIndex >= braIndex && quoteIndex <= endLimit)
            {
                /* ... distinguish between double and single quotes */
                if (quoteIndex == quoteMark.indexIn (text, quoteIndex))
                {
                    quoteExpression = quoteMark;
                    quote = currentBlockState() == htmlStyleState ? htmlStyleDoubleQuoteState
                                                                  : doubleQuoteState;
                }
                else
                {
                    quoteExpression = QRegExp ("\'");
                    quote = currentBlockState() == htmlStyleState ? htmlStyleSingleQuoteState
                                                                  : singleQuoteState;
                }
            }
        }
        else // but if we're inside a quotation...
        {
            /* ... distinguish between the two quote kinds
               by checking the previous line */
            quote = prevState;
            if (quote == doubleQuoteState || quote == htmlStyleDoubleQuoteState)
                quoteExpression = quoteMark;
            else
                quoteExpression = QRegExp ("\'");
        }

        while (quoteIndex >= braIndex && quoteIndex <= endLimit)
        {
            /* if the search is continued... */
            if (quoteExpression == QRegExp ("\"|\'"))
            {
                /* ... distinguish between double and single quotes
                   again because the quote mark may have changed */
                if (quoteIndex == quoteMark.indexIn (text, quoteIndex))
                {
                    quoteExpression = quoteMark;
                    quote = currentBlockState() == htmlStyleState ? htmlStyleDoubleQuoteState
                                                                  : doubleQuoteState;
                }
                else
                {
                    quoteExpression = QRegExp ("\'");
                    quote = currentBlockState() == htmlStyleState ? htmlStyleSingleQuoteState
                                                                  : singleQuoteState;
                }
            }

            int quoteEndIndex = quoteExpression.indexIn (text, quoteIndex + 1);
            if (quoteIndex == braIndex
                && (prevState == doubleQuoteState
                    || prevState == singleQuoteState
                    || prevState == htmlStyleSingleQuoteState
                    || prevState == htmlStyleDoubleQuoteState))
            {
                quoteEndIndex = quoteExpression.indexIn (text, braIndex);
            }

            int Matched = 0;
            if (quoteEndIndex == -1)
            {
                if (braEndIndex > -1) quoteEndIndex = braEndIndex;
            }
            else
            {
                if (quoteEndIndex > endLimit)
                    quoteEndIndex = endLimit;
                else
                    Matched = quoteExpression.matchedLength();
            }

            int quoteLength;
            if (quoteEndIndex == -1)
            {
                if (currentBlockState() == htmlBracketState
                    || currentBlockState() == htmlStyleState)
                {
                    setCurrentBlockState (quote);
                }
                quoteLength = text.length() - quoteIndex;
            }
            else
                quoteLength = quoteEndIndex - quoteIndex
                              + Matched;
            setFormat (quoteIndex, quoteLength, quoteExpression == quoteMark ? quoteFormat
                                                                             : altQuoteFormat);

            /* the next quote may be different */
            quoteExpression = QRegExp ("\"|\'");
            quoteIndex = quoteExpression.indexIn (text, quoteIndex + quoteLength);
        }

        /*******************************
         * (Multiline) HTML Attributes *
         *******************************/

        if (mainFormatting)
        {
            QTextCharFormat htmlAttributeFormat;
            htmlAttributeFormat.setFontItalic (true);
            htmlAttributeFormat.setForeground (Brown);
            QRegularExpressionMatch attMatch;
            QRegularExpression attExp ("[A-Za-z0-9_\\-]+(?=\\s*\\=)");
            int attIndex = text.indexOf (attExp, braIndex, &attMatch);
            QTextCharFormat fi = format (attIndex);
            while (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat)
            {
                attIndex += attMatch.capturedLength();
                fi = format (attIndex);
                while (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat)
                {
                    ++ attIndex;
                    fi = format (attIndex);
                }
                attIndex = text.indexOf (attExp, attIndex, &attMatch);
                fi = format (attIndex);
            }
            while (attIndex >= braIndex && attIndex < endLimit)
            {
                setFormat (attIndex, attMatch.capturedLength(), htmlAttributeFormat);
                attIndex = text.indexOf(attExp, attIndex + attMatch.capturedLength(), &attMatch);
                fi = format (attIndex);
                while (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat)
                {
                    attIndex += attMatch.capturedLength();
                    fi = format (attIndex);
                    while (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat)
                    {
                        ++ attIndex;
                        fi = format (attIndex);
                    }
                    attIndex = text.indexOf (attExp, attIndex, &attMatch);
                    fi = format (attIndex);
                }
            }
        }

        indx = braIndex + len;
        braIndex = braStartExp.indexIn (text, braIndex + len);
        while (format (braIndex) == commentFormat || format (braIndex) == urlFormat)
            braIndex = braStartExp.indexIn (text, braIndex + 1);
        if (braIndex > -1)
        {
            indx = styleExp.indexIn (text, indx);
            while (format (indx) == commentFormat || format (indx) == urlFormat)
                indx = styleExp.indexIn (text, indx + 1);
            isStyle = indx > -1 && braIndex == indx;
        }
        else isStyle = false;
    }

    /* at last, format whitespaces */
    if (mainFormatting)
    {
        static_cast<TextBlockData *>(currentBlock().userData())->insertHighlightInfo (true); // completely highlighted
        for (const HighlightingRule &rule : static_cast<const QVector<HighlightingRule>&>(highlightingRules))
        {
            if (rule.format == whiteSpaceFormat)
            {
                QRegularExpressionMatch match;
                int index = text.indexOf (rule.pattern, start, &match);
                while (index >= 0)
                {
                    setFormat (index, match.capturedLength(), rule.format);
                    index = text.indexOf (rule.pattern, index + match.capturedLength(), &match);
                }
            }
        }
    }
}
/*************************/
void Highlighter::htmlCSSHighlighter (const QString &text, const int start)
{
    if (progLan != "html") return;

    int cssIndex = start;

    QRegExp cssStartExp = QRegExp ("<(style|STYLE)>|<(style|STYLE)\\s+[^>]*>");
    QRegExp cssEndExp = QRegExp ("</(style|STYLE)\\s*>");
    QRegExp braEndExp = QRegExp (">");

    /* switch to css temporarily */
    commentStartExpression.setPattern ("/\\*");
    commentEndExpression.setPattern ("\\*/");
    progLan = "css";

    bool wasCSS (false);
    int prevState = previousBlockState();
    bool wasStyle (prevState == htmlStyleState
                   || prevState == htmlStyleSingleQuoteState
                   || prevState == htmlStyleDoubleQuoteState);
    QTextBlock prevBlock = currentBlock().previous();
    if (prevBlock.isValid())
    {
        if (TextBlockData *prevData = static_cast<TextBlockData *>(prevBlock.userData()))
            wasCSS = prevData->labelInfo() == "CSS"; // it's labeled below
    }

    QTextCharFormat fi;
    int matched = 0;
    if ((!wasCSS || start > 0)  && !wasStyle)
    {
        cssIndex = cssStartExp.indexIn (text, start);
        fi = format (cssIndex);
        while (cssIndex >= 0
               && (fi == commentFormat
                   || fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat))
        {
            cssIndex = cssStartExp.indexIn (text, cssIndex + cssStartExp.matchedLength());
            fi = format (cssIndex);
        }
    }
    else if (wasStyle)
    {
        cssIndex = braEndExp.indexIn (text, start);
        fi = format (cssIndex);
        while (cssIndex >= 0
               && (fi == commentFormat || fi == urlFormat
                   || fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat))
        {
            cssIndex = braEndExp.indexIn (text, cssIndex + 1);
            fi = format (cssIndex);
        }
        if (cssIndex > -1)
            matched = braEndExp.matchedLength(); // 1
    }
    TextBlockData *curData = static_cast<TextBlockData *>(currentBlock().userData());
    int bn = currentBlock().blockNumber();
    bool mainFormatting (bn >= startCursor.blockNumber() && bn <= endCursor.blockNumber());
    while (cssIndex >= 0)
    {
        /* single-line style bracket (<style ...>) */
        if (matched == 0 && (!wasCSS || cssIndex > 0))
            matched = cssStartExp.matchedLength();

        /* starting from here, clear all html formats... */
        setFormat (cssIndex + matched, text.length() - cssIndex - matched, neutralFormat);
        setCurrentBlockState (0);

        /* ... and apply the css formats */;
        multiLineQuote (text, cssIndex + matched, htmlCSSCommentState);
        int cssIndx = cssHighlighter (text, mainFormatting, cssIndex + matched);
        multiLineComment (text,
                          cssIndex + matched, cssIndx,
                          commentStartExpression, commentEndExpression,
                          htmlCSSCommentState,
                          commentFormat);
        if (mainFormatting)
        {
            for (const HighlightingRule &rule : static_cast<const QVector<HighlightingRule>&>(highlightingRules))
            { // CSS doesn't have any main formatting except for witesapces
                if (rule.format == whiteSpaceFormat)
                {
                    QRegularExpressionMatch match;
                    int index = text.indexOf (rule.pattern, start, &match);
                    while (index >= 0)
                    {
                        setFormat (index, match.capturedLength(), rule.format);
                        index = text.indexOf (rule.pattern, index + match.capturedLength(), &match);
                    }
                }
            }
        }

        /* now, search for the end of the css block */
        int cssEndIndex;
        if (cssIndex == 0 && wasCSS)
            cssEndIndex = cssEndExp.indexIn (text, 0);
        else
        {
            cssEndIndex = cssEndExp.indexIn (text,
                                               cssIndex + matched);
        }

        fi = format (cssEndIndex);
        while (cssEndIndex > -1
               && (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat
                   || fi == commentFormat || fi == urlFormat))
        {
            cssEndIndex = cssEndExp.indexIn (text, cssEndIndex + cssEndExp.matchedLength());
            fi = format (cssEndIndex);
        }

        int len;
        if (cssEndIndex == -1)
        {
            if (currentBlockState() == 0)
                setCurrentBlockState (htmlCSSState); // for updating the next line
            /* Since the next line couldn't be processed based on the state of this line,
               we label this line to show that it's written in css and not html. */
            curData->insertInfo ("CSS");
            len = text.length() - cssIndex;
        }
        else
        {
            len = cssEndIndex - cssIndex
                  + cssEndExp.matchedLength();
            /* if the css block ends at this line, format
               the rest of the line as an html code again */
            setFormat (cssEndIndex, text.length() - cssEndIndex, neutralFormat);
            setCurrentBlockState (0);
            progLan = "html";
            htmlBrackets (text, cssEndIndex);
            progLan = "css";
        }

        cssIndex = cssStartExp.indexIn (text, cssIndex + len);
        fi = format (cssIndex);
        while (cssIndex >= 0
               && (fi == commentFormat || fi == urlFormat
                   || fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat))
        {
            cssIndex = cssStartExp.indexIn (text, cssIndex + cssStartExp.matchedLength());
            fi = format (cssIndex);
        }
        matched = 0; // single-line style bracket (<style ...>)
    }

    /* revert to html */
    progLan = "html";
    commentStartExpression.setPattern ("<!--");
    commentEndExpression.setPattern ("-->");
}
/*************************/
void Highlighter::htmlJavascript (const QString &text)
{
    if (progLan != "html") return;

    int javaIndex = 0;

    QRegExp javaStartExp = QRegExp ("<(script|SCRIPT)\\s+(language|LANGUAGE)\\s*\\=\\s*\"\\s*JavaScript\\s*\"[A-Za-z0-9_\\.\"\\s\\=]*>");
    QRegExp javaEndExp = QRegExp ("</(script|SCRIPT)\\s*>");

    /* switch to javascript temporarily */
    commentStartExpression.setPattern ("/\\*");
    commentEndExpression.setPattern ("\\*/");
    progLan = "javascript";

    bool wasJavascript (false);
    QTextBlock prevBlock = currentBlock().previous();
    if (prevBlock.isValid())
    {
        if (TextBlockData *prevData = static_cast<TextBlockData *>(prevBlock.userData()))
            wasJavascript = prevData->labelInfo() == "JS"; // it's labeled below
    }

    QTextCharFormat fi;
    if (!wasJavascript)
    {
        javaIndex = javaStartExp.indexIn (text);
        fi = format (javaIndex);
        while (javaIndex >= 0
               && (fi == commentFormat || fi == urlFormat
                   || fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat))
        {
            javaIndex = javaStartExp.indexIn (text, javaIndex + javaStartExp.matchedLength());
            fi = format (javaIndex);
        }
    }
    int matched = 0;
    TextBlockData *curData = static_cast<TextBlockData *>(currentBlock().userData());
    int bn = currentBlock().blockNumber();
    bool mainFormatting (bn >= startCursor.blockNumber() && bn <= endCursor.blockNumber());
    while (javaIndex >= 0)
    {
        if (!wasJavascript || javaIndex > 0)
            matched = javaStartExp.matchedLength();

        /* starting from here, clear all html formats... */
        setFormat (javaIndex + matched, text.length() - javaIndex - matched, neutralFormat);
        setCurrentBlockState (0);

        /* ... and apply the javascript formats */
        singleLineComment (text, javaIndex + matched);
        multiLineQuote (text, javaIndex + matched, htmlJavaCommentState);
        multiLineComment (text,
                          javaIndex + matched, -1,
                          commentStartExpression, commentEndExpression,
                          htmlJavaCommentState,
                          commentFormat);
        multiLineJSRegex (text, javaIndex + matched);
        if (mainFormatting)
        {
            for (const HighlightingRule &rule : static_cast<const QVector<HighlightingRule>&>(highlightingRules))
            {
                if (rule.format == commentFormat)
                    continue;

                QRegularExpressionMatch match;
                int index = text.indexOf (rule.pattern, javaIndex + matched, &match);
                if (rule.format != whiteSpaceFormat)
                {
                    fi = format (index);
                    while (index >= 0
                           && (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat
                               || fi == commentFormat || fi == urlFormat))
                    {
                        index = text.indexOf (rule.pattern, index + match.capturedLength(), &match);
                        fi = format (index);
                    }
                }

                while (index >= 0)
                {
                    setFormat (index, match.capturedLength(), rule.format);
                    index = text.indexOf (rule.pattern, index + match.capturedLength(), &match);

                    if (rule.format != whiteSpaceFormat)
                    {
                        fi = format (index);
                        while (index >= 0
                               && (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat
                                   || fi == commentFormat || fi == urlFormat
                                   || fi == JSRegexFormat))
                        {
                            index = text.indexOf (rule.pattern, index + match.capturedLength(), &match);
                            fi = format (index);
                        }
                    }
                }
            }
        }

        /* now, search for the end of the javascript block */
        int javaEndIndex;
        if (javaIndex == 0 && wasJavascript)
            javaEndIndex = javaEndExp.indexIn (text, 0);
        else
        {
            javaEndIndex = javaEndExp.indexIn (text,
                                               javaIndex + matched);
        }

        fi = format (javaEndIndex);
        while (javaEndIndex > -1
               && (fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat
                   || fi == commentFormat || fi == urlFormat
                   || fi == JSRegexFormat))
        {
            javaEndIndex = javaEndExp.indexIn (text, javaEndIndex + javaEndExp.matchedLength());
            fi = format (javaEndIndex);
        }

        int len;
        if (javaEndIndex == -1)
        {
            if (currentBlockState() == 0)
                setCurrentBlockState (htmlJavaState); // for updating the next line
            /* Since the next line couldn't be processed based on the state of this line,
               we label this line to show that it's written in javascript and not html. */
            curData->insertInfo ("JS");
            len = text.length() - javaIndex;
        }
        else
        {
            len = javaEndIndex - javaIndex
                  + javaEndExp.matchedLength();
            /* if the javascript block ends at this line,
               format the rest of the line as an html code again */
            setFormat (javaEndIndex, text.length() - javaEndIndex, neutralFormat);
            setCurrentBlockState (0);
            progLan = "html";
            htmlBrackets (text, javaEndIndex);
            htmlCSSHighlighter (text, javaEndIndex);
            progLan = "javascript";
        }

        javaIndex = javaStartExp.indexIn (text, javaIndex + len);
        fi = format (javaEndIndex);
        while (javaIndex > -1
               && (fi == commentFormat || fi == urlFormat
                   || fi == quoteFormat || fi == altQuoteFormat || fi == urlInsideQuoteFormat))
        {
            javaIndex = javaStartExp.indexIn (text, javaIndex + javaStartExp.matchedLength());
            fi = format (javaEndIndex);
        }
    }

    /* revert to html */
    progLan = "html";
    commentStartExpression.setPattern ("<!--");
    commentEndExpression.setPattern ("-->");
}

}
