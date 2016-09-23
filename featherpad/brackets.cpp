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

void FPwin::matchBrackets()
{
    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;
    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    tabInfo *tabinfo = tabsInfo_[textEdit];
    TextBlockData *data = static_cast<TextBlockData *>(textEdit->textCursor().block().userData());
    if (!data) return;

    QList<QTextEdit::ExtraSelection> es = textEdit->extraSelections();
    int n = tabinfo->redSel.count();
    while (n > 0
           && !es.isEmpty()) // just a precaution against crash
    {
        es.removeLast();
        --n;
    }
    tabinfo->redSel = QList<QTextEdit::ExtraSelection>();
    textEdit->setExtraSelections (es);

    /* position of block's first character */
    int blockPos = textEdit->textCursor().block().position();
    /* position of cursor in block */
    int curBlockPos = textEdit->textCursor().position() - blockPos;

    /* parenthesis */
    QVector<ParenthesisInfo *> infos = data->parentheses();
    for (int i = 0; i < infos.size(); ++i)
    {
        ParenthesisInfo *info = infos.at (i);

        if (info->position == curBlockPos && info->character == '(')
        {
            if (matchLeftParenthesis (textEdit->textCursor().block(), i + 1, 0))
            {
                createSelection (blockPos + info->position);
                break;
            }
        }
        else if (info->position == curBlockPos - 1 && info->character == ')')
        {
            if (matchRightParenthesis (textEdit->textCursor().block(), infos.size() - i, 0))
            {
                createSelection (blockPos + info->position);
                break;
            }
        }
    }

    /* bracket */
    QVector<BraceInfo *> braceInfos = data->braces();
    for (int i = 0; i < braceInfos.size(); ++i)
    {
        BraceInfo *info = braceInfos.at (i);

        if (info->position == curBlockPos && info->character == '{')
        {
            if (matchLeftBrace (textEdit->textCursor().block(), i + 1, 0))
            {
                createSelection (blockPos + info->position);
                break;
            }
        }
        else if (info->position == curBlockPos - 1 && info->character == '}')
        {
            if (matchRightBrace (textEdit->textCursor().block(), braceInfos.size() - i, 0))
            {
                createSelection (blockPos + info->position);
                break;
            }
        }
    }
}
/*************************/
bool FPwin::matchLeftParenthesis (QTextBlock currentBlock, int i, int numLeftParentheses)
{
    TextBlockData *data = static_cast<TextBlockData *>(currentBlock.userData());
    QVector<ParenthesisInfo *> infos = data->parentheses();

    int docPos = currentBlock.position();
    for (; i < infos.size(); ++i)
    {
        ParenthesisInfo *info = infos.at (i);
        if (info->character == '(')
        {
            ++numLeftParentheses;
            continue;
        }

        if (info->character == ')' && numLeftParentheses == 0)
        {
            createSelection (docPos + info->position);
            return true;
        }
        else
            --numLeftParentheses;
    }

    currentBlock = currentBlock.next();
    if (currentBlock.isValid())
        return matchLeftParenthesis (currentBlock, 0, numLeftParentheses);

    return false;
}
/*************************/
bool FPwin::matchRightParenthesis (QTextBlock currentBlock, int i, int numRightParentheses)
{
    TextBlockData *data = static_cast<TextBlockData *>(currentBlock.userData());
    QVector<ParenthesisInfo *> infos = data->parentheses();

    int docPos = currentBlock.position();
    for (; i < infos.size(); ++i)
    {
        ParenthesisInfo *info = infos.at (infos.size() - 1 - i);
        if (info->character == ')')
        {
            ++numRightParentheses;
            continue;
        }
        if (info->character == '(' && numRightParentheses == 0)
        {
            createSelection (docPos + info->position);
            return true;
        }
        else
            --numRightParentheses;
    }

    currentBlock = currentBlock.previous();
    if (currentBlock.isValid())
        return matchRightParenthesis (currentBlock, 0, numRightParentheses);

    return false;
}
/*************************/
bool FPwin::matchLeftBrace (QTextBlock currentBlock, int i, int numRightBraces)
{
    TextBlockData *data = static_cast<TextBlockData *>(currentBlock.userData());
    QVector<BraceInfo *> infos = data->braces();

    int docPos = currentBlock.position();
    for (; i < infos.size(); ++i)
    {
        BraceInfo *info = infos.at (i);
        if (info->character == '{')
        {
            ++numRightBraces;
            continue;
        }

        if (info->character == '}' && numRightBraces == 0)
        {
            createSelection (docPos + info->position);
            return true;
        }
        else
            --numRightBraces;
    }

    currentBlock = currentBlock.next();
    if (currentBlock.isValid())
        return matchLeftBrace (currentBlock, 0, numRightBraces);

    return false;
}
/*************************/
bool FPwin::matchRightBrace (QTextBlock currentBlock, int i, int numLeftBraces)
{
    TextBlockData *data = static_cast<TextBlockData *>(currentBlock.userData());
    QVector<BraceInfo *> infos = data->braces();

    int docPos = currentBlock.position();
    for (; i < infos.size(); ++i)
    {
        BraceInfo *info = infos.at (infos.size() - 1 - i);
        if (info->character == '}')
        {
            ++numLeftBraces;
            continue;
        }
        if (info->character == '{' && numLeftBraces == 0)
        {
            createSelection (docPos + info->position);
            return true;
        }
        else
            --numLeftBraces;
    }

    currentBlock = currentBlock.previous();
    if (currentBlock.isValid())
        return matchRightBrace (currentBlock, 0, numLeftBraces);

    return false;
}
/*************************/
void FPwin::createSelection (int pos)
{
    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;
    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    tabInfo *tabinfo = tabsInfo_[textEdit];

    QList<QTextEdit::ExtraSelection> es = textEdit->extraSelections();

    QTextCursor cursor = textEdit->textCursor();
    cursor.setPosition (pos);
    cursor.movePosition (QTextCursor::NextCharacter, QTextCursor::KeepAnchor);

    QTextEdit::ExtraSelection extra;
    extra.format.setBackground (textEdit->hasDarkScheme() ? QColor (190, 0, 3) : QColor (255, 150, 150));
    extra.cursor = cursor;

    QList<QTextEdit::ExtraSelection> rsel = tabinfo->redSel;
    rsel.append (extra);
    tabinfo->redSel = rsel;
    es.append (extra);

    textEdit->setExtraSelections (es);
}

}
