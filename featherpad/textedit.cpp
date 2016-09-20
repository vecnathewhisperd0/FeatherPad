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

#include "textedit.h"
#include "vscrollbar.h"

namespace FeatherPad {

TextEdit::TextEdit (QWidget *parent) : QPlainTextEdit (parent), autoIndentation (true)
{
    scrollJumpWorkaround = false;
    resizeTimerId = 0;
    updateTimerId = 0;
    Dy = 0;
    setFrameShape (QFrame::NoFrame);
    /* first we replace the widget's vertical scrollbar with ours because
       we want faster wheel scrolling when the mouse cursor is on the scrollbar */
    VScrollBar *vScrollBar = new VScrollBar;
    setVerticalScrollBar (vScrollBar);

    lineNumberArea = new LineNumberArea (this);
    lineNumberArea->hide();

    connect (this, &QPlainTextEdit::updateRequest, this, &TextEdit::onUpdateRequesting);
}
/*************************/
TextEdit::~TextEdit()
{
    delete lineNumberArea;
}
/*************************/
void TextEdit::showLineNumbers (bool show)
{
    if (show)
    {
        lineNumberArea->show();
        connect (this, &QPlainTextEdit::blockCountChanged, this, &TextEdit::updateLineNumberAreaWidth);
        connect (this, &QPlainTextEdit::updateRequest, this, &TextEdit::updateLineNumberArea);
        connect (this, &QPlainTextEdit::cursorPositionChanged, this, &TextEdit::highlightCurrentLine);

        updateLineNumberAreaWidth (0);
        highlightCurrentLine();
    }
    else
    {
        disconnect (this, &QPlainTextEdit::blockCountChanged, this, &TextEdit::updateLineNumberAreaWidth);
        disconnect (this, &QPlainTextEdit::updateRequest, this, &TextEdit::updateLineNumberArea);
        disconnect (this, &QPlainTextEdit::cursorPositionChanged, this, &TextEdit::highlightCurrentLine);

        lineNumberArea->hide();
        setViewportMargins (0, 0, 0, 0);
        QList<QTextEdit::ExtraSelection> extraSelections;
        extraSelections.append (this->extraSelections());
        if (!extraSelections.isEmpty() && !currentLine.cursor.isNull())
            extraSelections.removeFirst();
        setExtraSelections (extraSelections);
        currentLine.cursor = QTextCursor(); // nullify currentLine
    }
}
/*************************/
int TextEdit::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax (1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    /* 4 = 2 + 2 */
    int space = 4 + fontMetrics().width (QLatin1Char ('9')) * digits;

    return space;
}
/*************************/
void TextEdit::updateLineNumberAreaWidth (int /* newBlockCount */)
{
    setViewportMargins (lineNumberAreaWidth(), 0, 0, 0);
}
/*************************/
void TextEdit::updateLineNumberArea (const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll (0, dy);
    else
        lineNumberArea->update (0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains (viewport()->rect()))
        updateLineNumberAreaWidth (0);
}
/*************************/
void TextEdit::resizeEvent (QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent (e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry (QRect (cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));

    if (resizeTimerId)
    {
        killTimer (resizeTimerId);
        resizeTimerId = 0;
    }
    resizeTimerId = startTimer (50);
}
/*************************/
void TextEdit::timerEvent (QTimerEvent *e)
{
    QPlainTextEdit::timerEvent (e);

    if (e->timerId() == resizeTimerId)
    {
        killTimer (e->timerId());
        resizeTimerId = 0;
        emit resized();
    }
    else if (e->timerId() == updateTimerId)
    {
        killTimer (e->timerId());
        updateTimerId = 0;
        /* we use TextEdit's rect because the last rect that
           updateRequest() provides after 50ms may be null */
        emit updateRect (rect(), Dy);
    }
}
/*************************/
void TextEdit::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    /* keep yellow and green highlights
       (related to serching and replacing) */
    extraSelections.append (this->extraSelections());
    if (!extraSelections.isEmpty() && !currentLine.cursor.isNull())
        extraSelections.removeFirst(); // line highlight always comes first when it exists

    QColor lineColor = QColor (Qt::gray).lighter (130);

    currentLine.format.setBackground (lineColor);
    currentLine.format.setProperty (QTextFormat::FullWidthSelection, true);
    currentLine.cursor = textCursor();
    currentLine.cursor.clearSelection();
    extraSelections.prepend (currentLine);

    setExtraSelections (extraSelections);
}
/*************************/
void TextEdit::lineNumberAreaPaintEvent (QPaintEvent *event)
{
    QPainter painter (lineNumberArea);
    painter.fillRect (event->rect(), Qt::black);


    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry (block).translated (contentOffset()).top();
    int bottom = top + (int) blockBoundingRect (block).height();

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number (blockNumber + 1);
            painter.setPen (Qt::white);
            painter.drawText (0, top, lineNumberArea->width() - 2, fontMetrics().height(),
                              Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int)blockBoundingRect (block).height();
        ++blockNumber;
    }
}
/*************************/
void TextEdit::updateEditorGeometry()
{
    updateGeometry();
}
/*************************/
void TextEdit::onUpdateRequesting (const QRect& /*rect*/, int dy)
{
    if (updateTimerId)
    {
        killTimer (updateTimerId);
        updateTimerId = 0;
        if (Dy == 0 || dy != 0) // dy can be zero at the end of 50ms
            Dy = dy;
    }
    else Dy = dy;

    updateTimerId = startTimer (50);
}

}
