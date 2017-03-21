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

#ifndef SESSION_H
#define SESSION_H

#include <QDialog>
#include <QListWidgetItem>

namespace FeatherPad {

namespace Ui {
class SessionDialog;
}

class SessionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SessionDialog (QWidget *parent = 0);
    ~SessionDialog();

private slots:
    void saveSession();
    void reallySaveSession();
    void selectionChanged();
    void restoreSession (QListWidgetItem*);
    void openSessions();
    void openPromptContainer();
    void closePromptContainer();
    void showMainContainer();
    void removeSelected();
    void removeAll();
    void activate();

private:
    void showPromptContainer (QString message = QString());

    Ui::SessionDialog *ui;
    QWidget * parent_;
};

}

#endif // SESSION_H
