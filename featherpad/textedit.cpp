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

TextEdit::TextEdit (QWidget *parent, int bgColorValue) : QPlainTextEdit (parent)
{
    autoIndentation = true;
    scrollJumpWorkaround = false;

    /* set the backgound color and ensure enough contrast
       between the selection and line highlight colors */
    QPalette p = palette();
    if (bgColorValue < 230 && bgColorValue > 50) // not good for a text editor
        bgColorValue = 230;
    if (bgColorValue < 230)
    {
        darkScheme = true;
        lineHColor = QColor (Qt::gray).darker (210); // a value of 78
        viewport()->setStyleSheet (QString (".QWidget {"
                                            "color: white;"
                                            "background-color: rgb(%1, %1, %1);}")
                                   .arg (bgColorValue));
        int lineHGray = qGray (lineHColor.rgb());
        if (qGray (p.highlight().color().rgb()) - lineHGray < 30)
        {
            setStyleSheet ("QPlainTextEdit {"
                           "selection-background-color: rgb(180, 180, 180);"
                           "selection-color: black;}");
        }
        else if (qGray (p.color (QPalette::Inactive, QPalette::Highlight).rgb()) - lineHGray < 30)
        { // also check the inactive highlight color
            p.setColor (QPalette::Inactive, QPalette::Highlight, p.highlight().color());
            setPalette (p);
        }
    }
    else
    {
        darkScheme = false;
        lineHColor = QColor (Qt::gray).lighter (130); // a value of 213
        viewport()->setStyleSheet (QString (".QWidget {"
                                            "color: black;"
                                            "background-color: rgb(%1, %1, %1);}")
                                   .arg (bgColorValue));
        int lineHGray = qGray (lineHColor.rgb());
        if (lineHGray - qGray (p.highlight().color().rgb()) < 30)
        {
            setStyleSheet ("QPlainTextEdit {"
                           "selection-background-color: rgb(100, 100, 100);"
                           "selection-color: white;}");
        }
        else if (lineHGray - qGray (p.color (QPalette::Inactive, QPalette::Highlight).rgb()) < 30)
        {
            p.setColor (QPalette::Inactive, QPalette::Highlight, p.highlight().color());
            setPalette (p);
        }
    }

    resizeTimerId = 0;
    updateTimerId = 0;
    Dy = 0;
    size_ = 0;
    wordNumber_ = -1; // not calculated yet
    encoding_= "UTF-8";
    highlighter_ = nullptr;
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
        QList<QTextEdit::ExtraSelection> es = extraSelections();
        if (!es.isEmpty() && !currentLine.cursor.isNull())
            es.removeFirst();
        setExtraSelections (es);
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
    /* keep yellow and green highlights
       (related to serching and replacing) */
    QList<QTextEdit::ExtraSelection> es = extraSelections();
    if (!es.isEmpty() && !currentLine.cursor.isNull())
        es.removeFirst(); // line highlight always comes first when it exists

    currentLine.format.setBackground (lineHColor);
    currentLine.format.setProperty (QTextFormat::FullWidthSelection, true);
    currentLine.cursor = textCursor();
    currentLine.cursor.clearSelection();
    es.prepend (currentLine);

    setExtraSelections (es);
}
/*************************/
void TextEdit::lineNumberAreaPaintEvent (QPaintEvent *event)
{
    QPainter painter (lineNumberArea);
    painter.fillRect (event->rect(), darkScheme ? Qt::lightGray : Qt::black);


    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry (block).translated (contentOffset()).top();
    int bottom = top + (int) blockBoundingRect (block).height();

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number (blockNumber + 1);
            painter.setPen (darkScheme ? Qt::black : Qt::white);
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
/*************************/
void TextEdit::zooming (float range)
{
    if (range == 0.f) return;
    QFont f = document()->defaultFont();
    const float newSize = f.pointSizeF() + range;
    if (newSize <= 0) return;
    f.setPointSizeF (newSize);
    setFont (f);
    QFontMetrics metrics (f);
    setTabStopWidth (4 * metrics.width (' '));

    /* if this is a zoom-out, the text will need
       to be formatted and/or highlighted again */
    if (range < 0) emit zoomedOut (this);
}

}
