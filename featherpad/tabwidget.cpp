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

#include "tabwidget.h"

namespace FeatherPad {

TabWidget::TabWidget (QWidget *parent) : QTabWidget (parent)
{
    tb = new TabBar;
    setTabBar (tb);
    tb->setFocusPolicy (Qt::NoFocus); // there are shortcuts for tab switching
    curIndx= -1;
    timerId = 0;
    connect (this, &QTabWidget::currentChanged, this, &TabWidget::tabSwitch);
}
/*************************/
TabWidget::~TabWidget()
{
    delete tb;
}
/*************************/
void TabWidget::tabSwitch (int index)
{
    curIndx = index;
    if (timerId)
    {
        killTimer (timerId);
        timerId = 0;
    }
    timerId = startTimer (50);
}
/*************************/
void TabWidget::timerEvent (QTimerEvent *e)
{
    QTabWidget::timerEvent (e);

    if (e->timerId() == timerId)
    {
        killTimer (e->timerId());
        timerId = 0;
        emit currentTabChanged (curIndx);
    }
}

}
