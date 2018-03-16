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

#include <QGridLayout>
#include <QToolButton>
#include "searchbar.h"

namespace FeatherPad {


SearchBar::SearchBar(QWidget *parent,
                     bool hasText,
                     const QStringList &shortcuts,
                     Qt::WindowFlags f)
    : QFrame (parent, f)
{
    lineEdit_ = new LineEdit (this);
    lineEdit_->setMinimumWidth (150);
    lineEdit_->setPlaceholderText (tr ("Search..."));
    //lineEdit_->setSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    shortcuts_ = shortcuts;
    QString nxtShortcut, prevShortcut, csShortcut, wholeShortcut;
    if (shortcuts.size() >= 4)
    {
        nxtShortcut = shortcuts.at (0);
        prevShortcut = shortcuts.at (1);
        csShortcut = shortcuts.at (2);
        wholeShortcut = shortcuts.at (3);
    }

    /* See the comment about KAcceleratorManager in "fpwin.cpp". */

    toolButton_nxt_ = new QToolButton (this);
    toolButton_prv_ = new QToolButton (this);
    toolButton_nxt_->setAutoRaise (true);
    toolButton_prv_->setAutoRaise (true);
    if (hasText)
    {
        toolButton_nxt_->setText (tr ("Next"));
        toolButton_prv_->setText (tr ("Previous"));
    }
    toolButton_nxt_->setShortcut (QKeySequence (nxtShortcut));
    toolButton_prv_->setShortcut (QKeySequence (prevShortcut));
    toolButton_nxt_->setToolTip (tr ("Next") + " (" + nxtShortcut + ")");
    toolButton_prv_->setToolTip (tr ("Previous") + " (" + prevShortcut + ")");

    button_case_ = new QToolButton (this);
    if (hasText)
    {
        button_case_->setText (tr ("Match Case"));
        button_case_->setToolTip (csShortcut);
    }
    else
        button_case_->setToolTip (tr ("Match Case") + " (" + csShortcut + ")");
    button_case_->setShortcut (QKeySequence (csShortcut));
    button_case_->setCheckable (true);
    button_case_->setFocusPolicy (Qt::NoFocus);

    button_whole_ = new QToolButton (this);
    if (hasText)
    {
        button_whole_->setText (tr ("Whole Word"));
        button_whole_->setToolTip (wholeShortcut);
    }
    else
        button_whole_->setToolTip (tr ("Whole Word") + " (" + wholeShortcut + ")");
    button_whole_->setShortcut (QKeySequence (wholeShortcut));
    button_whole_->setCheckable (true);
    button_whole_->setFocusPolicy (Qt::NoFocus);

    /* there are shortcuts for forward/backward search */
    toolButton_nxt_->setFocusPolicy (Qt::NoFocus);
    toolButton_prv_->setFocusPolicy (Qt::NoFocus);

    QGridLayout *mainGrid = new QGridLayout;
    mainGrid->setHorizontalSpacing (3);
    mainGrid->setContentsMargins (2, 0, 2, 0);
    mainGrid->addWidget (lineEdit_, 0, 0);
    mainGrid->addWidget (toolButton_nxt_, 0, 1);
    mainGrid->addWidget (toolButton_prv_, 0, 2);
    mainGrid->addItem (new QSpacerItem (6, 3), 0, 3);
    mainGrid->addWidget (button_case_, 0, 4);
    mainGrid->addWidget (button_whole_, 0, 5);
    setLayout (mainGrid);

    connect (lineEdit_, &QLineEdit::returnPressed, this, &SearchBar::findForward);
    connect (toolButton_nxt_, &QAbstractButton::clicked, this, &SearchBar::findForward);
    connect (toolButton_prv_, &QAbstractButton::clicked, this, &SearchBar::findBackward);
    connect (button_case_, &QAbstractButton::clicked, this, &SearchBar::searchFlagChanged);
    connect (button_whole_, &QAbstractButton::clicked, this, &SearchBar::searchFlagChanged);
}
/*************************/
void SearchBar::focusLineEdit()
{
    lineEdit_->setFocus();
    lineEdit_->selectAll();
}
/*************************/
bool SearchBar::lineEditHasFocus()
{
    return lineEdit_->hasFocus();
}
/*************************/
QString SearchBar::searchEntry() const
{
    return lineEdit_->text();
}
/*************************/
void SearchBar::clearSearchEntry()
{
    return lineEdit_->clear(); // doesn't remove the undo/redo history
}
/*************************/
void SearchBar::findForward()
{
    emit find (true);
}
/*************************/
void SearchBar::findBackward()
{
    emit find (false);
}
/*************************/
bool SearchBar::matchCase() const
{
    return button_case_->isChecked();
}
/*************************/
bool SearchBar::matchWhole() const
{
    return button_whole_->isChecked();
}
/*************************/
// Used only in a workaround (-> FPwin::updateShortcuts())
void SearchBar::updateShortcuts (bool disable)
{
    if (disable)
    {
        toolButton_nxt_->setShortcut (QKeySequence());
        toolButton_prv_->setShortcut (QKeySequence());
        button_case_->setShortcut (QKeySequence());
        button_whole_->setShortcut (QKeySequence());
    }
    else if (shortcuts_.size() >= 4)
    {
        toolButton_nxt_->setShortcut (QKeySequence (shortcuts_.at (0)));
        toolButton_prv_->setShortcut (QKeySequence (shortcuts_.at (1)));
        button_case_->setShortcut (QKeySequence (shortcuts_.at (2)));
        button_whole_->setShortcut (QKeySequence (shortcuts_.at (3)));
    }
}
/*************************/
void SearchBar::setSearchIcons (const QIcon& iconNext, const QIcon& iconPrev,
                                const QIcon& wholeIcon, const QIcon& caseIcon)
{
    toolButton_nxt_->setIcon (iconNext);
    toolButton_prv_->setIcon (iconPrev);
    button_whole_->setIcon (wholeIcon);
    button_case_->setIcon (caseIcon);
}

}
