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

Loading::Loading (QString fname, QString charset, bool reload, bool multiple) :
    fname_ (fname),
    charset_ (charset),
    reload_ (reload),
    multiple_ (multiple)
{}
/*************************/
Loading::~Loading() {}
/*************************/
void Loading::run()
{
    if (!QFile::exists (fname_))
    {
        emit completed (QString(), QString(), QString(), false, false, false);
        return;
    }

    QFile file (fname_);
    if (!file.open (QFile::ReadOnly))
    {
        emit completed (QString(), QString(), QString(), false, false, false);
        return;
    }

    /* read the file character by character
       in order to determine if it's in UTF-16 */
    QByteArray data;
    char c;
    qint64 charSize = sizeof (char);
    bool enforced = !charset_.isEmpty();
    bool wasNull = false;
    int num = 0;
    while (file.read (&c, charSize) > 0)
    {
        data.append (c);
        /* checking 4 bytes is enough to know
           whether the encoding is UTF-16 or UTF-32 */
        if (!enforced && num < 4)
        {
            ++ num;
            if (c == '\0')
            {
                if (wasNull)
                    charset_ = "UTF-32";
                else
                    charset_ = "UTF-16";
                wasNull = true;
            }
            else
                wasNull = false;
        }
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

    QString text = codec->toUnicode (data);
    emit completed (text, fname_, charset_, enforced, reload_, multiple_);
}

}
