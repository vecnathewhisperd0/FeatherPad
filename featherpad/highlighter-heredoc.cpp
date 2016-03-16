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
#include <QTextDocument>

namespace FeatherPad {

// Reset the header state of a here-doc to 7
// and the state of its body to 6.
void Highlighter::resetHereDocStates (QTextBlock block)
{
    if (block.userState() == hereDocTempState)
    {
        block.setUserState (hereDocHeaderState);
        block = block.next();
        while (block.isValid() && block.userState() != 0)
        {
            block.setUserState (hereDocBodyState);
            block = block.next();
        }
    }
}
/*************************/
// Changing of the start delimiter of a "here documnet" doesn't invoke
// highlightBlock() for the rest of it, hence the need for this slot.
// I don't like its complex logic but it works.
void Highlighter::docChanged (int pos, int, int)
{
    QTextBlock block = qobject_cast< QTextDocument *>(parent())->findBlock (pos);
    TextBlockData *data = static_cast<TextBlockData *>(block.userData());
    /* at start or end (state = 7 or 0 or < 6),
       or inside but previously at end (state = 6),
       or outside but previously at start (state = 0 or < 6):
       these are all states that we need to check */
    if (data->isdelimiter())
    {
        /*******************************************************************
         * It's important to know that this method is always called after  *
         * highlightBlock. There are three possibilities: state = 0, 6, 7. *
         * Other values are, like 0, related to a block outside here-docs. *
         *******************************************************************/

        /* when we're inside a here-doc, go up to its header */
        if (block.userState() == hereDocBodyState)
        {
            while (block.isValid())
            {
                block = block.previous();
                if (block.userState() == hereDocHeaderState)
                    break;
            }
        }

        /* when we're before, after or at end of a here-doc... */
        if (block.userState() != hereDocHeaderState)
        {
            /* ... the header state of the next here-doc should be 7;
               so, go to it and then do as for state 7 below */
            block = block.next();
            while (block.isValid() && block.userState() != hereDocHeaderState)
                block = block.next();
        }

        /* when this is a header (state = 7)... */
        if (block.isValid())
        {
            /* ... change the header state to 9 for this
               here-doc to be rehighlighted completely
               (for the highlighting downward 'wave' to go through it) */
            delimState = hereDocTempState;
            rehighlightBlock (block);
            /* then reset its block states... */
            resetHereDocStates (block);
            /* ... and also reset the global header state */
            delimState = hereDocHeaderState;
        }
        /* now, the header states of some here-docs
           below this one may be 9; so, reset them */
        block = block.next();
        while (block.isValid())
        {
            while (block.isValid() && block.userState() != hereDocTempState)
                block = block.next();
            if (block.isValid())
            {
                resetHereDocStates (block);
                block = block.next();
            }
        }
    }
}
/*************************/
// Check if the current block is inside a "here document" and format it accordingly.
bool Highlighter::isHereDocument (const QString &text)
{
    QTextCharFormat blockFormat;
    blockFormat.setForeground (QColor (126, 0, 230));
    QTextCharFormat delimFormat = blockFormat;
    delimFormat.setFontWeight (QFont::Bold);
    QString delimStr;
    /* Kate uses something like "<<(?:\\s*)([\\\\]{,1}[^\\s]+)" */
    QRegExp delim = QRegExp ("<<(?:\\s*)([\\\\]{,1}[A-Za-z0-9_]+)|<<(?:\\s*)(\'[A-Za-z0-9_]+\')|<<(?:\\s*)(\"[A-Za-z0-9_]+\")");
    int pos, i;

    /* format the start delimiter */
    if (previousBlockState() != delimState - 1
        && currentBlockState() != delimState - 1
        && (pos = delim.indexIn (text)) >= 0)
    {
        i = 1;
        while ((delimStr = delim.cap (i)).isEmpty() && i <= 3)
        {
            ++i;
            delimStr = delim.cap (i);
        }
        /* remove quotes */
        if (delimStr.contains ('\''))
            delimStr = delimStr.split ('\'').at (1);
        if (delimStr.contains ('\"'))
            delimStr = delimStr.split ('\"').at (1);
        /* remove the start backslash if it exists */
        if (QString (delimStr.at (0)) == "\\")
            delimStr = delimStr.remove (0, 1);

        if (!delimStr.isEmpty())
        {
            setCurrentBlockState (delimState);
            setFormat (text.indexOf (delimStr, pos),
                       delimStr.length(),
                       delimFormat);

            TextBlockData *data = static_cast<TextBlockData *>(currentBlock().userData());
            data->insertInfo (delimStr);
            data->insertInfo (true);
            setCurrentBlockUserData (data);

            return false;
        }
    }

    if (previousBlockState() == delimState - 1 || previousBlockState() == delimState)
    {
        QTextBlock block = currentBlock().previous();
        TextBlockData *data = static_cast<TextBlockData *>(block.userData());
        delimStr = data->delimiter();
        if (text == delimStr
            || (text.startsWith (delimStr)
                && text.indexOf (QRegExp ("\\s+")) == delimStr.length()))
        {
            /* format the end delimiter */
            setFormat (0,
                       delimStr.length(),
                       delimFormat);

            /* we need this in docChanged() */
            data = static_cast<TextBlockData *>(currentBlock().userData());
            data->insertInfo (true);
            setCurrentBlockUserData (data);

            return false;
        }
        else
        {
            /* format the contents */
            TextBlockData *data = static_cast<TextBlockData *>(currentBlock().userData());
            data->insertInfo (delimStr);
            setCurrentBlockUserData (data);
            setCurrentBlockState (delimState - 1);
            setFormat (0, text.length(), blockFormat);
            return true;
        }
    }

    return false;
}

}
