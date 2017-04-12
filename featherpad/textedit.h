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

#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <QtGui>
#include <QPlainTextEdit>

namespace FeatherPad {

/* This is for auto-indentation, line numbers, DnD, zooming, customized
   vertical scrollbar, appropriate signals, and saving/getting useful info. */
class TextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    TextEdit (QWidget *parent = 0, int bgColorValue = 255);
    ~TextEdit();

    void setTextCursor (const QTextCursor &cursor)
    {
        QPlainTextEdit::setTextCursor (cursor);
        /* this is needed for formatVisibleText() to be called (for syntax highlighting) */
        emit QPlainTextEdit::updateRequest (rect(), 1);
    }

    void lineNumberAreaPaintEvent (QPaintEvent *event);
    int lineNumberAreaWidth();
    void showLineNumbers (bool show);

    QTextEdit::ExtraSelection currentLineSelection() {
        return currentLine;
    }

    void setAutoIndentation (bool indent) {
        autoIndentation = indent;
    }
    bool getAutoIndentation() const {
        return autoIndentation;
    }

    bool hasDarkScheme() const {
        return darkScheme;
    }

    void setScrollJumpWorkaround (bool apply){
        scrollJumpWorkaround = apply;
    }

    void zooming (float range);

    qint64 getSize() const {
        return size_;
    }
    void setSize (qint64 size) {
        size_ = size;
    }

    int getWordNumber() const {
        return wordNumber_;
    }
    void setWordNumber (int n) {
        wordNumber_ = n;
    }

    QString getSearchedText() const {
        return searchedText_;
    }
    void setSearchedText (QString text) {
        searchedText_ = text;
    }

    QString getReplaceTitle() const {
        return replaceTitle_;
    }
    void setReplaceTitle (QString title) {
        replaceTitle_ = title;
    }

    QString getFileName() const {
        return fileName_;
    }
    void setFileName (QString name) {
        fileName_ = name;
    }

    QString getProg() const {
        return prog_;
    }
    void setProg (QString prog) {
        prog_ = prog;
    }

    QString getEncoding() const {
        return encoding_;
    }
    void setEncoding (QString encoding) {
        encoding_ = encoding;
    }

    QList<QTextEdit::ExtraSelection> getGreenSel() const {
        return greenSel_;
    }
    void setGreenSel (QList<QTextEdit::ExtraSelection> sel) {
        greenSel_ = sel;
    }
    QList<QTextEdit::ExtraSelection> getRedSel() const {
        return redSel_;
    }
    void setRedSel (QList<QTextEdit::ExtraSelection> sel) {
        redSel_ = sel;
    }

