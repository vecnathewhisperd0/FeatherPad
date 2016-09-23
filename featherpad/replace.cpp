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

void FPwin::removeGreenSel()
{
    /* remove green highlights, considering the selection order, namely,
       current line -> replacement -> found matches -> bracket matches */
    QHash<TextEdit*,tabInfo*>::iterator it;
    for (it = tabsInfo_.begin(); it != tabsInfo_.end(); ++it)
    {
        tabInfo *tabinfo = tabsInfo_[it.key()];
        QTextEdit::ExtraSelection curLineSel;
        QList<QTextEdit::ExtraSelection> es = it.key()->extraSelections();
        if (ui->actionLineNumbers->isChecked() || ui->spinBox->isVisible())
        {
            curLineSel = it.key()->currentLineSelection();
            if (!es.isEmpty())
                es.removeFirst();
        }
        int n = tabinfo->greenSel.count();
        while (n > 0 && !es.isEmpty())
        {
            es.removeFirst();
            --n;
        }
        es.prepend (curLineSel);
        tabinfo->greenSel = QList<QTextEdit::ExtraSelection>();
        it.key()->setExtraSelections (es);
    }
}
/*************************/
void FPwin::replaceDock()
{
    if (!ui->dockReplace->isVisible())
    {
        if (!ui->lineEdit->isVisible()) // replace dock needs searchbar
        {
            ui->lineEdit->setVisible (true);
            ui->pushButton_case->setVisible (true);
            ui->toolButton_nxt->setVisible (true);
            ui->toolButton_prv->setVisible (true);
            ui->pushButton_whole->setVisible (true);
        }
        ui->dockReplace->setWindowTitle (tr ("Replacement"));
        ui->dockReplace->setVisible (true);
        ui->dockReplace->setTabOrder (ui->lineEditFind, ui->lineEditReplace);
        ui->dockReplace->setTabOrder (ui->lineEditReplace, ui->toolButtonNext);
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

    /* return focus to the document */
    if (ui->tabWidget->count() > 0)
        qobject_cast< TextEdit *>(ui->tabWidget->currentWidget())->setFocus();
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
    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    if (textEdit->isReadOnly()) return;
    tabInfo *tabinfo = tabsInfo_[textEdit];

    ui->dockReplace->setWindowTitle (tr ("Replacement"));

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
    int n = tabinfo->redSel.count();
    while (n > 0 && !es.isEmpty())
    {
        es.removeLast();
        --n;
    }
    if (!es.isEmpty() && lineNumShown)
        es.removeFirst();

    QTextCursor start = textEdit->textCursor();
    QTextCursor tmp = start;
    QTextCursor found;
    if (QObject::sender() == ui->toolButtonNext)
        found = finding (txtFind, start, searchFlags_);
    else// if (QObject::sender() == ui->toolButtonPrv)
        found = finding (txtFind, start, searchFlags_ | QTextDocument::FindBackward);
    QColor color = QColor (textEdit->hasDarkScheme() ? Qt::darkGreen : Qt::green);
    int pos;
    QList<QTextEdit::ExtraSelection> gsel = tabinfo->greenSel;
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
    tabinfo->greenSel = gsel;
    if (lineNumShown)
        es.prepend (textEdit->currentLineSelection());
    /* append red highlights */
    es.append (tabinfo->redSel);
    textEdit->setExtraSelections (es);
    /* yellow highlights may need correction */
    hlight();
}
/*************************/
void FPwin::replaceAll()
{
    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    if (textEdit->isReadOnly()) return;
    tabInfo *tabinfo = tabsInfo_[textEdit];

    QString txtFind = ui->lineEditFind->text();
    if (txtFind.isEmpty()) return;

    /* remove previous green highlights if the replacing text is changed */
    if (txtReplace_ != ui->lineEditReplace->text())
    {
        txtReplace_ = ui->lineEditReplace->text();
        removeGreenSel();
    }

    QTextCursor orig = textEdit->textCursor();
    QTextCursor start = orig;
    QColor color = QColor (textEdit->hasDarkScheme() ? Qt::darkGreen : Qt::green);
    int pos; QTextCursor found;
    start.beginEditBlock();
    start.setPosition (0);
    QTextCursor tmp = start;
    QList<QTextEdit::ExtraSelection> gsel = tabinfo->greenSel;
    QList<QTextEdit::ExtraSelection> es;
    int count = 0;
    while (!(found = finding (txtFind, start, searchFlags_)).isNull())
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
    tabinfo->greenSel = gsel;
    start.endEditBlock();
    if ((ui->actionLineNumbers->isChecked() || ui->spinBox->isVisible()))
        es.prepend (textEdit->currentLineSelection());
    es.append (tabinfo->redSel);
    textEdit->setExtraSelections (es);
    hlight();
    /* restore the original cursor without selection */
    orig.setPosition (orig.anchor());
    textEdit->setTextCursor (orig);

    if (count == 0)
        ui->dockReplace->setWindowTitle (tr ("No Replacement"));
    else if (count == 1)
        ui->dockReplace->setWindowTitle (tr ("One Replacement"));
    else
        ui->dockReplace->setWindowTitle (tr ("%1 Replacements").arg (count));
}

}
