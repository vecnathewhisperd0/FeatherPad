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

#ifndef TABWIDGET_H
#define TABWIDGET_H

#include <QTabWidget>
#include "tabbar.h"

namespace FeatherPad {

/* At first, I did this subclassing just because QTabBar was
   a protected member of QTabWidget but afterward, I needed
   a signal for tab dropping, which I put in a TabBar subclass. */
class TabWidget : public QTabWidget
{
    Q_OBJECT

public:
    TabWidget (QWidget *parent = 0) : QTabWidget (parent)
    {
        tb = new TabBar;
        setTabBar (tb);
    }
    ~TabWidget() {delete tb;}
    /* make it public */
    QTabBar *tabBar() const {return tb;}

private:
    TabBar *tb;
};

}

#endif // TABWIDGET_H