    QSyntaxHighlighter *getHighlighter() const {
        return highlighter_;
    }
    void setHighlighter (QSyntaxHighlighter *h) {
        highlighter_ = h;
    }

signals:
    /* inform the main widget */
    void fileDropped (const QString& localFile,
                      bool multiple); // Multiple files are dropped?
    void resized(); // needed by syntax highlighting
    void updateRect (const QRect &rect, int dy);
    void zoomedOut (TextEdit *textEdit); // needed for reformatting text

protected:
    virtual void keyPressEvent (QKeyEvent *event)
    {
        if (isReadOnly())
        {
            QPlainTextEdit::keyPressEvent (event);
            return;
        }

        if (autoIndentation && (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter))
        {
            /* first get the current cursor for computing the indentation */
            QTextCursor start = textCursor();
            QString indent = computeIndentation (start);
            /* then press Enter normally... */
            QPlainTextEdit::keyPressEvent (event);
            /* ... and insert indentation */
            start = textCursor();
            start.insertText (indent);
            event->accept();
            return;
        }
        else if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right)
        {
            /* when text is selected, use arrow keys
               to go to the start or end of the selection */
            QTextCursor cursor = textCursor();
            if (event->modifiers() == Qt::NoModifier && cursor.hasSelection())
            {
                QString str = cursor.selectedText();
                if (event->key() == Qt::Key_Left)
                {
                    if (str.isRightToLeft())
                        cursor.setPosition (cursor.selectionEnd());
                    else
                        cursor.setPosition (cursor.selectionStart());
                }
                else
                {
                    if (str.isRightToLeft())
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
        /* because of a bug in Qt5, the non-breaking space (ZWNJ) isn't inserted with SHIFT+SPACE */
        else if (event->key() == 0x200c)
        {
            insertPlainText (QChar (0x200C));
            event->accept();
            return;
        }

        QPlainTextEdit::keyPressEvent (event);
    }

    // a workaround for Qt5's scroll jump bug
    virtual void wheelEvent (QWheelEvent *event)
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

    void resizeEvent (QResizeEvent *event);
    void timerEvent (QTimerEvent *event);

    /* we want to pass dropping of files to
       the main widget with a custom signal */
    bool canInsertFromMimeData (const QMimeData* source) const
    {
        return source->hasUrls() || QPlainTextEdit::canInsertFromMimeData (source);
    }
    void insertFromMimeData (const QMimeData* source)
    {
        if (source->hasUrls())
        {
            bool multiple (source->urls().count() > 1);
            foreach (QUrl url, source->urls())
                emit fileDropped (url.adjusted (QUrl::NormalizePathSegments) // KDE may give a double slash
                                     .toLocalFile(),
                                  multiple);
        }
        else
            QPlainTextEdit::insertFromMimeData (source);
    }

public slots:
    void updateEditorGeometry();

private slots:
    void updateLineNumberAreaWidth (int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea (const QRect&, int);
    void onUpdateRequesting (const QRect&, int dy);

private:
    QString computeIndentation (QTextCursor& cur) const
    {
        if (cur.hasSelection())
        {// this is more intuitive to me
            if (cur.anchor() <= cur.position())
                cur.setPosition (cur.anchor());
            else
                cur.setPosition (cur.position());
        }
        QTextCursor tmp = cur;
        tmp.movePosition (QTextCursor::StartOfBlock);
        QString str;
        if (tmp.atBlockEnd())
            return str;
        int pos = tmp.position();
        tmp.setPosition (++pos, QTextCursor::KeepAnchor);
        QString selected;
        while (!tmp.atBlockEnd()
               && tmp <= cur
               && ((selected = tmp.selectedText()) == " "
                   || (selected = tmp.selectedText()) == "\t"))
        {
            str.append (selected);
            tmp.setPosition (pos);
            tmp.setPosition (++pos, QTextCursor::KeepAnchor);
        }
        if (tmp.atBlockEnd()
            && tmp <= cur
            && ((selected = tmp.selectedText()) == " "
                || (selected = tmp.selectedText()) == "\t"))
        {
            str.append (selected);
        }
        return str;
    }

    QWidget *lineNumberArea;
    QTextEdit::ExtraSelection currentLine;
    bool autoIndentation;
    bool scrollJumpWorkaround; // for working around Qt5's scroll jump bug
    bool darkScheme;
    QColor lineHColor;
    int resizeTimerId, updateTimerId; // for not wasting CPU's time
    int Dy;
    /********************************************
     ***** All needed information on a page *****
     ********************************************/
    qint64 size_; // file size for limiting syntax highlighting (the file may be removed)
    int wordNumber_; // the calculated number of words (-1 if not counted yet)
    QString searchedText_; // the text that is being searched in the documnet
    QString replaceTitle_; // the title of the Replacement dock (can change)
    QString fileName_; // opened file
    QString prog_; // programming language (for syntax highlighting)
    QString encoding_; // text encoding (UTF-8 by default)
    /*
       Highlighting order: (1) current line;
                           (2) replacing;
                           (3) found matches;
                           (4) bracket matches.
    */
    QList<QTextEdit::ExtraSelection> greenSel_; // for replaced matches
    QList<QTextEdit::ExtraSelection> redSel_; // for bracket matches
    QSyntaxHighlighter *highlighter_; // // syntax highlighter
};
/*************************/
class LineNumberArea : public QWidget
{
public:
    LineNumberArea (TextEdit *Editor) : QWidget (Editor) {
        editor = Editor;
    }

    QSize sizeHint() const {
        return QSize (editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) {
        editor->lineNumberAreaPaintEvent (event);
    }

private:
    TextEdit *editor;
};

}

#endif // TEXTEDIT_H
