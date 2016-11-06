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

#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <QMessageBox>
#include <QAbstractButton>

namespace FeatherPad {

/* QMessageBox::setButtonText() is obsolete while
   we want custom texts, especially for translation. */
class MessageBox : public QMessageBox {
    Q_OBJECT
public:
    MessageBox (QWidget *parent = Q_NULLPTR) : QMessageBox (parent) {}
    MessageBox (Icon icon, const QString &title,
                const QString &text,
                StandardButtons buttons = NoButton,
                QWidget *parent = Q_NULLPTR,
                Qt::WindowFlags f = Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint) : QMessageBox (icon, title, text, buttons, parent, f) {}

    void changeButtonText (QMessageBox::StandardButton btn, const QString &text) {
        if (QAbstractButton *abtn = button (btn))
            abtn->setText (text);
    }
};

}

#endif // MESSAGEBOX_H
