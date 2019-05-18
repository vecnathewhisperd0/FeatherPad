/*
 * Copyright (C) Pedram Pourang (aka Tsu Jan) 2019 <tsujan2000@gmail.com>
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

#include "spellDialog.h"
#include "ui_spellDialog.h"
#include "spellChecker.h"

namespace FeatherPad {

SpellDialog::SpellDialog (SpellChecker *spellChecker, const QString& word, QWidget *parent)
    : QDialog (parent), ui (new Ui::SpellDialog)
{
    ui->setupUi (this);
    spellChecker_ = spellChecker;

    connect (ui->correctOnce, &QAbstractButton::clicked, [this] {emit spellChecked (SpellAction::CorrectOnce);});
    connect (ui->ignoreOnce, &QAbstractButton::clicked, [this] {emit spellChecked (SpellAction::IgnoreOnce);});
    connect (ui->correctAll, &QAbstractButton::clicked, [this] {emit spellChecked (SpellAction::CorrectAll);});
    connect (ui->ignoreAll, &QAbstractButton::clicked, [this] {emit spellChecked (SpellAction::IgnoreAll);});

    connect (ui->addToDict, &QAbstractButton::clicked, [this] {emit spellChecked (SpellAction::AddToDict);});

    connect (ui->listWidget, &QListWidget::currentTextChanged, ui->replace, &QLineEdit::setText);

    checkWord (word);
}
/*************************/
SpellDialog::~SpellDialog()
{
    delete ui; ui = nullptr;
}
/*************************/
QString SpellDialog::replacement() const
{
    return ui->replace->text();
}
/*************************/
void SpellDialog::checkWord (const QString &word)
{
    if (word.isEmpty()) return;
    ui->replace->clear();
    ui->listWidget->clear();

    ui->misspelledLabel->setText (QString ("<b>%1</b>").arg (word));

    QStringList suggestions = spellChecker_->suggest (word);
    ui->listWidget->addItems (suggestions);
    if (!suggestions.isEmpty())
        ui->listWidget->setCurrentRow (0, QItemSelectionModel::Select);
}

}
