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

#ifndef SEARCHBAR_H
#define SEARCHBAR_H

#include <QPointer>
#include <QPushButton>
#include "lineedit.h"

namespace FeatherPad {

class SearchBar : public QFrame
{
    Q_OBJECT
public:
    SearchBar (QWidget *parent = 0,
               bool hasText = false,
               const QStringList& shortcuts = QStringList(),
               Qt::WindowFlags f = Qt::WindowFlags());

    void focusLineEdit();
    bool lineEditHasFocus();
    QString searchEntry() const;
    void clearSearchEntry();

    bool matchCase() const;
    bool matchWhole() const;

    void updateShortcuts (bool disable);
    void setSearchIcons (const QIcon& iconNext, const QIcon& iconPrev,
                         const QIcon& wholeIcon, const QIcon& caseIcon);

signals:
    void searchFlagChanged();
    void find (bool forward);

private:
    void findForward();
    void findBackward();

    QPointer<LineEdit> lineEdit_;
    QPointer<QToolButton> toolButton_nxt_;
    QPointer<QToolButton> toolButton_prv_;
    QPointer<QToolButton> button_case_;
    QPointer<QToolButton> button_whole_;
    QStringList shortcuts_;
};

}

#endif // SEARCHBAR_H
