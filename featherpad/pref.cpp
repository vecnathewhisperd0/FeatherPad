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

#include "pref.h"
#include "ui_predDialog.h"

#include <QDesktopWidget>
#include <QWhatsThis>

namespace FeatherPad {

PrefDialog::PrefDialog (QWidget *parent):QDialog (parent), ui (new Ui::PrefDialog)
{
    ui->setupUi (this);
    parent_ = parent;
    setWindowModality (Qt::WindowModal);
    ui->promptLabel->hide();

    Config config = static_cast<FPsingleton*>(qApp)->getConfig();
    sysIcons_ = config.getSysIcon();
    iconless_ = config.getIconless();
    darkBg_ = config.getDarkColScheme();
    darkColValue_ = config.getDarkBgColorValue();
    lightColValue_ = config.getLightBgColorValue();
    recentNumber_ = config.getRecentFilesNumber();

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

    // no ccombo onnection because of mouse wheel; config is set at closeEvent() instead
    ui->tabCombo->setCurrentIndex (config.getTabPosition());

    ui->tabBox->setChecked (config.getTabWrapAround());
    connect (ui->tabBox, &QCheckBox::stateChanged, this, &PrefDialog::prefTabWrapAround);

    ui->singleTabBox->setChecked (config.getHideSingleTab());
    connect (ui->singleTabBox, &QCheckBox::stateChanged, this, &PrefDialog::prefHideSingleTab);

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

    ui->lastLineBox->setChecked (config.getAppendEmptyLine());
    connect (ui->lastLineBox, &QCheckBox::stateChanged, this, &PrefDialog::prefAppendEmptyLine);

    ui->scrollBox->setChecked (config.getScrollJumpWorkaround());
    connect (ui->scrollBox, &QCheckBox::stateChanged, this, &PrefDialog::prefScrollJumpWorkaround);

    ui->spinBox->setValue (config.getMaxSHSize());
    connect (ui->spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
             this, &PrefDialog::prefMaxSHSize);

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

    ui->nativeDialogBox->setChecked (config.getNativeDialog());
    connect (ui->nativeDialogBox, &QCheckBox::stateChanged, this, &PrefDialog::prefNativeDialog);

    connect (ui->closeButton, &QAbstractButton::clicked, this, &QDialog::close);
    connect (ui->helpButton, &QAbstractButton::clicked, this, &PrefDialog::showWhatsThis);

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
    delete ui; ui = nullptr;
}
/*************************/
void PrefDialog::closeEvent (QCloseEvent *event)
{
    prefTabPosition();
    event->accept();
    prefRecentFilesKind();
}
/*************************/
void PrefDialog::showPrompt()
{
    QString style ("QLabel {background-color: #7d0000; color: white; border-radius: 3px; margin: 2px; padding: 5px;}");
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    if (sysIcons_ != config.getSysIcon()
        || iconless_ != config.getIconless()
        || recentNumber_ != config.getRecentFilesNumber())
    {
        ui->promptLabel->setText ("<b>" + tr ("Application restart is needed for changes to take effect.") + "</b>");
        ui->promptLabel->setStyleSheet (style);
    }
    else if (darkBg_ != config.getDarkColScheme()
             || (darkBg_ && darkColValue_ != config.getDarkBgColorValue())
             || (!darkBg_ && lightColValue_ != config.getLightBgColorValue()))
    {
        ui->promptLabel->setText ("<b>" + tr ("Window reopening is needed for changes to take effect.") + "</b>");
        ui->promptLabel->setStyleSheet (style);
    }
    else
    {
        ui->promptLabel->clear();
        ui->promptLabel->setStyleSheet ("QLabel {margin: 2px; padding: 5px;}");
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
                int index = win->ui->tabWidget->currentIndex();
                if (index > -1)
                {
                    TextEdit *textEdit = qobject_cast< TabPage *>(win->ui->tabWidget->widget (index))->textEdit();
                    win->statusMsgWithLineCount (textEdit->document()->blockCount());
                    for (int j = 0; j < win->ui->tabWidget->count(); ++j)
                    {
                        TextEdit *thisTextEdit = qobject_cast< TabPage *>(win->ui->tabWidget->widget (j))->textEdit();
                        connect (thisTextEdit, &QPlainTextEdit::blockCountChanged, win, &FPwin::statusMsgWithLineCount);
                        connect (thisTextEdit, &QPlainTextEdit::selectionChanged, win, &FPwin::statusMsg);
                    }
                    win->ui->statusBar->setVisible (true);
                    if (QToolButton *wordButton = win->ui->statusBar->findChild<QToolButton *>())
                    {
                        if (textEdit->getWordNumber() == -1 // when words aren't counted yet
                            && (!textEdit->document()->isEmpty() || textEdit->document()->isModified()))
                        {
                            wordButton->setVisible (false); // becomes "true" below
                        }
                        else
                            wordButton->setVisible (true);
                    }
                    win->wordButtonStatus();
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
            int index = win->ui->tabWidget->currentIndex();
            if (index > -1)
            {
                config.setFont (qobject_cast< TabPage *>(win->ui->tabWidget->widget (index))
                                ->textEdit()->document()->defaultFont());
            }
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
            if (count > 0)
            {
                for (int j = 0; j < count; ++j)
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
            if (count > 0)
            {
                for (int j = 0; j < count; ++j)
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
    Config& config = static_cast<FPsingleton*>(qApp)->getConfig();
    if (checked == Qt::Checked)
        config.setSyntaxByDefault (true);
    else if (checked == Qt::Unchecked)
        config.setSyntaxByDefault (false);
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
            if (count > 0)
            {
                for (int j = 0; j < count; ++j)
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
            if (count > 0)
            {
                for (int j = 0; j < count; ++j)
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
            TabBar *tabBar = singleton->Wins.at (i)->ui->tabWidget->tabBar();
            tabBar->hideSingle (true);
            if (singleton->Wins.at (i)->ui->tabWidget->count() == 1)
                tabBar->hide();
        }
    }
    else if (checked == Qt::Unchecked)
    {
        config.setHideSingleTab (false);
        for (int i = 0; i < singleton->Wins.count(); ++i)
        {
            TabBar *tabBar = singleton->Wins.at (i)->ui->tabWidget->tabBar();
            tabBar->hideSingle (false);
            if (singleton->Wins.at (i)->ui->tabWidget->count() == 1)
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
            int index = win->ui->tabWidget->currentIndex();
            if (index > -1)
            {
                TextEdit *textEdit = qobject_cast< TabPage *>(win->ui->tabWidget->widget (index))->textEdit();
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
void PrefDialog::prefNativeDialog (int checked)
{
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    if (checked == Qt::Checked)
        config.setNativeDialog (true);
    else if (checked == Qt::Unchecked)
        config.setNativeDialog (false);
}

}
