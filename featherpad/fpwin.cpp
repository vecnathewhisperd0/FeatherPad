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

#include "singleton.h"
#include "ui_fp.h"
#include "encoding.h"
#include "filedialog.h"
#include "pref.h"
#include "loading.h"

#include <QMessageBox>
#include <QFontDialog>
#include <QPrintDialog>
#include <QToolTip>
#include <QDesktopWidget>
#include <fstream> // std::ofstream
#include <QPrinter>

#if defined Q_WS_X11 || defined Q_OS_LINUX
#include <QX11Info>
#endif

#include "x11.h"

namespace FeatherPad {

void BusyMaker::waiting() {
    QTimer::singleShot (timeout, this, SLOT (makeBusy()));
}

void BusyMaker::makeBusy() {
    if (QGuiApplication::overrideCursor() == nullptr)
        QGuiApplication::setOverrideCursor (Qt::WaitCursor);
    emit finished();
}


FPwin::FPwin (QWidget *parent):QMainWindow (parent), dummyWidget (nullptr), ui (new Ui::FPwin)
{
    ui->setupUi (this);

    loadingProcesses_ = 0;
    rightClicked_ = -1;
    closeAll_ = false;
    busyThread_ = nullptr;

    /* JumpTo bar*/
    ui->spinBox->hide();
    ui->label->hide();
    ui->checkBox->hide();

    /* status bar */
    QLabel *statusLabel = new QLabel();
    statusLabel->setTextInteractionFlags (Qt::TextSelectableByMouse);
    QToolButton *wordButton = new QToolButton();
    wordButton->setAutoRaise (true);
    wordButton->setToolButtonStyle (Qt::ToolButtonIconOnly);
    wordButton->setIconSize (QSize (16, 16));
    wordButton->setMaximumHeight (16);
    wordButton->setIcon (QIcon (":icons/view-refresh.svg"));
    //wordButton->setText (tr ("Refresh"));
    wordButton->setToolTip (tr ("Calculate number of words\n(For huge texts, this may be CPU-intensive.)"));
    connect (wordButton, &QAbstractButton::clicked, this, &FPwin::wordButtonStatus);
    ui->statusBar->addWidget (statusLabel);
    ui->statusBar->addWidget (wordButton);

    /* text unlocking */
    ui->actionEdit->setVisible (false);

    /* replace dock */
    ui->dockReplace->setVisible (false);

    lastFile_ = "";

    applyConfig();

    newTab();

    aGroup_ = new QActionGroup (this);
    ui->actionUTF_8->setActionGroup (aGroup_);
    ui->actionUTF_16->setActionGroup (aGroup_);
    ui->actionWindows_Arabic->setActionGroup (aGroup_);
    ui->actionISO_8859_1->setActionGroup (aGroup_);
    ui->actionISO_8859_15->setActionGroup (aGroup_);
    ui->actionWindows_1252->setActionGroup (aGroup_);
    ui->actionCryllic_CP1251->setActionGroup (aGroup_);
    ui->actionCryllic_KOI8_U->setActionGroup (aGroup_);
    ui->actionCryllic_ISO_8859_5->setActionGroup (aGroup_);
    ui->actionChineese_BIG5->setActionGroup (aGroup_);
    ui->actionChinese_GB18030->setActionGroup (aGroup_);
    ui->actionJapanese_ISO_2022_JP->setActionGroup (aGroup_);
    ui->actionJapanese_ISO_2022_JP_2->setActionGroup (aGroup_);
    ui->actionJapanese_ISO_2022_KR->setActionGroup (aGroup_);
    ui->actionJapanese_CP932->setActionGroup (aGroup_);
    ui->actionJapanese_EUC_JP->setActionGroup (aGroup_);
    ui->actionKorean_CP949->setActionGroup (aGroup_);
    ui->actionKorean_CP1361->setActionGroup (aGroup_);
    ui->actionKorean_EUC_KR->setActionGroup (aGroup_);
    ui->actionOther->setActionGroup (aGroup_);

    ui->actionUTF_8->setChecked (true);
    ui->actionOther->setDisabled (true);

    bool rtl (QApplication::layoutDirection() == Qt::RightToLeft);

    connect (ui->actionNew, &QAction::triggered, this, &FPwin::newTab);
    connect (ui->actionDetachTab, &QAction::triggered, this, &FPwin::detachTab);
    if (rtl)
    {
        connect (ui->actionRightTab, &QAction::triggered, this, &FPwin::previousTab);
        connect (ui->actionLeftTab, &QAction::triggered, this, &FPwin::nextTab);
    }
    else
    {
        connect (ui->actionRightTab, &QAction::triggered, this, &FPwin::nextTab);
        connect (ui->actionLeftTab, &QAction::triggered, this, &FPwin::previousTab);
    }
    connect (ui->actionLastTab, &QAction::triggered, this, &FPwin::lastTab);
    connect (ui->actionFirstTab, &QAction::triggered, this, &FPwin::firstTab);
    connect (ui->actionClose, &QAction::triggered, this, &FPwin::closeTab);
    connect (ui->tabWidget, &QTabWidget::tabCloseRequested, this, &FPwin::closeTabAtIndex);
    connect (ui->actionOpen, &QAction::triggered, this, &FPwin::fileOpen);
    connect (ui->actionReload, &QAction::triggered, this, &FPwin::reload);
    connect (aGroup_, &QActionGroup::triggered, this, &FPwin::enforceEncoding);
    connect (ui->actionSave, &QAction::triggered, this, &FPwin::fileSave);
    connect (ui->actionSaveAs, &QAction::triggered, this, &FPwin::fileSave);
    connect (ui->actionSaveCodec, &QAction::triggered, this, &FPwin::fileSave);

    connect (ui->actionCut, &QAction::triggered, this, &FPwin::cutText);
    connect (ui->actionCopy, &QAction::triggered, this, &FPwin::copyText);
    connect (ui->actionPaste, &QAction::triggered, this, &FPwin::pasteText);
    connect (ui->actionDelete, &QAction::triggered, this, &FPwin::deleteText);
    connect (ui->actionSelectAll, &QAction::triggered, this, &FPwin::selectAllText);

    connect (ui->actionEdit, &QAction::triggered, this, &FPwin::makeEditable);

    connect (ui->actionUndo, &QAction::triggered, this, &FPwin::undoing);
    connect (ui->actionRedo, &QAction::triggered, this, &FPwin::redoing);

    connect (ui->tabWidget, &TabWidget::currentTabChanged, this, &FPwin::tabSwitch);
    connect (static_cast<TabBar*>(ui->tabWidget->tabBar()), &TabBar::tabDetached, this, &FPwin::detachTab);
    ui->tabWidget->tabBar()->setContextMenuPolicy (Qt::CustomContextMenu);
    connect (ui->tabWidget->tabBar(), &QWidget::customContextMenuRequested, this, &FPwin::tabContextMenu);
    connect (ui->actionCopyName, &QAction::triggered, this, &FPwin::copyTabFileName);
    connect (ui->actionCopyPath, &QAction::triggered, this, &FPwin::copyTabFilePath);
    connect (ui->actionCloseAll, &QAction::triggered, this, &FPwin::closeAllTabs);
    if (rtl)
    {
        connect (ui->actionCloseRight, &QAction::triggered, this, &FPwin::closePreviousTabs);
        connect (ui->actionCloseLeft, &QAction::triggered, this, &FPwin::closeNextTabs);
    }
    else
    {
        connect (ui->actionCloseRight, &QAction::triggered, this, &FPwin::closeNextTabs);
        connect (ui->actionCloseLeft, &QAction::triggered, this, &FPwin::closePreviousTabs);
    }
    connect (ui->actionCloseOther, &QAction::triggered, this, &FPwin::closeOtherTabs);

    connect (ui->actionFont, &QAction::triggered, this, &FPwin::fontDialog);

    connect (ui->toolButton_nxt, &QAbstractButton::clicked, this, &FPwin::find);
    connect (ui->toolButton_prv, &QAbstractButton::clicked, this, &FPwin::find);
    connect (ui->pushButton_whole, &QAbstractButton::clicked, this, &FPwin::setSearchFlags);
    connect (ui->pushButton_case, &QAbstractButton::clicked, this, &FPwin::setSearchFlags);
    connect (ui->actionFind, &QAction::triggered, this, &FPwin::showHideSearch);
    connect (ui->actionJump, &QAction::triggered, this, &FPwin::jumpTo);
    connect (ui->spinBox, &QAbstractSpinBox::editingFinished, this, &FPwin::goTo);

    connect (ui->actionLineNumbers, &QAction::toggled, this, &FPwin::showLN);
    connect (ui->actionWrap, &QAction::triggered, this, &FPwin::toggleWrapping);
    connect (ui->actionSyntax, &QAction::triggered, this, &FPwin::toggleSyntaxHighlighting);
    connect (ui->actionIndent, &QAction::triggered, this, &FPwin::toggleIndent);
    connect (ui->lineEdit, &QLineEdit::returnPressed, this, &FPwin::find);

    connect (ui->actionPreferences, &QAction::triggered, this, &FPwin::prefDialog);

    connect (ui->actionReplace, &QAction::triggered, this, &FPwin::replaceDock);
    connect (ui->toolButtonNext, &QAbstractButton::clicked, this, &FPwin::replace);
    connect (ui->toolButtonPrv, &QAbstractButton::clicked, this, &FPwin::replace);
    connect (ui->toolButtonAll, &QAbstractButton::clicked, this, &FPwin::replaceAll);
    connect (ui->dockReplace, &QDockWidget::visibilityChanged, this, &FPwin::closeReplaceDock);
    connect (ui->dockReplace, &QDockWidget::topLevelChanged, this, &FPwin::resizeDock);

    connect (ui->actionDoc, &QAction::triggered, this, &FPwin::docProp);
    connect (ui->actionPrint, &QAction::triggered, this, &FPwin::filePrint);

    connect (ui->actionAbout, &QAction::triggered, this, &FPwin::aboutDialog);
    connect (ui->actionHelp, &QAction::triggered, this, &FPwin::helpDoc);

    QShortcut *zoomin = new QShortcut (QKeySequence (tr ("Ctrl+=", "Zoom in")), this);
    QShortcut *zoomout = new QShortcut (QKeySequence (tr ("Ctrl+-", "Zoom out")), this);
    QShortcut *zoomzero = new QShortcut (QKeySequence (tr ("Ctrl+0", "Zoom default")), this);
    connect (zoomin, &QShortcut::activated, this, &FPwin::zoomIn);
    connect (zoomout, &QShortcut::activated, this, &FPwin::zoomOut);
    connect (zoomzero, &QShortcut::activated, this, &FPwin::zoomZero);

    QShortcut *fullscreen = new QShortcut (QKeySequence (tr ("F11", "Fullscreen")), this);
    QShortcut *defaultsize = new QShortcut (QKeySequence (tr ("Ctrl+Shift+W", "Default size")), this);
    connect (fullscreen, &QShortcut::activated, this, &FPwin::fullScreening);
    connect (defaultsize, &QShortcut::activated, this, &FPwin::defaultSize);

    /* this is a workaround for the RTL bug in QPlainTextEdit */
    QShortcut *align = new QShortcut (QKeySequence (tr ("Ctrl+Shift+A", "Alignment")), this);
    connect (align, &QShortcut::activated, this, &FPwin::align);

    dummyWidget = new QWidget();
    setAcceptDrops (true);
    setAttribute (Qt::WA_AlwaysShowToolTips);
    setAttribute(Qt::WA_DeleteOnClose, false); // we delete windows in singleton
}
/*************************/
FPwin::~FPwin()
{
    delete dummyWidget; dummyWidget = nullptr;
    delete aGroup_; aGroup_ = nullptr;
    delete ui; ui = nullptr;
}
/*************************/
void FPwin::closeEvent (QCloseEvent *event)
{
    bool keep = closeTabs (-1, -1, false);
    if (keep)
        event->ignore();
    else
    {
        FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
        Config& config = singleton->getConfig();
        if (config.getRemSize() && windowState() == Qt::WindowNoState)
            config.setWinSize (size());
        singleton->removeWin (this);
        event->accept();
    }
}
/*************************/
void FPwin::applyConfig()
{
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();

    if (config.getRemSize())
    {
        resize (config.getWinSize());
        if (config.getIsMaxed())
            setWindowState (Qt::WindowMaximized);
        if (config.getIsFull() && config.getIsMaxed())
            setWindowState (windowState() ^ Qt::WindowFullScreen);
        else if (config.getIsFull())
            setWindowState (Qt::WindowFullScreen);
    }
    else
    {
        QSize startSize = config.getStartSize();
        QSize ag = QApplication::desktop()->availableGeometry().size() - QSize (50, 100);
        if (startSize.width() > ag.width() || startSize.height() > ag.height())
        {
            startSize = startSize.boundedTo (ag);
            config.setStartSize (startSize);
        }
        else if (startSize.isEmpty())
        {
            startSize = QSize (700, 500);
            config.setStartSize (startSize);
        }
        resize (startSize);
    }

    ui->mainToolBar->setVisible (!config.getNoToolbar());

    ui->lineEdit->setVisible (!config.getHideSearchbar());
    ui->pushButton_case->setVisible (!config.getHideSearchbar());
    ui->toolButton_nxt->setVisible (!config.getHideSearchbar());
    ui->toolButton_prv->setVisible (!config.getHideSearchbar());
    ui->pushButton_whole->setVisible (!config.getHideSearchbar());

    ui->actionDoc->setVisible (!config.getShowStatusbar());

    ui->actionWrap->setChecked (config.getWrapByDefault());

    ui->actionIndent->setChecked (config.getIndentByDefault());

    ui->actionLineNumbers->setChecked (config.getLineByDefault());
    ui->actionLineNumbers->setDisabled (config.getLineByDefault());

    ui->actionSyntax->setChecked (config.getSyntaxByDefault());

    if (!config.getShowStatusbar())
        ui->statusBar->hide();

    if (config.getTabPosition() != 0)
        ui->tabWidget->setTabPosition ((QTabWidget::TabPosition) config.getTabPosition());

    if (config.getSysIcon())
    {
        ui->actionNew->setIcon (QIcon::fromTheme ("document-new"));
        ui->actionOpen->setIcon (QIcon::fromTheme ("document-open"));
        ui->actionSave->setIcon (QIcon::fromTheme ("document-save"));
        ui->actionSaveAs->setIcon (QIcon::fromTheme ("document-save-as"));
        ui->actionSaveCodec->setIcon (QIcon::fromTheme ("document-save-as"));
        ui->actionPrint->setIcon (QIcon::fromTheme ("document-print"));
        ui->actionDoc->setIcon (QIcon::fromTheme ("document-properties"));
        ui->actionUndo->setIcon (QIcon::fromTheme ("edit-undo"));
        ui->actionRedo->setIcon (QIcon::fromTheme ("edit-redo"));
        ui->actionCut->setIcon (QIcon::fromTheme ("edit-cut"));
        ui->actionCopy->setIcon (QIcon::fromTheme ("edit-copy"));
        ui->actionPaste->setIcon (QIcon::fromTheme ("edit-paste"));
        ui->actionDelete->setIcon (QIcon::fromTheme ("edit-delete"));
        ui->actionSelectAll->setIcon (QIcon::fromTheme ("edit-select-all"));
        ui->actionReload->setIcon (QIcon::fromTheme ("view-refresh"));
        ui->actionFind->setIcon (QIcon::fromTheme ("edit-find"));
        ui->actionReplace->setIcon (QIcon::fromTheme ("edit-find-replace"));
        ui->actionClose->setIcon (QIcon::fromTheme ("window-close"));
        ui->actionQuit->setIcon (QIcon::fromTheme ("application-exit"));
        ui->actionFont->setIcon (QIcon::fromTheme ("preferences-desktop-font"));
        ui->actionPreferences->setIcon (QIcon::fromTheme ("preferences-system"));
        ui->actionHelp->setIcon (QIcon::fromTheme ("help-contents"));
        ui->actionAbout->setIcon (QIcon::fromTheme ("help-about"));
        ui->actionJump->setIcon (QIcon::fromTheme ("go-jump"));
        ui->actionEdit->setIcon (QIcon::fromTheme ("document-edit"));
        ui->actionCopyName->setIcon (QIcon::fromTheme ("edit-copy"));
        ui->actionCopyPath->setIcon (QIcon::fromTheme ("edit-copy"));
        ui->actionCloseRight->setIcon (QIcon::fromTheme ("go-next"));
        ui->actionCloseLeft->setIcon (QIcon::fromTheme ("go-previous"));
        ui->actionCloseOther->setIcon (QIcon::hasThemeIcon("tab-close-other") ? QIcon::fromTheme ("tab-close-other") : QIcon());
        ui->toolButton_nxt->setIcon (QIcon::fromTheme ("go-down"));
        ui->toolButton_prv->setIcon (QIcon::fromTheme ("go-up"));
        ui->toolButtonNext->setIcon (QIcon::fromTheme ("go-down"));
        ui->toolButtonPrv->setIcon (QIcon::fromTheme ("go-up"));
        ui->toolButtonAll->setIcon (QIcon::hasThemeIcon("arrow-down-double") ? QIcon::fromTheme ("arrow-down-double") : QIcon());
        ui->actionRightTab->setIcon (QIcon::fromTheme ("go-next"));
        ui->actionLeftTab->setIcon (QIcon::fromTheme ("go-previous"));

        setWindowIcon (QIcon::fromTheme ("featherpad"));

        if (QToolButton *wordButton = ui->statusBar->findChild<QToolButton *>())
            wordButton->setIcon (QIcon::fromTheme ("view-refresh"));
    }
}
/*************************/
// We want all dialogs to be window-modal as far as possible. However there is a problem:
// If a dialog is opened in a FeatherPad window and is closed after another dialog is
// opened in another window, the second dialog will be seen as a child of the first window.
// This could cause a crash if the dialog is closed after closing the first window.
// As a workaround, we keep window-modality but don't let the user open two window-modal dialogs.
bool FPwin::hasAnotherDialog()
{
    bool res = false;
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    for (int i = 0; i < singleton->Wins.count(); ++i)
    {
        FPwin *win = singleton->Wins.at (i);
        if (win != this && win->findChildren<QDialog *>().count() > 0)
        {
            res = true;
            break;
        }
    }
    if (res)
    {
        QMessageBox msgBox (this);
        msgBox.setIcon (QMessageBox::Warning);
        msgBox.setText (tr ("<center><b><big>Another FeatherPad window has a dialog! </big></b></center>"));
        msgBox.setInformativeText (tr ("<center><i>Please close this dialog first and then </i></center>"\
                                       "<center><i>attend to that window or just close its dialog! </i></center><p></p>"));
        msgBox.setStandardButtons (QMessageBox::Close);
        msgBox.setWindowModality (Qt::ApplicationModal);
        msgBox.exec();
    }
    return res;
}
/*************************/
void FPwin::deleteTextEdit (int index)
{
    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    /* Because deleting the syntax highlighter later will change
       the text, we have two options to prevent a possible crash:
         (1) disconnecting the textChanged() signals now; or
         (2) truncating searchEntries after deleting the syntax highlighter. */
    disconnect (textEdit, &QPlainTextEdit::textChanged, this, &FPwin::wordButtonStatus);
    disconnect (textEdit, &QPlainTextEdit::textChanged, this, &FPwin::hlight);
    tabInfo *tabinfo = tabsInfo_[textEdit];
    if (Highlighter *highlighter = tabinfo->highlighter)
    {
        disconnect (textEdit, &QPlainTextEdit::cursorPositionChanged, this, &FPwin::matchBrackets);
        disconnect (textEdit, &QPlainTextEdit::blockCountChanged, this, &FPwin::formatOnBlockChange);
        disconnect (textEdit, &TextEdit::updateRect, this, &FPwin::formatVisibleText);
        disconnect (textEdit, &TextEdit::resized, this, &FPwin::formatonResizing);
        delete highlighter; highlighter = nullptr;
    }
    tabsInfo_.remove (textEdit);
    delete tabinfo; tabinfo = nullptr;
    ui->tabWidget->removeTab (index);
    delete textEdit; textEdit = nullptr;
}
/*************************/
// Here leftIndx is the tab's index, to whose right all tabs are to be closed.
// Similarly, rightIndx is the tab's index, to whose left all tabs are to be closed.
// If they're both equal to -1, all tabs will be closed.
bool FPwin::closeTabs (int leftIndx, int rightIndx, bool closeall)
{
    if (isLoading()) return true;

    bool keep = false;
    int index, count;
    int unsaved = 0;
    while (unsaved == 0 && ui->tabWidget->count() > 0)
    {
        if (QGuiApplication::overrideCursor() == nullptr)
            waitToMakeBusy();

        if (rightIndx == 0) break; // no tab on the left
        if (rightIndx > -1)
            index = rightIndx - 1;
        else
            index = ui->tabWidget->count() - 1;

        if (index == leftIndx)
            break;
        else
            ui->tabWidget->setCurrentIndex (index);
        if (closeall && closeAll_)
            unsaved = 2;
        else if ((leftIndx > -1 && leftIndx == ui->tabWidget->count() - 2) || rightIndx == 1)
            unsaved = unSaved (index, false);
        else
            unsaved = unSaved (index, true);
        switch (unsaved) {
        case 0: // close this tab and go to the next one
            keep = false;
            deleteTextEdit (index);

            if (rightIndx > -1) --rightIndx; // a left tab is removed

            /* final changes */
            count = ui->tabWidget->count();
            if (count == 0)
            {
                ui->actionReload->setDisabled (true);
                ui->actionSave->setDisabled (true);
                enableWidgets (false);
            }
            if (count == 1)
            {
                ui->actionDetachTab->setDisabled (true);
                ui->actionRightTab->setDisabled (true);
                ui->actionLeftTab->setDisabled (true);
                ui->actionLastTab->setDisabled (true);
                ui->actionFirstTab->setDisabled (true);
            }
            break;
        case 1: // stop quitting
            keep = true;
            break;
        case 2: // no to all: close all tabs (and quit)
            keep = false;
            closeAll_ = true; // needed only in closeOtherTabs()
            while (index > leftIndx)
            {
                if (rightIndx == 0) break;
                ui->tabWidget->setCurrentIndex (index);

                deleteTextEdit (index);

                if (rightIndx > -1)
                {
                    --rightIndx; // a left tab is removed
                    index = rightIndx - 1;
                }
                else
                    index = ui->tabWidget->count() - 1;

                count = ui->tabWidget->count();
                if (count == 0)
                {
                    ui->actionReload->setDisabled (true);
                    ui->actionSave->setDisabled (true);
                    enableWidgets (false);
                }
                if (count == 1)
                {
                    ui->actionDetachTab->setDisabled (true);
                    ui->actionRightTab->setDisabled (true);
                    ui->actionLeftTab->setDisabled (true);
                    ui->actionLastTab->setDisabled (true);
                    ui->actionFirstTab->setDisabled (true);
                }
            }
            break;
        default:
            break;
        }
    }

    unbusy();
    return keep;
}
/*************************/
void FPwin::copyTabFileName()
{
    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (rightClicked_));
    QString fname = tabsInfo_[textEdit]->fileName;
    QApplication::clipboard()->setText (QFileInfo (fname).fileName());
}
/*************************/
void FPwin::copyTabFilePath()
{
    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (rightClicked_));
    QString str = tabsInfo_[textEdit]->fileName;
    str.chop (QFileInfo (str).fileName().count());
    QApplication::clipboard()->setText (str);
}
/*************************/
void FPwin::closeAllTabs()
{
    closeAll_ = false; // may have been set to true
    closeTabs (-1, -1, true);
}
/*************************/
void FPwin::closeNextTabs()
{
    closeTabs (rightClicked_, -1, false);
}
/*************************/
void FPwin::closePreviousTabs()
{
    closeTabs (-1, rightClicked_, false);
}
/*************************/
void FPwin::closeOtherTabs()
{
    closeAll_ = false; // may have been set to true
    if (!closeTabs (rightClicked_, -1, false))
        closeTabs (-1, rightClicked_, true);
}
/*************************/
void FPwin::dragEnterEvent (QDragEnterEvent *event)
{
    if (findChildren<QDialog *>().count() == 0
        && (event->mimeData()->hasUrls()
            || event->mimeData()->hasFormat ("application/featherpad-tab")))
    {
        event->acceptProposedAction();
    }
}
/*************************/
void FPwin::dropEvent (QDropEvent *event)
{
    if (event->mimeData()->hasFormat ("application/featherpad-tab"))
        dropTab (event->mimeData()->data("application/featherpad-tab"));
    else
    {
        QList<QUrl> urlList = event->mimeData()->urls();
        bool multiple (urlList.count() > 1 || isLoading());
        foreach (QUrl url, urlList)
            newTabFromName (url.toLocalFile(), multiple);
    }

    event->acceptProposedAction();
}
/*************************/
// This method checks if there's any text that isn't saved yet.
int FPwin::unSaved (int index, bool noToAll)
{
    int unsaved = 0;
    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    QString fname = tabsInfo_[textEdit]->fileName;
    if (textEdit->document()->isModified()
        || (!fname.isEmpty() && !QFile::exists (fname)))
    {
        if (hasAnotherDialog()) return 1; // cancel
        disableShortcuts (true);
       /* made busy at closeTabs() */
        unbusy();

        QMessageBox msgBox (this);
        msgBox.setIcon (QMessageBox::Warning);
        msgBox.setText (tr ("<center><b><big>Save changes?</big></b></center>"));
        if (textEdit->document()->isModified())
            msgBox.setInformativeText (tr ("<center><i>The document has been modified.</i></center>"));
        else
            msgBox.setInformativeText (tr ("<center><i>The file has been removed.</i></center>"));
        if (noToAll && ui->tabWidget->count() > 1)
            msgBox.setStandardButtons (QMessageBox::Save
                                       | QMessageBox::Discard
                                       | QMessageBox::Cancel
                                       | QMessageBox::NoToAll);
        else
            msgBox.setStandardButtons (QMessageBox::Save
                                       | QMessageBox::Discard
                                       | QMessageBox::Cancel);
        msgBox.setButtonText (QMessageBox::Discard, tr ("Discard changes"));
        if (noToAll)
            msgBox.setButtonText (QMessageBox::NoToAll, tr ("No to all"));
        msgBox.setDefaultButton (QMessageBox::Save);
        msgBox.setWindowModality (Qt::WindowModal);
        /* enforce a central position (QtCurve bug?) */
        /*msgBox.show();
        msgBox.move (x() + width()/2 - msgBox.width()/2,
                     y() + height()/2 - msgBox.height()/ 2);*/
        switch (msgBox.exec()) {
        case QMessageBox::Save:
            if (!fileSave())
                unsaved = 1;
            break;
        case QMessageBox::Discard:
            break;
        case QMessageBox::Cancel:
            unsaved = 1;
            break;
        case QMessageBox::NoToAll:
            unsaved = 2;
            break;
        default:
            unsaved = 1;
            break;
        }
        disableShortcuts (false);
    }
    return unsaved;
}
/*************************/
// Enable or disable some widgets.
void FPwin::enableWidgets (bool enable) const
{
    Config config = static_cast<FPsingleton*>(qApp)->getConfig();
    if (!config.getHideSearchbar() || !enable)
    {
        ui->lineEdit->setVisible (enable);
        ui->pushButton_case->setVisible (enable);
        ui->toolButton_nxt->setVisible (enable);
        ui->toolButton_prv->setVisible (enable);
        ui->pushButton_whole->setVisible (enable);
    }
    if (!enable && ui->dockReplace->isVisible())
        ui->dockReplace->setVisible (false);
    if (!enable && ui->spinBox->isVisible())
    {
        ui->spinBox->setVisible (false);
        ui->label->setVisible (false);
        ui->checkBox->setVisible (false);
    }
    if ((!enable && ui->statusBar->isVisible())
        || (config.getShowStatusbar() && enable)) // starting from no tab
    {
        ui->statusBar->setVisible (enable);
    }

    ui->actionSelectAll->setEnabled (enable);
    ui->actionFind->setEnabled (enable);
    ui->actionJump->setEnabled (enable);
    ui->actionReplace->setEnabled (enable);
    ui->actionClose->setEnabled (enable);
    ui->actionSaveAs->setEnabled (enable);
    ui->menuEncoding->setEnabled (enable);
    ui->actionSaveCodec->setEnabled (enable);
    ui->actionFont->setEnabled (enable);
    if (!config.getShowStatusbar())
        ui->actionDoc->setEnabled (enable);
    ui->actionPrint->setEnabled (enable);

    if (!enable)
    {
        ui->actionUndo->setEnabled (false);
        ui->actionRedo->setEnabled (false);

        ui->actionEdit->setVisible (false);

        ui->actionCut->setEnabled (false);
        ui->actionCopy->setEnabled (false);
        ui->actionPaste->setEnabled (false);
        ui->actionDelete->setEnabled (false);
    }
}
/*************************/
// When a window-modal dialog is shown, Qt doesn't disable the main window shortcuts.
// This is definitely a bug in Qt. As a workaround, we use this function to disable
// all shortcuts on showing a dialog and to enable them on hiding it.
void FPwin::disableShortcuts (bool disable) const
{
    if (disable)
    {
        ui->actionLineNumbers->setShortcut (QKeySequence());
        ui->actionWrap->setShortcut (QKeySequence());
        ui->actionIndent->setShortcut (QKeySequence());
        ui->actionSyntax->setShortcut (QKeySequence());

        ui->actionNew->setShortcut (QKeySequence());
        ui->actionOpen->setShortcut (QKeySequence());
        ui->actionSave->setShortcut (QKeySequence());
        ui->actionFind->setShortcut (QKeySequence());
        ui->actionReplace->setShortcut (QKeySequence());
        ui->actionSaveAs->setShortcut (QKeySequence());
        ui->actionPrint->setShortcut (QKeySequence());
        ui->actionDoc->setShortcut (QKeySequence());
        ui->actionClose->setShortcut (QKeySequence());
        ui->actionQuit->setShortcut (QKeySequence());
        ui->actionPreferences->setShortcut (QKeySequence());
        ui->actionHelp->setShortcut (QKeySequence());
        ui->actionEdit->setShortcut (QKeySequence());
        ui->actionDetachTab->setShortcut (QKeySequence());
        ui->actionReload->setShortcut (QKeySequence());

        ui->actionUndo->setShortcut (QKeySequence());
        ui->actionRedo->setShortcut (QKeySequence());
        ui->actionCut->setShortcut (QKeySequence());
        ui->actionCopy->setShortcut (QKeySequence());
        ui->actionPaste->setShortcut (QKeySequence());
        ui->actionSelectAll->setShortcut (QKeySequence());

        ui->toolButton_nxt->setShortcut (QKeySequence());
        ui->toolButton_prv->setShortcut (QKeySequence());
        ui->pushButton_case->setShortcut (QKeySequence());
        ui->pushButton_whole->setShortcut (QKeySequence());
        ui->toolButtonNext->setShortcut (QKeySequence());
        ui->toolButtonPrv->setShortcut (QKeySequence());
        ui->toolButtonAll->setShortcut (QKeySequence());

        ui->actionRightTab->setShortcut (QKeySequence());
        ui->actionLeftTab->setShortcut (QKeySequence());
        ui->actionLastTab->setShortcut (QKeySequence());
        ui->actionFirstTab->setShortcut (QKeySequence());
    }
    else
    {
        ui->actionLineNumbers->setShortcut (QKeySequence (tr ("Ctrl+L")));
        ui->actionWrap->setShortcut (QKeySequence (tr ("Ctrl+W")));
        ui->actionIndent->setShortcut (QKeySequence (tr ("Ctrl+I")));
        ui->actionSyntax->setShortcut (QKeySequence (tr ("Ctrl+Shift+H")));

        ui->actionNew->setShortcut (QKeySequence (tr ("Ctrl+N")));
        ui->actionOpen->setShortcut (QKeySequence (tr ("Ctrl+O")));
        ui->actionSave->setShortcut (QKeySequence (tr ("Ctrl+S")));
        ui->actionFind->setShortcut (QKeySequence (tr ("Ctrl+F")));
        ui->actionReplace->setShortcut (QKeySequence (tr ("Ctrl+R")));
        ui->actionSaveAs->setShortcut (QKeySequence (tr ("Ctrl+Shift+S")));
        ui->actionPrint->setShortcut (QKeySequence (tr ("Ctrl+P")));
        ui->actionDoc->setShortcut (QKeySequence (tr ("Ctrl+Shift+D")));
        ui->actionClose->setShortcut (QKeySequence (tr ("Ctrl+Shift+Q")));
        ui->actionQuit->setShortcut (QKeySequence (tr ("Ctrl+Q")));
        ui->actionPreferences->setShortcut (QKeySequence (tr ("Ctrl+Shift+P")));
        ui->actionHelp->setShortcut (QKeySequence (tr ("Ctrl+H")));
        ui->actionEdit->setShortcut (QKeySequence (tr ("Ctrl+E")));
        ui->actionDetachTab->setShortcut (QKeySequence (tr ("Ctrl+T")));
        ui->actionReload->setShortcut (QKeySequence (tr ("Ctrl+Shift+R")));

        ui->actionUndo->setShortcut (QKeySequence (tr ("Ctrl+Z")));
        ui->actionRedo->setShortcut (QKeySequence (tr ("Ctrl+Shift+Z")));
        ui->actionCut->setShortcut (QKeySequence (tr ("Ctrl+X")));
        ui->actionCopy->setShortcut (QKeySequence (tr ("Ctrl+C")));
        ui->actionPaste->setShortcut (QKeySequence (tr ("Ctrl+V")));
        ui->actionSelectAll->setShortcut (QKeySequence (tr ("Ctrl+A")));

        ui->toolButton_nxt->setShortcut (QKeySequence (tr ("F3")));
        ui->toolButton_prv->setShortcut (QKeySequence (tr ("F4")));
        ui->pushButton_case->setShortcut (QKeySequence (tr ("F5")));
        ui->pushButton_whole->setShortcut (QKeySequence (tr ("F6")));
        ui->toolButtonNext->setShortcut (QKeySequence (tr ("F7")));
        ui->toolButtonPrv->setShortcut (QKeySequence (tr ("F8")));
        ui->toolButtonAll->setShortcut (QKeySequence (tr ("F9")));

        ui->actionRightTab->setShortcut (QKeySequence (tr ("Alt+Right")));
        ui->actionLeftTab->setShortcut (QKeySequence (tr ("Alt+Left")));
        ui->actionLastTab->setShortcut (QKeySequence (tr ("Alt+Up")));
        ui->actionFirstTab->setShortcut (QKeySequence (tr ("Alt+Down")));
    }
}
/*************************/
void FPwin::newTab()
{
    createEmptyTab (!isLoading());
}
/*************************/
TextEdit* FPwin::createEmptyTab (bool setCurrent)
{
    Config config = static_cast<FPsingleton*>(qApp)->getConfig();
    TextEdit *textEdit = new TextEdit;
    textEdit->setScrollJumpWorkaround (config.getScrollJumpWorkaround());
    QPalette palette = QApplication::palette();
    if (!config.getDarkColScheme())
    {
        QBrush brush = palette.base();
        if (brush.color().value() <= 120)
        {
            textEdit->viewport()->setStyleSheet (".QWidget {"
                                                 "color: black;"
                                                 "background-color: rgb(236, 236, 236);}");
            brush = palette.highlight();
            if (brush.color().value() > 160) // dark themes with very light selection color
                textEdit->setStyleSheet ("QPlainTextEdit {"
                                         "selection-background-color: black;"
                                         "selection-color: white;}");
        }
        else
            textEdit->viewport()->setStyleSheet (".QWidget {"
                                                 "color: black;"
                                                 "background-color: rgb(255, 255, 255);}");
    }
    else
    {
        textEdit->useDarkScheme (true);
        textEdit->viewport()->setStyleSheet (".QWidget {"
                                             "color: white;"
                                             "background-color: rgb(15, 15, 15);}");
        QBrush brush = palette.highlight();
        if (brush.color().value() < 120) // themes with very dark selection color
            textEdit->setStyleSheet ("QPlainTextEdit {"
                                     "selection-background-color: white;"
                                     "selection-color: black;}");
    }
    textEdit->document()->setDefaultFont (config.getFont());
    /* we want consistent tabs */
    QFontMetrics metrics (config.getFont());
    textEdit->setTabStopWidth (4 * metrics.width (' '));

    int index = ui->tabWidget->currentIndex();
    if (index == -1) enableWidgets (true);

    /* first make the preliminary info
       and only then add a new tab */
    tabInfo *tabinfo = new tabInfo;
    tabinfo->fileName = QString();
    tabinfo->size = 0;
    tabinfo->searchEntry = QString();
    tabinfo->encoding = "UTF-8";
    tabinfo->prog = QString();
    tabinfo->wordNumber = -1;
    tabinfo->highlighter = nullptr;
    tabinfo->greenSel = QList<QTextEdit::ExtraSelection>();
    tabinfo->redSel = QList<QTextEdit::ExtraSelection>();
    tabsInfo_[textEdit] = tabinfo;

    ui->tabWidget->insertTab (index + 1, textEdit, "Untitled");

    /* set all preliminary properties */
    if (index >= 0)
    {
        ui->actionDetachTab->setEnabled (true);
        ui->actionRightTab->setEnabled (true);
        ui->actionLeftTab->setEnabled (true);
        ui->actionLastTab->setEnabled (true);
        ui->actionFirstTab->setEnabled (true);
    }
    ui->tabWidget->setTabToolTip (index + 1, "Unsaved");
    if (!ui->actionWrap->isChecked())
        textEdit->setLineWrapMode (QPlainTextEdit::NoWrap);
    if (!ui->actionIndent->isChecked())
        textEdit->setAutoIndentation (false);
    if (ui->actionLineNumbers->isChecked() || ui->spinBox->isVisible())
        textEdit->showLineNumbers (true);
    if (ui->spinBox->isVisible())
        connect (textEdit->document(), &QTextDocument::blockCountChanged, this, &FPwin::setMax);
    if (ui->statusBar->isVisible()
        || config.getShowStatusbar()) // when the main window is being created, isVisible() isn't set yet
    {
        if (setCurrent)
        {
            if (QToolButton *wordButton = ui->statusBar->findChild<QToolButton *>())
                wordButton->setVisible (false);
            QLabel *statusLabel = ui->statusBar->findChild<QLabel *>();
            statusLabel->setText (tr ("<b>Encoding:</b> <i>UTF-8</i>"
                                      "&nbsp;&nbsp;&nbsp;&nbsp;<b>Lines:</b> <i>1</i>"
                                      "&nbsp;&nbsp;&nbsp;&nbsp;<b>Sel. Chars.:</b> <i>0</i>"
                                      "&nbsp;&nbsp;&nbsp;&nbsp;<b>Words:</b> <i>0</i>"));
        }
        connect (textEdit, &QPlainTextEdit::blockCountChanged, this, &FPwin::statusMsgWithLineCount);
        connect (textEdit, &QPlainTextEdit::selectionChanged, this, &FPwin::statusMsg);
    }
    connect (textEdit->document(), &QTextDocument::undoAvailable, ui->actionUndo, &QAction::setEnabled);
    connect (textEdit->document(), &QTextDocument::redoAvailable, ui->actionRedo, &QAction::setEnabled);
    connect (textEdit->document(), &QTextDocument::modificationChanged, ui->actionSave, &QAction::setEnabled);
    connect (textEdit->document(), &QTextDocument::modificationChanged, this, &FPwin::asterisk);
    connect (textEdit, &QPlainTextEdit::copyAvailable, ui->actionCut, &QAction::setEnabled);
    connect (textEdit, &QPlainTextEdit::copyAvailable, ui->actionDelete, &QAction::setEnabled);
    connect (textEdit, &QPlainTextEdit::copyAvailable, ui->actionCopy, &QAction::setEnabled);
    connect (textEdit, &TextEdit::fileDropped, this, &FPwin::newTabFromName);

    /* I don't know why, under KDE, when text is selected
       for the first time, it isn't copied to the selection
       clipboard. Perhaps it has something to do with Klipper.
       I neither know why this s a workaround: */
    QApplication::clipboard()->text (QClipboard::Selection);

    if (setCurrent)
    {
        ui->tabWidget->setCurrentWidget (textEdit);
        textEdit->setFocus();
    }

    /* this isn't enough for unshading under all WMs */
    /*if (isMinimized())
        setWindowState (windowState() & (~Qt::WindowMinimized | Qt::WindowActive));*/
    if (isWindowShaded (winId()))
        unshadeWindow (winId());
    activateWindow();
    raise();

    return textEdit;
}
/*************************/
void FPwin::zoomIn()
{
    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    QFont currentFont = textEdit->document()->defaultFont();
    int size = currentFont.pointSize();
    currentFont.setPointSize (size + 1);
    textEdit->document()->setDefaultFont (currentFont);
    QFontMetrics metrics (currentFont);
    textEdit->setTabStopWidth (4 * metrics.width (' '));

    /* this is sometimes needed for the
       scrollbar range(s) to be updated (a Qt bug?) */
    QTimer::singleShot (0, textEdit, SLOT (updateEditorGeometry()));
}
/*************************/
void FPwin::zoomOut()
{
    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    QFont currentFont = textEdit->document()->defaultFont();
    int size = currentFont.pointSize();
    if (size <= 3) return;
    currentFont.setPointSize (size - 1);
    textEdit->document()->setDefaultFont (currentFont);
    QFontMetrics metrics (currentFont);
    textEdit->setTabStopWidth (4 * metrics.width (' '));

    formatTextRect (textEdit->rect());
    if (!tabsInfo_[textEdit]->searchEntry.isEmpty())
        hlight();
}
/*************************/
void FPwin::zoomZero()
{
    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    Config config = static_cast<FPsingleton*>(qApp)->getConfig();
    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    textEdit->document()->setDefaultFont (config.getFont());
    QFontMetrics metrics (config.getFont());
    textEdit->setTabStopWidth (4 * metrics.width (' '));

    QTimer::singleShot (0, textEdit, SLOT (updateEditorGeometry()));

    /* this may be a zoom-out */
    formatTextRect (textEdit->rect());
    if (!tabsInfo_[textEdit]->searchEntry.isEmpty())
        hlight();
}
/*************************/
void FPwin::fullScreening()
{
    setWindowState (windowState() ^ Qt::WindowFullScreen);
}
/*************************/
void FPwin::defaultSize()
{
    if (size() == QSize (700, 500)) return;
    if (isMaximized() && isFullScreen())
        showMaximized();
    if (isMaximized())
        showNormal();
    /* instead of hiding, reparent with the dummy
       widget to guarantee resizing under all DEs */
    setParent (dummyWidget, Qt::SubWindow);
    resize (static_cast<FPsingleton*>(qApp)->getConfig().getStartSize());
    if (parent() != nullptr)
        setParent (nullptr, Qt::Window);
    QTimer::singleShot (0, this, SLOT (show()));
}
/*************************/
void FPwin::align()
{
    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    QTextOption opt = textEdit->document()->defaultTextOption();
    if (opt.alignment() == (Qt::AlignLeft))
    {
        opt = QTextOption (Qt::AlignRight);
        opt.setTextDirection (Qt::LayoutDirectionAuto);
        textEdit->document()->setDefaultTextOption (opt);
    }
    else if (opt.alignment() == (Qt::AlignRight))
    {
        opt = QTextOption (Qt::AlignLeft);
        opt.setTextDirection (Qt::LayoutDirectionAuto);
        textEdit->document()->setDefaultTextOption (opt);
    }
}
/*************************/
void FPwin::closeTab()
{
    if (isLoading()) return;

    int index = ui->tabWidget->currentIndex();
    if (index == -1) return; // not needed

    if (unSaved (index, false)) return;

    deleteTextEdit (index);
    int count = ui->tabWidget->count();
    if (count == 0)
    {
        ui->actionReload->setDisabled (true);
        ui->actionSave->setDisabled (true);
        enableWidgets (false);
    }
    else // set focus to text-edit
    {
        if (count == 1)
        {
            ui->actionDetachTab->setDisabled (true);
            ui->actionRightTab->setDisabled (true);
            ui->actionLeftTab->setDisabled (true);
            ui->actionLastTab->setDisabled (true);
            ui->actionFirstTab->setDisabled (true);
        }
        if (TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->currentWidget()))
            textEdit->setFocus();
    }
}
/*************************/
void FPwin::closeTabAtIndex (int index)
{
    if (unSaved (index, false)) return;

    deleteTextEdit (index);
    int count = ui->tabWidget->count();
    if (count == 0)
    {
        ui->actionReload->setDisabled (true);
        ui->actionSave->setDisabled (true);
        enableWidgets (false);
    }
    else
    {
        if (count == 1)
        {
            ui->actionDetachTab->setDisabled (true);
            ui->actionRightTab->setDisabled (true);
            ui->actionLeftTab->setDisabled (true);
            ui->actionLastTab->setDisabled (true);
            ui->actionFirstTab->setDisabled (true);
        }
        if (TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->currentWidget()))
            textEdit->setFocus();
    }
}
/*************************/
void FPwin::setTitle (const QString& fileName, int indx)
{
    int index = indx;
    if (index < 0)
        index = ui->tabWidget->currentIndex(); // is never -1

    QString shownName;
    if (fileName.isEmpty())
        shownName = tr ("Untitled");
    else
        shownName = QFileInfo (fileName).fileName();

    if (indx < 0)
        setWindowTitle (shownName);

    shownName.replace ("&", "&&"); // single ampersand is for shortcut

    QFontMetrics metrics = QFontMetrics (ui->tabWidget->font());
    int w = 100 * metrics.width (' '); // 100 characters are more than enough
    QString elidedName = metrics.elidedText (shownName, Qt::ElideMiddle, w);
    ui->tabWidget->setTabText (index, elidedName);
}
/*************************/
void FPwin::asterisk (bool modified)
{
    int index = ui->tabWidget->currentIndex();

    QString fname = tabsInfo_[qobject_cast< TextEdit *>(ui->tabWidget->widget (index))]
                    ->fileName;
    QString shownName;
    if (modified)
    {
        if (fname.isEmpty())
            shownName = tr ("*Untitled");
        else
            shownName = QFileInfo (fname).fileName().prepend("*");
    }
    else
    {
        if (fname.isEmpty())
            shownName = tr ("Untitled");
        else
            shownName = QFileInfo (fname).fileName();
    }

    setWindowTitle (shownName);

    shownName.replace ("&", "&&");

    QFontMetrics metrics = QFontMetrics (ui->tabWidget->font());
    int w = 100 * metrics.width (' ');
    QString elidedName = metrics.elidedText (shownName, Qt::ElideMiddle, w);
    ui->tabWidget->setTabText (index, elidedName);
}
/*************************/
void FPwin::waitToMakeBusy()
{
    if (busyThread_ != nullptr) return;

    busyThread_ = new QThread;
    BusyMaker *makeBusy = new BusyMaker();
    makeBusy->moveToThread(busyThread_);
    connect (busyThread_, &QThread::started, makeBusy, &BusyMaker::waiting);
    connect (busyThread_, &QThread::finished, busyThread_, &QObject::deleteLater);
    connect (makeBusy, &BusyMaker::finished, busyThread_, &QThread::quit);
    connect (makeBusy, &BusyMaker::finished, makeBusy, &QObject::deleteLater);
    busyThread_->start();
}
/*************************/
void FPwin::unbusy()
{
    if (busyThread_ && !busyThread_->isFinished())
    {
        busyThread_->quit();
        busyThread_->wait();
    }
    if (QGuiApplication::overrideCursor() != nullptr)
        QGuiApplication::restoreOverrideCursor();
}
/*************************/
void FPwin::loadText (const QString fileName, bool enforceEncod, bool reload, bool multiple)
{
    ++ loadingProcesses_;
    QString charset = "";
    if (enforceEncod)
        charset = checkToEncoding();
    Loading *thread = new Loading (fileName, charset, enforceEncod, reload, multiple);
    connect (thread, &Loading::completed, this, &FPwin::addText);
    connect (thread, &Loading::finished, thread, &QObject::deleteLater);
    thread->start();

    if (QGuiApplication::overrideCursor() == nullptr)
        waitToMakeBusy();
}
/*************************/
// When multiple files are being loaded, we don't change the current tab.
void FPwin::addText (const QString text, const QString fileName, const QString charset,
                     bool enforceEncod, bool reload, bool multiple)
{
    if (fileName.isEmpty())
    {
        -- loadingProcesses_;
        if (!isLoading()) unbusy();
        return;
    }

    if (enforceEncod || reload)
        multiple = false; // respect the logic

    TextEdit *textEdit;
    if (ui->tabWidget->currentIndex() == -1)
        textEdit = createEmptyTab (!multiple);
    else
        textEdit = qobject_cast< TextEdit *>(ui->tabWidget->currentWidget());

    bool openInCurrentTab (true);
    if (!reload
        && !enforceEncod
        && (!textEdit->document()->isEmpty()
            || textEdit->document()->isModified()
            || !tabsInfo_[textEdit]->fileName.isEmpty()))
    {
        textEdit = createEmptyTab (!multiple);
        openInCurrentTab = false;
    }
    else
    {
        /*if (isMinimized())
            setWindowState (windowState() & (~Qt::WindowMinimized | Qt::WindowActive));*/
        if (isWindowShaded (winId()))
            unshadeWindow (winId());
        activateWindow();
        raise();
    }

    /* this is a workaround for the RTL bug in QPlainTextEdit */
    QTextOption opt = textEdit->document()->defaultTextOption();
    if (text.isRightToLeft())
    {
        if (opt.alignment() == (Qt::AlignLeft))
        {
            opt = QTextOption (Qt::AlignRight);
            opt.setTextDirection (Qt::LayoutDirectionAuto);
            textEdit->document()->setDefaultTextOption (opt);
        }
    }
    else if (opt.alignment() == (Qt::AlignRight))
    {
        opt = QTextOption (Qt::AlignLeft);
        opt.setTextDirection (Qt::LayoutDirectionAuto);
        textEdit->document()->setDefaultTextOption (opt);
    }

    /* we want to restore the cursor later */
    int pos = 0, anchor = 0;
    if (reload)
    {
        pos = textEdit->textCursor().position();
        anchor = textEdit->textCursor().anchor();
    }

    /* set the text */
    disconnect (textEdit->document(), &QTextDocument::modificationChanged, ui->actionSave, &QAction::setEnabled);
    disconnect (textEdit->document(), &QTextDocument::modificationChanged, this, &FPwin::asterisk);
    textEdit->setPlainText (text);
    connect (textEdit->document(), &QTextDocument::modificationChanged, ui->actionSave, &QAction::setEnabled);
    connect (textEdit->document(), &QTextDocument::modificationChanged, this, &FPwin::asterisk);

    /* now, restore the cursor */
    if (reload)
    {
        QTextCursor cur = textEdit->textCursor();
        cur.movePosition (QTextCursor::End, QTextCursor::MoveAnchor);
        int curPos = cur.position();
        if (anchor <= curPos && pos <= curPos)
        {
            cur.setPosition (anchor);
            cur.setPosition (pos, QTextCursor::KeepAnchor);
        }
        textEdit->setTextCursor (cur);
    }

    tabInfo *tabinfo = tabsInfo_[textEdit];
    if (enforceEncod || reload)
    {
        tabinfo->greenSel = QList<QTextEdit::ExtraSelection>();

        if (Highlighter *highlighter = tabinfo->highlighter)
        {
            disconnect (textEdit, &QPlainTextEdit::cursorPositionChanged, this, &FPwin::matchBrackets);
            disconnect (textEdit, &QPlainTextEdit::blockCountChanged, this, &FPwin::formatOnBlockChange);
            disconnect (textEdit, &TextEdit::updateRect, this, &FPwin::formatVisibleText);
            disconnect (textEdit, &TextEdit::resized, this, &FPwin::formatonResizing);
            tabinfo->highlighter = nullptr;
            delete highlighter; highlighter = nullptr;
        }
    }

    QFileInfo fInfo = (fileName);

    tabinfo->fileName = fileName;
    tabinfo->size = fInfo.size();
    lastFile_ = fileName;
    tabinfo->encoding = charset;
    tabinfo->wordNumber = -1;
    setProgLang (tabinfo);
    if (ui->actionSyntax->isChecked()
        && tabinfo->size <= static_cast<FPsingleton*>(qApp)->getConfig().getMaxSHSize()*1024*1024)
    {
        syntaxHighlighting (tabinfo);
    }
    setTitle (fileName, (multiple && !openInCurrentTab) ?
                        /* the index may have changed because syntaxHighlighting() waits for
                           all events to be processed (but it won't change from here on) */
                        ui->tabWidget->indexOf (textEdit) : -1);
    QString tip (fInfo.absolutePath() + "/");
    QFontMetrics metrics (QToolTip::font());
    int w = QApplication::desktop()->screenGeometry().width();
    if (w > 200 * metrics.width (' ')) w = 200 * metrics.width (' ');
    QString elidedTip = metrics.elidedText (tip, Qt::ElideMiddle, w);
    ui->tabWidget->setTabToolTip (ui->tabWidget->indexOf (textEdit), elidedTip);

    if (alreadyOpen (fileName, multiple ? tabinfo : nullptr))
    {
        textEdit->setReadOnly (true);
        if (!textEdit->hasDarkScheme())
            textEdit->viewport()->setStyleSheet (".QWidget {"
                                                 "color: black;"
                                                 "background-color: rgb(236, 236, 208);}");
        else
            textEdit->viewport()->setStyleSheet (".QWidget {"
                                                 "color: white;"
                                                 "background-color: rgb(60, 0, 0);}");
        if (!multiple || openInCurrentTab)
        {
            ui->actionEdit->setVisible (true);
            ui->actionCut->setDisabled (true);
            ui->actionPaste->setDisabled (true);
            ui->actionDelete->setDisabled (true);
        }
        disconnect (textEdit, &QPlainTextEdit::copyAvailable, ui->actionCut, &QAction::setEnabled);
        disconnect (textEdit, &QPlainTextEdit::copyAvailable, ui->actionDelete, &QAction::setEnabled);
    }
    else if (textEdit->isReadOnly())
        makeEditable();

    if (!multiple || openInCurrentTab)
    {
        if (ui->statusBar->isVisible())
        {
            statusMsgWithLineCount (textEdit->document()->blockCount());
            if (QToolButton *wordButton = ui->statusBar->findChild<QToolButton *>())
                wordButton->setVisible (true);
        }
        encodingToCheck (charset);
        ui->actionReload->setEnabled (true);
        textEdit->setFocus(); // the text may have been opened in this (empty) tab
    }

    /* a file is completely loaded */
    -- loadingProcesses_;
    if (!isLoading()) unbusy();
}

