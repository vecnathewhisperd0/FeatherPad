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

// This should be called before htmlStyleHighlighter();
void Highlighter::htmlBrackets (const QString &text, const int start)
{
    if (progLan != "html") return;

    /*****************************
     * (Multiline) HTML Brackets *
     *****************************/

    int braIndex = start;
    QRegExp braStartExp = QRegExp ("<(?!\\!)/{,1}[A-Za-z0-9_\\-]+");
    QRegExp braEndExp = QRegExp (">");
    QTextCharFormat htmlBraFormat;
    htmlBraFormat.setFontWeight (QFont::Bold);
    htmlBraFormat.setForeground (Violet);
    if ((previousBlockState() != htmlBracketState
         && previousBlockState() != singleQuoteState
         && previousBlockState() != doubleQuoteState)
        || braIndex > 0)
    {
        braIndex = braStartExp.indexIn (text, braIndex);
        while (format (braIndex) == commentFormat || format (braIndex) == urlFormat)
            braIndex = braStartExp.indexIn (text, braIndex + 1);

    }

    int tmpIndex = braIndex; // to check progress in the following loop
    while (braIndex >= 0)
    {
        int braEndIndex;

        int matched = 0;
        if ((previousBlockState() == htmlBracketState
             || previousBlockState() == singleQuoteState
             || previousBlockState() == doubleQuoteState)  && braIndex == 0)
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
            setCurrentBlockState (htmlBracketState);
            len = text.length() - braIndex;
        }
        else
            len = braEndIndex - braIndex
                  + braEndExp.matchedLength();

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
        if ((previousBlockState() != doubleQuoteState
             && previousBlockState() != singleQuoteState)
            || tmpIndex < braIndex) // when we're in another bracket
        {
            quoteIndex = quoteExpression.indexIn (text, braIndex);

            /* if the start quote is found... */
            if (quoteIndex >= braIndex && quoteIndex <= endLimit)
            {
                /* ... distinguish between double and single quotes */
                if (quoteIndex == quoteMark.indexIn (text, quoteIndex))
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
                    quote = doubleQuoteState;
                }
                else
                {
                    quoteExpression = QRegExp ("\'");
                    quote = singleQuoteState;
                }
            }

            int quoteEndIndex = quoteExpression.indexIn (text, quoteIndex + 1);
            if (quoteIndex == braIndex
                && (previousBlockState() == doubleQuoteState
                    || previousBlockState() == singleQuoteState))
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
                if (currentBlockState() == htmlBracketState)
                    setCurrentBlockState (quote);
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

        QTextCharFormat htmlAttributeFormat;
        htmlAttributeFormat.setFontItalic (true);
        htmlAttributeFormat.setForeground (Brown);
        QRegExp attExp = QRegExp ("[A-Za-z0-9_\\-]+(?=\\s*\\=)");
        int attIndex = attExp.indexIn (text, braIndex);
        while (format (attIndex) == quoteFormat
               || format (attIndex) == altQuoteFormat)
        {
            attIndex = attExp.indexIn (text, attIndex + 1);
        }
        while (attIndex >= braIndex && attIndex < endLimit)
        {
            int length = attExp.matchedLength();
            setFormat (attIndex, length, htmlAttributeFormat);
            attIndex = attExp.indexIn (text, attIndex + length);
        }

        braIndex = braStartExp.indexIn (text, braIndex + len);
        while (format (braIndex) == commentFormat || format (braIndex) == urlFormat)
            braIndex = braStartExp.indexIn (text, braIndex + 1);
    }
}
/*************************/
void Highlighter::htmlStyleHighlighter (const QString &text, const int start)
{
    if (progLan != "html") return;

    int styleIndex = start;
    QRegExp styleStartExp = QRegExp ("<style");
    QRegExp styleEndExp = QRegExp ("</style>");
    QTextCharFormat htmlStyleFormat;
    htmlStyleFormat.setFontWeight (QFont::Bold);
    htmlStyleFormat.setForeground (Violet);
    if ((previousBlockState() != htmlStyleState
         && previousBlockState() != htmlBlockState
         && previousBlockState() != htmlValueState
         && previousBlockState() != htmlStyleBracketState
         && previousBlockState() != htmlStyleSingleQuoteState
         && previousBlockState() != htmlStyleDoubleQuoteState)
        || styleIndex > 0)
    {
        styleIndex = styleStartExp.indexIn (text, start);
        while (format (styleIndex) == commentFormat || format (styleIndex) == urlFormat)
            styleIndex = styleStartExp.indexIn (text, styleIndex + 1);
    }

    int tmpIndex = styleIndex; // to check progress in the following loop
    while (styleIndex >= 0)
    {
        int styleEndIndex;

        int matched = 0;
        if ((previousBlockState() == htmlStyleState
             || previousBlockState() == htmlBlockState
             || previousBlockState() == htmlValueState
             || previousBlockState() == htmlStyleBracketState
             || previousBlockState() == htmlStyleSingleQuoteState
             || previousBlockState() == htmlStyleDoubleQuoteState)
            && styleIndex == 0)
        {
            styleEndIndex = styleEndExp.indexIn (text, 0);
        }
        else
        {
            matched = styleStartExp.matchedLength();
            styleEndIndex = styleEndExp.indexIn (text,
                                                 styleIndex + matched);
        }

        int len;
        if (styleEndIndex == -1)
        {
            setCurrentBlockState (htmlStyleState);
            len = text.length() - styleIndex;
        }
        else
            len = styleEndIndex - styleIndex
                  + styleEndExp.matchedLength();
        if (matched > 0)
            setFormat (styleIndex, matched, htmlStyleFormat);

        int endLimit;
        if (styleEndIndex == -1)
            endLimit = text.length();
        else
            endLimit = styleEndIndex;

        /*****************************
         * (Multiline) HTML Brackets *
         *****************************/

        int braIndex = styleIndex;
        QRegExp braStartExp = QRegExp ("<style");
        QRegExp braEndExp = QRegExp (">");
        QTextCharFormat htmlBraFormat;
        htmlBraFormat.setFontWeight (QFont::Bold);
        htmlBraFormat.setForeground (Violet);
        QTextCharFormat htmlValueFormat;
        htmlValueFormat.setFontItalic (true);
        htmlValueFormat.setForeground (Qt::magenta);
        if ((previousBlockState() != htmlStyleBracketState
             && previousBlockState() != htmlStyleSingleQuoteState
             && previousBlockState() != htmlStyleDoubleQuoteState)
            || braIndex < styleIndex) // another style
        {
            braIndex = braStartExp.indexIn (text, start);
        }

        int tempIndex = braIndex; // to check progress in the following loop
        while (braIndex >= styleIndex && braIndex <= endLimit)
        {
            int braEndIndex;

            int matched = 0;
            if ((previousBlockState() == htmlStyleBracketState
                 || previousBlockState() == htmlStyleSingleQuoteState
                 || previousBlockState() == htmlStyleDoubleQuoteState)
                && braIndex == 0)
            {
                braEndIndex = braEndExp.indexIn (text, styleIndex);
            }
            else
            {
                matched = braStartExp.matchedLength();
                braEndIndex = braEndExp.indexIn (text,
                                                 braIndex + matched);
            }

            int braLen;
            if (braEndIndex == -1)
            {
                if (currentBlockState() == htmlStyleState)
                    setCurrentBlockState (htmlStyleBracketState);
                braLen = text.length() - braIndex;
            }
            else
                braLen = braEndIndex - braIndex
                         + braEndExp.matchedLength();

            if (matched > 0)
                setFormat (braIndex, matched, htmlBraFormat);
            if (braEndIndex > -1)
                setFormat (braEndIndex, braEndExp.matchedLength(), htmlBraFormat);


            int endLimit1;
            if (braEndIndex == -1)
                endLimit1 = text.length();
            else
                endLimit1 = braEndIndex;

            /***************************
             * (Multiline) HTML Quotes *
             ***************************/

            int quoteIndex = braIndex;
            QRegExp quoteExpression = QRegExp ("\"|\'");
            int quote = htmlStyleDoubleQuoteState;

            /* find the start quote */
            if ((previousBlockState() != htmlStyleDoubleQuoteState
                 && previousBlockState() != htmlStyleSingleQuoteState)
                || tempIndex < braIndex) // when we're in another bracket
            {
                quoteIndex = quoteExpression.indexIn (text, braIndex);

                /* if the start quote is found... */
                if (quoteIndex >= braIndex && quoteIndex <= endLimit1)
                {
                    /* ... distinguish between double and single quotes */
                    if (quoteIndex == quoteMark.indexIn (text, quoteIndex))
                    {
                        quoteExpression = quoteMark;
                        quote = htmlStyleDoubleQuoteState;
                    }
                    else
                    {
                        quoteExpression = QRegExp ("\'");
                        quote = htmlStyleSingleQuoteState;
                    }
                }
            }
            else // but if we're inside a quotation...
            {
                /* ... distinguish between the two quote kinds
                   by checking the previous line */
                quote = previousBlockState();
                if (quote == htmlStyleDoubleQuoteState)
                    quoteExpression = quoteMark;
                else
                    quoteExpression = QRegExp ("\'");
            }

            while (quoteIndex >= braIndex && quoteIndex <= endLimit1)
            {
                /* if the search is continued... */
                if (quoteExpression == QRegExp ("\"|\'"))
                {
                    /* ... distinguish between double and single quotes
                       again because the quote mark may have changed */
                    if (quoteIndex == quoteMark.indexIn (text, quoteIndex))
                    {
                        quoteExpression = quoteMark;
                        quote = htmlStyleDoubleQuoteState;
                    }
                    else
                    {
                        quoteExpression = QRegExp ("\'");
                        quote = htmlStyleSingleQuoteState;
                    }
                }

                int quoteEndIndex = quoteExpression.indexIn (text, quoteIndex + 1);
                if (quoteIndex == braIndex
                    && (previousBlockState() == htmlStyleDoubleQuoteState
                        || previousBlockState() == htmlStyleSingleQuoteState))
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
                    if (quoteEndIndex > endLimit1)
                        quoteEndIndex = endLimit1;
                    else
                        Matched = quoteExpression.matchedLength();
                }

                int quoteLength;
                if (quoteEndIndex == -1)
                {
                    if (currentBlockState() == htmlStyleBracketState)
                        setCurrentBlockState (quote);
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

            QTextCharFormat htmlAttributeFormat;
            htmlAttributeFormat.setFontItalic (true);
            htmlAttributeFormat.setForeground (Brown);
            QRegExp attExp = QRegExp ("[A-Za-z0-9_\\-]+(?=\\s*\\=)");
            int attIndex = attExp.indexIn (text, braIndex);
            while (format (attIndex) == quoteFormat
                   || format (attIndex) == altQuoteFormat)
            {
                attIndex = attExp.indexIn (text, attIndex + 1);
            }
            while (attIndex >= braIndex && attIndex < endLimit)
            {
                int length = attExp.matchedLength();
                setFormat (attIndex, length, htmlAttributeFormat);
                attIndex = attExp.indexIn (text, attIndex + length);
            }

            braIndex = braStartExp.indexIn (text, braIndex + braLen);
        }

        if (currentBlockState() != htmlStyleBracketState
            && currentBlockState() != htmlStyleSingleQuoteState
            && currentBlockState() != htmlStyleDoubleQuoteState)
        {
            /***************************
             * (Multiline) HTML Blocks *
             ***************************/

            QRegExp htmlStartExp = QRegExp ("\\{");
            QRegExp htmlEndExp = QRegExp ("\\}");
            int index = styleIndex;
            QTextCharFormat htmlFormat;
            htmlFormat.setFontUnderline (true);
            htmlFormat.setForeground (Qt::red);
            if ((previousBlockState() != htmlBlockState
                 && previousBlockState() != htmlValueState)
                || tmpIndex < styleIndex) // another style
            {
                index = htmlStartExp.indexIn (text, start);
            }

            while (index >= styleIndex && index <= endLimit)
            {
                int endIndex;
                /* when the HTML block start is in the prvious line
                   and the search for the block end has just begun... */
                if ((previousBlockState() == htmlBlockState
                     || previousBlockState() == htmlValueState) // subset of htmlBlockState
                    && index == styleIndex)
                    /* search for the block end from the line start */
                    endIndex = htmlEndExp.indexIn (text, styleIndex);
                else
                    endIndex = htmlEndExp.indexIn (text,
                                                   index + htmlStartExp.matchedLength());

                int htmlLength;
                if (endIndex == -1)
                {
                    endIndex = text.length() - 1;
                    if (currentBlockState() == htmlStyleState)
                        setCurrentBlockState (htmlBlockState);
                    htmlLength = text.length() - index;
                }
                else
                    htmlLength = endIndex - index
                                 + htmlEndExp.matchedLength();

                /* at first, we suppose all syntax is wrong */
                QRegExp expression = QRegExp ("[^\\{^\\}^\\s]+");
                int indxTmp = expression.indexIn (text, index);
                while (isQuoted (text, indxTmp))
                    indxTmp = expression.indexIn (text, indxTmp + 1);
                while (indxTmp >= 0 && indxTmp < endIndex)
                {
                    int length = expression.matchedLength();
                    setFormat (indxTmp, length, htmlFormat);
                    indxTmp = expression.indexIn (text, indxTmp + length);
                }

                /* HTML attribute format (before :...;) */
                QTextCharFormat htmlAttFormat;
                htmlAttFormat.setFontItalic (true);
                //htmlAttFormat.setForeground (Qt::blue);
                htmlAttFormat.setFontWeight (QFont::Bold);
                expression = QRegExp ("[A-Za-z0-9_\\-]+(?=\\s*:.*;*)");
                indxTmp = expression.indexIn (text, index);
                while (isQuoted (text, indxTmp))
                    indxTmp = expression.indexIn (text, indxTmp + 1);
                while (indxTmp >= 0 && indxTmp < endIndex)
                {
                    int length = expression.matchedLength();
                    setFormat (indxTmp, length, htmlAttFormat);
                    indxTmp = expression.indexIn (text, indxTmp + length);
                }

                index = htmlStartExp.indexIn (text, index + htmlLength);
            }

            /***************************
             * (Multiline) HTML Values *
             ***************************/

            htmlStartExp = QRegExp (":");
            htmlEndExp = QRegExp (";|\\}");
            index = styleIndex;
            if (previousBlockState() != htmlValueState
                || tmpIndex < styleIndex)
            {
                index = htmlStartExp.indexIn (text, start);
                if (index > -1)
                {
                    while (format (index) != htmlFormat)
                    {
                        index = htmlStartExp.indexIn (text, index + 1);
                        if (index == -1) break;
                    }
                }
            }

            while (index >= styleIndex && index <= endLimit)
            {
                int endIndex;
                int startMatch = 0;
                if (previousBlockState() == htmlValueState
                    && index == styleIndex)
                    endIndex = htmlEndExp.indexIn (text, styleIndex);
                else
                {
                    startMatch = htmlStartExp.matchedLength();
                    endIndex = htmlEndExp.indexIn (text,
                                                   index + startMatch);
                }

                int htmlLength;
                if (endIndex == -1)
                {
                    if (currentBlockState() == htmlBlockState)
                        setCurrentBlockState (htmlValueState);
                    htmlLength = text.length() - index;
                }
                else
                    htmlLength = endIndex - index
                                 + htmlEndExp.matchedLength();

                setFormat (index, htmlLength, htmlValueFormat);

                QTextCharFormat neutral;
                neutral.setForeground (QBrush());
                setFormat (index, startMatch, neutral);
                if (endIndex > -1)
                    setFormat (endIndex, 1, neutral);

                index = htmlStartExp.indexIn (text, index + htmlLength);
                if (index > -1)
                {
                    while (format (index) != htmlFormat)
                    {
                        index = htmlStartExp.indexIn (text, index + 1);
                        if (index == -1) break;
                    }
                }
            }

            /* color value format (#xyz) */
            QTextCharFormat htmlColorFormat;
            htmlColorFormat.setFontItalic (true);
            htmlColorFormat.setForeground (Qt::magenta);
            htmlColorFormat.setFontWeight (QFont::Bold);
            QRegExp expression = QRegExp ("#\\b([A-Za-z0-9]{3}){,4}(?![A-Za-z0-9_]+)");
            int indxTmp = expression.indexIn (text, start);
            while (isQuoted (text, indxTmp))
                indxTmp = expression.indexIn (text, indxTmp + 1);
            while (indxTmp >= 0 && indxTmp < endLimit)
            {
                int length = expression.matchedLength();
                /* now htmlFormat is really the error format */
                if (format (indxTmp) != htmlFormat
                    && format (indxTmp) == htmlValueFormat) // should be a value
                {
                    setFormat (indxTmp, length, htmlColorFormat);
                }
                indxTmp = expression.indexIn (text, indxTmp + length);
            }
        }

        styleIndex = styleStartExp.indexIn (text, styleIndex + len);
        while (format (styleIndex) == commentFormat || format (styleIndex) == urlFormat)
            styleIndex = styleStartExp.indexIn (text, styleIndex + 1);
    }

    /* at last, format whitespaces */
    for (const HighlightingRule &rule : static_cast<const QVector<HighlightingRule>&>(highlightingRules))
    {
        if (rule.format == whiteSpaceFormat)
        {
            QRegExp expression (rule.pattern);
            int index = expression.indexIn (text, start);
            while (index >= 0)
            {
                int length = expression.matchedLength();
                setFormat (index, length, rule.format);
                index = expression.indexIn (text, index + length);
            }
        }
    }
}
/*************************/
void Highlighter::htmlJavascript (const QString &text)
{
    if (progLan != "html") return;

    int javaIndex = 0;

    QRegExp javaStartExp = QRegExp ("<(script|SCRIPT)\\s+(language|LANGUAGE)\\s*\\=\\s*\"\\s*JavaScript\\s*\"[A-Za-z0-9_\\.\"\\s\\=]*>");
    QRegExp javaEndExp = QRegExp ("</(script|SCRIPT)\\s*>");

    /* switch to javascript temporarily */
    commentStartExpression = QRegExp ("/\\*");
    commentEndExpression = QRegExp ("\\*/");
    progLan = "javascript";

    bool wasJavascript (false);
    QTextBlock prevBlock = currentBlock().previous();
    if (prevBlock.isValid())
    {
        if (TextBlockData *prevData = static_cast<TextBlockData *>(prevBlock.userData()))
            wasJavascript = prevData->labelInfo() == "JS"; // it was labeled below
    }

    if (!wasJavascript)
    {
        javaIndex = javaStartExp.indexIn (text);
        while (format (javaIndex) == commentFormat
               || format (javaIndex) == quoteFormat
               || format (javaIndex) == altQuoteFormat
               || format (javaIndex) == urlFormat)
        {
            javaIndex = javaStartExp.indexIn (text, javaIndex + 1);
        }
    }
    int matched = 0;
    TextBlockData *curData = static_cast<TextBlockData *>(currentBlock().userData());
    while (javaIndex >= 0)
    {
        if (!wasJavascript || javaIndex > 0)
            matched = javaStartExp.matchedLength();

        /* starting from here, clear all html formats... */
        QTextCharFormat neutral;
        neutral.setForeground (QBrush());
        setFormat (javaIndex + matched, text.length() - javaIndex - matched, neutral);
        setCurrentBlockState (0);

        /* ... and apply the javascript formats */
        singleLineComment (text, javaIndex + matched);
        multiLineQuote (text, htmlJavaCommentState);
        multiLineComment (text,
                          javaIndex + matched, -1,
                          commentStartExpression, commentEndExpression,
                          htmlJavaCommentState,
                          commentFormat);
        multiLineJSRegex (text, javaIndex + matched);
        for (const HighlightingRule &rule : static_cast<const QVector<HighlightingRule>&>(highlightingRules))
        {
            if (rule.format == commentFormat)
                continue;

            QRegExp expression (rule.pattern);
            int index = expression.indexIn (text, javaIndex + matched);
            while (rule.format != whiteSpaceFormat
                   && (format (index) == quoteFormat
                       || format (index) == altQuoteFormat
                       || format (index) == commentFormat
                       || format (index) == urlFormat))
            {
                index = expression.indexIn (text, index + 1);
            }

            while (index >= 0)
            {
                int length = expression.matchedLength();
                setFormat (index, length, rule.format);
                index = expression.indexIn (text, index + length);

                while (rule.format != whiteSpaceFormat
                       && (format (index) == quoteFormat
                           || format (index) == altQuoteFormat
                           || format (index) == commentFormat
                           || format (index) == urlFormat
                           || format (index) == JSRegexFormat))
                {
                    index = expression.indexIn (text, index + 1);
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

        while (javaEndIndex > -1
               && (format (javaEndIndex) == quoteFormat
                   || format (javaEndIndex) == altQuoteFormat
                   || format (javaEndIndex) == commentFormat
                   || format (javaEndIndex) == urlFormat
                   || format (javaEndIndex) == JSRegexFormat))
        {
            javaEndIndex = javaEndExp.indexIn (text, javaEndIndex + 1);
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
            setFormat (javaEndIndex, text.length() - javaEndIndex, neutral);
            setCurrentBlockState (0);
            progLan = "html";
            htmlBrackets (text, javaEndIndex);
            htmlStyleHighlighter (text, javaEndIndex);
            progLan = "javascript";
        }

        javaIndex = javaStartExp.indexIn (text, javaIndex + len);
        while (format (javaIndex) == commentFormat
               || format (javaIndex) == urlFormat
               || format (javaIndex) == quoteFormat
               || format (javaIndex) == altQuoteFormat)
        {
            javaIndex = javaStartExp.indexIn (text, javaIndex + 1);
        }
    }

    /* revert to html */
    progLan = "html";
    commentStartExpression = QRegExp ("<!--");
    commentEndExpression = QRegExp ("-->");
}

}
