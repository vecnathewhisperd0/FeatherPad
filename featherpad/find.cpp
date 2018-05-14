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

#include "fpwin.h"
#include "ui_fp.h"
#include <QTextDocumentFragment>

namespace FeatherPad {

/* This order is preserved everywhere for selections:
   current line -> replacement -> found matches -> bracket matches */

/************************************************************************
 ***** Qt's backward search has some bugs. Therefore, we do our own *****
 ***** backward search by using the following two static functions. *****
 ************************************************************************/
static bool findBackwardInBlock (const QTextBlock &block, const QString &str, int offset,
                                 QTextCursor &cursor, QTextDocument::FindFlags flags)
{
    Qt::CaseSensitivity cs = !(flags & QTextDocument::FindCaseSensitively)
                             ? Qt::CaseInsensitive : Qt::CaseSensitive;

    QString text = block.text();
    text.replace (QChar::Nbsp, QLatin1Char (' '));

    int idx = -1;
    while (offset >= 0 && offset <= text.length())
    {
        idx = text.lastIndexOf (str, offset, cs);
        if (idx == -1)
            return false;
        if (flags & QTextDocument::FindWholeWords)
        {
            const int start = idx;
            const int end = start + str.length();
            if ((start != 0 && text.at (start - 1).isLetterOrNumber())
                || (end != text.length() && text.at (end).isLetterOrNumber()))
            { // if this is not a whole word, continue the backward search
                offset = idx - 1;
                idx = -1;
                continue;
            }
        }
        cursor.setPosition (block.position() + idx);
        cursor.setPosition (cursor.position() + str.length(), QTextCursor::KeepAnchor);
        return true;
    }
    return false;
}

static bool findBackward (const QTextDocument *txtdoc, const QString str,
                          QTextCursor &cursor, QTextDocument::FindFlags flags)
{
    if (!str.isEmpty() && !cursor.isNull())
    {
        int pos = cursor.anchor()
                  - str.size(); // we don't want a match with the cursor inside it
        if (pos >= 0)
        {
            QTextBlock block = txtdoc->findBlock (pos);
            int blockOffset = pos - block.position();
            while (block.isValid())
            {
                if (findBackwardInBlock (block, str, blockOffset, cursor, flags))
                    return true;
                block = block.previous();
                blockOffset = block.length() - 2; // newline is included in length()
            }
        }
    }
    cursor = QTextCursor();
    return false;
}
/*************************/
// This method extends the searchable strings to those with line breaks.
// It also corrects the behavior of Qt's backward search and can set an
// end limit to the forward search.
QTextCursor FPwin::finding (const QString& str, const QTextCursor& start, QTextDocument::FindFlags flags,
                            const int end) const
{
    /* let's be consistent first */
    if (ui->tabWidget->currentIndex() == -1 || str.isEmpty())
        return QTextCursor(); // null cursor

    QTextDocument *txtdoc = qobject_cast< TabPage *>(ui->tabWidget->currentWidget())
                            ->textEdit()->document();
    QTextCursor res = start;
    if (str.contains ('\n'))
    {
        QTextCursor cursor = start;
        QTextCursor found;
        QStringList sl = str.split ("\n");
        int i = 0;
        Qt::CaseSensitivity cs = !(flags & QTextDocument::FindCaseSensitively)
                                 ? Qt::CaseInsensitive : Qt::CaseSensitive;
        QString subStr;
        if (!(flags & QTextDocument::FindBackward))
        {
            /* this loop searches for the consecutive
               occurrences of newline separated strings */
            while (i < sl.count())
            {
                if (i == 0) // the first string
                {
                    subStr = sl.at (0);
                    /* when the first string is empty... */
                    if (subStr.isEmpty())
                    {
                        /* ... search anew from the next block */
                        cursor.movePosition (QTextCursor::EndOfBlock);
                        if (end > 0 && cursor.anchor() > end)
                            return QTextCursor();
                        res.setPosition (cursor.position());
                        if (!cursor.movePosition (QTextCursor::NextBlock))
                            return QTextCursor();
                        ++i;
                    }
                    else
                    {
                        if ((found = txtdoc->find (subStr, cursor, flags)).isNull())
                            return QTextCursor();
                        if (end > 0 && found.anchor() > end)
                            return QTextCursor();
                        cursor.setPosition (found.position());
                        /* if the match doesn't end the block... */
                        while (!cursor.atBlockEnd())
                        {
                            /* ... move the cursor to right and search until a match is found */
                            cursor.movePosition (QTextCursor::EndOfBlock);
                            cursor.setPosition (cursor.position() - subStr.length());
                            if ((found = txtdoc->find (subStr, cursor, flags)).isNull())
                                return QTextCursor();
                            if (end > 0 && found.anchor() > end)
                                return QTextCursor();
                            cursor.setPosition (found.position());
                        }

                        res.setPosition (found.anchor());
                        if (!cursor.movePosition (QTextCursor::NextBlock))
                            return QTextCursor();
                        ++i;
                    }
                }
                else if (i != sl.count() - 1) // middle strings
                {
                    /* when the next block's test isn't the next string... */
                    if (QString::compare (cursor.block().text(), sl.at (i), cs) != 0)
                    {
                        /* ... reset the loop cautiously */
                        cursor.setPosition (res.position());
                        if (!cursor.movePosition (QTextCursor::NextBlock))
                            return QTextCursor();
                        i = 0;
                        continue;
                    }

                    if (!cursor.movePosition (QTextCursor::NextBlock))
                        return QTextCursor();
                    ++i;
                }
                else // the last string (i == sl.count() - 1)
                {
                    subStr = sl.at (i);
                    if (subStr.isEmpty()) break;
                    if (!(flags & QTextDocument::FindWholeWords))
                    {
                        /* when the last string doesn't start the next block... */
                        if (!cursor.block().text().startsWith (subStr, cs))
                        {
                            /* ... reset the loop cautiously */
                            cursor.setPosition (res.position());
                            if (!cursor.movePosition (QTextCursor::NextBlock))
                                return QTextCursor();
                            i = 0;
                            continue;
                        }
                        cursor.setPosition (cursor.anchor() + subStr.count());
                        break;
                    }
                    else
                    {
                        if ((found = txtdoc->find (subStr, cursor, flags)).isNull()
                            || found.anchor() != cursor.position())
                        {
                            cursor.setPosition (res.position());
                            if (!cursor.movePosition (QTextCursor::NextBlock))
                                return QTextCursor();
                            i = 0;
                            continue;
                        }
                        cursor.setPosition (found.position());
                        break;
                    }
                }
            }
            res.setPosition (cursor.position(), QTextCursor::KeepAnchor);
        }
        else // backward search
        {
            cursor.setPosition (cursor.anchor());
            int endPos = cursor.position();
            while (i < sl.count())
            {
                if (i == 0) // the last string
                {
                    subStr = sl.at (sl.count() - 1);
                    if (subStr.isEmpty())
                    {
                        cursor.movePosition (QTextCursor::StartOfBlock);
                        endPos = cursor.position();
                        if (!cursor.movePosition (QTextCursor::PreviousBlock))
                            return QTextCursor();
                        cursor.movePosition (QTextCursor::EndOfBlock);
                        ++i;
                    }
                    else
                    {
                        if (!findBackward (txtdoc, subStr, cursor, flags))
                            return QTextCursor();
                        /* if the match doesn't start the block... */
                        while (cursor.anchor() > cursor.block().position())
                        {
                            /* ... move the cursor to left and search backward until a match is found */
                            cursor.setPosition (cursor.block().position() + subStr.count());
                            if (!findBackward (txtdoc, subStr, cursor, flags))
                                return QTextCursor();
                        }

                        endPos = cursor.position();
                        if (!cursor.movePosition (QTextCursor::PreviousBlock))
                            return QTextCursor();
                        cursor.movePosition (QTextCursor::EndOfBlock);
                        ++i;
                    }
                }
                else if (i != sl.count() - 1) // the middle strings
                {
                    if (QString::compare (cursor.block().text(), sl.at (sl.count() - i - 1), cs) != 0)
                    { // reset the loop if the block text doesn't match
                        cursor.setPosition (endPos);
                        if (!cursor.movePosition (QTextCursor::PreviousBlock))
                            return QTextCursor();
                        cursor.movePosition (QTextCursor::EndOfBlock);
                        i = 0;
                        continue;
                    }

                    if (!cursor.movePosition (QTextCursor::PreviousBlock))
                        return QTextCursor();
                    cursor.movePosition (QTextCursor::EndOfBlock);
                    ++i;
                }
                else // the first string
                {
                    subStr = sl.at (0);
                    if (subStr.isEmpty()) break;
                    if (!(flags & QTextDocument::FindWholeWords))
                    {
                        /* when the first string doesn't end the previous block... */
                        if (!cursor.block().text().endsWith (subStr, cs))
                        {
                            /* ... reset the loop */
                            cursor.setPosition (endPos);
                            if (!cursor.movePosition (QTextCursor::PreviousBlock))
                                return QTextCursor();
                            cursor.movePosition (QTextCursor::EndOfBlock);
                            i = 0;
                            continue;
                        }
                        cursor.setPosition (cursor.anchor() - subStr.count());
                        break;
                    }
                    else
                    {
                        found = cursor; // block end
                        if (!findBackward (txtdoc, subStr, found, flags)
                            || found.position() != cursor.position())
                        {
                            cursor.setPosition (endPos);
                            if (!cursor.movePosition (QTextCursor::PreviousBlock))
                                return QTextCursor();
                            cursor.movePosition (QTextCursor::EndOfBlock);
                            i = 0;
                            continue;
                        }
                        cursor.setPosition (found.anchor());
                        break;
                    }
                }
            }
            res.setPosition (cursor.anchor());
            res.setPosition (endPos, QTextCursor::KeepAnchor);
        }
    }
    else // there's no line break
    {
        if (!(flags & QTextDocument::FindBackward))
        {
            res = txtdoc->find (str, start, flags);
            if (end > 0 && res.anchor() > end)
                return QTextCursor();
        }
        else
            findBackward (txtdoc, str, res, flags);
    }

    return res;
}
/*************************/
void FPwin::find (bool forward)
{
    if (!isReady()) return;

    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    TabPage *tabPage = qobject_cast< TabPage *>(ui->tabWidget->widget (index));
    TextEdit *textEdit = tabPage->textEdit();
    QString txt = tabPage->searchEntry();
    bool newSrch = false;
    if (textEdit->getSearchedText() != txt)
    {
        textEdit->setSearchedText (txt);
        newSrch = true;
    }

    disconnect (textEdit, &TextEdit::resized, this, &FPwin::hlight);
    disconnect (textEdit, &TextEdit::updateRect, this, &FPwin::hlighting);
    disconnect (textEdit, &QPlainTextEdit::textChanged, this, &FPwin::hlight);

    if (txt.isEmpty())
    {
        /* remove all yellow and green highlights */
        QList<QTextEdit::ExtraSelection> es;
        textEdit->setGreenSel (es); // not needed
        if (ui->actionLineNumbers->isChecked() || ui->spinBox->isVisible())
            es.prepend (textEdit->currentLineSelection());
        es.append (textEdit->getRedSel());
        textEdit->setExtraSelections (es);
        return;
    }

    QTextDocument::FindFlags searchFlags = getSearchFlags();
    QTextDocument::FindFlags newFlags = searchFlags;
    if (!forward)
        newFlags = searchFlags | QTextDocument::FindBackward;
    QTextCursor start = textEdit->textCursor();
    QTextCursor found = finding (txt, start, newFlags);

    if (found.isNull())
    {
        if (!forward)
            start.movePosition (QTextCursor::End, QTextCursor::MoveAnchor);
        else
            start.movePosition (QTextCursor::Start, QTextCursor::MoveAnchor);
        found = finding (txt, start, newFlags);
    }

    if (!found.isNull())
    {
        start.setPosition (found.anchor());
        /* this is needed for selectionChanged() to be emitted */
        if (newSrch) textEdit->setTextCursor (start);
        start.setPosition (found.position(), QTextCursor::KeepAnchor);
        textEdit->setTextCursor (start);
    }
    /* matches highlights should come here, after the text area is
       scrolled and even when no match is found (it may be added later) */
    hlight();
    connect (textEdit, &QPlainTextEdit::textChanged, this, &FPwin::hlight);
    connect (textEdit, &TextEdit::updateRect, this, &FPwin::hlighting);
    connect (textEdit, &TextEdit::resized, this, &FPwin::hlight);
}
/*************************/
// Highlight found matches in the visible part of the text.
void FPwin::hlight() const
{
    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    TabPage *tabPage = qobject_cast< TabPage *>(ui->tabWidget->widget (index));
    TextEdit *textEdit = tabPage->textEdit();

    QString txt = textEdit->getSearchedText();
    if (txt.isEmpty()) return;

    QTextDocument::FindFlags searchFlags = getSearchFlags();

    /* prepend green highlights */
    QList<QTextEdit::ExtraSelection> es = textEdit->getGreenSel();
    QColor color = QColor (textEdit->hasDarkScheme() ? QColor (255, 255, 0,
                                                               /* a quadratic equation for darkValue -> opacity: 0 -> 90,  27 -> 75, 50 -> 65 */
                                                               (qreal)(textEdit->getDarkValue() * (75 * textEdit->getDarkValue() - 19275)) / (qreal)31050 + 90)
                                                     : Qt::yellow);
    QTextCursor found;
    /* first put a start cursor at the top left edge... */
    QPoint Point (0, 0);
    QTextCursor start = textEdit->cursorForPosition (Point);
    /* ... then move it backward by the search text length */
    int startPos = start.position() - txt.length();
    if (startPos >= 0)
        start.setPosition (startPos);
    else
        start.setPosition (0);
    int w = textEdit->geometry().width();
    int h = textEdit->geometry().height();
    /* get the visible text to check if
       the search string is inside it */
    Point = QPoint (w, h);
    QTextCursor end = textEdit->cursorForPosition (Point);
    int endLimit = end.anchor();
    int endPos = end.position() + txt.length();
    end.movePosition (QTextCursor::End);
    if (endPos <= end.position())
        end.setPosition (endPos);
    QTextCursor visCur = start;
    visCur.setPosition (end.position(), QTextCursor::KeepAnchor);
    QString str = visCur.selection().toPlainText(); // '\n' is included in this way
    Qt::CaseSensitivity cs = Qt::CaseInsensitive;
    if (tabPage->matchCase()) cs = Qt::CaseSensitive;
    while (str.contains (txt, cs) // don't waste time if the searched text isn't visible
           && !(found = finding (txt, start, searchFlags, endLimit)).isNull())
    {
        QTextEdit::ExtraSelection extra;
        extra.format.setBackground (color);
        extra.cursor = found;
        es.append (extra);
        start.setPosition (found.position());
    }

    /* also prepend the current line highlight,
       so that it always comes first when it exists */
    if (ui->actionLineNumbers->isChecked() || ui->spinBox->isVisible())
        es.prepend (textEdit->currentLineSelection());
    /* append red highlights */
    es.append (textEdit->getRedSel());
    textEdit->setExtraSelections (es);
}
/*************************/
void FPwin::hlighting (const QRect&, int dy) const
{
    if (dy) hlight();
}
/*************************/
void FPwin::searchFlagChanged()
{
    if (!isReady()) return;

    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    /* deselect text for consistency */
    TextEdit *textEdit = qobject_cast< TabPage *>(ui->tabWidget->widget (index))->textEdit();
    QTextCursor start = textEdit->textCursor();
    if (start.hasSelection())
    {
        start.setPosition (start.anchor());
        textEdit->setTextCursor (start);
    }

    hlight();
}
/*************************/
QTextDocument::FindFlags FPwin::getSearchFlags() const
{
    TabPage *tabPage = qobject_cast< TabPage *>(ui->tabWidget->currentWidget());
    QTextDocument::FindFlags searchFlags = 0;
    if (tabPage->matchWhole())
        searchFlags = QTextDocument::FindWholeWords;
    if (tabPage->matchCase())
        searchFlags |= QTextDocument::FindCaseSensitively;
    return searchFlags;
}

}
