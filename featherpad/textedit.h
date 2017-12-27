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

    void setEditorFont (const QFont &f);

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

    void setDrawIndetLines (bool draw) {
        drawIndetLines = draw;
    }

    void setAutoBracket (bool autoB) {
        autoBracket = autoB;
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

    bool getInertialScrolling() const {
        return inertialScrolling_;
    }
    void setInertialScrolling (bool inertial) {
        inertialScrolling_ = inertial;
    }

    bool getSaveCursor() const {
        return saveCursor_;
    }
    void setSaveCursor (bool save) {
        saveCursor_ = save;
    }

signals:
    /* inform the main widget */
    void fileDropped (const QString& localFile,
                      bool saveCursor,
                      bool multiple); // Multiple files are dropped?
    void resized(); // needed by syntax highlighting
    void updateRect (const QRect &rect, int dy);
    void zoomedOut (TextEdit *textEdit); // needed for reformatting text
    void updateBracketMatching();

protected:
    void keyPressEvent (QKeyEvent *event);
    void wheelEvent (QWheelEvent *event);
    void resizeEvent (QResizeEvent *event);
    void timerEvent (QTimerEvent *event);
    void paintEvent (QPaintEvent *event); // only for working around the RTL bug

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
                                  false,
                                  multiple);
        }
        else
            QPlainTextEdit::insertFromMimeData (source);
    }

public slots:
    void adjustScrollbars();

private slots:
    void updateLineNumberAreaWidth (int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea (const QRect&, int);
    void onUpdateRequesting (const QRect&, int dy);
    void onSelectionChanged();
    void scrollWithInertia();

private:
    QString computeIndentation (const QTextCursor &cur) const;

    int prevAnchor, prevPos; // used only for bracket matching
    QWidget *lineNumberArea;
    QTextEdit::ExtraSelection currentLine;
    bool autoIndentation;
    bool drawIndetLines;
    bool autoBracket;
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
    QSyntaxHighlighter *highlighter_; // syntax highlighter
    bool saveCursor_;
    /******************************
     ***** Inertial scrolling *****
     ******************************/
    bool inertialScrolling_;
    struct scollData {
      int delta;
      int leftSteps;
      int totalSteps;
    };
    QTimer *scrollTimer_;
    QWheelEvent *wheelEvent_;
    QList<scollData> queuedScrollSteps_;
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
