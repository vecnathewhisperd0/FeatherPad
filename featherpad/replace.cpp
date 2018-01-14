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

namespace FeatherPad {

void FPwin::removeGreenSel()
{
    /* remove green highlights, considering the selection order, namely,
       current line -> replacement -> found matches -> bracket matches */
    int count = ui->tabWidget->count();
    for (int i = 0; i < count; ++i)
    {
        TextEdit *textEdit = qobject_cast< TabPage *>(ui->tabWidget->widget (i))->textEdit();
        QTextEdit::ExtraSelection curLineSel;
        QList<QTextEdit::ExtraSelection> es = textEdit->extraSelections();
        if (ui->actionLineNumbers->isChecked() || ui->spinBox->isVisible())
        {
            curLineSel = textEdit->currentLineSelection();
            if (!es.isEmpty())
                es.removeFirst();
        }
        int n = textEdit->getGreenSel().count();
        while (n > 0 && !es.isEmpty())
        {
            es.removeFirst();
            --n;
        }
        es.prepend (curLineSel);
        textEdit->setGreenSel (QList<QTextEdit::ExtraSelection>());
        textEdit->setExtraSelections (es);
    }
}
/*************************/
void FPwin::replaceDock()
{
    if (!isReady()) return;

    if (!ui->dockReplace->isVisible())
    {
        int count = ui->tabWidget->count();
        for (int i = 0; i < count; ++i) // replace dock needs searchbar
            qobject_cast< TabPage *>(ui->tabWidget->widget (i))->setSearchBarVisible (true);
        ui->dockReplace->setWindowTitle (tr ("Rep&lacement"));
        ui->dockReplace->setVisible (true);
        ui->dockReplace->activateWindow();
        ui->dockReplace->raise();
        if (!ui->lineEditFind->hasFocus())
            ui->lineEditFind->setFocus();
        return;
    }

    ui->dockReplace->setVisible (false);
    // closeReplaceDock(false) is automatically called here
}
/*************************/
// When the dock becomes invisible, clear the replacing text and remove only green highlights.
// Although it doesn't concern us, when docking or undocking, the widget first becomes invisible
// for a moment and then visible again.
void FPwin::closeReplaceDock (bool visible)
{
    if (visible) return;

    txtReplace_.clear();
    removeGreenSel();

    /* return focus to the document and remove the title
       (other titles will be removed by tabSwitch()) */
    if (ui->tabWidget->count() > 0)
    {
        TextEdit *textEdit = qobject_cast< TabPage *>(ui->tabWidget->currentWidget())->textEdit();
        textEdit->setFocus();
        textEdit->setReplaceTitle (QString());
    }
}
/*************************/
// Resize the floating dock widget to its minimum size.
void FPwin::resizeDock (bool topLevel)
{
    if (topLevel)
        ui->dockReplace->resize (ui->dockReplace->minimumWidth(),
                                 ui->dockReplace->minimumHeight());
}
/*************************/
void FPwin::replace()
{
    if (!isReady()) return;

    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    TextEdit *textEdit = qobject_cast< TabPage *>(ui->tabWidget->widget (index))->textEdit();
    if (textEdit->isReadOnly()) return;

    textEdit->setReplaceTitle (QString());
    ui->dockReplace->setWindowTitle (tr ("Rep&lacement"));

    QString txtFind = ui->lineEditFind->text();
    if (txtFind.isEmpty()) return;

    /* remove previous green highlights if the replacing text is changed */
    if (txtReplace_ != ui->lineEditReplace->text())
    {
        txtReplace_ = ui->lineEditReplace->text();
        removeGreenSel();
    }

    bool lineNumShown (ui->actionLineNumbers->isChecked() || ui->spinBox->isVisible());

    /* remember all previous (yellow and) green highlights */
    QList<QTextEdit::ExtraSelection> es = textEdit->extraSelections();
    int n = textEdit->getRedSel().count();
    while (n > 0 && !es.isEmpty())
    {
        es.removeLast();
        --n;
    }
    if (!es.isEmpty() && lineNumShown)
        es.removeFirst();

    QTextDocument::FindFlags searchFlags = getSearchFlags();
    QTextCursor start = textEdit->textCursor();
    QTextCursor tmp = start;
    QTextCursor found;
    if (QObject::sender() == ui->toolButtonNext)
        found = finding (txtFind, start, searchFlags);
    else// if (QObject::sender() == ui->toolButtonPrv)
        found = finding (txtFind, start, searchFlags | QTextDocument::FindBackward);
    QColor color = QColor (textEdit->hasDarkScheme() ? Qt::darkGreen : Qt::green);
    int pos;
    QList<QTextEdit::ExtraSelection> gsel = textEdit->getGreenSel();
    if (!found.isNull())
    {
        start.setPosition (found.anchor());
        pos = found.anchor();
        start.setPosition (found.position(), QTextCursor::KeepAnchor);
        textEdit->setTextCursor (start);
        textEdit->insertPlainText (txtReplace_);

        start = textEdit->textCursor();
        tmp.setPosition (pos);
        tmp.setPosition (start.position(), QTextCursor::KeepAnchor);
        QTextEdit::ExtraSelection extra;
        extra.format.setBackground (color);
        extra.cursor = tmp;
        es.prepend (extra);
        gsel.append (extra);
    }
    textEdit->setGreenSel (gsel);
    if (lineNumShown)
        es.prepend (textEdit->currentLineSelection());
    /* append red highlights */
    es.append (textEdit->getRedSel());
    textEdit->setExtraSelections (es);
    /* yellow highlights may need correction */
    hlight();
}
/*************************/
void FPwin::replaceAll()
{
    if (!isReady()) return;

    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    TextEdit *textEdit = qobject_cast< TabPage *>(ui->tabWidget->widget (index))->textEdit();
    if (textEdit->isReadOnly()) return;

    QString txtFind = ui->lineEditFind->text();
    if (txtFind.isEmpty()) return;

    /* remove previous green highlights if the replacing text is changed */
    if (txtReplace_ != ui->lineEditReplace->text())
    {
        txtReplace_ = ui->lineEditReplace->text();
        removeGreenSel();
    }

    QTextDocument::FindFlags searchFlags = getSearchFlags();

    QTextCursor orig = textEdit->textCursor();
    QTextCursor start = orig;
    QColor color = QColor (textEdit->hasDarkScheme() ? Qt::darkGreen : Qt::green);
    int pos; QTextCursor found;
    start.beginEditBlock();
    start.setPosition (0);
    QTextCursor tmp = start;
    QList<QTextEdit::ExtraSelection> gsel = textEdit->getGreenSel();
    QList<QTextEdit::ExtraSelection> es;
    int count = 0;
    while (!(found = finding (txtFind, start, searchFlags)).isNull())
    {
        start.setPosition (found.anchor());
        pos = found.anchor();
        start.setPosition (found.position(), QTextCursor::KeepAnchor);
        start.insertText (txtReplace_);

        tmp.setPosition (pos);
        tmp.setPosition (start.position(), QTextCursor::KeepAnchor);
        start.setPosition (start.position());
        QTextEdit::ExtraSelection extra;
        extra.format.setBackground (color);
        extra.cursor = tmp;
        es.prepend (extra);
        gsel.append (extra);
        ++count;
    }
    textEdit->setGreenSel (gsel);
    start.endEditBlock();
    if ((ui->actionLineNumbers->isChecked() || ui->spinBox->isVisible()))
        es.prepend (textEdit->currentLineSelection());
    es.append (textEdit->getRedSel());
    textEdit->setExtraSelections (es);
    hlight();
    /* restore the original cursor without selection */
    orig.setPosition (orig.anchor());
    textEdit->setTextCursor (orig);

    QString title;
    if (count == 0)
        title = tr ("No Replacement");
    else if (count == 1)
        title =  tr ("One Replacement");
    else
        title = QString ("%1 ").arg (count) + tr ("Replacements");
    ui->dockReplace->setWindowTitle (title);
    textEdit->setReplaceTitle (title);
}

}
