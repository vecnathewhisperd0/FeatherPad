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

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include "utils.h"

namespace FeatherPad {

class WarningBar : public QFrame
{
    Q_OBJECT
public:
    WarningBar (const QString& message, ICONMODE iconMode = OWN, QWidget *parent = Q_NULLPTR) : QFrame (parent) {
        message_ = message;
        QLabel *warningLabel = new QLabel (message);
        warningLabel->setWordWrap (true);
        QHBoxLayout *l = new QHBoxLayout;
        l->setSpacing (5);
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
        l->addWidget (warningLabel, 1);
        l->addWidget (b);
        setLayout (l);
        setStyleSheet ("QFrame {background-color: #7d0000; color: white; border-radius: 3px; margin: 2px; padding: 0px;}");
        connect (b, &QAbstractButton::clicked, [=]{emit closeButtonPressed();});
    }

    QString getMessage() const {
        return message_;
    }

signals:
    void closeButtonPressed();

private:
    QString message_;
};

}

#endif // WARNINGBAR_H
