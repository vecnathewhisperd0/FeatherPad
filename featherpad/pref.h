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

#ifndef PREF_H
#define PREF_H

#include <QDialog>
#include "config.h"

namespace FeatherPad {

namespace Ui {
class PrefDialog;
}

class PrefDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PrefDialog (QWidget *parent = 0);
    ~PrefDialog();

private slots:
    void prefSize (int checked);
    void prefStartSize (int value);
    void prefIcon (int checked);
    void prefToolbar (int checked);
    void prefMenubar (int checked);
    void prefSearchbar (int checked);
    void prefStatusbar (int checked);
    void prefFont (int checked);
    void prefWrap (int checked);
    void prefIndent (int checked);
    void prefLine (int checked);
    void prefSyntax (int checked);
    void prefDarkColScheme (int checked);
    void prefColValue (int value);
    void prefScrollJumpWorkaround (int checked);
    void prefTabWrapAround (int checked);
    void prefHideSingleTab (int checked);
    void prefMaxSHSize (int value);
    void prefExecute (int checked);
    void prefCommand (QString command);
    void prefRecentFilesNumber (int value);
    void prefOpenRecentFile (int value);
    void showWhatsThis();

private:
    void closeEvent (QCloseEvent *event);
    void prefTabPosition();
    void prefRecentFilesKind();
    void showPrompt();

    Ui::PrefDialog *ui;
    QWidget * parent_;
    bool darkBg_;
    bool sysIcons_;
    int darkColValue_;
    int lightColValue_;
    int recentNumber_;
};

}

#endif // PREF_H
