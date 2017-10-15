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
    autoBracket = false;
    scrollJumpWorkaround = false;
    drawIndetLines = false;

    //document()->setUseDesignMetrics (true);

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
        QColor col = p.highlight().color();
        if (qGray (col.rgb()) - lineHGray < 30 && col.hslSaturation() < 100)
        {
            setStyleSheet ("QPlainTextEdit {"
                           "selection-background-color: rgb(180, 180, 180);"
                           "selection-color: black;}");
        }
        else
        {
            col = p.color (QPalette::Inactive, QPalette::Highlight);
            if (qGray (col.rgb()) - lineHGray < 30 && col.hslSaturation() < 100)
            { // also check the inactive highlight color
                p.setColor (QPalette::Inactive, QPalette::Highlight, p.highlight().color());
                p.setColor (QPalette::Inactive, QPalette::HighlightedText, p.highlightedText().color());
                setPalette (p);
            }
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
        QColor col = p.highlight().color();
        if (lineHGray - qGray (col.rgb()) < 30 && col.hslSaturation() < 100)
        {
            setStyleSheet ("QPlainTextEdit {"
                           "selection-background-color: rgb(100, 100, 100);"
                           "selection-color: white;}");
        }
        else
        {
            col = p.color (QPalette::Inactive, QPalette::Highlight);
            if (lineHGray - qGray (col.rgb()) < 30 && col.hslSaturation() < 100)
            {
                p.setColor (QPalette::Inactive, QPalette::Highlight, p.highlight().color());
                p.setColor (QPalette::Inactive, QPalette::HighlightedText, p.highlightedText().color());
                setPalette (p);
            }
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
void TextEdit::setEditorFont (const QFont &f)
{
    setFont (f);
    viewport()->setFont (f); // needed when whitespaces are shown
    lineNumberArea->setFont (f);
    document()->setDefaultFont (f);
    /* we want consistent tabs */
    QFontMetricsF metrics (f);
    QTextOption opt = document()->defaultTextOption();
    opt.setTabStop (metrics.width ("    "));
    document()->setDefaultTextOption (opt);
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

    /* 4 = 2 + 2 (-> lineNumberAreaPaintEvent) */
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
QString TextEdit::computeIndentation (const QTextCursor &cur) const
{
    QTextCursor cusror = cur;
    if (cusror.hasSelection())
    {// this is more intuitive to me
        if (cusror.anchor() <= cusror.position())
            cusror.setPosition (cusror.anchor());
        else
            cusror.setPosition (cusror.position());
    }
    QTextCursor tmp = cusror;
    tmp.movePosition (QTextCursor::StartOfBlock);
    QString str;
    if (tmp.atBlockEnd())
        return str;
    int pos = tmp.position();
    tmp.setPosition (++pos, QTextCursor::KeepAnchor);
    QString selected;
    while (!tmp.atBlockEnd()
           && tmp <= cusror
           && ((selected = tmp.selectedText()) == " "
               || (selected = tmp.selectedText()) == "\t"))
    {
        str.append (selected);
        tmp.setPosition (pos);
        tmp.setPosition (++pos, QTextCursor::KeepAnchor);
    }
    if (tmp.atBlockEnd()
        && tmp <= cusror
        && ((selected = tmp.selectedText()) == " "
            || (selected = tmp.selectedText()) == "\t"))
    {
        str.append (selected);
    }
    return str;
}
/*************************/
static inline bool isOnlySpaces (const QString &str)
{
    int i = 0;
    while (i < str.size())
    { // always skip the starting spaces
        QChar ch = str.at (i);
        if (ch == QChar (QChar::Space) || ch == QChar (QChar::Tabulation))
            ++i;
        else return false;
    }
    return true;
}

void TextEdit::keyPressEvent (QKeyEvent *event)
{
    if (isReadOnly())
    {
        QPlainTextEdit::keyPressEvent (event);
        return;
    }

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        QTextCursor cur = textCursor();
        bool isBracketed (false);
        QString selTxt = cur.selectedText();
        QString prefix, indent;
        bool withShift (event->modifiers() & Qt::ShiftModifier);

        /* with Shift+Enter, find the non-letter prefix */
        if (withShift)
        {
            cur.clearSelection();
            setTextCursor (cur);
            QString blockText = cur.block().text();
            int i = 0;
            int curBlockPos = cur.position() - cur.block().position();
            while (i < curBlockPos)
            {
                QChar ch = blockText.at (i);
                if (!ch.isLetterOrNumber())
                {
                    prefix += ch;
                    ++i;
                }
                else break;
            }
        }
        else
        {
            /* find the indentation */
            if (autoIndentation)
                indent = computeIndentation (cur);
            /* check whether a bracketed text is selected
               so that the cursor position is at its start */
            QTextCursor anchorCur = cur;
            anchorCur.setPosition (cur.anchor());
            if (autoBracket
                && cur.position() == cur.selectionStart()
                && !cur.atBlockStart() && !anchorCur.atBlockEnd())
            {
                cur.setPosition (cur.position());
                cur.movePosition (QTextCursor::PreviousCharacter);
                cur.movePosition (QTextCursor::NextCharacter,
                                  QTextCursor::KeepAnchor,
                                  selTxt.size() + 2);
                QString selTxt1 = cur.selectedText();
                if (selTxt1 == "{" + selTxt + "}" || selTxt1 == "(" + selTxt + ")")
                    isBracketed = true;
                cur = textCursor(); // reset the current cursor
            }
        }

        if (withShift || autoIndentation || isBracketed)
        {
            cur.beginEditBlock();
            /* first press Enter normally... */
            cur.insertText (QChar (QChar::ParagraphSeparator));
            /* ... then, insert indentation... */
            cur.insertText (indent);
            /* ... and handle Shift+Enter or brackets */
            if (withShift)
                cur.insertText (prefix);
            else if (isBracketed)
            {
                cur.movePosition (QTextCursor::PreviousBlock);
                cur.movePosition (QTextCursor::EndOfBlock);
                int start = -1;
                QStringList lines = selTxt.split (QChar (QChar::ParagraphSeparator));
                if (lines.size() == 1)
                {
                    cur.insertText (QChar (QChar::ParagraphSeparator));
                    cur.insertText (indent);
                    start = cur.position();
                    if (!isOnlySpaces (lines. at (0)))
                        cur.insertText (lines. at (0));
                }
                else // multi-line
                {
                    for (int i = 0; i < lines.size(); ++i)
                    {
                        if (i == 0 && isOnlySpaces (lines. at (0)))
                            continue;
                        cur.insertText (QChar (QChar::ParagraphSeparator));
                        if (i == 0)
                        {
                            cur.insertText (indent);
                            start = cur.position();
                        }
                        else if (i == 1 && start == -1)
                            start = cur.position(); // the first line was only spaces
                        cur.insertText (lines. at (i));
                    }
                }
                cur.setPosition (start, start >= cur.block().position()
                                            ? QTextCursor::MoveAnchor
                                            : QTextCursor::KeepAnchor);
                setTextCursor (cur);
            }
            cur.endEditBlock();
            event->accept();
            return;
        }
    }
    else if (event->key() == Qt::Key_ParenLeft
             || event->key() == Qt::Key_BraceLeft
             || event->key() == Qt::Key_BracketLeft
             || event->key() == Qt::Key_QuoteDbl)
    {
        if (autoBracket)
        {
            QTextCursor cursor = textCursor();
            bool autoB (false);
            if (!cursor.hasSelection())
            {
                if (cursor.atBlockEnd())
                    autoB = true;
                else
                {
                    QTextCursor tmp = cursor;
                    tmp.movePosition (QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                    if (!tmp.selectedText().at (0).isLetterOrNumber())
                        autoB = true;
                }
            }
            else if (cursor.position() == cursor.selectionStart())
                autoB = true;
            if (autoB)
            {
                int pos = cursor.position();
                int anch = cursor.anchor();
                cursor.beginEditBlock();
                cursor.setPosition (anch);
                if (event->key() == Qt::Key_ParenLeft)
                {
                    cursor.insertText (")");
                    cursor.setPosition (pos);
                    cursor.insertText ("(");
                }
                else if (event->key() == Qt::Key_BraceLeft)
                {
                    cursor.insertText ("}");
                    cursor.setPosition (pos);
                    cursor.insertText ("{");
                }
                else if (event->key() == Qt::Key_BracketLeft)
                {
                    cursor.insertText ("]");
                    cursor.setPosition (pos);
                    cursor.insertText ("[");
                }
                else// if (event->key() == Qt::Key_QuoteDbl)
                {
                    cursor.insertText ("\"");
                    cursor.setPosition (pos);
                    cursor.insertText ("\"");
                }
                /* select the text and set the cursor at its start */
                cursor.setPosition (anch + 1, QTextCursor::MoveAnchor);
                cursor.setPosition (pos + 1, QTextCursor::KeepAnchor);
                setTextCursor (cursor);
                cursor.endEditBlock();
                event->accept();
                return;
            }
        }
    }
    else if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right)
    {
        /* when text is selected, use arrow keys
           to go to the start or end of the selection */
        QTextCursor cursor = textCursor();
        if (event->modifiers() == Qt::NoModifier && cursor.hasSelection())
        {
            QString selTxt = cursor.selectedText();
            if (event->key() == Qt::Key_Left)
            {
                if (selTxt.isRightToLeft())
                    cursor.setPosition (cursor.selectionEnd());
                else
                    cursor.setPosition (cursor.selectionStart());
            }
            else
            {
                if (selTxt.isRightToLeft())
                    cursor.setPosition (cursor.selectionStart());
                else
                    cursor.setPosition (cursor.selectionEnd());
            }
            cursor.clearSelection();
            setTextCursor (cursor);
            event->accept();
            return;
        }
    }
    else if (event->key() == Qt::Key_Tab)
    {
        QTextCursor cursor = textCursor();
        int newLines = cursor.selectedText().count (QChar (QChar::ParagraphSeparator));
        if (newLines > 0)
        {
            cursor.beginEditBlock();
            if (cursor.anchor() <= cursor.position())
                cursor.setPosition (cursor.anchor());
            else
                cursor.setPosition (cursor.position());
            cursor.movePosition (QTextCursor::StartOfBlock);
            for (int i = 0; i <= newLines; ++i)
            {
                if (event->modifiers() & Qt::ControlModifier)
                {
                    if (event->modifiers() & Qt::MetaModifier)
                        cursor.insertText ("  ");
                    else
                        cursor.insertText ("    ");
                }
                else
                    cursor.insertText ("\t");
                if (!cursor.movePosition (QTextCursor::NextBlock))
                    break; // not needed
            }
            cursor.endEditBlock();
            event->accept();
            return;
        }
        else if (!cursor.hasSelection() && (event->modifiers() & Qt::ControlModifier))
        {
            if (event->modifiers() & Qt::MetaModifier)
                cursor.insertText ("  ");
            else
                cursor.insertText ("    ");
            event->accept();
            return;
        }
    }
    else if (event->key() == Qt::Key_Backtab)
    {
        QTextCursor cursor = textCursor();
        int newLines = cursor.selectedText().count (QChar (QChar::ParagraphSeparator));
        if (cursor.anchor() <= cursor.position())
            cursor.setPosition (cursor.anchor());
        else
            cursor.setPosition (cursor.position());
        cursor.beginEditBlock();
        cursor.movePosition (QTextCursor::StartOfBlock);
        for (int i = 0; i <= newLines; ++i)
        {
            if (cursor.atBlockEnd())
            {
                if (!cursor.movePosition (QTextCursor::NextBlock))
                    break; // not needed
                continue;
            }
            cursor.movePosition (QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            QString selTxt = cursor.selectedText();
            if (selTxt == " ")
            { // remove 4 successive ordinary spaces at most
                for (int i = 0; i < 3; ++i)
                {
                    if (cursor.atBlockEnd())
                        break;
                    cursor.movePosition (QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                    QString newSelTxt = cursor.selectedText();
                    if (newSelTxt != selTxt + " ")
                    {
                        cursor.movePosition (QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
                        break;
                    }
                    else
                        selTxt = newSelTxt;
                }
                cursor.removeSelectedText();
            }
            else if (selTxt == "\t")
                cursor.removeSelectedText();
            if (!cursor.movePosition (QTextCursor::NextBlock))
                break; // not needed
        }
        cursor.endEditBlock();

        /* otherwise, do nothing with SHIFT+TAB */
        event->accept();
        return;
    }
    /* because of a bug in Qt5, the non-breaking space (ZWNJ) isn't inserted with SHIFT+SPACE */
    else if (event->key() == 0x200c)
    {
        insertPlainText (QChar (0x200C));
        event->accept();
        return;
    }

    QPlainTextEdit::keyPressEvent (event);
}
/*************************/
// A workaround for Qt5's scroll jump bug
void TextEdit::wheelEvent (QWheelEvent *event)
{
    if (scrollJumpWorkaround && event->angleDelta().manhattanLength() > 240)
        event->ignore();
    else
    {
        if (event->modifiers() & Qt::ControlModifier)
        {
            float delta = event->angleDelta().y() / 120.f;
            zooming (delta);
            return;
        }
        /* as in QPlainTextEdit::wheelEvent() */
        QAbstractScrollArea::wheelEvent (event);
        updateMicroFocus();
    }
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
/*******************************************************
***** Workaround for the RTL bug in QPlainTextEdit *****
********************************************************/
static void fillBackground (QPainter *p, const QRectF &rect, QBrush brush, const QRectF &gradientRect = QRectF())
{
    p->save();
    if (brush.style() >= Qt::LinearGradientPattern && brush.style() <= Qt::ConicalGradientPattern)
    {
        if (!gradientRect.isNull())
        {
            QTransform m = QTransform::fromTranslate (gradientRect.left(), gradientRect.top());
            m.scale (gradientRect.width(), gradientRect.height());
            brush.setTransform (m);
            const_cast<QGradient *>(brush.gradient())->setCoordinateMode (QGradient::LogicalMode);
        }
    }
    else
        p->setBrushOrigin(rect.topLeft());
    p->fillRect (rect, brush);
    p->restore();
}
// Exactly like QPlainTextEdit::paintEvent(),
// except for setting layout text option for RTL
// and drawing vertical indentation lines (if needed).
void TextEdit::paintEvent (QPaintEvent *event)
{
    QPainter painter (viewport());
    Q_ASSERT (qobject_cast<QPlainTextDocumentLayout*>(document()->documentLayout()));

    QPointF offset (contentOffset());

    QRect er = event->rect();
    QRect viewportRect = viewport()->rect();
    qreal maximumWidth = document()->documentLayout()->documentSize().width();
    painter.setBrushOrigin (offset);

    int maxX = offset.x() + qMax ((qreal)viewportRect.width(), maximumWidth)
               - document()->documentMargin();
    er.setRight (qMin (er.right(), maxX));
    painter.setClipRect (er);

    bool editable = !isReadOnly();
    QAbstractTextDocumentLayout::PaintContext context = getPaintContext();
    QTextBlock block = firstVisibleBlock();
    while (block.isValid())
    {
        QRectF r = blockBoundingRect (block).translated (offset);
        QTextLayout *layout = block.layout();

        if (!block.isVisible())
        {
            offset.ry() += r.height();
            block = block.next();
            continue;
        }

        if (r.bottom() >= er.top() && r.top() <= er.bottom())
        {
            /* take care of RTL */
            bool rtl (block.text().isRightToLeft());
            if (rtl)
            {
                QTextOption opt = document()->defaultTextOption();
                opt = QTextOption (Qt::AlignRight);
                opt.setTextDirection (Qt::RightToLeft);
                layout->setTextOption (opt);
            }

            QTextBlockFormat blockFormat = block.blockFormat();
            QBrush bg = blockFormat.background();
            if (bg != Qt::NoBrush)
            {
                QRectF contentsRect = r;
                contentsRect.setWidth (qMax (r.width(), maximumWidth));
                fillBackground (&painter, contentsRect, bg);
            }

            QVector<QTextLayout::FormatRange> selections;
            int blpos = block.position();
            int bllen = block.length();
            for (int i = 0; i < context.selections.size(); ++i)
            {
                const QAbstractTextDocumentLayout::Selection &range = context.selections.at (i);
                const int selStart = range.cursor.selectionStart() - blpos;
                const int selEnd = range.cursor.selectionEnd() - blpos;
                if (selStart < bllen && selEnd > 0
                    && selEnd > selStart)
                {
                    QTextLayout::FormatRange o;
                    o.start = selStart;
                    o.length = selEnd - selStart;
                    o.format = range.format;
                    selections.append (o);
                }
                else if (!range.cursor.hasSelection() && range.format.hasProperty (QTextFormat::FullWidthSelection)
                         && block.contains(range.cursor.position()))
                {
                    QTextLayout::FormatRange o;
                    QTextLine l = layout->lineForTextPosition (range.cursor.position() - blpos);
                    o.start = l.textStart();
                    o.length = l.textLength();
                    if (o.start + o.length == bllen - 1)
                        ++o.length; // include newline
                    o.format = range.format;
                    selections.append (o);
                }
            }

            bool drawCursor ((editable || (textInteractionFlags() & Qt::TextSelectableByKeyboard))
                             && context.cursorPosition >= blpos
                             && context.cursorPosition < blpos + bllen);
            bool drawCursorAsBlock (drawCursor && overwriteMode());

            if (drawCursorAsBlock)
            {
                if (context.cursorPosition == blpos + bllen - 1)
                    drawCursorAsBlock = false;
                else
                {
                    QTextLayout::FormatRange o;
                    o.start = context.cursorPosition - blpos;
                    o.length = 1;
                    o.format.setForeground (palette().base());
                    o.format.setBackground (palette().text());
                    selections.append (o);
                }
            }

            if (!placeholderText().isEmpty() && document()->isEmpty())
            {
                QColor col = palette().text().color();
                col.setAlpha (128);
                painter.setPen (col);
                const int margin = int(document()->documentMargin());
                painter.drawText (r.adjusted (margin, 0, 0, 0), Qt::AlignTop | Qt::TextWordWrap, placeholderText());
            }
            else
                layout->draw (&painter, offset, selections, er);
            if ((drawCursor && !drawCursorAsBlock)
                || (editable && context.cursorPosition < -1
                    && !layout->preeditAreaText().isEmpty()))
            {
                int cpos = context.cursorPosition;
                if (cpos < -1)
                    cpos = layout->preeditAreaPosition() - (cpos + 2);
                else
                    cpos -= blpos;
                layout->drawCursor (&painter, offset, cpos, cursorWidth());
            }

            /* indentation lines should be drawn after selections */
            if (drawIndetLines
                && !rtl) // no indentation line with RTL (for now)
            {
                QRegExp exp = QRegExp ("\\s+");
                if (exp.indexIn (block.text()) == 0)
                {
                    painter.save();
                    painter.setOpacity (0.2);
                    int len = exp.matchedLength();
                    QTextCursor cur = textCursor();
                    cur.setPosition (block.position() + len);
                    qreal rightMost = cursorRect (cur).right();
                    QFontMetricsF fm = QFontMetricsF (document()->defaultFont());
                    int yTop = qRound (r.topLeft().y());
                    int yBottom =  qRound (r.height() >= (qreal)2 * fm.lineSpacing()
                                               ? yTop + fm.height()
                                               : r.bottomLeft().y() - (qreal)1);
                    qreal x = r.topLeft().x();
                    qreal tabWidth = (qreal)fm.width ("    ");
                    x += tabWidth;
                    while (x <= rightMost)
                    {
                        painter.drawLine (QLine (qRound (x), yTop, qRound (x), yBottom));
                        x += tabWidth;
                    }
                    painter.restore();
                }
            }
        }

        offset.ry() += r.height();
        if (offset.y() > viewportRect.height())
            break;
        block = block.next();
    }

    if (backgroundVisible() && !block.isValid() && offset.y() <= er.bottom()
        && (centerOnScroll() || verticalScrollBar()->maximum() == verticalScrollBar()->minimum()))
    {
        painter.fillRect (QRect (QPoint ((int)er.left(), (int)offset.y()), er.bottomRight()), palette().background());
    }
}
/************************************************
***** End of the Workaround for the RTL bug *****
*************************************************/
void TextEdit::highlightCurrentLine()
{
    /* keep yellow and green highlights
       (related to searching and replacing) */
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
// This calls the private function _q_adjustScrollbars()
// by calling QPlainTextEdit::resizeEvent().
void TextEdit::adjustScrollbars()
{
    QSize vSize = viewport()->size();
    QResizeEvent *_resizeEvent = new QResizeEvent (vSize, vSize);
    QCoreApplication::postEvent (viewport(), _resizeEvent);
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
    setEditorFont (f);

    /* if this is a zoom-out, the text will need
       to be formatted and/or highlighted again */
    if (range < 0) emit zoomedOut (this);

    /* due to a Qt bug, this is needed for the
       scrollbar range to be updated correctly */
    adjustScrollbars();
}

}
