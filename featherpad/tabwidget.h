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

#ifndef TABWIDGET_H
#define TABWIDGET_H

#include <QTabWidget>
#include <QTimerEvent>
#include "tabbar.h"

namespace FeatherPad {

/* At first, I subclassed QTabWidget just because QTabBar was
   a protected member of it but afterward, I implemented tab DND
   in TabBar and also provided currentChanged() with a 50-ms buffer. */
class TabWidget : public QTabWidget
{
    Q_OBJECT

public:
    TabWidget (QWidget *parent = nullptr);
    ~TabWidget();
    /* make it public */
    TabBar *tabBar() const {return tb;}

signals:
    void currentTabChanged (int curIndx);

protected:
    void timerEvent (QTimerEvent *event);

private slots:
    void tabSwitch (int index);

private:
    TabBar *tb;
    int timerId;
    int curIndx;
};

}

#endif // TABWIDGET_H
