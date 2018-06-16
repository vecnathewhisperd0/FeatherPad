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

#ifndef WARNINGBAR_H
#define WARNINGBAR_H

#include <QEvent>
#include <QGridLayout>
#include <QPalette>
#include <QLabel>
#include <QToolButton>
#include <QPropertyAnimation>
#include "utils.h"

namespace FeatherPad {

class WarningBar : public QWidget
{
    Q_OBJECT
public:
    WarningBar (const QString& message, ICONMODE iconMode = OWN, const int verticalOffset = 0, QWidget *parent = Q_NULLPTR) : QWidget (parent) {
        if (parent)
        { // only one warning bar at a time
            const QList<WarningBar*> warningBars = parent->findChildren<WarningBar*>();
            for (WarningBar *wb : warningBars)
            {
                if (wb != this) delete wb;
            }
        }

        message_ = message;
        vOffset_ = verticalOffset;

        /* make it like a translucent layer */
        setAutoFillBackground (true);
        QPalette p = palette();
        p.setColor (foregroundRole(), Qt::white);
        p.setColor (backgroundRole(), QColor (125, 0, 0, 200));
        setPalette (p);

        grid_ = new QGridLayout;
        grid_->setSpacing (5);
        if (layoutDirection() == Qt::RightToLeft)
            grid_->setContentsMargins (0, 0, 5 ,5);
        else
            grid_->setContentsMargins (5, 0, 0 ,5); // the top margin is added when setting the geometry
        /* use a spacer to compress the label vertically */
        QSpacerItem *spacer = new QSpacerItem (0, 0, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
        grid_->addItem (spacer, 0, 0);
        /* add a close button */
        QToolButton *b = new QToolButton;
        b->setAutoRaise (true);
        b->setText (tr ("Close"));
        if (iconMode == NONE)
            b->setToolButtonStyle (Qt::ToolButtonTextOnly);
        else
        {
            b->setToolButtonStyle (Qt::ToolButtonIconOnly);
            b->setIconSize (QSize (16, 16));
            b->setIcon (iconMode == OWN ? QIcon (":icons/window-close.svg")
                                        : QIcon::fromTheme ("window-close"));
            b->setToolTip (tr ("Close"));
        }
        connect (b, &QAbstractButton::clicked, [this]{deleteLater();});
        /* add the label */
        QLabel *warningLabel = new QLabel (message);
        warningLabel->setWordWrap (true);
        grid_->addWidget (warningLabel, 1, 0);
        grid_->addWidget (b, 1, 1);
        setLayout (grid_);

        /* compress the bar vertically and show it with animation */
        if (parent)
        {
            parent->installEventFilter (this);
            int h = grid_->minimumHeightForWidth (parent->width()) + grid_->contentsMargins().bottom();
            QRect g (0, parent->height() - h - vOffset_, parent->width(), h);
            setGeometry (g);

            QPropertyAnimation *animation = new QPropertyAnimation (this, "geometry");
            animation->setEasingCurve (QEasingCurve::Linear);
            animation->setDuration (150);
            animation->setStartValue (QRect (0, parent->height() - vOffset_, parent->width(), 0));
            animation->setEndValue (g);
            animation->start();
        }
        show();
    }

    bool eventFilter (QObject *o, QEvent *e) {
        if (e->type() == QEvent::Resize)
        {
            if (QWidget *w = qobject_cast<QWidget*>(o))
            {
                if (w == parentWidget())
                { // compress the bar as far as its text is shown completely
                    int h = grid_->minimumHeightForWidth (w->width()) + grid_->contentsMargins().bottom();
                    setGeometry (QRect (0, w->height() - h - vOffset_, w->width(), h));
                }
            }
        }
        return false;
    }

    QString getMessage() const {
        return message_;
    }

private:
    QString message_;
    int vOffset_;
    QGridLayout *grid_;
};

}

#endif // WARNINGBAR_H
