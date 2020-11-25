/*
 * Copyright (C) Pedram Pourang (aka Tsu Jan) 2020 <tsujan2000@gmail.com>
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

#ifndef PRINTING_H
#define PRINTING_H

#include <QThread>
#include <QColor>
#include <QTextDocument>
#include <QPrinter>

namespace FeatherPad {

class Printing : public QThread {
    Q_OBJECT

public:
    Printing (QTextDocument *document, const QString &fileName,
              const QColor &textColor, int darkValue,
              qreal sourceDpiX, qreal sourceDpiY);
    ~Printing();

    QPrinter* printer() const {
        return printer_;
    }

private:
    void run();

    QTextDocument *document_;
    QPrinter *printer_;
    QColor textColor_;
    QColor darkColor_;
    qreal sourceDpiX_;
    qreal sourceDpiY_;
};

}

#endif // PRINTING_H
