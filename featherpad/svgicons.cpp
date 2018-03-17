/* Adapted from:
 * Cantata - Qt5 Graphical MPD Client for Linux, Windows, macOS, Haiku
 * File: support/monoicon.cpp
 * Copyright: 2011-2018 Craig Drummond
 * License: GPL-3.0+
 * Homepage: https://github.com/CDrummond/cantata
 */

/*
 * Copyright (C) Pedram Pourang (aka Tsu Jan) 2018 <tsujan2000@gmail.com>
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

#include <QFile>
#include <QIconEngine>
#include <QSvgRenderer>
#include <QPainter>
#include <QPixmapCache>
#include <QRect>
#include <QFontDatabase>
#include <QApplication>
#include <QPalette>

#include "svgicons.h"

namespace FeatherPad {

class symbolicIconEngine : public QIconEngine
{
public:
    symbolicIconEngine (const QString &file) : fileName (file) {}

    ~symbolicIconEngine() override {}

    symbolicIconEngine * clone() const override {
        return new symbolicIconEngine (fileName);
    }

    void paint (QPainter *painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override {
        Q_UNUSED (state)

        QColor col;
        if (mode == QIcon::Selected)
            col = QApplication::palette().highlightedText().color();
        else
            col = QApplication::palette().windowText().color();
        QString key = fileName
                      + "-" + QString::number (rect.width())
                      + "-" + QString::number (rect.height())
                      + "-" + col.name();
        QPixmap pix;
        if (!QPixmapCache::find (key, &pix))
        {
            pix = QPixmap (rect.width(), rect.height());
            pix.fill (Qt::transparent);
            if (!fileName.isEmpty())
            {
                QSvgRenderer renderer;
                QFile f (fileName);
                QByteArray bytes;
                if (f.open (QIODevice::ReadOnly))
                    bytes=  f.readAll();
                if (!bytes.isEmpty())
                    bytes.replace ("#000", col.name().toLatin1());
                renderer.load (bytes);
                QPainter p (&pix);
                renderer.render (&p, QRect (0, 0, rect.width(), rect.height()));
            }
            QPixmapCache::insert (key, pix);
        }
        if (mode == QIcon::Disabled)
        {
            /* this part is taken from Qt -> QCommonStyle::generatedIconPixmap()
               because it correctly grays out disabled icons */
            QImage im = pix.toImage().convertToFormat (QImage::Format_ARGB32);
            QColor bg = QApplication::palette().color (QPalette::Disabled, QPalette::Window);
            int red = bg.red();
            int green = bg.green();
            int blue = bg.blue();
            uchar reds[256], greens[256], blues[256];
            for (int i = 0; i < 128; ++i)
            {
                reds[i]   = uchar ((red   * (i<<1)) >> 8);
                greens[i] = uchar ((green * (i<<1)) >> 8);
                blues[i]  = uchar ((blue  * (i<<1)) >> 8);
            }
            for (int i = 0; i < 128; ++i)
            {
                reds[i+128]   = uchar (qMin (red   + (i << 1), 255));
                greens[i+128] = uchar (qMin (green + (i << 1), 255));
                blues[i+128]  = uchar (qMin (blue  + (i << 1), 255));
            }
            int intensity = (77 * red + 150 * green + 28 * blue) / 255; // 30% red, 59% green, 11% blue
            const int factor = 191;
            if ((red - factor > green && red - factor > blue)
                || (green - factor > red && green - factor > blue)
                || (blue - factor > red && blue - factor > green))
            {
                intensity = qMin (255, intensity + 91);
            }
            else if (intensity <= 128)
                intensity -= 51;
            for (int y = 0; y < im.height(); ++y)
            {
                QRgb *scanLine = (QRgb*)im.scanLine (y);
                for (int x = 0; x < im.width(); ++x)
                {
                    QRgb pixel = *scanLine;
                    uint ci = uint (qGray (pixel) / 3 + (130 - intensity / 3));
                    *scanLine = qRgba (reds[ci], greens[ci], blues[ci], qAlpha (pixel));
                    ++scanLine;
                }
            }
            pix = QPixmap::fromImage (im);
        }
        painter->drawPixmap (rect.topLeft(), pix);
    }

    QPixmap pixmap (const QSize& size, QIcon::Mode mode, QIcon::State state) override {
        QPixmap pix (size);
        pix.fill (Qt::transparent);
        QPainter painter (&pix);
        paint (&painter, QRect (QPoint (0, 0), size), mode, state);
        return pix;
    }

private:
    QString fileName;
};

QIcon symbolicIcon::icon (const QString& fileName) {
    return QIcon (new symbolicIconEngine (fileName));
}

}