/*************************/
void FPwin::newTabFromName (const QString& fileName, bool multiple)
{
    if (!fileName.isEmpty())
        loadText (fileName, false, false, multiple);
}
/*************************/
void FPwin::fileOpen()
{
    if (isLoading()) return;

    /* find a suitable directory */
    int index = ui->tabWidget->currentIndex();
    QString fname;
    if (index > -1)
        fname = tabsInfo_[qobject_cast< TextEdit *>(ui->tabWidget->widget (index))]
                ->fileName;

    QString path;
    if (!fname.isEmpty())
    {
        if (QFile::exists (fname))
            path = fname;
        else
        {
            QDir dir = QFileInfo (fname).absoluteDir();
            if (!dir.exists())
                dir = QDir::home();
            path = dir.path();
        }
    }
    else
    {
        /* I like the last opened file to be remembered */
        fname = lastFile_;
        if (!fname.isEmpty())
        {
            QDir dir = QFileInfo (fname).absoluteDir();
            if (!dir.exists())
                dir = QDir::home();
            path = dir.path();
        }
        else
        {
            QDir dir = QDir::home();
            path = dir.path();
        }
    }

    if (hasAnotherDialog()) return;
    disableShortcuts (true);
    FileDialog dialog (this);
    dialog.setAcceptMode (QFileDialog::AcceptOpen);
    dialog.setWindowTitle (tr ("Open file..."));
    dialog.setFileMode (QFileDialog::ExistingFiles);
    if (QFileInfo (path).isDir())
        dialog.setDirectory (path);
    else
    {
        dialog.selectFile (path);
        dialog.autoScroll();
    }
    if (dialog.exec())
    {
        QStringList files = dialog.selectedFiles();
        bool multiple (files.count() > 1 || isLoading());
        foreach (fname, files)
            newTabFromName (fname, multiple);
    }
    disableShortcuts (false);
}
/*************************/
// Check if the file is already opened for editing somewhere else.
bool FPwin::alreadyOpen (const QString& fileName, tabInfo *tabinfo) const
{
    bool res = false;
    TextEdit *textEdit;
    if (tabinfo != nullptr)
        textEdit = tabsInfo_.key (tabinfo);
    else
        textEdit = qobject_cast< TextEdit *>(ui->tabWidget->currentWidget());
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    for (int i = 0; i < singleton->Wins.count(); ++i)
    {
        FPwin *thisOne = singleton->Wins.at (i);
        QHash<TextEdit*,tabInfo*>::iterator it;
        for (it = thisOne->tabsInfo_.begin(); it != thisOne->tabsInfo_.end(); ++it)
        {
            if (thisOne == this && it.key() == textEdit)
                continue;
            if (thisOne->tabsInfo_[it.key()]->fileName == fileName &&
                !it.key()->isReadOnly())
            {
                res = true;
                break;
            }
        }
        if (res) break;
    }
    return res;
}
/*************************/
void FPwin::enforceEncoding (QAction*)
{
    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    tabInfo *tabinfo = tabsInfo_[textEdit];
    QString fname = tabinfo->fileName;
    if (!fname.isEmpty())
    {
        if (unSaved (index, false))
        {
            /* back to the previous encoding */
            encodingToCheck (tabinfo->encoding);
            return;
        }
        loadText (fname, true, true);
    }
    else
    {
        tabinfo->encoding = checkToEncoding();
        if (ui->statusBar->isVisible())
        {
            QLabel *statusLabel = ui->statusBar->findChild<QLabel *>();
            QString str = statusLabel->text();
            int i = str.indexOf ("Encoding");
            int j = str.indexOf ("</i>&nbsp;&nbsp;&nbsp;&nbsp;<b>Lines");
            str.replace (i + 17, j - i - 17, checkToEncoding());
            statusLabel->setText (str);
        }
    }
}
/*************************/
void FPwin::reload()
{
    if (isLoading()) return;

    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    if (unSaved (index, false)) return;

    QString fname = tabsInfo_[qobject_cast< TextEdit *>(ui->tabWidget->widget (index))]
                    ->fileName;
    if (!fname.isEmpty()) loadText (fname, false, true);
}
/*************************/
// This is for both "Save" and "Save As"
bool FPwin::fileSave()
{
    if (isLoading()) return false;

    int index = ui->tabWidget->currentIndex();
    if (index == -1) return false;

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    tabInfo *tabinfo = tabsInfo_[textEdit];
    QString fname = tabinfo->fileName;
    if (fname.isEmpty()) fname = lastFile_;
    QString filter;
    if (!fname.isEmpty()
        && QFileInfo (fname).fileName().contains ('.'))
    {
        /* if relevant, do filtering to prevent disastrous overwritings */
        filter = tr (".%1 Files(*.%1);;All Files(*)").arg (fname.section ('.', -1, -1));
    }

    if (fname.isEmpty()
        || !QFile::exists (fname)
        || tabinfo->fileName.isEmpty())
    {
        bool restorable = false;
        if (fname.isEmpty())
        {
            QDir dir = QDir::home();
            fname = dir.filePath ("Untitled");
        }
        else if (!QFile::exists (fname))
        {
            QDir dir = QFileInfo (fname).absoluteDir();
            if (!dir.exists())
            {
                dir = QDir::home();
                if (tabinfo->fileName.isEmpty())
                    filter = QString();
            }
            /* if the removed file is opened in this tab and its
               containing folder still exists, it's restorable */
            else if (!tabinfo->fileName.isEmpty())
                restorable = true;
            /* add the file name */
            if (restorable)
                fname = dir.filePath (QFileInfo (fname).fileName());
            else
                fname = dir.filePath ("Untitled");
        }
        else
            fname = QFileInfo (fname).absoluteDir().filePath ("Untitled");

        /* use Save-As for Save or saving */
        if (!restorable
            && QObject::sender() != ui->actionSaveAs
            && QObject::sender() != ui->actionSaveCodec)
        {
            if (hasAnotherDialog()) return false;
            disableShortcuts (true);
            FileDialog dialog (this);
            dialog.setAcceptMode (QFileDialog::AcceptSave);
            dialog.setWindowTitle (tr ("Save as..."));
            dialog.setFileMode (QFileDialog::AnyFile);
            dialog.setNameFilter (filter);
            dialog.selectFile (fname);
            dialog.autoScroll();
            if (dialog.exec())
            {
                fname = dialog.selectedFiles().at (0);
                if (fname.isEmpty() || QFileInfo (fname).isDir())
                {
                    disableShortcuts (false);
                    return false;
                }
            }
            else
            {
                disableShortcuts (false);
                return false;
            }
            disableShortcuts (false);
        }
    }

    if (QObject::sender() == ui->actionSaveAs)
    {
        if (hasAnotherDialog()) return false;
        disableShortcuts (true);
        FileDialog dialog (this);
        dialog.setAcceptMode (QFileDialog::AcceptSave);
        dialog.setWindowTitle (tr ("Save as..."));
        dialog.setFileMode (QFileDialog::AnyFile);
        dialog.setNameFilter (filter);
        dialog.selectFile (fname);
        dialog.autoScroll();
        if (dialog.exec())
        {
            fname = dialog.selectedFiles().at (0);
            if (fname.isEmpty() || QFileInfo (fname).isDir())
            {
                disableShortcuts (false);
                return false;
            }
        }
        else
        {
            disableShortcuts (false);
            return false;
        }
        disableShortcuts (false);
    }
    else if (QObject::sender() == ui->actionSaveCodec)
    {
        if (hasAnotherDialog()) return false;
        disableShortcuts (true);
        FileDialog dialog (this);
        dialog.setAcceptMode (QFileDialog::AcceptSave);
        dialog.setWindowTitle (tr ("Keep encoding and save as..."));
        dialog.setFileMode (QFileDialog::AnyFile);
        dialog.setNameFilter (filter);
        dialog.selectFile (fname);
        dialog.autoScroll();
        if (dialog.exec())
        {
            fname = dialog.selectedFiles().at (0);
            if (fname.isEmpty() || QFileInfo (fname).isDir())
            {
                disableShortcuts (false);
                return false;
            }
        }
        else
        {
            disableShortcuts (false);
            return false;
        }
        disableShortcuts (false);
    }

    /* try to write */
    QTextDocumentWriter writer (fname, "plaintext");
    bool success = false;
    if (QObject::sender() == ui->actionSaveCodec)
    {
        QString encoding  = checkToEncoding();

        if (hasAnotherDialog()) return false;
        disableShortcuts (true);
        QMessageBox msgBox (this);
        msgBox.setIcon (QMessageBox::Question);
        msgBox.addButton (QMessageBox::Yes);
        msgBox.addButton (QMessageBox::No);
        msgBox.addButton (QMessageBox::Cancel);
        msgBox.setText (tr ("<center>Do you want to use <b>MS Windows</b> end-of-lines?</center>"));
        msgBox.setInformativeText (tr ("<center><i>This may be good for readability under MS Windows.</i></center>"));
        msgBox.setWindowModality (Qt::WindowModal);
        QString contents;
        int ln;
        QTextCodec *codec;
        QByteArray encodedString;
        const char *txt;
        switch (msgBox.exec()) {
        case QMessageBox::Yes:
            contents = textEdit->document()->toPlainText();
            contents.replace ("\n", "\r\n");
            ln = contents.length(); // for fwrite();
            codec = QTextCodec::codecForName (checkToEncoding().toUtf8());
            encodedString = codec->fromUnicode (contents);
            txt = encodedString.constData();
            if (encoding != "UTF-16")
            {
                std::ofstream file;
                file.open (fname.toUtf8().constData());
                if (file.is_open())
                {
                    file << txt;
                    file.close();
                    success = true;
                }
            }
            else
            {
                FILE * file;
                file = fopen (fname.toUtf8().constData(), "wb");
                if (file != nullptr)
                {
                    /* this worked correctly as I far as I tested */
                    fwrite (txt , 2 , ln + 1 , file);
                    fclose (file);
                    success = true;
                }
            }
            break;
        case QMessageBox::No:
            writer.setCodec (QTextCodec::codecForName (encoding.toUtf8()));
            break;
        default:
            disableShortcuts (false);
            return false;
            break;
        }
        disableShortcuts (false);
    }
    if (!success)
        success = writer.write (textEdit->document());

    if (success)
    {
        QFileInfo fInfo = (fname);

        textEdit->document()->setModified (false);
        tabinfo->fileName = fname;
        tabinfo->size = fInfo.size();
        ui->actionReload->setDisabled (false);
        setTitle (fname);
        QString tip (fInfo.absolutePath() + "/");
        QFontMetrics metrics (QToolTip::font());
        int w = QApplication::desktop()->screenGeometry().width();
        if (w > 200 * metrics.width (' ')) w = 200 * metrics.width (' ');
        QString elidedTip = metrics.elidedText (tip, Qt::ElideMiddle, w);
        ui->tabWidget->setTabToolTip (index, elidedTip);
        lastFile_ = fname;
    }
    else
    {
        if (hasAnotherDialog()) return success;
        disableShortcuts (true);
        QString str = writer.device()->errorString();
        QMessageBox msgBox (QMessageBox::Warning,
                            tr ("FeatherPad"),
                            tr ("<center><b><big>Cannot be saved!</big></b></center>"),
                            QMessageBox::Close,
                            this);
        msgBox.setInformativeText (tr ("<center><i>%1.</i></center>").arg (str));
        msgBox.setWindowModality (Qt::WindowModal);
        msgBox.exec();
        disableShortcuts (false);
    }
    return success;
}
/*************************/
void FPwin::cutText()
{
    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    qobject_cast< TextEdit *>(ui->tabWidget->widget (index))->cut();
}
/*************************/
void FPwin::copyText()
{
    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    qobject_cast< TextEdit *>(ui->tabWidget->widget (index))->copy();
}
/*************************/
void FPwin::pasteText()
{
    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    qobject_cast< TextEdit *>(ui->tabWidget->widget (index))->paste();
}
/*************************/
void FPwin::deleteText()
{
    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    if (!textEdit->isReadOnly())
        textEdit->insertPlainText ("");
}
/*************************/
void FPwin::selectAllText()
{
    qobject_cast< TextEdit *>(ui->tabWidget->currentWidget())->selectAll();
}
/*************************/
void FPwin::makeEditable()
{
    if (isLoading()) return;

    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    bool textIsSelected = textEdit->textCursor().hasSelection();

    textEdit->setReadOnly (false);
    if (!textEdit->hasDarkScheme())
    {
        QPalette palette = QApplication::palette();
        QBrush brush = palette.window();
        if (brush.color().value() <= 120)
            textEdit->viewport()->setStyleSheet (".QWidget {"
                                                 "color: black;"
                                                 "background-color: rgb(236, 236, 236);}");
        else
            textEdit->viewport()->setStyleSheet (".QWidget {"
                                                 "color: black;"
                                                 "background-color: rgb(255, 255, 255);}");
    }
    else
    {
        textEdit->viewport()->setStyleSheet (".QWidget {"
                                             "color: white;"
                                             "background-color: rgb(15, 15, 15);}");
    }
    ui->actionEdit->setVisible (false);

    ui->actionPaste->setEnabled (true);
    ui->actionCopy->setEnabled (textIsSelected);
    ui->actionCut->setEnabled (textIsSelected);
    ui->actionDelete->setEnabled (textIsSelected);
    connect (textEdit, &QPlainTextEdit::copyAvailable, ui->actionCut, &QAction::setEnabled);
    connect (textEdit, &QPlainTextEdit::copyAvailable, ui->actionDelete, &QAction::setEnabled);
}
/*************************/
void FPwin::undoing()
{
    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    tabInfo *tabinfo = tabsInfo_[textEdit];

    /* remove green highlights */
    tabinfo->greenSel = QList<QTextEdit::ExtraSelection>();
    if (tabinfo->searchEntry.isEmpty()) // hlight() won't be called
    {
        QList<QTextEdit::ExtraSelection> es;
        if (ui->actionLineNumbers->isChecked() || ui->spinBox->isVisible())
            es.prepend (textEdit->currentLineSelection());
        es.append (tabinfo->redSel);
        textEdit->setExtraSelections (es);
    }

    textEdit->undo();
}
/*************************/
void FPwin::redoing()
{
    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    qobject_cast< TextEdit *>(ui->tabWidget->widget (index))->redo();
}
/*************************/
// Change the window title and the search entry when switching tabs and...
void FPwin::tabSwitch (int index)
{
    if (index == -1)
    {
        setWindowTitle (tr("%1[*]").arg ("FeatherPad"));
        setWindowModified (false);
        return;
    }

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    tabInfo *tabinfo = tabsInfo_[textEdit];
    QString fname = tabinfo->fileName;
    bool modified (textEdit->document()->isModified());

    QString shownName;
    if (modified)
    {
        if (fname.isEmpty())
            shownName = tr ("*Untitled");
        else
            shownName = QFileInfo (fname).fileName().prepend("*");
    }
    else
    {
        if (fname.isEmpty())
            shownName = tr ("Untitled");
        else
            shownName = QFileInfo (fname).fileName();
    }
    setWindowTitle (shownName);

    /* also change the search entry */
    QString txt = tabinfo->searchEntry;
    ui->lineEdit->setText (txt);
    /* although the window size, wrapping state or replacing text may have changed or
       the replace dock may have been closed, hlight() will be called automatically */
    //if (!txt.isEmpty()) hlight();

    /* correct the encoding menu */
    encodingToCheck (tabinfo->encoding);

    /* correct the states of some buttons */
    ui->actionUndo->setEnabled (textEdit->document()->isUndoAvailable());
    ui->actionRedo->setEnabled (textEdit->document()->isRedoAvailable());
    ui->actionSave->setEnabled (modified);
    if (fname.isEmpty())
        ui->actionReload->setEnabled (false);
    else
        ui->actionReload->setEnabled (true);
    bool readOnly = textEdit->isReadOnly();
    if (fname.isEmpty()
        && !modified
        && !textEdit->document()->isEmpty()) // 'Help' is the exception
    {
        ui->actionEdit->setVisible (false);
    }
    else
    {
        ui->actionEdit->setVisible (readOnly);
    }
    ui->actionPaste->setEnabled (!readOnly);
    bool textIsSelected = textEdit->textCursor().hasSelection();
    ui->actionCopy->setEnabled (textIsSelected);
    ui->actionCut->setEnabled (!readOnly & textIsSelected);
    ui->actionDelete->setEnabled (!readOnly & textIsSelected);

    /* handle the spinbox */
    if (ui->spinBox->isVisible())
        ui->spinBox->setMaximum (textEdit->document()->blockCount());

    /* handle the statusbar */
    if (ui->statusBar->isVisible())
    {
        statusMsgWithLineCount (textEdit->document()->blockCount());
        QToolButton *wordButton = ui->statusBar->findChild<QToolButton *>();
        if (tabinfo->wordNumber == -1)
        {
            if (wordButton)
                wordButton->setVisible (true);
            if (textEdit->document()->isEmpty()) // make an exception
                wordButtonStatus();
        }
        else
        {
            if (wordButton)
                wordButton->setVisible (false);
            QLabel *statusLabel = ui->statusBar->findChild<QLabel *>();
            statusLabel->setText (tr ("%1 <i>%2</i>")
                                  .arg (statusLabel->text())
                                  .arg (tabinfo->wordNumber));
        }
    }
}
/*************************/
void FPwin::fontDialog()
{
    if (isLoading()) return;

    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    if (hasAnotherDialog()) return;
    disableShortcuts (true);

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));

    QFont currentFont = textEdit->document()->defaultFont();
    QFontDialog fd (currentFont, this);
    fd.setOption (QFontDialog::DontUseNativeDialog);
    fd.setWindowModality (Qt::WindowModal);
    fd.move (this->x() + this->width()/2 - fd.width()/2,
             this->y() + this->height()/2 - fd.height()/ 2);
    if (fd.exec())
    {
        QFont newFont = fd.selectedFont();
        FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
        for (int i = 0; i < singleton->Wins.count(); ++i)
        {
            FPwin *thisWin = singleton->Wins.at (i);
            QHash<TextEdit*,tabInfo*>::iterator it;
            for (it = thisWin->tabsInfo_.begin(); it != thisWin->tabsInfo_.end(); ++it)
            {
                it.key()->document()->setDefaultFont (newFont);
                QFontMetrics metrics (newFont);
                it.key()->setTabStopWidth (4 * metrics.width (' '));
            }
        }

        Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
        if (config.getRemFont())
            config.setFont (newFont);

        /* the font can become larger... */
        QTimer::singleShot (0, textEdit, SLOT (updateEditorGeometry()));
        /* ... or smaller */
        formatTextRect (textEdit->rect());
        if (!tabsInfo_[textEdit]->searchEntry.isEmpty())
            hlight();
    }
    disableShortcuts (false);
}
/*************************/
void FPwin::changeEvent (QEvent *event)
{
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    if (config.getRemSize() && event->type() == QEvent::WindowStateChange)
    {
        if (windowState() == Qt::WindowFullScreen)
        {
            config.setIsFull (true);
            config.setIsMaxed (false);
        }
        else if (windowState() == (Qt::WindowFullScreen ^ Qt::WindowMaximized))
        {
            config.setIsFull (true);
            config.setIsMaxed (true);
        }
        else
        {
            config.setIsFull (false);
            if (windowState() == Qt::WindowMaximized)
                config.setIsMaxed (true);
            else
                config.setIsMaxed (false);
        }
    }
    QWidget::changeEvent (event);
}
/*************************/
void FPwin::jumpTo()
{
    if (isLoading()) return;

    bool visibility = ui->spinBox->isVisible();

    QHash<TextEdit*,tabInfo*>::iterator it;
    for (it = tabsInfo_.begin(); it != tabsInfo_.end(); ++it)
    {
        if (!ui->actionLineNumbers->isChecked())
            it.key()->showLineNumbers (!visibility);

        if (!visibility)
        {
            /* setMaximum() isn't a slot */
            connect (it.key()->document(),
                     &QTextDocument::blockCountChanged,
                     this,
                     &FPwin::setMax);
        }
        else
            disconnect (it.key()->document(),
                        &QTextDocument::blockCountChanged,
                        this,
                        &FPwin::setMax);
    }

    if (!visibility && ui->tabWidget->count() > 0)
        ui->spinBox->setMaximum (qobject_cast< TextEdit *>(ui->tabWidget->currentWidget())
                                 ->document()
                                 ->blockCount());
    ui->spinBox->setVisible (!visibility);
    ui->label->setVisible (!visibility);
    ui->checkBox->setVisible (!visibility);
    if (!visibility)
    {
        ui->spinBox->setFocus();
        ui->spinBox->selectAll();
    }
    else /* return focus to doc */
        qobject_cast< TextEdit *>(ui->tabWidget->currentWidget())->setFocus();
}
/*************************/
void FPwin::setMax (const int max)
{
    ui->spinBox->setMaximum (max);
}
/*************************/
void FPwin::goTo()
{
    /* workaround for not being able to use returnPressed()
       because of protectedness of spinbox's QLineEdit */
    if (!ui->spinBox->hasFocus()) return;

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->currentWidget());
    QTextBlock block = textEdit->document()->findBlockByNumber (ui->spinBox->value() - 1);
    int pos = block.position();
    QTextCursor start = textEdit->textCursor();
    if (ui->checkBox->isChecked())
        start.setPosition (pos, QTextCursor::KeepAnchor);
    else
        start.setPosition (pos);
    textEdit->setTextCursor (start);
}
/*************************/
void FPwin::showLN (bool checked)
{
    int count = ui->tabWidget->count();
    if (count == 0) return;

    if (checked)
    {
        for (int i = 0; i < count; ++i)
            qobject_cast< TextEdit *>(ui->tabWidget->widget (i))->showLineNumbers (true);
    }
    else if (!ui->spinBox->isVisible()) // also the spinBox affects line numbers visibility
    {
        for (int i = 0; i < count; ++i)
            qobject_cast< TextEdit *>(ui->tabWidget->widget (i))->showLineNumbers (false);
    }
}
/*************************/
void FPwin::toggleWrapping()
{
    int count = ui->tabWidget->count();
    if (count == 0) return;

    if (ui->actionWrap->isChecked())
    {
        for (int i = 0; i < count; ++i)
            qobject_cast< TextEdit *>(ui->tabWidget->widget (i))->setLineWrapMode (QPlainTextEdit::WidgetWidth);
    }
    else
    {
        QHash<TextEdit*,tabInfo*>::iterator it;
        for (it = tabsInfo_.begin(); it != tabsInfo_.end(); ++it)
            it.key()->setLineWrapMode (QPlainTextEdit::NoWrap);
    }
}
/*************************/
void FPwin::toggleIndent()
{
    int count = ui->tabWidget->count();
    if (count == 0) return;

    if (ui->actionIndent->isChecked())
    {
        QHash<TextEdit*,tabInfo*>::iterator it;
        for (it = tabsInfo_.begin(); it != tabsInfo_.end(); ++it)
            it.key()->setAutoIndentation (true);
    }
    else
    {
        QHash<TextEdit*,tabInfo*>::iterator it;
        for (it = tabsInfo_.begin(); it != tabsInfo_.end(); ++it)
            it.key()->setAutoIndentation (false);
    }
}
/*************************/
void FPwin::encodingToCheck (const QString& encoding)
{
    if (encoding != "UTF-8")
        ui->actionOther->setDisabled (true);

    if (encoding == "UTF-8")
        ui->actionUTF_8->setChecked (true);
    else if (encoding == "UTF-16")
        ui->actionUTF_16->setChecked (true);
    else if (encoding == "CP1256")
        ui->actionWindows_Arabic->setChecked (true);
    else if (encoding == "ISO-8859-1")
        ui->actionISO_8859_1->setChecked (true);
    else if (encoding == "ISO-8859-15")
        ui->actionISO_8859_15->setChecked (true);
    else if (encoding == "CP1252")
        ui->actionWindows_1252->setChecked (true);
    else if (encoding == "CP1251")
        ui->actionCryllic_CP1251->setChecked (true);
    else if (encoding == "KOI8-U")
        ui->actionCryllic_KOI8_U->setChecked (true);
    else if (encoding == "ISO-8859-5")
        ui->actionCryllic_ISO_8859_5->setChecked (true);
    else if (encoding == "BIG5")
        ui->actionChineese_BIG5->setChecked (true);
    else if (encoding == "B18030")
        ui->actionChinese_GB18030->setChecked (true);
    else if (encoding == "ISO-2022-JP")
        ui->actionJapanese_ISO_2022_JP->setChecked (true);
    else if (encoding == "ISO-2022-JP-2")
        ui->actionJapanese_ISO_2022_JP_2->setChecked (true);
    else if (encoding == "ISO-2022-KR")
        ui->actionJapanese_ISO_2022_KR->setChecked (true);
    else if (encoding == "CP932")
        ui->actionJapanese_CP932->setChecked (true);
    else if (encoding == "EUC-JP")
        ui->actionJapanese_EUC_JP->setChecked (true);
    else if (encoding == "CP949")
        ui->actionKorean_CP949->setChecked (true);
    else if (encoding == "CP1361")
        ui->actionKorean_CP1361->setChecked (true);
    else if (encoding == "EUC-KR")
        ui->actionKorean_EUC_KR->setChecked (true);
    else
    {
        ui->actionOther->setDisabled (false);
        ui->actionOther->setChecked (true);
    }
}
/*************************/
const QString FPwin::checkToEncoding() const
{
    QString encoding;

    if (ui->actionUTF_8->isChecked())
        encoding = "UTF-8";
    else if (ui->actionUTF_16->isChecked())
        encoding = "UTF-16";
    else if (ui->actionWindows_Arabic->isChecked())
        encoding = "CP1256";
    else if (ui->actionISO_8859_1->isChecked())
        encoding = "ISO-8859-1";
    else if (ui->actionISO_8859_15->isChecked())
        encoding = "ISO-8859-15";
    else if (ui->actionWindows_1252->isChecked())
        encoding = "CP1252";
    else if (ui->actionCryllic_CP1251->isChecked())
        encoding = "CP1251";
    else if (ui->actionCryllic_KOI8_U->isChecked())
        encoding = "KOI8-U";
    else if (ui->actionCryllic_ISO_8859_5->isChecked())
        encoding = "ISO-8859-5";
    else if (ui->actionChineese_BIG5->isChecked())
        encoding = "BIG5";
    else if (ui->actionChinese_GB18030->isChecked())
        encoding = "B18030";
    else if (ui->actionJapanese_ISO_2022_JP->isChecked())
        encoding = "ISO-2022-JP";
    else if (ui->actionJapanese_ISO_2022_JP_2->isChecked())
        encoding = "ISO-2022-JP-2";
    else if (ui->actionJapanese_ISO_2022_KR->isChecked())
        encoding = "ISO-2022-KR";
    else if (ui->actionJapanese_CP932->isChecked())
        encoding = "CP932";
    else if (ui->actionJapanese_EUC_JP->isChecked())
        encoding = "EUC-JP";
    else if (ui->actionKorean_CP949->isChecked())
        encoding == "CP949";
    else if (ui->actionKorean_CP1361->isChecked())
        encoding = "CP1361";
    else if (ui->actionKorean_EUC_KR->isChecked())
        encoding = "EUC-KR";
    else
        encoding = "UTF-8";

    return encoding;
}
/*************************/
void FPwin::docProp()
{
    QHash<TextEdit*,tabInfo*>::iterator it;
    if (ui->statusBar->isVisible())
    {
        for (it = tabsInfo_.begin(); it != tabsInfo_.end(); ++it)
        {
            disconnect (it.key(), &QPlainTextEdit::blockCountChanged, this, &FPwin::statusMsgWithLineCount);
            disconnect (it.key(), &QPlainTextEdit::selectionChanged, this, &FPwin::statusMsg);
        }
        ui->statusBar->setVisible (false);
        return;
    }

    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    statusMsgWithLineCount (qobject_cast< TextEdit *>(ui->tabWidget->widget (index))
               ->document()->blockCount());
    for (it = tabsInfo_.begin(); it != tabsInfo_.end(); ++it)
    {
        connect (it.key(), &QPlainTextEdit::blockCountChanged, this, &FPwin::statusMsgWithLineCount);
        connect (it.key(), &QPlainTextEdit::selectionChanged, this, &FPwin::statusMsg);
    }

    ui->statusBar->setVisible (true);
    if (QToolButton *wordButton = ui->statusBar->findChild<QToolButton *>())
        wordButton->setVisible (true);
    wordButtonStatus();
}
/*************************/
// Set the status bar text according to the block count.
void FPwin::statusMsgWithLineCount (const int lines)
{
    int index = ui->tabWidget->currentIndex();
    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    /* ensure that the signal comes from the active tab if this is about a tab a signal */
    if (qobject_cast<TextEdit*>(QObject::sender()) && QObject::sender() != textEdit)
        return;

    tabInfo *tabinfo = tabsInfo_[textEdit];
    QLabel *statusLabel = ui->statusBar->findChild<QLabel *>();

    if (!tabinfo->prog.isEmpty())
        statusLabel->setText (tr ("<b>Encoding:</b> <i>%1</i>"
                                  "&nbsp;&nbsp;&nbsp;&nbsp;<b>Syntax:</b> <i>%2</i>"
                                  "&nbsp;&nbsp;&nbsp;&nbsp;<b>Lines:</b> <i>%3</i>"
                                  "&nbsp;&nbsp;&nbsp;&nbsp;<b>Sel. Chars.:</b> <i>%4</i>"
                                  "&nbsp;&nbsp;&nbsp;&nbsp;<b>Words:</b>")
                              .arg (tabinfo->encoding)
                              .arg (tabinfo->prog)
                              .arg (QString::number (lines))
                              .arg (textEdit->textCursor().selectedText().size()));
    else
        statusLabel->setText (tr ("<b>Encoding:</b> <i>%1</i>"
                                  "&nbsp;&nbsp;&nbsp;&nbsp;<b>Lines:</b> <i>%2</i>"
                                  "&nbsp;&nbsp;&nbsp;&nbsp;<b>Sel. Chars.:</b> <i>%3</i>"
                                  "&nbsp;&nbsp;&nbsp;&nbsp;<b>Words:</b>")
                              .arg (tabinfo->encoding)
                              .arg (QString::number (lines))
                              .arg (textEdit->textCursor().selectedText().size()));
}
/*************************/
// Change the status bar text when the selection changes.
void FPwin::statusMsg()
{
    QLabel *statusLabel = ui->statusBar->findChild<QLabel *>();
    QString charN;
    charN.setNum (qobject_cast< TextEdit *>(ui->tabWidget->currentWidget())
                  ->textCursor().selectedText().size());
    QString str = statusLabel->text();
    int i = str.indexOf ("Sel.");
    int j = str.indexOf ("</i>&nbsp;&nbsp;&nbsp;&nbsp;<b>Words");
    str.replace (i + 20, j - i - 20, charN);
    statusLabel->setText (str);
}
/*************************/
void FPwin::wordButtonStatus()
{
    QToolButton *wordButton = ui->statusBar->findChild<QToolButton *>();
    if (!wordButton) return;
    int index = ui->tabWidget->currentIndex();
    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    /* ensure that the signal comes from the active tab if this is about a tab a signal */
    if (qobject_cast<TextEdit*>(QObject::sender()) && QObject::sender() != textEdit)
        return;

    tabInfo *tabinfo = tabsInfo_[textEdit];
    if (wordButton->isVisible())
    {
        QLabel *statusLabel = ui->statusBar->findChild<QLabel *>();
        int words = tabinfo->wordNumber;
        if (words == -1)
        {
            words = textEdit->toPlainText()
                    .split (QRegExp("(\\s|\\n|\\r)+"), QString::SkipEmptyParts)
                    .count();
            tabinfo->wordNumber = words;
        }

        wordButton->setVisible (false);
        statusLabel->setText (tr ("%1 <i>%2</i>")
                              .arg (statusLabel->text())
                              .arg (words));
        connect (textEdit, &QPlainTextEdit::textChanged, this, &FPwin::wordButtonStatus);
    }
    else
    {
        disconnect (textEdit, &QPlainTextEdit::textChanged, this, &FPwin::wordButtonStatus);
        tabinfo->wordNumber = -1;
        wordButton->setVisible (true);
        statusMsgWithLineCount (textEdit->document()->blockCount());
    }
}
/*************************/
void FPwin::filePrint()
{
    if (isLoading()) return;

    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    if (hasAnotherDialog()) return;
    disableShortcuts (true);

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    QPrinter printer (QPrinter::HighResolution);

    /* choose an appropriate name and directory */
    QString fileName = tabsInfo_[textEdit]->fileName;
    if (fileName.isEmpty())
    {
        QDir dir = QDir::home();
        fileName= dir.filePath ("Untitled");
    }
    if (printer.outputFormat() == QPrinter::PdfFormat)
        printer.setOutputFileName (fileName.append (".pdf"));
    /*else if (printer.outputFormat() == QPrinter::PostScriptFormat)
        printer.setOutputFileName (fileName.append (".ps"));*/

    QPrintDialog dlg (&printer, this);
    dlg.setWindowModality (Qt::WindowModal);
    if (textEdit->textCursor().hasSelection())
        dlg.setOption (QAbstractPrintDialog::PrintSelection);
    dlg.setWindowTitle (tr ("Print Document"));
    if (dlg.exec() == QDialog::Accepted)
        textEdit->print (&printer);

    disableShortcuts (false);
}
/*************************/
void FPwin::nextTab()
{
    if (isLoading()) return;

    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    if (QWidget *widget = ui->tabWidget->widget (index + 1))
        ui->tabWidget->setCurrentWidget (widget);
}
/*************************/
void FPwin::previousTab()
{
    if (isLoading()) return;

    int index = ui->tabWidget->currentIndex();
    if (index == -1) return;

    if (QWidget *widget = ui->tabWidget->widget (index - 1))
        ui->tabWidget->setCurrentWidget (widget);
}
/*************************/
void FPwin::lastTab()
{
    if (isLoading()) return;

    int count = ui->tabWidget->count();
    if (count > 0)
        ui->tabWidget->setCurrentIndex (count - 1);
}
/*************************/
void FPwin::firstTab()
{
    if (isLoading()) return;

    if ( ui->tabWidget->count() > 0)
        ui->tabWidget->setCurrentIndex (0);
}
/*************************/
void FPwin::detachTab()
{
    if (isLoading()) return;

    int index = ui->tabWidget->currentIndex();
    if (index == -1 || ui->tabWidget->count() == 1)
    {
        ui->tabWidget->tabBar()->finishMouseMoveEvent();
        return;
    }

    /*****************************************************
     *****          Get all necessary info.          *****
     ***** Then, remove the tab but keep its widget. *****
     *****************************************************/

    QString tooltip = ui->tabWidget->tabToolTip (index);
    QString tabText = ui->tabWidget->tabText (index);
    QString title = windowTitle();
    bool hl = true;
    bool spin = false;
    bool ln = false;
    bool status = false;
    if (!ui->actionSyntax->isChecked())
        hl = false;
    if (ui->spinBox->isVisible())
        spin = true;
    if (ui->actionLineNumbers->isChecked())
        ln = true;
    if (ui->statusBar->isVisible())
        status = true;

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));

    disconnect (textEdit, &TextEdit::updateRect, this ,&FPwin::hlighting);
    disconnect (textEdit, &QPlainTextEdit::textChanged, this ,&FPwin::hlight);
    disconnect (textEdit, &QPlainTextEdit::textChanged, this, &FPwin::wordButtonStatus);
    disconnect (textEdit, &QPlainTextEdit::blockCountChanged, this, &FPwin::statusMsgWithLineCount);
    disconnect (textEdit, &QPlainTextEdit::selectionChanged, this, &FPwin::statusMsg);
    disconnect (textEdit, &QPlainTextEdit::copyAvailable, ui->actionCut, &QAction::setEnabled);
    disconnect (textEdit, &QPlainTextEdit::copyAvailable, ui->actionDelete, &QAction::setEnabled);
    disconnect (textEdit, &QPlainTextEdit::copyAvailable, ui->actionCopy, &QAction::setEnabled);
    disconnect (textEdit, &TextEdit::fileDropped, this, &FPwin::newTabFromName);
    disconnect (textEdit, &QPlainTextEdit::cursorPositionChanged, this, &FPwin::matchBrackets);
    disconnect (textEdit, &QPlainTextEdit::blockCountChanged, this, &FPwin::formatOnBlockChange);
    disconnect (textEdit, &TextEdit::updateRect, this, &FPwin::formatVisibleText);
    disconnect (textEdit, &TextEdit::resized, this, &FPwin::formatonResizing);

    disconnect (textEdit->document(), &QTextDocument::blockCountChanged, this, &FPwin::setMax);
    disconnect (textEdit->document(), &QTextDocument::modificationChanged, this, &FPwin::asterisk);
    disconnect (textEdit->document(), &QTextDocument::undoAvailable, ui->actionUndo, &QAction::setEnabled);
    disconnect (textEdit->document(), &QTextDocument::redoAvailable, ui->actionRedo, &QAction::setEnabled);
    disconnect (textEdit->document(), &QTextDocument::modificationChanged, ui->actionSave, &QAction::setEnabled);

    /* for tabbar to be updated peoperly with tab reordering during a
       fast drag-and-drop, mouse should be released before tab removal */
    ui->tabWidget->tabBar()->releaseMouse();

    tabInfo *tabinfo = tabsInfo_[textEdit];
    tabsInfo_.remove (textEdit);
    ui->tabWidget->removeTab (index);
    if (ui->tabWidget->count() == 1)
    {
        ui->actionDetachTab->setDisabled (true);
        ui->actionRightTab->setDisabled (true);
        ui->actionLeftTab->setDisabled (true);
        ui->actionLastTab->setDisabled (true);
        ui->actionFirstTab->setDisabled (true);
    }

    /*******************************************************************
     ***** create a new window and replace its tab by this widget. *****
     *******************************************************************/

    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    FPwin * dropTarget = singleton->newWin ("");
    dropTarget->closeTabAtIndex (0);

    /* first, set the new info... */
    dropTarget->lastFile_ = tabinfo->fileName;
    tabinfo->greenSel = QList<QTextEdit::ExtraSelection>();
    tabinfo->redSel = QList<QTextEdit::ExtraSelection>();
    dropTarget->tabsInfo_[textEdit] = tabinfo;
    /* ... then insert the detached widget... */
    dropTarget->ui->tabWidget->insertTab (0, textEdit, tabText);
    /* ... and remove all yellow and green highlights
       (the yellow ones will be recreated later if needed) */
    QList<QTextEdit::ExtraSelection> es;
    if (ln || spin)
        es.prepend (textEdit->currentLineSelection());
    textEdit->setExtraSelections (es);

    /* at last, set all properties correctly */
    dropTarget->enableWidgets (true);
    dropTarget->setWindowTitle (title);
    dropTarget->ui->tabWidget->setTabToolTip (0, tooltip);
    /* reload buttons, syntax highlighting, jump bar, line numbers */
    dropTarget->encodingToCheck (tabinfo->encoding);
    if (!tabinfo->fileName.isEmpty())
        dropTarget->ui->actionReload->setEnabled (true);
    if (!hl)
        dropTarget->ui->actionSyntax->setChecked (false);
    if (spin)
    {
        dropTarget->ui->spinBox->setVisible (true);
        dropTarget->ui->label->setVisible (true);
        dropTarget->ui->spinBox->setMaximum (textEdit->document()->blockCount());
        connect (textEdit->document(), &QTextDocument::blockCountChanged, dropTarget, &FPwin::setMax);
    }
    if (ln)
        dropTarget->ui->actionLineNumbers->setChecked (true);
    /* searching */
    if (!tabinfo->searchEntry.isEmpty())
    {
        if (!dropTarget->ui->lineEdit->isVisible())
            dropTarget->showHideSearch();
        connect (textEdit, &QPlainTextEdit::textChanged, dropTarget, &FPwin::hlight);
        connect (textEdit, &TextEdit::updateRect, dropTarget, &FPwin::hlighting);
        /* restore yellow highlights, which will automatically
           set the current line highlight if needed because the
           spin button and line number menuitem are set above */
        dropTarget->hlight();
    }
    /* status bar */
    if (status)
    {
        dropTarget->ui->statusBar->setVisible (true);
        dropTarget->statusMsgWithLineCount (textEdit->document()->blockCount());
        if (tabinfo->wordNumber == -1)
        {
            if (QToolButton *wordButton = dropTarget->ui->statusBar->findChild<QToolButton *>())
                wordButton->setVisible (true);
        }
        else
        {
            if (QToolButton *wordButton = dropTarget->ui->statusBar->findChild<QToolButton *>())
                wordButton->setVisible (false);
            QLabel *statusLabel = dropTarget->ui->statusBar->findChild<QLabel *>();
            statusLabel->setText (tr ("%1 <i>%2</i>")
                                  .arg (statusLabel->text())
                                  .arg (tabinfo->wordNumber));
            connect (textEdit, &QPlainTextEdit::textChanged, dropTarget, &FPwin::wordButtonStatus);
        }
        connect (textEdit, &QPlainTextEdit::blockCountChanged, dropTarget, &FPwin::statusMsgWithLineCount);
        connect (textEdit, &QPlainTextEdit::selectionChanged, dropTarget, &FPwin::statusMsg);
    }
    if (textEdit->lineWrapMode() == QPlainTextEdit::NoWrap)
        dropTarget->ui->actionWrap->setChecked (false);
    /* auto indentation */
    if (textEdit->getAutoIndentation() == false)
        dropTarget->ui->actionIndent->setChecked (false);
    /* the remaining signals */
    connect (textEdit->document(), &QTextDocument::undoAvailable, dropTarget->ui->actionUndo, &QAction::setEnabled);
    connect (textEdit->document(), &QTextDocument::redoAvailable, dropTarget->ui->actionRedo, &QAction::setEnabled);
    connect (textEdit->document(), &QTextDocument::modificationChanged, dropTarget->ui->actionSave, &QAction::setEnabled);
    connect (textEdit->document(), &QTextDocument::modificationChanged, dropTarget, &FPwin::asterisk);
    connect (textEdit, &QPlainTextEdit::copyAvailable, dropTarget->ui->actionCopy, &QAction::setEnabled);
    if (!textEdit->isReadOnly())
    {
        connect (textEdit, &QPlainTextEdit::copyAvailable, dropTarget->ui->actionCut, &QAction::setEnabled);
        connect (textEdit, &QPlainTextEdit::copyAvailable, dropTarget->ui->actionDelete, &QAction::setEnabled);
    }
    connect (textEdit, &TextEdit::fileDropped, dropTarget, &FPwin::newTabFromName);

    if (tabinfo->highlighter)
    {
        dropTarget->matchBrackets();
        connect (textEdit, &QPlainTextEdit::cursorPositionChanged, dropTarget, &FPwin::matchBrackets);
        connect (textEdit, &QPlainTextEdit::blockCountChanged, dropTarget, &FPwin::formatOnBlockChange);
        connect (textEdit, &TextEdit::updateRect, dropTarget, &FPwin::formatVisibleText);
        connect (textEdit, &TextEdit::resized, dropTarget, &FPwin::formatonResizing);
    }

    textEdit->setFocus();

    dropTarget->activateWindow();
    dropTarget->raise();
}
/*************************/
void FPwin::dropTab (QString str)
{
    QStringList list = str.split ("+");
    if (list.count() != 2)
    {
        ui->tabWidget->tabBar()->finishMouseMoveEvent();
        return;
    }
    int index = list.at (1).toInt();
    if (index == -1) // impossible
    {
        ui->tabWidget->tabBar()->finishMouseMoveEvent();
        return;
    }

    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    FPwin *dragSource = nullptr;
    for (int i = 0; i < singleton->Wins.count(); ++i)
    {
        if (singleton->Wins.at (i)->winId() == (uint) list.at (0).toInt())
        {
            dragSource = singleton->Wins.at (i);
            break;
        }
    }
    if (dragSource == this
        || dragSource == nullptr) // impossible
    {
        ui->tabWidget->tabBar()->finishMouseMoveEvent();
        return;
    }

    QString tooltip = dragSource->ui->tabWidget->tabToolTip (index);
    QString tabText = dragSource->ui->tabWidget->tabText (index);
    bool spin = false;
    bool ln = false;
    if (dragSource->ui->spinBox->isVisible())
        spin = true;
    if (dragSource->ui->actionLineNumbers->isChecked())
        ln = true;

    TextEdit *textEdit = qobject_cast< TextEdit *>(dragSource->ui->tabWidget->widget (index));

    disconnect (textEdit, &TextEdit::updateRect, dragSource ,&FPwin::hlighting);
    disconnect (textEdit, &QPlainTextEdit::textChanged, dragSource ,&FPwin::hlight);
    disconnect (textEdit, &QPlainTextEdit::textChanged, dragSource, &FPwin::wordButtonStatus);
    disconnect (textEdit, &QPlainTextEdit::blockCountChanged, dragSource, &FPwin::statusMsgWithLineCount);
    disconnect (textEdit, &QPlainTextEdit::selectionChanged, dragSource, &FPwin::statusMsg);
    disconnect (textEdit, &QPlainTextEdit::copyAvailable, dragSource->ui->actionCut, &QAction::setEnabled);
    disconnect (textEdit, &QPlainTextEdit::copyAvailable, dragSource->ui->actionDelete, &QAction::setEnabled);
    disconnect (textEdit, &QPlainTextEdit::copyAvailable, dragSource->ui->actionCopy, &QAction::setEnabled);
    disconnect (textEdit, &TextEdit::fileDropped, dragSource, &FPwin::newTabFromName);
    disconnect (textEdit, &QPlainTextEdit::cursorPositionChanged, dragSource, &FPwin::matchBrackets);
    disconnect (textEdit, &QPlainTextEdit::blockCountChanged, dragSource, &FPwin::formatOnBlockChange);
    disconnect (textEdit, &TextEdit::updateRect, dragSource, &FPwin::formatVisibleText);
    disconnect (textEdit, &TextEdit::resized, dragSource, &FPwin::formatonResizing);

    disconnect (textEdit->document(), &QTextDocument::blockCountChanged, dragSource, &FPwin::setMax);
    disconnect (textEdit->document(), &QTextDocument::modificationChanged, dragSource, &FPwin::asterisk);
    disconnect (textEdit->document(), &QTextDocument::undoAvailable, dragSource->ui->actionUndo, &QAction::setEnabled);
    disconnect (textEdit->document(), &QTextDocument::redoAvailable, dragSource->ui->actionRedo, &QAction::setEnabled);
    disconnect (textEdit->document(), &QTextDocument::modificationChanged, dragSource->ui->actionSave, &QAction::setEnabled);

    /* it's important to release mouse before tab removal because otherwise, the source
       tabbar might not be updated properly with tab reordering during a fast drag-and-drop */
    dragSource->ui->tabWidget->tabBar()->releaseMouse();

    tabInfo *tabinfo = dragSource->tabsInfo_[textEdit];
    dragSource->tabsInfo_.remove (textEdit);
    dragSource->ui->tabWidget->removeTab (index);
    int count = dragSource->ui->tabWidget->count();
    if (count == 1)
    {
        dragSource->ui->actionDetachTab->setDisabled (true);
        dragSource->ui->actionRightTab->setDisabled (true);
        dragSource->ui->actionLeftTab->setDisabled (true);
        dragSource->ui->actionLastTab->setDisabled (true);
        dragSource->ui->actionFirstTab->setDisabled (true);
    }

    /***************************************************************************
     ***** The tab is dropped into this window; so insert it as a new tab. *****
     ***************************************************************************/

    int insertIndex = ui->tabWidget->currentIndex() + 1;

    /* first, set the new info... */
    lastFile_ = tabinfo->fileName;
    tabinfo->greenSel = QList<QTextEdit::ExtraSelection>();
    tabinfo->redSel = QList<QTextEdit::ExtraSelection>();
    tabsInfo_[textEdit] = tabinfo;
    /* ... then insert the detached widget... */
    ui->tabWidget->insertTab (insertIndex, textEdit, tabText);
    ui->tabWidget->setCurrentIndex (insertIndex);
    /* ... and remove all yellow and green highlights
       (the yellow ones will be recreated later if needed) */
    QList<QTextEdit::ExtraSelection> es;
    if ((ln || spin)
        && (ui->actionLineNumbers->isChecked() || ui->spinBox->isVisible()))
    {
        es.prepend (textEdit->currentLineSelection());
    }
    textEdit->setExtraSelections (es);

    /* at last, set all properties correctly */
    if (ui->tabWidget->count() == 1)
        enableWidgets (true);
    ui->tabWidget->setTabToolTip (insertIndex, tooltip);
    /* detach button */
    if (ui->tabWidget->count() == 2)
    {
        ui->actionDetachTab->setEnabled (true);
        ui->actionRightTab->setEnabled (true);
        ui->actionLeftTab->setEnabled (true);
        ui->actionLastTab->setEnabled (true);
        ui->actionFirstTab->setEnabled (true);
    }
    /* reload buttons, syntax highlighting, jump bar, line numbers */
    if (ui->actionSyntax->isChecked() && !tabinfo->highlighter)
        syntaxHighlighting (tabinfo);
    else if (!ui->actionSyntax->isChecked() && tabinfo->highlighter)
    {
        Highlighter *highlighter = tabinfo->highlighter;
        tabinfo->highlighter = nullptr;
        delete highlighter; highlighter = nullptr;
    }
    if (ui->spinBox->isVisible())
        connect (textEdit->document(), &QTextDocument::blockCountChanged, this, &FPwin::setMax);
    if (ui->actionLineNumbers->isChecked() || ui->spinBox->isVisible())
        textEdit->showLineNumbers (true);
    else
        textEdit->showLineNumbers (false);
    /* searching */
    if (!tabinfo->searchEntry.isEmpty())
    {
        if (!ui->lineEdit->isVisible())
            showHideSearch();
        connect (textEdit, &QPlainTextEdit::textChanged, this, &FPwin::hlight);
        connect (textEdit, &TextEdit::updateRect, this, &FPwin::hlighting);
        /* restore yellow highlights, which will automatically
           set the current line highlight if needed because the
           spin button and line number menuitem are set above */
        hlight();
    }
    /* status bar */
    if (ui->statusBar->isVisible())
    {
        connect (textEdit, &QPlainTextEdit::blockCountChanged, this, &FPwin::statusMsgWithLineCount);
        connect (textEdit, &QPlainTextEdit::selectionChanged, this, &FPwin::statusMsg);
        if (tabinfo->wordNumber != 1)
            connect (textEdit, &QPlainTextEdit::textChanged, this, &FPwin::wordButtonStatus);
    }
    if (ui->actionWrap->isChecked() && textEdit->lineWrapMode() == QPlainTextEdit::NoWrap)
        textEdit->setLineWrapMode (QPlainTextEdit::WidgetWidth);
    else if (!ui->actionWrap->isChecked() && textEdit->lineWrapMode() == QPlainTextEdit::WidgetWidth)
        textEdit->setLineWrapMode (QPlainTextEdit::NoWrap);
    /* auto indentation */
    if (ui->actionIndent->isChecked() && textEdit->getAutoIndentation() == false)
        textEdit->setAutoIndentation (true);
    else if (!ui->actionIndent->isChecked() && textEdit->getAutoIndentation() == true)
        textEdit->setAutoIndentation (false);
    /* the remaining signals */
    connect (textEdit->document(), &QTextDocument::undoAvailable, ui->actionUndo, &QAction::setEnabled);
    connect (textEdit->document(), &QTextDocument::redoAvailable, ui->actionRedo, &QAction::setEnabled);
    connect (textEdit->document(), &QTextDocument::modificationChanged, ui->actionSave, &QAction::setEnabled);
    connect (textEdit->document(), &QTextDocument::modificationChanged, this, &FPwin::asterisk);
    connect (textEdit, &QPlainTextEdit::copyAvailable, ui->actionCopy, &QAction::setEnabled);
    if (!textEdit->isReadOnly())
    {
        connect (textEdit, &QPlainTextEdit::copyAvailable, ui->actionCut, &QAction::setEnabled);
        connect (textEdit, &QPlainTextEdit::copyAvailable, ui->actionDelete, &QAction::setEnabled);
    }
    connect (textEdit, &TextEdit::fileDropped, this, &FPwin::newTabFromName);

    if (tabinfo->highlighter) // it's set to NULL above when syntax highlighting is disabled
    {
        matchBrackets();
        connect (textEdit, &QPlainTextEdit::cursorPositionChanged, this, &FPwin::matchBrackets);
        connect (textEdit, &QPlainTextEdit::blockCountChanged, this, &FPwin::formatOnBlockChange);
        connect (textEdit, &TextEdit::updateRect, this, &FPwin::formatVisibleText);
        connect (textEdit, &TextEdit::resized, this, &FPwin::formatonResizing);
    }

    textEdit->setFocus();

    activateWindow();
    raise();

    if (count == 0)
        QTimer::singleShot (0, dragSource, SLOT (close()));
}
/*************************/
void FPwin::tabContextMenu (const QPoint& p)
{
    int tabNum = ui->tabWidget->count();
    QTabBar *tbar = ui->tabWidget->tabBar();
    rightClicked_ = tbar->tabAt (p);
    if (rightClicked_ < 0) return;

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (rightClicked_));
    QString fname = tabsInfo_[textEdit]->fileName;
    QMenu menu;
    if (tabNum > 1)
    {
        if (QApplication::layoutDirection() == Qt::RightToLeft)
        {
            if (rightClicked_ < tabNum - 1)
                menu.addAction (ui->actionCloseLeft);
            if (rightClicked_ > 0)
                menu.addAction (ui->actionCloseRight);
        }
        else
        {
            if (rightClicked_ < tabNum - 1)
                menu.addAction (ui->actionCloseRight);
            if (rightClicked_ > 0)
                menu.addAction (ui->actionCloseLeft);
        }
        menu.addSeparator();
        if (rightClicked_ < tabNum - 1 && rightClicked_ > 0)
            menu.addAction (ui->actionCloseOther);
        menu.addAction (ui->actionCloseAll);
        if (!fname.isEmpty())
            menu.addSeparator();
    }
    if (!fname.isEmpty())
    {
        menu.addAction (ui->actionCopyName);
        menu.addAction (ui->actionCopyPath);
    }
    menu.exec (tbar->mapToGlobal (p));
}
/*************************/
void FPwin::prefDialog()
{
    if (isLoading()) return;

    if (hasAnotherDialog()) return;
    disableShortcuts (true);
    PrefDialog dlg (this);
    /*dlg.show();
    move (x() + width()/2 - dlg.width()/2,
          y() + height()/2 - dlg.height()/ 2);*/
    dlg.exec();
    disableShortcuts (false);
}
/*************************/
void FPwin::aboutDialog()
{
    if (isLoading()) return;

    if (hasAnotherDialog()) return;
    disableShortcuts (true);
    QMessageBox msgBox (this);
    msgBox.setText (tr ("<center><b><big>FeatherPad 0.5.8</big></b></center><br>"));
    msgBox.setInformativeText (tr ("<center> A lightweight, tabbed, plain-text editor </center>\n"\
                                   "<center> based on Qt5 </center><br>"\
                                   "<center> Author: <a href='mailto:tsujan2000@gmail.com?Subject=My%20Subject'>Pedram Pourang (aka. Tsu Jan)</a> </center><p></p>"));
    msgBox.setStandardButtons (QMessageBox::Ok);
    msgBox.setWindowModality (Qt::WindowModal);
    Config config = static_cast<FPsingleton*>(qApp)->getConfig();
    QIcon FPIcon;
    if (config.getSysIcon())
        FPIcon = QIcon::fromTheme ("featherpad");
    else
        FPIcon = QIcon (":icons/featherpad.svg");
    msgBox.setIconPixmap (FPIcon.pixmap(64, 64));
    msgBox.setWindowTitle (tr ("About FeatherPad"));
    msgBox.exec();
    disableShortcuts (false);
}
/*************************/
void FPwin::helpDoc()
{
    int index = ui->tabWidget->currentIndex();
    if (index == -1)
        newTab();
    else
    {
        QHash<TextEdit*,tabInfo*>::iterator it;
        for (it = tabsInfo_.begin(); it != tabsInfo_.end(); ++it)
        {
            if (it.value()->fileName.isEmpty()
                && !it.key()->document()->isModified()
                && !it.key()->document()->isEmpty())
            {
                ui->tabWidget->setCurrentIndex (ui->tabWidget->indexOf (it.key()));
                return;
            }
        }
    }

    QFile helpFile (DATADIR "/featherpad/help");

    if (!helpFile.exists()) return;
    if (!helpFile.open (QFile::ReadOnly)) return;

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->currentWidget());
    if (!textEdit->document()->isEmpty()
        || textEdit->document()->isModified())
    {
        newTab();
        textEdit = qobject_cast< TextEdit *>(ui->tabWidget->currentWidget());
    }

    QByteArray data = helpFile.readAll();
    helpFile.close();
    QTextCodec *codec = QTextCodec::codecForName ("UTF-8");
    QString str = codec->toUnicode (data);
    textEdit->setPlainText (str);

    textEdit->setReadOnly (true);
    if (!textEdit->hasDarkScheme())
        textEdit->viewport()->setStyleSheet (".QWidget {"
                                             "color: black;"
                                             "background-color: rgb(225, 238, 255);}");
    else
        textEdit->viewport()->setStyleSheet (".QWidget {"
                                             "color: white;"
                                             "background-color: rgb(0, 60, 110);}");
    ui->actionCut->setDisabled (true);
    ui->actionPaste->setDisabled (true);
    ui->actionDelete->setDisabled (true);
    disconnect (textEdit, &QPlainTextEdit::copyAvailable, ui->actionCut, &QAction::setEnabled);
    disconnect (textEdit, &QPlainTextEdit::copyAvailable, ui->actionDelete, &QAction::setEnabled);

    index = ui->tabWidget->currentIndex();
    tabInfo *tabinfo = tabsInfo_[textEdit];
    tabinfo->encoding = "UTF-8";
    tabinfo->wordNumber = -1;
    if (ui->statusBar->isVisible())
    {
        statusMsgWithLineCount (textEdit->document()->blockCount());
        if (QToolButton *wordButton = ui->statusBar->findChild<QToolButton *>())
            wordButton->setVisible (true);
    }
    encodingToCheck ("UTF-8");
    ui->tabWidget->setTabText (index, tr ("%1").arg ("** Help **"));
    setWindowTitle (tr ("%1[*]").arg ("** Help **"));
    setWindowModified (false);
    ui->tabWidget->setTabToolTip (index, "** Help **");
}

}
