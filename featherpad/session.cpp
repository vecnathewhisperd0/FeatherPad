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

#include "session.h"
#include "ui_sessionDialog.h"

namespace FeatherPad {

// Since we don't want extra prompt dialogs, we make the
// session dialog behave like a prompt dialog when needed.
SessionDialog::SessionDialog (QWidget *parent):QDialog (parent), ui (new Ui::SessionDialog)
{
    ui->setupUi (this);
    parent_ = parent;
    setObjectName ("sessionDialog");
    ui->promptContainer->hide();
    ui->promptLabel->setStyleSheet ("QLabel {background-color: #7d0000; color: white; border-radius: 3px; margin: 2px; padding: 5px;}");
    ui->listWidget->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

    connect (ui->listWidget, &QListWidget::itemDoubleClicked, this, &SessionDialog::restoreSession);
    connect (ui->listWidget, &QListWidget::itemSelectionChanged, this, &SessionDialog::selectionChanged);
    /* we want to open sessions by pressing Enter inside the list widget but the line-edit
       removes the default property from the Open button when it gets the focus (see below) */
    connect (ui->listWidget, &QListWidget::itemClicked, [=]{ui->openBtn->setDefault (true);});

    QSettings settings ("featherpad", "fp");
    settings.beginGroup ("sessions");
    QStringList keys = settings.allKeys();
    settings.endGroup();
    if (keys.count() > 0)
    {
        ui->listWidget->addItems (keys);
        ui->listWidget->setCurrentRow (0);
        QTimer::singleShot (0, ui->listWidget, SLOT (setFocus()));
    }
    else
    {
        ui->clearBtn->setEnabled (false);
        QTimer::singleShot (0, ui->lineEdit, SLOT (setFocus()));
    }

    connect (ui->saveBtn, &QAbstractButton::clicked, this, &SessionDialog::saveSession);
    connect (ui->lineEdit, &QLineEdit::returnPressed, this, &SessionDialog::saveSession);
    /* we don't want to open a session by pressing Enter inside the line-edit */
    connect (ui->lineEdit, &LineEdit::receivedFocus, [=](void){ui->openBtn->setDefault (false);});
    connect (ui->lineEdit, &QLineEdit::textEdited, [=](const QString &text){ui->saveBtn->setEnabled (!text.isEmpty());});
    connect (ui->openBtn, &QAbstractButton::clicked, this, &SessionDialog::openSessions);
    connect (ui->clearBtn, &QAbstractButton::clicked, this, &SessionDialog::openPromptContainer);
    connect (ui->removeBtn, &QAbstractButton::clicked, this, &SessionDialog::openPromptContainer);

    connect (ui->cancelBtn, &QAbstractButton::clicked, this, &SessionDialog::closePromptContainer);
    connect (ui->confirmBtn, &QAbstractButton::clicked, this, &SessionDialog::closePromptContainer);

    connect (ui->closeButton, &QAbstractButton::clicked, this, &QDialog::close);

    resize (parent_->size()/2);
}
/*************************/
SessionDialog::~SessionDialog()
{
    delete ui; ui = nullptr;
}
/*************************/
void SessionDialog::saveSession()
{
    if (ui->lineEdit->text().isEmpty()) return;

    bool hasFile (false);
    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    for (int i = 0; i < singleton->Wins.count(); ++i)
    {
        FPwin *win = singleton->Wins.at (i);
        for (int j = 0; j < win->ui->tabWidget->count(); ++j)
        {
            if (!qobject_cast< TabPage *>(win->ui->tabWidget->widget (j))
                ->textEdit()->getFileName().isEmpty())
            {
                hasFile = true;
                break;
            }
        }
        if (hasFile) break;
    }

    if (!hasFile)
        showPromptContainer (tr ("Nothing saved.<br>No file was opened."));
    else if (!ui->listWidget->findItems (ui->lineEdit->text(), Qt::MatchExactly).isEmpty())
        showPromptContainer();
    else
        reallySaveSession();
}
/*************************/
void SessionDialog::reallySaveSession()
{
    QList<QListWidgetItem*> sameItems = ui->listWidget->findItems (ui->lineEdit->text(), Qt::MatchExactly);
    for (int i = 0; i < sameItems.count(); ++i)
        delete ui->listWidget->takeItem (ui->listWidget->row (sameItems.at (i)));

    FPsingleton *singleton = static_cast<FPsingleton*>(qApp);
    QStringList files;
    for (int i = 0; i < singleton->Wins.count(); ++i)
    {
        FPwin *win = singleton->Wins.at (i);
        for (int j = 0; j < win->ui->tabWidget->count(); ++j)
        {
            TextEdit *textEdit = qobject_cast< TabPage *>(win->ui->tabWidget->widget (j))->textEdit();
            if (!textEdit->getFileName().isEmpty())
                files << textEdit->getFileName();
        }
    }
    /* there's always an opened file here */
    ui->listWidget->addItem (ui->lineEdit->text());
    ui->clearBtn->setEnabled (true);
    QSettings settings ("featherpad", "fp");
    settings.beginGroup ("sessions");
    settings.setValue (ui->lineEdit->text(), files);
    settings.endGroup();
}
/*************************/
void SessionDialog::restoreSession (QListWidgetItem* /*item*/)
{
    openSessions();
}
/*************************/
void SessionDialog::openSessions()
{
    QList<QListWidgetItem*> items = ui->listWidget->selectedItems();
    int count = items.count();
    if (count == 0) return;
    QSettings settings ("featherpad", "fp");
    settings.beginGroup ("sessions");
    QStringList files;
    for (int i = 0; i < count; ++i)
        files += settings.value (items.at (i)->text()).toStringList();
    settings.endGroup();
    if (!files.isEmpty())
    {
        if (FPwin *win = static_cast<FPwin *>(parent_))
        {
            bool multiple (files.count() > 1 || win->isLoading());
            for (int i = 0; i < files.count(); ++i)
                win->newTabFromName (files.at (i), multiple);
            /* return the focus to the dialog */
            connect (win, &FPwin::finishedLoading, this, &SessionDialog::activate);
        }
    }
}
/*************************/
void SessionDialog::activate()
{
    if (FPwin *win = static_cast<FPwin *>(parent_))
        disconnect (win, &FPwin::finishedLoading, this, &SessionDialog::activate);
    activateWindow();
    raise();
}
/*************************/
void SessionDialog::openPromptContainer()
{
    showPromptContainer();
}
/*************************/
void SessionDialog::showPromptContainer (QString message)
{
    disconnect (ui->confirmBtn, &QAbstractButton::clicked, this, &SessionDialog::removeAll);
    disconnect (ui->confirmBtn, &QAbstractButton::clicked, this, &SessionDialog::removeSelected);
    disconnect (ui->confirmBtn, &QAbstractButton::clicked, this, &SessionDialog::reallySaveSession);

    /* give time to processes, especially to the returnPressed signal of the line-edit */
    QTimer::singleShot (0, ui->mainContainer, SLOT (hide()));
    ui->promptContainer->show();

    ui->cancelBtn->setVisible (message.isEmpty());
    QTimer::singleShot (0, ui->confirmBtn, SLOT (setFocus()));

    if (!message.isEmpty())
        ui->promptLabel->setText ("<b>" + message + "</b>");
    else if (QObject::sender() == ui->clearBtn)
    {
        ui->promptLabel->setText ("<b>" + tr ("Do you really want to remove all saved sessions?") + "</b>");
        connect (ui->confirmBtn, &QAbstractButton::clicked, this, &SessionDialog::removeAll);
    }
    else if (QObject::sender() == ui->removeBtn)
    {
        if (ui->listWidget->selectedItems().count() > 1)
            ui->promptLabel->setText ("<b>" + tr ("Do you really want to remove the selected sessions?") + "</b>");
        else
            ui->promptLabel->setText ("<b>" + tr ("Do you really want to remove the selected session?") + "</b>");
        connect (ui->confirmBtn, &QAbstractButton::clicked, this, &SessionDialog::removeSelected);
    }
    else // same name prompt
    {
        ui->promptLabel->setText ("<b>" + tr ("A session with the same name exists.<br>Do you want to overwrite it?") + "</b>");
        connect (ui->confirmBtn, &QAbstractButton::clicked, this, &SessionDialog::reallySaveSession);
    }
}
/*************************/
void SessionDialog::closePromptContainer()
{
    ui->promptLabel->clear();
    ui->promptContainer->hide();
    QTimer::singleShot (0, this, SLOT (showMainContainer()));
}
/*************************/
void SessionDialog::showMainContainer()
{
    /* give the focus to the line-edit after showing the main container */
    ui->mainContainer->show();
    QTimer::singleShot (0, ui->lineEdit, SLOT (setFocus()));
}
/*************************/
void SessionDialog::removeSelected()
{
    QList<QListWidgetItem*> items = ui->listWidget->selectedItems();
    int count = items.count();
    if (count == 0) return;

    QSettings settings ("featherpad", "fp");
    settings.beginGroup ("sessions");
    for (int i = 0; i < count; ++i)
    {
        settings.remove (items.at (i)->text());
        delete ui->listWidget->takeItem (ui->listWidget->row (items.at (i)));
    }
    settings.endGroup();

    if (ui->listWidget->count() == 0)
        ui->clearBtn->setEnabled (false);
}
/*************************/
void SessionDialog::removeAll()
{
    ui->listWidget->clear();
    ui->clearBtn->setEnabled (false);
    QSettings settings ("featherpad", "fp");
    settings.beginGroup ("sessions");
    settings.remove ("");
    settings.endGroup();
}
/*************************/
void SessionDialog::selectionChanged()
{
    bool noSel = ui->listWidget->selectedItems().isEmpty();
    ui->openBtn->setEnabled (!noSel);
    ui->removeBtn->setEnabled (!noSel);
    /* we want to open sessions by pressing Enter inside the list widget */
    ui->openBtn->setDefault (true);
}

}
