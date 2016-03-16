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

#include <QtGui>
#include <QToolTip>
#include "tabbar.h"

namespace FeatherPad {

TabBar::TabBar (QWidget *parent)
    : QTabBar (parent)
{
    setMouseTracking (true);
}
/*************************/
void TabBar::mouseReleaseEvent (QMouseEvent *event)
{
    /* first do what a QTabBar does... */
    QTabBar::mouseReleaseEvent (event);
    /* ...then emit an extra signal,
       that will be useful for tab detachment */
    QPoint dropPos = event->globalPos();
    emit tabDropped (dropPos);
}
/*************************/
void TabBar::mouseMoveEvent (QMouseEvent *event)
{
    QTabBar::mouseMoveEvent (event);
    int index = tabAt (event->pos());
    if (index != -1)
        QToolTip::showText (event->globalPos(), tabToolTip (index));
}
/*************************/
// Don't show tooltip with setTabToolTip().
bool TabBar::event (QEvent *event)
{
#ifndef QT_NO_TOOLTIP
    if (event->type() == QEvent::ToolTip)
        return QWidget::event (event);
    else
       return QTabBar::event (event);
#endif
    return QTabBar::event (event);
}

}
