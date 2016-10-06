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

#include "loading.h"
#include "encoding.h"
#include <QFile>
#include <QTextCodec>

namespace FeatherPad {

Loading::Loading (QString fname, QString charset, bool enforceEncod, bool reload, bool multiple) :
    fname_ (fname),
    charset_ (charset),
    enforceEncod_ (enforceEncod),
    reload_ (reload),
    multiple_ (multiple)
{}
/*************************/
Loading::~Loading() {}
/*************************/
void Loading::run()
{
    if (!QFile::exists (fname_)) return;

    QFile file (fname_);
    if (!file.open (QFile::ReadOnly)) return;

    /* read the file character by character
       in order to determine if it's in UTF-16 */

    QByteArray data ("");
    char c;
    qint64 rchar;
    while ((rchar = file.read (&c, sizeof (char))) > 0)
    {
        data.append (c);
        if (!enforceEncod_ /*&& charset_.isEmpty()*/ && c == '\0')
            charset_ = "UTF-16";
    }
    file.close();

    if (charset_.isEmpty())
        charset_ = detectCharset (data);

    QTextCodec *codec = QTextCodec::codecForName (charset_.toUtf8()); // or charset.toStdString().c_str()
    if (!codec) // prevent any chance of crash if there's a bug
    {
        charset_ = "UTF-8";
        codec = QTextCodec::codecForName ("UTF-8");
    }

    QString text;
    text = codec->toUnicode (data);
    emit completed (text, fname_, charset_, enforceEncod_, reload_, multiple_);
}

}
