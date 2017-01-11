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

namespace FeatherPad {

PrefDialog::PrefDialog (QWidget *parent):QDialog (parent), ui (new Ui::PrefDialog)
{
    ui->setupUi (this);
    parent_ = parent;
    setWindowModality (Qt::WindowModal);

    Config config = static_cast<FPsingleton*>(qApp)->getConfig();

    /**************
     *** Window ***
     **************/

    ui->winSizeBox->setChecked (config.getRemSize());
    connect (ui->winSizeBox, &QCheckBox::stateChanged, this, &PrefDialog::prefSize);
    ui->iconBox->setChecked (!config.getSysIcon());
    connect (ui->iconBox, &QCheckBox::stateChanged, this, &PrefDialog::prefIcon);
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

    if (ui->winSizeBox->isChecked())
    {
        ui->spinX->setEnabled (false);
        ui->spinY->setEnabled (false);
        ui->mLabel->setEnabled (false);
        ui->sizeLable->setEnabled (false);
    }
    QSize ag = QApplication::desktop()->availableGeometry().size() - QSize (50, 100);
    ui->spinX->setMaximum (ag.width());
    ui->spinY->setMaximum (ag.height());
    ui->spinX->setValue (config.getStartSize().width());
    ui->spinY->setValue (config.getStartSize().height());
    connect (ui->spinX, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PrefDialog::prefStartSize);
    connect (ui->spinY, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PrefDialog::prefStartSize);

    /************
     *** Text ***
     ************/

    ui->fontBox->setChecked (config.getRemFont());
    connect (ui->fontBox, &QCheckBox::stateChanged, this, &PrefDialog::prefFont);
    ui->wrapBox->setChecked (config.getWrapByDefault());
    connect (ui->wrapBox, &QCheckBox::stateChanged, this, &PrefDialog::prefWrap);
    ui->indentBox->setChecked (config.getIndentByDefault());
    connect (ui->indentBox, &QCheckBox::stateChanged, this, &PrefDialog::prefIndent);
    ui->lineBox->setChecked (config.getLineByDefault());
    connect (ui->lineBox, &QCheckBox::stateChanged, this, &PrefDialog::prefLine);
    ui->syntaxBox->setChecked (config.getSyntaxByDefault());
    connect (ui->syntaxBox, &QCheckBox::stateChanged, this, &PrefDialog::prefSyntax);
    ui->colBox->setChecked (config.getDarkColScheme());
    connect (ui->colBox, &QCheckBox::stateChanged, this, &PrefDialog::prefDarkColScheme);
    ui->scrollBox->setChecked (config.getScrollJumpWorkaround());
    connect (ui->scrollBox, &QCheckBox::stateChanged, this, &PrefDialog::prefScrollJumpWorkaround);

    ui->spinBox->setValue (config.getMaxSHSize());
    connect (ui->spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PrefDialog::prefMaxSHSize);

    QPushButton *closeButton = ui->buttonBox->button (QDialogButtonBox::Close);
    closeButton->setText (tr ("Close"));
    connect (closeButton, &QAbstractButton::clicked, this, &QDialog::close);
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
    preftabPosition();
    event->accept();
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
                    win->statusMsgWithLineCount (qobject_cast< TextEdit *>(win->ui->tabWidget->widget (index))
                                    ->document()->blockCount());
                    TextEdit *textEdit = nullptr;
                    QHash<TextEdit*,tabInfo*>::iterator it;
                    for (it = win->tabsInfo_.begin(); it != win->tabsInfo_.end(); ++it)
                    {
                        connect (it.key(), &QPlainTextEdit::blockCountChanged, win, &FPwin::statusMsgWithLineCount);
                        connect (it.key(), &QPlainTextEdit::selectionChanged, win, &FPwin::statusMsg);
                    }
                    textEdit = qobject_cast< TextEdit *>(win->ui->tabWidget->widget (index));
                    tabInfo *tabinfo = win->tabsInfo_[textEdit];
                    win->ui->statusBar->setVisible (true);
                    if (QToolButton *wordButton = win->ui->statusBar->findChild<QToolButton *>())
                    {
                        if (tabinfo->wordNumber == -1 // when words aren't counted yet
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
        {
            FPwin *win = singleton->Wins.at (i);
            if (win->ui->tabWidget->currentIndex() > -1)
                win->ui->actionDoc->setVisible (true);
        }
    }
}
/*************************/
void PrefDialog::preftabPosition()
{
    int index = ui->tabCombo->currentIndex();
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    Config& config = singleton->getConfig();
    config.setTabPosition (index);
    for (int i = 0; i < singleton->Wins.count(); ++i)
    {
        FPwin *win = singleton->Wins.at (i);
        win->ui->tabWidget->setTabPosition ((QTabWidget::TabPosition) index);
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
                config.setFont (qobject_cast< TextEdit *>(win->ui->tabWidget->widget (index))
                                ->document()->defaultFont());
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
    if (checked == Qt::Checked)
        config.setDarkColScheme (true);
    else if (checked == Qt::Unchecked)
        config.setDarkColScheme (false); 
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
                    qobject_cast< TextEdit *>(singleton->Wins.at (i)->ui->tabWidget->widget (j))
                                             ->setScrollJumpWorkaround (true);
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
                    qobject_cast< TextEdit *>(singleton->Wins.at (i)->ui->tabWidget->widget (j))
                                             ->setScrollJumpWorkaround (false);
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
void PrefDialog::prefMaxSHSize (int value)
{
    static_cast<FPsingleton*>(qApp)->getConfig().setMaxSHSize (value);
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

}
