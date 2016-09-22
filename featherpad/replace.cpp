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

void FPwin::replaceDock()
{
    if (!ui->dockReplace->isVisible())
    {
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
    closeReplaceDock (false);
}
/*************************/
// When the dock is closed with its titlebar button,
// clear the replacing text and remove green highlights.
void FPwin::closeReplaceDock (bool visible)
{
    if (visible) return;

    txtReplace_.clear();
    /* remove green highlights */
    if (ui->actionLineNumbers->isChecked() || ui->spinBox->isVisible())
    {
        QHash<TextEdit*,tabInfo*>::iterator it;
        for (it = tabsInfo_.begin(); it != tabsInfo_.end(); ++it)
        {
            tabInfo *tabinfo = tabsInfo_[it.key()];
            QList<QTextEdit::ExtraSelection> extraSelections;
            tabinfo->greenSel = extraSelections;
            extraSelections.prepend (it.key()->currentLineSelection());
            extraSelections.append (tabinfo->redSel);
            it.key()->setExtraSelections (extraSelections);
        }
    }
    else
    {
        QHash<TextEdit*,tabInfo*>::iterator it;
        for (it = tabsInfo_.begin(); it != tabsInfo_.end(); ++it)
        {
            tabInfo *tabinfo = tabsInfo_[it.key()];
            QList<QTextEdit::ExtraSelection> extraSelections;
            tabinfo->greenSel = extraSelections;
            extraSelections.append (tabinfo->redSel);
            it.key()->setExtraSelections (extraSelections);
        }
    }
    hlight();

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

    bool lineNumShown = false;
    if (ui->actionLineNumbers->isChecked() || ui->spinBox->isVisible())
        lineNumShown = true;

    if (txtReplace_ != ui->lineEditReplace->text())
    {
        txtReplace_ = ui->lineEditReplace->text();
        /* remove previous green highlights
           if the replacing text is changed */
        if (lineNumShown)
        {
            QHash<TextEdit*,tabInfo*>::iterator it;
            for (it = tabsInfo_.begin(); it != tabsInfo_.end(); ++it)
            {
                tabInfo *tabinfoIth = tabsInfo_[it.key()];
                QList<QTextEdit::ExtraSelection> extraSelectionsIth;
                tabinfoIth->greenSel = extraSelectionsIth;
                extraSelectionsIth.prepend (it.key()->currentLineSelection());
                extraSelectionsIth.append (tabinfoIth->redSel);
                it.key()->setExtraSelections (extraSelectionsIth);
            }
        }
        else
        {
            QHash<TextEdit*,tabInfo*>::iterator it;
            for (it = tabsInfo_.begin(); it != tabsInfo_.end(); ++it)
            {
                tabInfo *tabinfoIth = tabsInfo_[it.key()];
                QList<QTextEdit::ExtraSelection> extraSelectionsIth;
                tabinfoIth->greenSel = extraSelectionsIth;
                extraSelectionsIth.append (tabinfoIth->redSel);
                it.key()->setExtraSelections (extraSelectionsIth);
            }
        }
        hlight();
    }

    /* remember all previous (yellow and) green highlights */
    QList<QTextEdit::ExtraSelection> extraSelections;
    extraSelections.append (textEdit->extraSelections());
    int n = tabinfo->redSel.count();
    while (n > 0)
    {
        extraSelections.removeLast();
        --n;
    }
    if (!extraSelections.isEmpty() && lineNumShown)
        extraSelections.removeFirst();

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
        extraSelections.prepend (extra);
        gsel.append (extra);
    }
    tabinfo->greenSel = gsel;
    if (lineNumShown)
        extraSelections.prepend (textEdit->currentLineSelection());
    /* append red highlights */
    extraSelections.append (tabinfo->redSel);
    textEdit->setExtraSelections (extraSelections);
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

    bool lineNumShown = false;
    if (ui->actionLineNumbers->isChecked() || ui->spinBox->isVisible())
        lineNumShown = true;

    if (txtReplace_ != ui->lineEditReplace->text())
    {
        txtReplace_ = ui->lineEditReplace->text();
        /* remove previous green highlights
           if the replacing text is changed */
        if (lineNumShown)
        {
            QHash<TextEdit*,tabInfo*>::iterator it;
            for (it = tabsInfo_.begin(); it != tabsInfo_.end(); ++it)
            {
                tabInfo *tabinfoIth = tabsInfo_[it.key()];
                QList<QTextEdit::ExtraSelection> extraSelectionsIth;
                tabinfoIth->greenSel = extraSelectionsIth;
                extraSelectionsIth.prepend (it.key()->currentLineSelection());
                extraSelectionsIth.append (tabinfoIth->redSel);
                it.key()->setExtraSelections (extraSelectionsIth);
            }
        }
        else
        {
            QHash<TextEdit*,tabInfo*>::iterator it;
            for (it = tabsInfo_.begin(); it != tabsInfo_.end(); ++it)
            {
                tabInfo *tabinfoIth = tabsInfo_[it.key()];
                QList<QTextEdit::ExtraSelection> extraSelectionsIth;
                tabinfoIth->greenSel = extraSelectionsIth;
                extraSelectionsIth.append (tabinfoIth->redSel);
                it.key()->setExtraSelections (extraSelectionsIth);
            }
        }
        hlight();
    }

    QTextCursor orig = textEdit->textCursor();
    QTextCursor start = orig;
    QColor color = QColor (textEdit->hasDarkScheme() ? Qt::darkGreen : Qt::green);
    int pos; QTextCursor found;
    start.beginEditBlock();
    start.setPosition (0);
    QTextCursor tmp = start;
    QList<QTextEdit::ExtraSelection> gsel = tabinfo->greenSel;
    QList<QTextEdit::ExtraSelection> extraSelections;
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
        extraSelections.prepend (extra);
        gsel.append (extra);
        ++count;
    }
    tabinfo->greenSel = gsel;
    start.endEditBlock();
    if (lineNumShown)
        extraSelections.prepend (textEdit->currentLineSelection());
    extraSelections.append (tabinfo->redSel);
    textEdit->setExtraSelections (extraSelections);
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
