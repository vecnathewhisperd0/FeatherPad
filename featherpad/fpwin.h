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
#include "tabpage.h"
#include "config.h"

namespace FeatherPad {

namespace Ui {
class FPwin;
}

class BusyMaker : public QObject {
    Q_OBJECT

public:
    BusyMaker(){}
    ~BusyMaker(){}

public slots:
    void waiting();

private slots :
    void makeBusy();

signals:
    void finished();

private:
    static const int timeout = 1000;
};


// A FeatherPad window.
class FPwin : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit FPwin (QWidget *parent = 0);
    ~FPwin();

    bool isScriptLang (QString lang);

    bool isLoading() {
        return (loadingProcesses_ > 0);
    }
    bool isReady() {
        if (loadingProcesses_ <= 0)
        {
            closeWarningBar();
            return true;
        }
        return false;
    }

    void showCrashWarning();
    void updateCustomizableShortcuts (bool disable = false);

signals:
    void finishedLoading();

public slots:
    void newTabFromName (const QString& fileName, bool multiple = false);
    void newTab();
    void statusMsg();
    void statusMsgWithLineCount (const int lines);
    void updateWordInfo (int position = -1, int charsRemoved = 0, int charsAdded = 0);

private slots:
    void newTabFromRecent();
    void clearRecentMenu();
    void updateRecenMenu();
    void closeTab();
    void closeTabAtIndex (int index);
    void copyTabFileName();
    void copyTabFilePath();
    void closeAllTabs();
    void closeNextTabs();
    void closePreviousTabs();
    void closeOtherTabs();
    void fileOpen();
    void reload();
    void enforceEncoding (QAction*);
    void fileSave();
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
    void find (bool forward);
    void hlight() const;
    void hlighting (const QRect&, int dy) const;
    void searchFlagChanged();
    void showHideSearch();
    void showLN (bool checked);
    void toggleSyntaxHighlighting();
    void formatOnBlockChange (int) const;
    void formatOnTextChange (int, int charsRemoved, int charsAdded) const;
    void formatVisibleText (const QRect &rect, int dy) const;
    void formatOnResizing() const;
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
    void reformat (TextEdit *textEdit);
    void zoomIn();
    void zoomOut();
    void zoomZero();
    void fullScreening();
    void defaultSize();
    //void align();
    void manageSessions();
    void executeProcess();
    void exitProcess();
    void displayError();
    void displayOutput();
    void docProp();
    void filePrint();
    void detachTab();
    void nextTab();
    void previousTab();
    void lastTab();
    void firstTab();
    void tabContextMenu (const QPoint& p);
    void prefDialog();
    void aboutDialog();
    void helpDoc();
    void matchBrackets();
    void addText (const QString text, const QString fileName, const QString charset,
                  bool enforceEncod, bool reload,
                  bool multiple); // Multiple files are being loaded?
    void onOpeningHugeFiles();

public:
    QWidget *dummyWidget; // Bypasses KDE's demand for a new window.
    Ui::FPwin *ui;

private:
    TabPage *createEmptyTab(bool setCurrent);
    bool hasAnotherDialog();
    void deleteTabPage (int index);
    void loadText (const QString fileName, bool enforceEncod, bool reload, bool multiple = false);
    bool alreadyOpen (TabPage *tabPage) const;
    void setTitle (const QString& fileName, int indx = -1);
    int unSaved (int index, bool noToAll);
    bool saveFile (bool keepSyntax);
    void closeEvent (QCloseEvent *event);
    bool closeTabs (int leftIndx, int rightIndx);
    void dragEnterEvent (QDragEnterEvent *event);
    void dropEvent (QDropEvent *event);
    void dropTab (QString str);
    void changeEvent (QEvent *event);
    QTextDocument::FindFlags getSearchFlags() const;
    void enableWidgets (bool enable) const;
    void updateShortcuts (bool disable, bool page = true);
    QTextCursor finding (const QString str, const QTextCursor& start, QTextDocument::FindFlags flags = 0,
                         const int end = 0) const;
    void setProgLang (TextEdit *textEdit);
    void syntaxHighlighting (TextEdit *textEdit);
    void encodingToCheck (const QString& encoding);
    const QString checkToEncoding() const;
    void applyConfig();
    bool matchLeftParenthesis (QTextBlock currentBlock, int index, int numRightParentheses);
    bool matchRightParenthesis (QTextBlock currentBlock, int index, int numLeftParentheses);
    bool matchLeftBrace (QTextBlock currentBlock, int index, int numRightBraces);
    bool matchRightBrace (QTextBlock currentBlock, int index, int numLeftBraces);
    bool matchLeftBracket (QTextBlock currentBlock, int index, int numRightBrackets);
    bool matchRightBracket (QTextBlock currentBlock, int index, int numLeftBrackets);
    void createSelection (int pos);
    void formatTextRect (QRect rect) const;
    void removeGreenSel();
    void waitToMakeBusy();
    void unbusy();
    void displayMessage (bool error);
    void showWarningBar (const QString& message);
    void closeWarningBar();
    void disconnectLambda();

    QActionGroup *aGroup_;
    QString lastFile_; // The last opened or saved file (for file dialogs).
    QString txtReplace_; // The replacing text.
    int rightClicked_; // The index of the right-clicked tab.
    int loadingProcesses_; // The number of loading processes (used to prevent early closing).
    QPointer<QThread> busyThread_; // Used to wait one second for making the cursor busy.
    ICONMODE iconMode_; // Used only internally.
    QMetaObject::Connection lambdaConnection_; // Captures a lambda connection to disconnect it later.
};

}

#endif // FPWIN_H
