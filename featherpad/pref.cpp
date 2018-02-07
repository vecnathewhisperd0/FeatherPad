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
 *
 * @license GPL-3.0+ <https://spdx.org/licenses/GPL-3.0+.html>
 */

#include "singleton.h"
#include "ui_fp.h"

#include "pref.h"
#include "ui_predDialog.h"

#include <QDesktopWidget>
#include <QWhatsThis>
#include <QKeySequenceEdit>
#include <QFileInfo>

namespace FeatherPad {

static QHash<QString, QString> OBJECT_NAMES;

Delegate::Delegate (QObject *parent)
    : QStyledItemDelegate (parent)
{
}

QWidget* Delegate::createEditor (QWidget *parent,
                                 const QStyleOptionViewItem& /*option*/,
                                 const QModelIndex& /*index*/) const
{
    return new QKeySequenceEdit (parent);
}
/*************************/
bool Delegate::eventFilter (QObject *object, QEvent *event)
{
    QWidget *editor = qobject_cast<QWidget*>(object);
    if (editor && event->type() == QEvent::KeyPress)
    {
        int k = static_cast<QKeyEvent *>(event)->key();
        if (k == Qt::Key_Return || k == Qt::Key_Enter)
        {
            emit QAbstractItemDelegate::commitData (editor);
            emit QAbstractItemDelegate::closeEditor (editor);
            return true;
        }
    }
    return QStyledItemDelegate::eventFilter (object, event);
}
/*************************/
PrefDialog::PrefDialog (const QHash<QString, QString> &defaultShortcuts, QWidget *parent)
    : QDialog (parent), ui (new Ui::PrefDialog)
{
    ui->setupUi (this);
    parent_ = parent;
    setWindowModality (Qt::WindowModal);
    ui->promptLabel->hide();
    promptTimer_ = nullptr;

    Delegate *del = new Delegate (ui->tableWidget);
    ui->tableWidget->setItemDelegate (del);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode (QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionsClickable (false);
    ui->tableWidget->sortByColumn (0, Qt::AscendingOrder);

    Config config = static_cast<FPsingleton*>(qApp)->getConfig();
    sysIcons_ = config.getSysIcon();
    iconless_ = config.getIconless();
    darkBg_ = config.getDarkColScheme();
    darkColValue_ = config.getDarkBgColorValue();
    lightColValue_ = config.getLightBgColorValue();
    recentNumber_ = config.getRecentFilesNumber();
    showWhiteSpace_ = config.getShowWhiteSpace();
    showEndings_ = config.getShowEndings();
    vLineDistance_ = config.getVLineDistance();

    /**************
     *** Window ***
     **************/

    ui->winSizeBox->setChecked (config.getRemSize());
    connect (ui->winSizeBox, &QCheckBox::stateChanged, this, &PrefDialog::prefSize);
    if (ui->winSizeBox->isChecked())
    {
        ui->spinX->setEnabled (false);
        ui->spinY->setEnabled (false);
        ui->mLabel->setEnabled (false);
        ui->sizeLable->setEnabled (false);
    }
    QSize ag = QApplication::desktop()->availableGeometry().size();
    ui->spinX->setMaximum (ag.width());
    ui->spinY->setMaximum (ag.height());
    ui->spinX->setValue (config.getStartSize().width());
    ui->spinY->setValue (config.getStartSize().height());
    connect (ui->spinX, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
             this, &PrefDialog::prefStartSize);
    connect (ui->spinY, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
             this, &PrefDialog::prefStartSize);

    ui->iconBox->setChecked (!config.getSysIcon());
    ui->iconBox->setEnabled (!config.getIconless());
    connect (ui->iconBox, &QCheckBox::stateChanged, this, &PrefDialog::prefIcon);
    ui->iconlessBox->setChecked (config.getIconless());
    connect (ui->iconlessBox, &QCheckBox::stateChanged, this, &PrefDialog::prefIconless);

    ui->toolbarBox->setChecked (config.getNoToolbar());
    connect (ui->toolbarBox, &QCheckBox::stateChanged, this, &PrefDialog::prefToolbar);
    ui->menubarBox->setChecked (config.getNoMenubar());
    connect (ui->menubarBox, &QCheckBox::stateChanged, this, &PrefDialog::prefMenubar);

    ui->searchbarBox->setChecked (config.getHideSearchbar());
    connect (ui->searchbarBox, &QCheckBox::stateChanged, this, &PrefDialog::prefSearchbar);

    ui->statusBox->setChecked (config.getShowStatusbar());
    connect (ui->statusBox, &QCheckBox::stateChanged, this, &PrefDialog::prefStatusbar);

    ui->statusCursorsBox->setChecked (config.getShowCursorPos());
    connect (ui->statusCursorsBox, &QCheckBox::stateChanged, this, &PrefDialog::prefStatusCursor);

    // no ccombo onnection because of mouse wheel; config is set at closeEvent() instead
    ui->tabCombo->setCurrentIndex (config.getTabPosition());

    ui->tabBox->setChecked (config.getTabWrapAround());
    connect (ui->tabBox, &QCheckBox::stateChanged, this, &PrefDialog::prefTabWrapAround);

    ui->singleTabBox->setChecked (config.getHideSingleTab());
    connect (ui->singleTabBox, &QCheckBox::stateChanged, this, &PrefDialog::prefHideSingleTab);

    ui->windowBox->setChecked (config.getOpenInWindows());
    connect (ui->windowBox, &QCheckBox::stateChanged, this, &PrefDialog::prefOpenInWindows);

    ui->nativeDialogBox->setChecked (config.getNativeDialog());
    connect (ui->nativeDialogBox, &QCheckBox::stateChanged, this, &PrefDialog::prefNativeDialog);

    ui->sidePaneBox->setChecked (config.getSidePaneMode());
    ui->sidePaneSizeBox->setChecked (config.getRemSplitterPos());
    connect (ui->sidePaneBox, &QCheckBox::stateChanged, this, &PrefDialog::prefSidePaneMode);
    connect (ui->sidePaneSizeBox, &QCheckBox::stateChanged, this, &PrefDialog::prefSplitterPos);

    /************
     *** Text ***
     ************/

    ui->fontBox->setChecked (config.getRemFont());
    connect (ui->fontBox, &QCheckBox::stateChanged, this, &PrefDialog::prefFont);

    ui->wrapBox->setChecked (config.getWrapByDefault());
    connect (ui->wrapBox, &QCheckBox::stateChanged, this, &PrefDialog::prefWrap);

    ui->indentBox->setChecked (config.getIndentByDefault());
    connect (ui->indentBox, &QCheckBox::stateChanged, this, &PrefDialog::prefIndent);

    ui->autoBracketBox->setChecked (config.getAutoBracket());
    connect (ui->autoBracketBox, &QCheckBox::stateChanged, this, &PrefDialog::prefAutoBracket);

    ui->lineBox->setChecked (config.getLineByDefault());
    connect (ui->lineBox, &QCheckBox::stateChanged, this, &PrefDialog::prefLine);

    ui->syntaxBox->setChecked (config.getSyntaxByDefault());
    connect (ui->syntaxBox, &QCheckBox::stateChanged, this, &PrefDialog::prefSyntax);

    ui->enforceSyntaxBox->setChecked (config.getShowLangSelector());
    ui->enforceSyntaxBox->setEnabled (config.getSyntaxByDefault());

    ui->whiteSpaceBox->setChecked (config.getShowWhiteSpace());
    connect (ui->whiteSpaceBox, &QCheckBox::stateChanged, this, &PrefDialog::prefWhiteSpace);

    ui->vLineBox->setChecked (vLineDistance_ >= 10);
    connect (ui->vLineBox, &QCheckBox::stateChanged, this, &PrefDialog::prefVLine);
    ui->vLineSpin->setEnabled (vLineDistance_ >= 10);
    ui->vLineSpin->setValue (qAbs (vLineDistance_));
    connect (ui->vLineSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
             this, &PrefDialog::prefVLineDistance);

    ui->endingsBox->setChecked (config.getShowEndings());
    connect (ui->endingsBox, &QCheckBox::stateChanged, this, &PrefDialog::prefEndings);

    ui->colBox->setChecked (config.getDarkColScheme());
    connect (ui->colBox, &QCheckBox::stateChanged, this, &PrefDialog::prefDarkColScheme);
    if (!ui->colBox->isChecked())
    {
        ui->colorValueSpin->setMinimum (230);
        ui->colorValueSpin->setMaximum (255);
        ui->colorValueSpin->setValue (config.getLightBgColorValue());
    }
    else
    {
        ui->colorValueSpin->setMinimum (0);
        ui->colorValueSpin->setMaximum (50);
        ui->colorValueSpin->setValue (config.getDarkBgColorValue());
    }
    connect (ui->colorValueSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
             this, &PrefDialog::prefColValue);

    ui->dateEdit->setText (config.getDateFormat());

    ui->lastLineBox->setChecked (config.getAppendEmptyLine());
    connect (ui->lastLineBox, &QCheckBox::stateChanged, this, &PrefDialog::prefAppendEmptyLine);

    ui->scrollBox->setChecked (config.getScrollJumpWorkaround());
    connect (ui->scrollBox, &QCheckBox::stateChanged, this, &PrefDialog::prefScrollJumpWorkaround);

    ui->spinBox->setValue (config.getMaxSHSize());
    connect (ui->spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
             this, &PrefDialog::prefMaxSHSize);

    ui->inertiaBox->setChecked (config.getInertialScrolling());
    connect (ui->inertiaBox, &QCheckBox::stateChanged, this, &PrefDialog::prefInertialScrolling);

    /*************
     *** Files ***
     *************/

    ui->exeBox->setChecked (config.getExecuteScripts());
    connect (ui->exeBox, &QCheckBox::stateChanged, this, &PrefDialog::prefExecute);
    ui->commandEdit->setText (config.getExecuteCommand());
    ui->commandEdit->setEnabled (config.getExecuteScripts());
    ui->commandLabel->setEnabled (config.getExecuteScripts());
    connect (ui->commandEdit, &QLineEdit::textEdited, this, &PrefDialog::prefCommand);

    ui->recentSpin->setValue (config.getRecentFilesNumber());
    ui->recentSpin->setSuffix(" " + (ui->recentSpin->value() > 1 ? tr ("files") : tr ("file")));
    connect (ui->recentSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
             this, &PrefDialog::prefRecentFilesNumber);

    ui->openRecentSpin->setValue (config.getOpenRecentFiles());
    ui->openRecentSpin->setMaximum (config.getRecentFilesNumber());
    ui->openRecentSpin->setSuffix(" " + (ui->openRecentSpin->value() > 1 ? tr ("files") : tr ("file")));
    ui->openRecentSpin->setSpecialValueText (tr ("No file"));
    connect (ui->openRecentSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
             this, &PrefDialog::prefOpenRecentFile);

    ui->openedButton->setChecked (config.getRecentOpened());
    // no QButtonGroup connection because we want to see if we should clear the recent list at the end

    ui->autoSaveBox->setChecked (config.getAutoSave());
    ui->autoSaveSpin->setValue (config.getAutoSaveInterval());
    ui->autoSaveSpin->setEnabled (ui->autoSaveBox->isChecked());
    connect (ui->autoSaveBox, &QCheckBox::stateChanged, this, &PrefDialog::prefAutoSave);

    /*****************
     *** Shortcuts ***
     *****************/

    defaultShortcuts_ = defaultShortcuts;
    FPwin *win = static_cast<FPwin *>(parent_);

    if (OBJECT_NAMES.isEmpty())
    {
        OBJECT_NAMES.insert (win->ui->actionNew->text().remove ("&"), "actionNew");
        OBJECT_NAMES.insert (win->ui->actionOpen->text().remove ("&"), "actionOpen");
        OBJECT_NAMES.insert (win->ui->actionSave->text().remove ("&"), "actionSave");
        OBJECT_NAMES.insert (win->ui->actionReload->text().remove ("&"), "actionReload");
        OBJECT_NAMES.insert (win->ui->actionFind->text().remove ("&"), "actionFind");
        OBJECT_NAMES.insert (win->ui->actionReplace->text().remove ("&"), "actionReplace");
        OBJECT_NAMES.insert (win->ui->actionSaveAs->text().remove ("&"), "actionSaveAs");
        OBJECT_NAMES.insert (win->ui->actionPrint->text().remove ("&"), "actionPrint");
        OBJECT_NAMES.insert (win->ui->actionDoc->text().remove ("&"), "actionDoc");
        OBJECT_NAMES.insert (win->ui->actionClose->text().remove ("&"), "actionClose");
        OBJECT_NAMES.insert (win->ui->actionQuit->text().remove ("&"), "actionQuit");
        OBJECT_NAMES.insert (win->ui->actionLineNumbers->text().remove ("&"), "actionLineNumbers");
        OBJECT_NAMES.insert (win->ui->actionWrap->text().remove ("&"), "actionWrap");
        OBJECT_NAMES.insert (win->ui->actionIndent->text().remove ("&"), "actionIndent");
        OBJECT_NAMES.insert (win->ui->actionSyntax->text().remove ("&"), "actionSyntax");
        OBJECT_NAMES.insert (win->ui->actionPreferences->text().remove ("&"), "actionPreferences");
        OBJECT_NAMES.insert (win->ui->actionHelp->text().remove ("&"), "actionHelp");
        OBJECT_NAMES.insert (win->ui->actionJump->text().remove ("&"), "actionJump");
        OBJECT_NAMES.insert (win->ui->actionEdit->text().remove ("&"), "actionEdit");
        OBJECT_NAMES.insert (win->ui->actionDetachTab->text().remove ("&"), "actionDetachTab");
        OBJECT_NAMES.insert (win->ui->actionRun->text().remove ("&"), "actionRun");
        OBJECT_NAMES.insert (win->ui->actionSession->text().remove ("&"), "actionSession");
        OBJECT_NAMES.insert (win->ui->actionSidePane->text().remove ("&"), "actionSidePane");
    }

    QHash<QString, QString> ca = config.customShortcutActions();
    QList<QString> keys = ca.keys();

    shortcuts_.insert (win->ui->actionNew->text().remove ("&"),
                       keys.contains ("actionNew") ? ca.value ("actionNew") : defaultShortcuts_.value ("actionNew"));
    shortcuts_.insert (win->ui->actionOpen->text().remove ("&"),
                       keys.contains ("actionOpen") ? ca.value ("actionOpen") : defaultShortcuts_.value ("actionOpen"));
    shortcuts_.insert (win->ui->actionSave->text().remove ("&"),
                       keys.contains ("actionSave") ? ca.value ("actionSave") : defaultShortcuts_.value ("actionSave"));
    shortcuts_.insert (win->ui->actionReload->text().remove ("&"),
                       keys.contains ("actionReload") ? ca.value ("actionReload") : defaultShortcuts_.value ("actionReload"));
    shortcuts_.insert (win->ui->actionFind->text().remove ("&"),
                       keys.contains ("actionFind") ? ca.value ("actionFind") : defaultShortcuts_.value ("actionFind"));
    shortcuts_.insert (win->ui->actionReplace->text().remove ("&"),
                       keys.contains ("actionReplace") ? ca.value ("actionReplace") : defaultShortcuts_.value ("actionReplace"));
    shortcuts_.insert (win->ui->actionSaveAs->text().remove ("&"),
                       keys.contains ("actionSaveAs") ? ca.value ("actionSaveAs") : defaultShortcuts_.value ("actionSaveAs"));
    shortcuts_.insert (win->ui->actionPrint->text().remove ("&"),
                       keys.contains ("actionPrint") ? ca.value ("actionPrint") :defaultShortcuts_.value ("actionPrint"));
    shortcuts_.insert (win->ui->actionDoc->text().remove ("&"),
                       keys.contains ("actionDoc") ? ca.value ("actionDoc") : defaultShortcuts_.value ("actionDoc"));
    shortcuts_.insert (win->ui->actionClose->text().remove ("&"),
                       keys.contains ("actionClose") ? ca.value ("actionClose") : defaultShortcuts_.value ("actionClose"));
    shortcuts_.insert (win->ui->actionQuit->text().remove ("&"),
                       keys.contains ("actionQuit") ? ca.value ("actionQuit") : defaultShortcuts_.value ("actionQuit"));
    shortcuts_.insert (win->ui->actionLineNumbers->text().remove ("&"),
                       keys.contains ("actionLineNumbers") ? ca.value ("actionLineNumbers") : defaultShortcuts_.value ("actionLineNumbers"));
    shortcuts_.insert (win->ui->actionWrap->text().remove ("&"),
                       keys.contains ("actionWrap") ? ca.value ("actionWrap") : defaultShortcuts_.value ("actionWrap"));
    shortcuts_.insert (win->ui->actionIndent->text().remove ("&"),
                       keys.contains ("actionIndent") ? ca.value ("actionIndent") : defaultShortcuts_.value ("actionIndent"));
    shortcuts_.insert (win->ui->actionSyntax->text().remove ("&"),
                       keys.contains ("actionSyntax") ? ca.value ("actionSyntax") : defaultShortcuts_.value ("actionSyntax"));
    shortcuts_.insert (win->ui->actionPreferences->text().remove ("&"),
                       keys.contains ("actionPreferences") ? ca.value ("actionPreferences") : defaultShortcuts_.value ("actionPreferences"));
    shortcuts_.insert (win->ui->actionHelp->text().remove ("&"),
                       keys.contains ("actionHelp") ? ca.value ("actionHelp") : defaultShortcuts_.value ("actionHelp"));
    shortcuts_.insert (win->ui->actionJump->text().remove ("&"),
                       keys.contains ("actionJump") ? ca.value ("actionJump") : defaultShortcuts_.value ("actionJump"));
    shortcuts_.insert (win->ui->actionEdit->text().remove ("&"),
                       keys.contains ("actionEdit") ? ca.value ("actionEdit") : defaultShortcuts_.value ("actionEdit"));
    shortcuts_.insert (win->ui->actionDetachTab->text().remove ("&"),
                       keys.contains ("actionDetachTab") ? ca.value ("actionDetachTab") : defaultShortcuts_.value ("actionDetachTab"));
    shortcuts_.insert (win->ui->actionRun->text().remove ("&"),
                       keys.contains ("actionRun") ? ca.value ("actionRun") : defaultShortcuts_.value ("actionRun"));
    shortcuts_.insert (win->ui->actionSession->text().remove ("&"),
                       keys.contains ("actionSession") ? ca.value ("actionSession") : defaultShortcuts_.value ("actionSession"));
    shortcuts_.insert (win->ui->actionSidePane->text().remove ("&"),
                       keys.contains ("actionSidePane") ? ca.value ("actionSidePane") : defaultShortcuts_.value ("actionSidePane"));

    QList<QString> val = shortcuts_.values();
    for (int i = 0; i < val.size(); ++i)
    {
        if (val.indexOf (val.at (i), i + 1) > -1)
        {
            showPrompt (tr ("Warning: Ambiguous shortcut detected!"), false);
            break;
        }
    }

    ui->tableWidget->setRowCount (shortcuts_.size());
    ui->tableWidget->setSortingEnabled (false);
    int index = 0;
    QHash<QString, QString>::const_iterator it = shortcuts_.constBegin();
    while (it != shortcuts_.constEnd())
    {
        QTableWidgetItem *item = new QTableWidgetItem (it.key());
        item->setFlags (item->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
        ui->tableWidget->setItem (index, 0, item);
        ui->tableWidget->setItem (index, 1, new QTableWidgetItem (it.value()));
        ++ it;
        ++ index;
    }
    ui->tableWidget->setSortingEnabled (true);
    ui->tableWidget->setCurrentCell (0, 1);
    connect (ui->tableWidget, &QTableWidget::itemChanged, this, &PrefDialog::onShortcutChange);
    connect (ui->defaultButton, &QAbstractButton::clicked, this, &PrefDialog::defaultSortcuts);
    ui->defaultButton->setDisabled (ca.isEmpty());

    /*************
     *** Other ***
     *************/

    connect (ui->closeButton, &QAbstractButton::clicked, this, &QDialog::close);
    connect (ui->helpButton, &QAbstractButton::clicked, this, &PrefDialog::showWhatsThis);
    connect (this, &QDialog::rejected, this, &PrefDialog::onClosing);

    /* set tooltip as "whatsthis" */
    QList<QWidget*> widgets = findChildren<QWidget*>();
    for (int i = 0; i < widgets.count(); ++i)
    {
        QWidget *w = widgets.at (i);
        w->setWhatsThis (w->toolTip().replace ('\n', ' ').replace ("  ", "\n\n"));
    }

    resize (minimumSize());
}
/*************************/
PrefDialog::~PrefDialog()
{
    if (promptTimer_)
    {
        promptTimer_->stop();
        delete promptTimer_;
    }
    delete ui; ui = nullptr;
}
/*************************/
void PrefDialog::closeEvent (QCloseEvent *event)
{
    onClosing();
    event->accept();
}
/*************************/
void PrefDialog::onClosing()
{
    prefShortcuts();
    prefTabPosition();
    prefRecentFilesKind();
    prefApplyAutoSave();
    prefApplySyntax();
    prefApplyDateFormat();
}
/*************************/
void PrefDialog::showPrompt (QString str, bool temporary)
{
    static const QString style ("QLabel {background-color: #7d0000; color: white; border-radius: 3px; margin: 2px; padding: 5px;}");
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    if (!str.isEmpty())
    { // show the provided message
        ui->promptLabel->setText ("<b>" + str + "</b>");
        ui->promptLabel->setStyleSheet (style);
        if (temporary) // show it temporarily
        {
            if (!promptTimer_)
            {
                promptTimer_ = new QTimer();
                promptTimer_->setSingleShot (true);
                connect (promptTimer_, &QTimer::timeout, [this] {
                    if (!prevtMsg_.isEmpty()
                        && ui->tabWidget->currentIndex() == 3) // Shortcuts page
                    { // show the previous message if it exists
                        ui->promptLabel->setText (prevtMsg_);
                        ui->promptLabel->setStyleSheet (style);
                    }
                    else showPrompt();
                });
            }
            promptTimer_->start (3300);
        }
        else
            prevtMsg_ = "<b>" + str + "</b>";
    }
    else if (sysIcons_ != config.getSysIcon()
            || iconless_ != config.getIconless()
            || recentNumber_ != config.getRecentFilesNumber())
    {
        ui->promptLabel->setText ("<b>" + tr ("Application restart is needed for changes to take effect.") + "</b>");
        ui->promptLabel->setStyleSheet (style);
    }
    else if (darkBg_ != config.getDarkColScheme()
             || (darkBg_ && darkColValue_ != config.getDarkBgColorValue())
             || (!darkBg_ && lightColValue_ != config.getLightBgColorValue())
             || showWhiteSpace_ != config.getShowWhiteSpace()
             || showEndings_ != config.getShowEndings()
             || (vLineDistance_ * config.getVLineDistance() < 0
                 || (vLineDistance_ > 0 && vLineDistance_ != config.getVLineDistance())))
    {
        ui->promptLabel->setText ("<b>" + tr ("Window reopening is needed for changes to take effect.") + "</b>");
        ui->promptLabel->setStyleSheet (style);
    }
    else
    {
        if (prevtMsg_.isEmpty()) // clear prompt
        {
            ui->promptLabel->clear();
            ui->promptLabel->setStyleSheet ("QLabel {margin: 2px; padding: 5px;}");
        }
        else // show the previous message
        {
            ui->promptLabel->setText (prevtMsg_);
            ui->promptLabel->setStyleSheet (style);
        }
    }
    ui->promptLabel->show();
}
/*************************/
void PrefDialog::showWhatsThis()
{
    QWhatsThis::enterWhatsThisMode();
}
/*************************/
void PrefDialog::prefSize (int checked)
{
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    if (checked == Qt::Checked)
    {
        config.setRemSize (true);
        ui->spinX->setEnabled (false);
        ui->spinY->setEnabled (false);
        ui->mLabel->setEnabled (false);
        ui->sizeLable->setEnabled (false);
    }
    else if (checked == Qt::Unchecked)
    {
        config.setRemSize (false);
        ui->spinX->setEnabled (true);
        ui->spinY->setEnabled (true);
        ui->mLabel->setEnabled (true);
        ui->sizeLable->setEnabled (true);
    }
}
/*************************/
void PrefDialog::prefIcon (int checked)
{
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    if (checked == Qt::Checked)
        config.setSysIcon (false);
    else if (checked == Qt::Unchecked)
        config.setSysIcon (true);

    showPrompt();
}
/*************************/
void PrefDialog::prefIconless (int checked)
{
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    if (checked == Qt::Checked)
    {
        qApp->setAttribute (Qt::AA_DontShowIconsInMenus, true);
        config.setIconless (true);
        ui->iconBox->setEnabled (false);
    }
    else if (checked == Qt::Unchecked)
    {
        qApp->setAttribute (Qt::AA_DontShowIconsInMenus, false);
        config.setIconless (false);
        ui->iconBox->setEnabled (true);
    }

    showPrompt();
}
/*************************/
void PrefDialog::prefToolbar (int checked)
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    if (checked == Qt::Checked)
    {
        if (ui->menubarBox->checkState() == Qt::Checked)
            ui->menubarBox->setCheckState (Qt::Unchecked);
        config.setNoToolbar (true);
        for (int i = 0; i < singleton->Wins.count(); ++i)
            singleton->Wins.at (i)->ui->mainToolBar->setVisible (false);
    }
    else if (checked == Qt::Unchecked)
    {
        config.setNoToolbar (false);
        for (int i = 0; i < singleton->Wins.count(); ++i)
            singleton->Wins.at (i)->ui->mainToolBar->setVisible (true);
    }
}
/*************************/
void PrefDialog::prefMenubar (int checked)
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    if (checked == Qt::Checked)
    {
        if (ui->toolbarBox->checkState() == Qt::Checked)
            ui->toolbarBox->setCheckState (Qt::Unchecked);
        config.setNoMenubar (true);
        for (int i = 0; i < singleton->Wins.count(); ++i)
        {
            singleton->Wins.at (i)->ui->menuBar->setVisible (false);
            singleton->Wins.at (i)->ui->actionMenu->setVisible (true);
        }
    }
    else if (checked == Qt::Unchecked)
    {
        config.setNoMenubar (false);
        for (int i = 0; i < singleton->Wins.count(); ++i)
        {
            singleton->Wins.at (i)->ui->menuBar->setVisible (true);
            singleton->Wins.at (i)->ui->actionMenu->setVisible (false);
        }
    }
}
/*************************/
void PrefDialog::prefSearchbar (int checked)
{
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    if (checked == Qt::Checked)
        config.setHideSearchbar (true);
    else if (checked == Qt::Unchecked)
        config.setHideSearchbar (false);
}
/*************************/
void PrefDialog::prefStatusbar (int checked)
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    bool showCurPos = config.getShowCursorPos();
    if (checked == Qt::Checked)
    {
        config.setShowStatusbar (true);
        for (int i = 0; i < singleton->Wins.count(); ++i)
        {
            FPwin *win = singleton->Wins.at (i);

            if (!win->ui->statusBar->isVisible())
            {
                /* here we can't use docProp() directly
                   because we don't want to count words */
                if (TabPage *tabPage = qobject_cast<TabPage*>(win->ui->tabWidget->currentWidget()))
                {
                    TextEdit *textEdit = tabPage->textEdit();
                    win->statusMsgWithLineCount (textEdit->document()->blockCount());
                    for (int j = 0; j < win->ui->tabWidget->count(); ++j)
                    {
                        TextEdit *thisTextEdit = qobject_cast< TabPage *>(win->ui->tabWidget->widget (j))->textEdit();
                        connect (thisTextEdit, &QPlainTextEdit::blockCountChanged, win, &FPwin::statusMsgWithLineCount);
                        connect (thisTextEdit, &QPlainTextEdit::selectionChanged, win, &FPwin::statusMsg);
                        if (showCurPos)
                            connect (thisTextEdit, &QPlainTextEdit::cursorPositionChanged, win, &FPwin::showCursorPos);
                    }
                    win->ui->statusBar->setVisible (true);
                    if (showCurPos)
                    {
                        win->addCursorPosLabel();
                        win->showCursorPos();
                    }
                    if (QToolButton *wordButton = win->ui->statusBar->findChild<QToolButton *>("wordButton"))
                    {
                        wordButton->setVisible (true);
                        if (textEdit->getWordNumber() != -1 // when words are already counted
                            || textEdit->document()->isEmpty())
                        {
                            win->updateWordInfo();
                        }
                    }
                }
            }
            /* no need for this menu item anymore */
            win->ui->actionDoc->setVisible (false);
        }
    }
    else if (checked == Qt::Unchecked)
    {
        config.setShowStatusbar (false);
        for (int i = 0; i < singleton->Wins.count(); ++i)
            singleton->Wins.at (i)->ui->actionDoc->setVisible (true);
    }
}
/*************************/
void PrefDialog::prefStatusCursor (int checked)
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    if (checked == Qt::Checked)
    {
        config.setShowCursorPos (true);
        for (int i = 0; i < singleton->Wins.count(); ++i)
        {
            FPwin *win = singleton->Wins.at (i);
            int count = win->ui->tabWidget->count();
            if (count > 0 && win->ui->statusBar->isVisible())
            {
                win->addCursorPosLabel();
                win->showCursorPos();
                for (int j = 0; j < count; ++j)
                {
                    TextEdit *textEdit = qobject_cast< TabPage *>(win->ui->tabWidget->widget (j))->textEdit();
                    connect (textEdit, &QPlainTextEdit::cursorPositionChanged, win, &FPwin::showCursorPos);
                }
            }
        }
    }
    else if (checked == Qt::Unchecked)
    {
        config.setShowCursorPos (false);
        for (int i = 0; i < singleton->Wins.count(); ++i)
        {
            FPwin *win = singleton->Wins.at (i);
            if (QLabel *posLabel = win->ui->statusBar->findChild<QLabel *>("posLabel"))
            {
                int count = win->ui->tabWidget->count();
                if (count > 0 && win->ui->statusBar->isVisible())
                {
                    for (int j = 0; j < count; ++j)
                    {
                        TextEdit *textEdit = qobject_cast< TabPage *>(win->ui->tabWidget->widget (j))->textEdit();
                        disconnect (textEdit, &QPlainTextEdit::cursorPositionChanged, win, &FPwin::showCursorPos);
                    }
                }
                posLabel->deleteLater();
            }
        }
    }
}
/*************************/
void PrefDialog::prefTabPosition()
{
    int index = ui->tabCombo->currentIndex();
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    config.setTabPosition (index);
    if (singleton->Wins.at (0)->ui->tabWidget->tabPosition() != (QTabWidget::TabPosition) index)
    {
        for (int i = 0; i < singleton->Wins.count(); ++i)
            singleton->Wins.at (i)->ui->tabWidget->setTabPosition ((QTabWidget::TabPosition) index);
    }
}
/*************************/
void PrefDialog::prefFont (int checked)
{
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    if (checked == Qt::Checked)
    {
        config.setRemFont (true);
        // get the document font of the current window
        if (FPwin *win = static_cast<FPwin *>(parent_))
        {
            if (TabPage *tabPage = qobject_cast<TabPage*>(win->ui->tabWidget->currentWidget()))
                config.setFont (tabPage->textEdit()->document()->defaultFont());
        }
    }
    else if (checked == Qt::Unchecked)
    {
        config.setRemFont (false);
        // return to our default font
        config.setFont (QFont ("Monospace", 9));
    }
}
/*************************/
void PrefDialog::prefWrap (int checked)
{
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    if (checked == Qt::Checked)
        config.setWrapByDefault (true);
    else if (checked == Qt::Unchecked)
        config.setWrapByDefault (false);
}
/*************************/
void PrefDialog::prefIndent (int checked)
{
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    if (checked == Qt::Checked)
        config.setIndentByDefault (true);
    else if (checked == Qt::Unchecked)
        config.setIndentByDefault (false);
}
/*************************/
void PrefDialog::prefAutoBracket (int checked)
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    if (checked == Qt::Checked)
    {
        config.setAutoBracket (true);
        for (int i = 0; i < singleton->Wins.count(); ++i)
        {
            int count = singleton->Wins.at (i)->ui->tabWidget->count();
            for (int j = 0; j < count; ++j)
            {
                qobject_cast< TabPage *>(singleton->Wins.at (i)->ui->tabWidget->widget (j))
                        ->textEdit()->setAutoBracket (true);
            }
        }
    }
    else if (checked == Qt::Unchecked)
    {
        config.setAutoBracket (false);
        for (int i = 0; i < singleton->Wins.count(); ++i)
        {
            int count = singleton->Wins.at (i)->ui->tabWidget->count();
            for (int j = 0; j < count; ++j)
            {
                qobject_cast< TabPage *>(singleton->Wins.at (i)->ui->tabWidget->widget (j))
                        ->textEdit()->setAutoBracket (false);
            }
        }
    }
}
/*************************/
void PrefDialog::prefLine (int checked)
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    if (checked == Qt::Checked)
    {
        config.setLineByDefault (true);
        for (int i = 0; i < singleton->Wins.count(); ++i)
        {
            FPwin *thisWin = singleton->Wins.at (i);
            thisWin->ui->actionLineNumbers->setChecked (true);
            thisWin->ui->actionLineNumbers->setDisabled (true);
        }
    }
    else if (checked == Qt::Unchecked)
    {
        config.setLineByDefault (false);
        for (int i = 0; i < singleton->Wins.count(); ++i)
            singleton->Wins.at (i)->ui->actionLineNumbers->setEnabled (true);
    }
}
/*************************/
void PrefDialog::prefSyntax (int checked)
{
    /* only set the state of the syntax enforcing checkbox */
    if (checked == Qt::Checked)
        ui->enforceSyntaxBox->setEnabled (true);
    else if (checked == Qt::Unchecked)
        ui->enforceSyntaxBox->setEnabled (false);
}
/*************************/
void PrefDialog::prefApplySyntax()
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    if ((!config.getShowLangSelector() || !config.getSyntaxByDefault())
        && (ui->enforceSyntaxBox->isChecked() && ui->syntaxBox->isChecked()))
    {
        for (int i = 0; i < singleton->Wins.count(); ++i)
        {
            FPwin *win = singleton->Wins.at (i);
            win->addLangButton();
            if (TabPage *tabPage = qobject_cast<TabPage*>(win->ui->tabWidget->currentWidget()))
                win->showLang (tabPage->textEdit());
        }
    }
    else if (config.getShowLangSelector() && config.getSyntaxByDefault()
             && (!ui->enforceSyntaxBox->isChecked() || ! ui->syntaxBox->isChecked()))
    {
        for (int i = 0; i < singleton->Wins.count(); ++i)
            singleton->Wins.at (i)->removeLangButton();
    }
    config.setSyntaxByDefault (ui->syntaxBox->isChecked());
    config.setShowLangSelector (ui->enforceSyntaxBox->isChecked());
}
/*************************/
void PrefDialog::prefApplyDateFormat()
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    QString format = ui->dateEdit->text();
    /* if "\n" is typed in the line-edit, interpret
       it as a newline because we're on Linux */
    if (!format.isEmpty())
        format.replace ("\\n", "\n");
    config.setDateFormat (format);
    for (int i = 0; i < singleton->Wins.count(); ++i)
    {
        int count = singleton->Wins.at (i)->ui->tabWidget->count();
        for (int j = 0; j < count; ++j)
        {
            qobject_cast< TabPage *>(singleton->Wins.at (i)->ui->tabWidget->widget (j))
                    ->textEdit()->setDateFormat (format);
        }
    }
}
/*************************/
void PrefDialog::prefWhiteSpace (int checked)
{
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    if (checked == Qt::Checked)
        config.setShowWhiteSpace (true);
    else if (checked == Qt::Unchecked)
        config.setShowWhiteSpace (false);

    showPrompt();
}
/*************************/
void PrefDialog::prefVLine (int checked)
{
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    int dsitance = qMax (qMin (ui->vLineSpin->value(), 999), 10);
    if (checked == Qt::Checked)
    {
        config.setVLineDistance (dsitance);
        ui->vLineSpin->setEnabled (true);
    }
    else if (checked == Qt::Unchecked)
    {
        config.setVLineDistance (-1 * dsitance);
        ui->vLineSpin->setEnabled (false);
    }

    showPrompt();
}
/*************************/
void PrefDialog::prefVLineDistance (int value)
{
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    int dsitance = qMax (qMin (value, 999), 10);
    config.setVLineDistance (dsitance);
    showPrompt();
}
/*************************/
void PrefDialog::prefEndings (int checked)
{
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    if (checked == Qt::Checked)
        config.setShowEndings (true);
    else if (checked == Qt::Unchecked)
        config.setShowEndings (false);

    showPrompt();
}
/*************************/
void PrefDialog::prefDarkColScheme (int checked)
{
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    disconnect (ui->colorValueSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                this, &PrefDialog::prefColValue);
    if (checked == Qt::Checked)
    {
        config.setDarkColScheme (true);
        ui->colorValueSpin->setMinimum (0);
        ui->colorValueSpin->setMaximum (50);
        ui->colorValueSpin->setValue (config.getDarkBgColorValue());
    }
    else if (checked == Qt::Unchecked)
    {
        config.setDarkColScheme (false);
        ui->colorValueSpin->setMinimum (230);
        ui->colorValueSpin->setMaximum (255);
        ui->colorValueSpin->setValue (config.getLightBgColorValue());
    }
    connect (ui->colorValueSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
             this, &PrefDialog::prefColValue);

    showPrompt();
}
/*************************/
void PrefDialog::prefColValue (int value)
{
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    if (!ui->colBox->isChecked())
        config.setLightBgColorValue (value);
    else
        config.setDarkBgColorValue (value);

    showPrompt();
}
/*************************/
void PrefDialog::prefAppendEmptyLine (int checked)
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    if (checked == Qt::Checked)
        config.setAppendEmptyLine (true);
    else if (checked == Qt::Unchecked)
        config.setAppendEmptyLine (false);
}
/*************************/
void PrefDialog::prefScrollJumpWorkaround (int checked)
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    if (checked == Qt::Checked)
    {
        config.setScrollJumpWorkaround (true);
        for (int i = 0; i < singleton->Wins.count(); ++i)
        {
            int count = singleton->Wins.at (i)->ui->tabWidget->count();
            for (int j = 0; j < count; ++j)
            {
                qobject_cast< TabPage *>(singleton->Wins.at (i)->ui->tabWidget->widget (j))
                        ->textEdit()->setScrollJumpWorkaround (true);
            }
        }
    }
    else if (checked == Qt::Unchecked)
    {
        config.setScrollJumpWorkaround (false);
        for (int i = 0; i < singleton->Wins.count(); ++i)
        {
            int count = singleton->Wins.at (i)->ui->tabWidget->count();
            for (int j = 0; j < count; ++j)
            {
                qobject_cast< TabPage *>(singleton->Wins.at (i)->ui->tabWidget->widget (j))
                        ->textEdit()->setScrollJumpWorkaround (false);
            }
        }
    }
}
/*************************/
void PrefDialog::prefTabWrapAround (int checked)
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    if (checked == Qt::Checked)
        config.setTabWrapAround (true);
    else if (checked == Qt::Unchecked)
        config.setTabWrapAround (false);
}
/*************************/
void PrefDialog::prefHideSingleTab (int checked)
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    if (checked == Qt::Checked)
    {
        config.setHideSingleTab (true);
        for (int i = 0; i < singleton->Wins.count(); ++i)
        {
            FPwin *win = singleton->Wins.at (i);
            TabBar *tabBar = win->ui->tabWidget->tabBar();
            if (!win->hasSidePane())
                tabBar->hideSingle (true);
            if (win->ui->tabWidget->count() == 1)
                tabBar->hide();
        }
    }
    else if (checked == Qt::Unchecked)
    {
        config.setHideSingleTab (false);
        for (int i = 0; i < singleton->Wins.count(); ++i)
        {
            FPwin *win = singleton->Wins.at (i);
            TabBar *tabBar = win->ui->tabWidget->tabBar();
            tabBar->hideSingle (false);
            if (!win->hasSidePane() && win->ui->tabWidget->count() == 1)
                tabBar->show();
        }
    }
}
/*************************/
void PrefDialog::prefMaxSHSize (int value)
{
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    config.setMaxSHSize (value);
}
/*************************/
void PrefDialog::prefInertialScrolling (int checked)
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    if (checked == Qt::Checked)
    {
        config.setInertialScrolling (true);
        for (int i = 0; i < singleton->Wins.count(); ++i)
        {
            FPwin *win = singleton->Wins.at (i);
            for (int j = 0; j < win->ui->tabWidget->count(); ++j)
                qobject_cast< TabPage *>(win->ui->tabWidget->widget (j))->textEdit()->setInertialScrolling (true);
        }
    }
    else if (checked == Qt::Unchecked)
    {
        config.setInertialScrolling (false);
        for (int i = 0; i < singleton->Wins.count(); ++i)
        {
            FPwin *win = singleton->Wins.at (i);
            for (int j = 0; j < win->ui->tabWidget->count(); ++j)
                qobject_cast< TabPage *>(win->ui->tabWidget->widget (j))->textEdit()->setInertialScrolling (false);
        }
    }
}
/*************************/
void PrefDialog::prefExecute (int checked)
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    if (checked == Qt::Checked)
    {
        config.setExecuteScripts (true);
        ui->commandEdit->setEnabled (true);
        ui->commandLabel->setEnabled (true);
        for (int i = 0; i < singleton->Wins.count(); ++i)
        {
            FPwin *win = singleton->Wins.at (i);
            if (TabPage *tabPage = qobject_cast<TabPage*>(win->ui->tabWidget->currentWidget()))
            {
                TextEdit *textEdit = tabPage->textEdit();
                if (win->isScriptLang (textEdit->getProg()) && QFileInfo (textEdit->getFileName()).isExecutable())
                    win->ui->actionRun->setVisible (true);
            }
        }
    }
    else if (checked == Qt::Unchecked)
    {
        config.setExecuteScripts (false);
        ui->commandEdit->setEnabled (false);
        ui->commandLabel->setEnabled (false);
        for (int i = 0; i < singleton->Wins.count(); ++i)
            singleton->Wins.at (i)->ui->actionRun->setVisible (false);
    }
}
/*************************/
void PrefDialog::prefCommand (QString command)
{
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    config.setExecuteCommand (command);
}
/*************************/
void PrefDialog::prefRecentFilesNumber (int value)
{
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    config.setRecentFilesNumber (value); // doesn't take effect until the next session
    ui->recentSpin->setSuffix(" " + (value > 1 ? tr ("files") : tr ("file")));

    /* also correct the maximum value of openRecentSpin
       (its value will be corrected automatically if needed) */
    ui->openRecentSpin->setMaximum (value);
    ui->openRecentSpin->setSuffix(" " + (ui->openRecentSpin->value() > 1 ? tr ("files") : tr ("file")));

    showPrompt();
}
/*************************/
void PrefDialog::prefOpenRecentFile (int value)
{
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    config.setOpenRecentFiles (value);
    ui->openRecentSpin->setSuffix(" " + (value > 1 ? tr ("files") : tr ("file")));
}
/*************************/
void PrefDialog::prefRecentFilesKind()
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    bool openedKind = ui->openedButton->isChecked();
    if (config.getRecentOpened() != openedKind)
    {
        config.setRecentOpened (openedKind);
        config.clearRecentFiles();
        for (int i = 0; i < singleton->Wins.count(); ++i)
        {
            singleton->Wins.at (i)->ui->menuOpenRecently->setTitle (openedKind
                                                                    ? tr ("&Recently Opened")
                                                                    : tr ("Recently &Modified"));
        }
    }
}
/*************************/
void PrefDialog::prefStartSize (int value)
{
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    QSize startSize = config.getStartSize();
    if (QObject::sender() == ui->spinX)
        startSize.setWidth (value);
    else if (QObject::sender() == ui->spinY)
        startSize.setHeight (value);
    config.setStartSize (startSize);
}
/*************************/
void PrefDialog::prefOpenInWindows (int checked)
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    if (checked == Qt::Checked)
        config.setOpenInWindows (true);
    else if (checked == Qt::Unchecked)
        config.setOpenInWindows (false);
}
/*************************/
void PrefDialog::prefNativeDialog (int checked)
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    if (checked == Qt::Checked)
        config.setNativeDialog (true);
    else if (checked == Qt::Unchecked)
        config.setNativeDialog (false);
}
/*************************/
void PrefDialog::prefSidePaneMode (int checked)
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    if (checked == Qt::Checked)
        config.setSidePaneMode (true);
    else if (checked == Qt::Unchecked)
        config.setSidePaneMode (false);
}
/*************************/
void PrefDialog::prefSplitterPos (int checked)
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    if (checked == Qt::Checked)
        config.setRemSplitterPos (true);
    else if (checked == Qt::Unchecked)
        config.setRemSplitterPos (false);
}
/*************************/
void PrefDialog::onShortcutChange (QTableWidgetItem *item)
{
    Config config = static_cast<FPsingleton*>(qApp)->getConfig();
    QString txt = item->text();
    QString desc = ui->tableWidget->item (ui->tableWidget->currentRow(), 0)->text();
    if (txt.isEmpty() || !txt.contains ("+") || config.reservedShortcuts().contains (txt))
    {
        if (txt.isEmpty() || !txt.contains ("+"))
            showPrompt (tr ("The typed shortcut was not valid."), true);
        else
            showPrompt (tr ("The typed shortcut was reserved."), true);
        disconnect (ui->tableWidget, &QTableWidget::itemChanged, this, &PrefDialog::onShortcutChange);
        item->setText (shortcuts_.value (desc));
        connect (ui->tableWidget, &QTableWidget::itemChanged, this, &PrefDialog::onShortcutChange);
    }
    else
    {
        if (!shortcuts_.keys (txt).isEmpty())
            showPrompt (tr ("Warning: Ambiguous shortcut detected!"), false);
        else if (ui->promptLabel->isVisible())
        {
            prevtMsg_ = QString();
            showPrompt();
        }
        shortcuts_.insert (desc, txt);
        newShortcuts_.insert (OBJECT_NAMES.value (desc), txt);
        /* also set the state of the Default button */
        QHash<QString, QString>::const_iterator it = shortcuts_.constBegin();
        while (it != shortcuts_.constEnd())
        {
            if (defaultShortcuts_.value (OBJECT_NAMES.value (it.key())) != it.value())
            {
                ui->defaultButton->setEnabled (true);
                return;
            }
            ++it;
        }
        ui->defaultButton->setEnabled (false);
    }
}
/*************************/
void PrefDialog::defaultSortcuts()
{
    if (newShortcuts_.isEmpty()
        && static_cast<FPsingleton*>(qApp)->getConfig().customShortcutActions().isEmpty())
    { // do nothing if there's no custom shortcut
        return;
    }

    disconnect (ui->tableWidget, &QTableWidget::itemChanged, this, &PrefDialog::onShortcutChange);
    int cur = ui->tableWidget->currentColumn() == 0
                  ? 0
                  : ui->tableWidget->currentRow();
    ui->tableWidget->setSortingEnabled (false);
    newShortcuts_ = defaultShortcuts_;
    int index = 0;
    QMutableHashIterator<QString, QString> it (shortcuts_);
    while (it.hasNext())
    {
        it.next();
        ui->tableWidget->item (index, 0)->setText (it.key());
        QString s = defaultShortcuts_.value (OBJECT_NAMES.value (it.key()));
        ui->tableWidget->item (index, 1)->setText (s);
        it.setValue (s);
        ++ index;
    }
    ui->tableWidget->setSortingEnabled (true);
    ui->tableWidget->setCurrentCell (cur, 1);
    connect (ui->tableWidget, &QTableWidget::itemChanged, this, &PrefDialog::onShortcutChange);

    ui->defaultButton->setEnabled (false);
    if (ui->promptLabel->isVisible())
    {
        prevtMsg_ = QString();
        showPrompt();
    }
}
/*************************/
void PrefDialog::prefShortcuts()
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    QHash<QString, QString>::const_iterator it = newShortcuts_.constBegin();
    while (it != newShortcuts_.constEnd())
    {
        if (defaultShortcuts_.value (it.key()) == it.value())
            config.removeShortcut (it.key());
        else
            config.setActionShortcut (it.key(), it.value());
        ++it;
    }
    /* update the shortcuts for all windows
       (the current window will update them on closing this dialog) */
    for (int i = 0; i < singleton->Wins.count(); ++i)
    {
        FPwin *win = singleton->Wins.at (i);
        if (win != parent_)
            win->updateCustomizableShortcuts();
    }
}
/*************************/
void PrefDialog::prefAutoSave (int checked)
{
    /* don't do anything other than enabling/disabling the spinbox */
    if (checked == Qt::Checked)
        ui->autoSaveSpin->setEnabled (true);
    else if (checked == Qt::Unchecked)
        ui->autoSaveSpin->setEnabled (false);
}
/*************************/
void PrefDialog::prefApplyAutoSave()
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    bool as = ui->autoSaveBox->isChecked();
    int interval = ui->autoSaveSpin->value();
    if (config.getAutoSave() != as || interval != config.getAutoSaveInterval())
    {
        config.setAutoSave (as);
        config.setAutoSaveInterval (interval);
        for (int i = 0; i < singleton->Wins.count(); ++i)
            singleton->Wins.at (i)->startAutoSaving (as, interval);
    }
}

}
