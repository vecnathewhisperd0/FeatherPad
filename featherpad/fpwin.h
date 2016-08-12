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

#ifndef FPWIN_H
#define FPWIN_H

#include <QMainWindow>
#include <QActionGroup>
#include "highlighter.h"
#include "textedit.h"
#include "config.h"

namespace FeatherPad {

namespace Ui {
class FPwin;
}

// All needed information on a tab.
struct tabInfo
{
    QString fileName; // Opened file.
    qint64 size; // File size for limiting syntax highlighting (the file may be removed).
    QString searchEntry; // Search entry.
    QString encoding; // Text encoding.
    QString prog; // Language for syntax highlighting.
    int wordNumber;
    Highlighter *highlighter; // Syntax highlighter.
    /*
       Highlighting order: (1) current line;
                           (2) replacing;
                           (3) found matches;
                           (4) bracket matches.
    */
    QList<QTextEdit::ExtraSelection> greenSel; // For replaced matches.
    QList<QTextEdit::ExtraSelection> redSel; // For bracket matches.
};

// A FeatherPad window.
class FPwin : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit FPwin (QWidget *parent = 0);
    ~FPwin();

public slots:
    void newTabFromName (const QString& fileName);
    void newTab();
    void statusMsg();
    void statusMsgWithLineCount (const int lines);
    void wordButtonStatus();

private slots:
    void closeTab();
    void closeTabAtIndex (int index);
    void closeAllTabs();
    void closeRightTabs();
    void closeLeftTabs();
    void closeOtherTabs();
    void fileOpen();
    void reload();
    void enforceEncoding (QAction*);
    bool fileSave();
    void cutText();
    void copyText();
    void pasteText();
    void deleteText();
    void selectAllText();
    void makeEditable();
    void undoing();
    void redoing();
    void tabSwitch (int index);
    void fontDialog();
    void find();
    void hlight() const;
    void hlighting (const QRect&, int dy) const;
    void setSearchFlags();
    void showHideSearch() const;
    void showLN (bool checked);
    void toggleSyntaxHighlighting();
    void toggleWrapping();
    void toggleIndent();
    void replace();
    void replaceAll();
    void closeReplaceDock (bool visible);
    void replaceDock();
    void resizeDock (bool topLevel);
    void jumpTo();
    void setMax (const int max);
    void goTo();
    void asterisk (bool modified);
    void zoomIn();
    void zoomOut();
    void zoomZero();
    void fullScreening();
    void defaultSize();
    void align();
    void docProp();
    void filePrint();
    void detachTab();
    void detachAndDropTab (QPoint& dropPos);
    void tabContextMenu (const QPoint& p);
    void prefDialog();
    void aboutDialog();
    void helpDoc();
    void matchBrackets();
    void addText (const QString text, const QString fileName, const QString charset,
                  bool enforceEncod, bool reload);
    void updateEditorGeometry();

public:
    QWidget *dummyWidget; // Bypasses KDE's demand for a new window.
    Ui::FPwin *ui;
    QHash<TextEdit*,tabInfo*> tabsInfo_; // Needed info on each tab.

private:
    void loadText (const QString& fileName, bool enforceEncod, bool reload);
    bool alreadyOpen (const QString& fileName) const;
    void setTitle (const QString& fileName);
    int unSaved (int index, bool noToAll);
    void closeEvent (QCloseEvent *event);
    bool closeTabs (int leftIndx, int rightIndx, bool closeall);
    void dragEnterEvent (QDragEnterEvent *event);
    void dropEvent (QDropEvent *event);
    void resizeEvent (QResizeEvent *event);
    void changeEvent (QEvent *event);
    void setSearchFlagsAndHighlight (bool h);
    void enableWidgets (bool enable) const;
    QTextCursor finding (const QString str, const QTextCursor& start, QTextDocument::FindFlags flags = 0) const;
    void getSyntax (const int index);
    void syntaxHighlighting (const int index);
    void encodingToCheck (const QString& encoding);
    const QString checkToEncoding() const;
    void applyConfig();
    bool matchLeftParenthesis (QTextBlock currentBlock, int index, int numRightParentheses);
    bool matchRightParenthesis (QTextBlock currentBlock, int index, int numLeftParentheses);
    bool matchLeftBrace (QTextBlock currentBlock, int index, int numRightBraces);
    bool matchRightBrace (QTextBlock currentBlock, int index, int numLeftBraces);
    void createSelection (int pos);

    QActionGroup *aGroup_;
    QString lastFile_; // The last opened file.
    QTextDocument::FindFlags searchFlags_; // Whole word and case sensitivity flags (for all tabs).
    QString txtReplace_; // The replacing text.
    int rightClicked_; // The index of the right-clicked tab.
    bool closeAll_; // Should all tabs be closed? (Needed only in closeOtherTabs().)
};

}

#endif // FPWIN_H
