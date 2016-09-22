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

#include "fpwin.h"
#include "ui_fp.h"

namespace FeatherPad {

// This method extends the searchable strings to those with line breaks.
// It also corrects the behavior of Qt's backward search.
QTextCursor FPwin::finding (const QString str, const QTextCursor& start, QTextDocument::FindFlags flags) const
{
    /* let's be consistent first */
    if (ui->tabWidget->currentIndex() == -1 || str.isEmpty())
        return QTextCursor(); // null cursor

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->currentWidget());
    QTextDocument *txtdoc = textEdit->document();
    QTextCursor cursor = QTextCursor (start);
    QTextCursor res = QTextCursor (start);
    if (str.contains ('\n'))
    {
        QTextCursor found;
        QStringList sl = str.split ("\n");
        int i = 0;
        if (!(flags & QTextDocument::FindBackward))
        {
            /* this loop searches for the consecutive
               occurrences of newline separated strings */
            while (i < sl.count())
            {
                if (i == 0)
                {
                    /* when the first substring is empty... */
                    if (sl.at (0).isEmpty())
                    {
                        /* ... continue the search from the next block */
                        cursor.movePosition (QTextCursor::EndOfBlock);
                        res.setPosition (cursor.position());
                        if (!cursor.movePosition (QTextCursor::NextBlock))
                            return QTextCursor();
                        ++i;
                    }
                    else if (!(found = txtdoc->find (sl.at (0), cursor, flags)).isNull())
                    {
                        cursor.setPosition (found.position());
                        if (!cursor.atBlockEnd())
                        {
                            cursor.movePosition (QTextCursor::EndOfBlock);
                            cursor.setPosition (cursor.position() - sl.at (0).length());
                            continue;
                        }

                        res.setPosition (found.anchor());
                        if (!cursor.movePosition (QTextCursor::NextBlock))
                            return QTextCursor();
                        ++i;
                    }
                    else
                        return QTextCursor();
                }
                else if (i != sl.count() - 1)
                {
                    /* when the next block's test isn't the next substring... */
                    if (cursor.block().text() != sl.at (i))
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
                else// if (i == sl.count() - 1)
                {
                    if (sl.at (i).isEmpty()) break;
                    /* when the last substring doesn't start the next block... */
                    if (!cursor.block().text().startsWith (sl.at (i))
                        || (found = txtdoc->find (sl.at (i), cursor, flags)).isNull()
                        || found.anchor() != cursor.position())
                    {
                        /* ... reset the loop cautiously */
                        cursor.setPosition (res.position());
                        if (!cursor.movePosition (QTextCursor::NextBlock))
                            return QTextCursor();
                        i = 0;
                        continue;
                    }
                    cursor.setPosition (found.position());
                    ++i;
                }
            }
            res.setPosition (cursor.position(), QTextCursor::KeepAnchor);
        }
        else // backward search
        {
            /* we'll keep "anchor" before "position"
               because Qt does so in backward search */
            int endPos = cursor.position();
            while (i < sl.count())
            {
                if (i == 0)
                {
                    if (sl.at (sl.count() - 1).isEmpty())
                    {
                        cursor.movePosition (QTextCursor::StartOfBlock);
                        endPos = cursor.position();
                        if (!cursor.movePosition (QTextCursor::PreviousBlock))
                            return QTextCursor();
                        ++i;
                    }
                    else if (!(found = txtdoc->find (sl.at (sl.count() - 1), cursor, flags)).isNull())
                    {
                        cursor.setPosition (found.anchor());
                        if (!cursor.atBlockStart())
                        {
                            /* Qt's backward search finds a string
                               even when the cursor is inside it */
                            continue;
                        }

                        endPos = found.position();
                        if (!cursor.movePosition (QTextCursor::PreviousBlock))
                            return QTextCursor();
                        cursor.movePosition (QTextCursor::EndOfBlock);
                        ++i;
                    }
                    else
                        return QTextCursor();
                }
                else if (i != sl.count() - 1)
                {
                    if (cursor.block().text() != sl.at (sl.count() - i - 1))
                    {
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
                else
                {
                    if (sl.at (0).isEmpty()) break;
                    if (!cursor.block().text().endsWith (sl.at (0))
                        || (found = txtdoc->find (sl.at (0), cursor, flags)).isNull()
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
                    ++i;
                }
            }
            res.setPosition (cursor.anchor());
            res.setPosition (endPos, QTextCursor::KeepAnchor);
        }
    }
    else // there's no line break
    {
        res = txtdoc->find (str, start, flags);
        /* when the cursor is inside the backward searched string */
        if ((flags & QTextDocument::FindBackward)
            && start.position() - res.anchor() < str.length())
        {
            res.setPosition (res.anchor());
            res = txtdoc->find (str, res, flags);
        }
    }

    return res;
}
/*************************/
void FPwin::find()
{
    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    tabInfo *tabinfo = tabsInfo_[textEdit];
    QString txt = ui->lineEdit->text();
    bool newSrch = false;
    if (tabinfo->searchEntry != txt)
    {
        tabinfo->searchEntry = txt;
        newSrch = true;
    }

    disconnect (textEdit, &TextEdit::updateRect, this, &FPwin::hlighting);
    disconnect (textEdit, &QPlainTextEdit::textChanged, this, &FPwin::hlight);

    if (txt.isEmpty())
    {
        /* remove all yellow and green highlights */
        QList<QTextEdit::ExtraSelection> extraSelections;
        tabinfo->greenSel = extraSelections; // not needed
        if (ui->actionLineNumbers->isChecked() || ui->spinBox->isVisible())
            extraSelections.prepend (textEdit->currentLineSelection());
        extraSelections.append (tabinfo->redSel);
        textEdit->setExtraSelections (extraSelections);
        return;
    }

    setSearchFlagsAndHighlight (false);
    QTextDocument::FindFlags newFlags = searchFlags_;
    if (QObject::sender() == ui->toolButton_prv)
        newFlags = searchFlags_ | QTextDocument::FindBackward;

    QTextCursor start = textEdit->textCursor();
    QTextCursor found = finding (txt, start, newFlags);

    if (found.isNull())
    {
        if (QObject::sender() == ui->toolButton_prv)
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
}
/*************************/
// Highlight found matches in the visible part of the text.
void FPwin::hlight() const
{
    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    tabInfo *tabinfo = tabsInfo_[textEdit];

    QString txt = tabinfo->searchEntry;
    if (txt.isEmpty()) return;

    QList<QTextEdit::ExtraSelection> extraSelections;
    /* prepend green highlights */
    extraSelections.append (tabinfo->greenSel);
    QColor color = QColor (textEdit->hasDarkScheme() ? QColor (115, 115, 0) : Qt::yellow);
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
    int h = textEdit->geometry().height();
    int w = textEdit->geometry().width();
    /* get the visible text to check if
       the search string is inside it */
    Point = QPoint (h, w);
    QTextCursor end = textEdit->cursorForPosition (Point);
    int endPos = end.position() + txt.length();
    end.movePosition (QTextCursor::End);
    if (endPos <= end.position())
        end.setPosition (endPos);
    QTextCursor visCur = start;
    visCur.setPosition (end.position(), QTextCursor::KeepAnchor);
    QString str = visCur.selection().toPlainText(); // '\n' is included in this way
    Qt::CaseSensitivity cs = Qt::CaseInsensitive;
    if (ui->pushButton_case->isChecked()) cs = Qt::CaseSensitive;
    while (str.contains (txt, cs) // don't waste time if the seach text isn't visible
           && !(found = finding (txt, start, searchFlags_)).isNull())
    {
        QTextEdit::ExtraSelection extra;
        extra.format.setBackground (color);
        extra.cursor = found;
        extraSelections.append (extra);
        start.setPosition (found.position());
        if (textEdit->cursorRect (start).top() >= h) break;
    }

    /* also prepend the current line highlight,
       so that it always comes first when it exists */
    if (ui->actionLineNumbers->isChecked() || ui->spinBox->isVisible())
        extraSelections.prepend (textEdit->currentLineSelection());
    /* append red highlights */
    extraSelections.append (tabinfo->redSel);
    textEdit->setExtraSelections (extraSelections);
}
/*************************/
void FPwin::hlighting (const QRect&, int dy) const
{
    if (dy) hlight();
}
/*************************/
void FPwin::setSearchFlagsAndHighlight (bool h)
{
    if (ui->pushButton_whole->isChecked() && ui->pushButton_case->isChecked())
        searchFlags_ = QTextDocument::FindWholeWords | QTextDocument::FindCaseSensitively;
    else if (ui->pushButton_whole->isChecked() && !ui->pushButton_case->isChecked())
        searchFlags_ = QTextDocument::FindWholeWords;
    else if (!ui->pushButton_whole->isChecked() && ui->pushButton_case->isChecked())
        searchFlags_ = QTextDocument::FindCaseSensitively;
    else
        searchFlags_ = 0;

    /* deselect text for consistency */
    if (QObject::sender() == ui->pushButton_case || (QObject::sender() == ui->pushButton_whole))
    {
        QHash<TextEdit*,tabInfo*>::iterator it;
        for (it = tabsInfo_.begin(); it != tabsInfo_.end(); ++it)
        {
            QTextCursor start = it.key()->textCursor();
            if (start.hasSelection())
            {
                start.setPosition (start.anchor());
                it.key()->setTextCursor (start);
            }
        }

    }

    if (h) hlight();
}
/*************************/
void FPwin::setSearchFlags()
{
    setSearchFlagsAndHighlight (true);
}
/*************************/
void FPwin::showHideSearch()
{
    bool visibility = ui->lineEdit->isVisible();

    ui->lineEdit->setVisible (!visibility);
    ui->pushButton_case->setVisible (!visibility);
    ui->toolButton_nxt->setVisible (!visibility);
    ui->toolButton_prv->setVisible (!visibility);
    ui->pushButton_whole->setVisible (!visibility);

    if (!visibility)
        ui->lineEdit->setFocus();
    else
    {
        /* return focus to the document,... */
        qobject_cast< TextEdit *>(ui->tabWidget->currentWidget())->setFocus();
        /* ... empty all search entries,... */
        ui->lineEdit->clear();
        /* ... and remove all yellow and green highlights */
        int count = ui->tabWidget->count();
        if (count > 0)
        {
            for (int index = 0; index < count; ++index)
            {
                TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
                tabInfo *tabinfo = tabsInfo_[textEdit];
                tabinfo->searchEntry = QString();
                QList<QTextEdit::ExtraSelection> extraSelections;
                tabinfo->greenSel = extraSelections; // not needed
                if (ui->actionLineNumbers->isChecked() || ui->spinBox->isVisible())
                    extraSelections.prepend (textEdit->currentLineSelection());
                extraSelections.append (tabinfo->redSel);
                textEdit->setExtraSelections (extraSelections);
            }
        }
    }
}

}
