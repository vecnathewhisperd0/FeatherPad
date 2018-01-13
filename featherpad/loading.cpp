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
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 */

#include "loading.h"
#include "encoding.h"
#include <QFile>
#include <QTextCodec>

namespace FeatherPad {

Loading::Loading (QString fname, QString charset, bool reload, bool saveCursor, bool multiple) :
    fname_ (fname),
    charset_ (charset),
    reload_ (reload),
    saveCursor_ (saveCursor),
    multiple_ (multiple)
{}
/*************************/
Loading::~Loading() {}
/*************************/
void Loading::run()
{
    if (!QFile::exists (fname_))
    {
        emit completed (QString(), QString(), QString(), false, false, false, false);
        return;
    }

    QFile file (fname_);
    if (file.size() > 500*1024*1024) // don't open files with sizes > 500 Mib
    {
        emit completed (QString(), fname_, QString(), false, false, false, false);
        return;
    }
    if (!file.open (QFile::ReadOnly))
    {
        emit completed (QString(), QString(), QString(), false, false, false, false);
        return;
    }

    /* read the file character by character to know
       if it includes null (and because that's faster) */
    bool enforced = !charset_.isEmpty();
    bool hasNull = false;
    QByteArray data;
    char c;
    qint64 charSize = sizeof (char); // 1
    if (enforced)
    { // no need to check for the null character here
        while (file.read (&c, charSize) > 0)
            data.append (c);
    }
    else
    {
        unsigned char C[4];
        int num = 0;
        /* checking 4 bytes is enough to guess
           whether the encoding is UTF-16 or UTF-32 */
        while (num < 4 && file.read (&c, charSize) > 0)
        {
            data.append (c);
            if (c == '\0')
                hasNull = true;
            C[num] = c;
            ++ num;
        }
        if (num == 2 && ((C[0] != '\0' && C[1] == '\0') || (C[0] == '\0' && C[1] != '\0')))
            charset_ = "UTF-16"; // single character
        else if (num == 4)
        {
            if (hasNull)
            {
                if ((C[0] == 0xFF && C[1] == 0xFE && C[2] != '\0' && C[3] == '\0') // le
                    || (C[0] == 0xFE && C[1] == 0xFF && C[2] == '\0' && C[3] != '\0') // be
                    || (C[0] != '\0' && C[1] == '\0' && C[2] != '\0' && C[3] == '\0') // le
                    || (C[0] == '\0' && C[1] != '\0' && C[2] == '\0' && C[3] != '\0')) // be
                {
                    charset_ = "UTF-16";
                }
                /*else if ((C[0] == 0xFF && C[1] == 0xFE && C[2] == '\0' && C[3] == '\0')
                          || (C[0] == '\0' && C[1] == '\0' && C[2] == 0xFE && C[3] == 0xFF))*/
                else if ((C[0] != '\0' && C[1] != '\0' && C[2] == '\0' && C[3] == '\0') // le
                         || (C[0] == '\0' && C[1] == '\0' && C[2] != '\0' && C[3] != '\0')) // be
                {
                    charset_ = "UTF-32";
                }
            }
            /* reading may still be possible */
            if (charset_.isEmpty() && !hasNull)
            {
                while (file.read (&c, charSize) > 0)
                {
                    data.append (c);
                    if (c == '\0' && !hasNull)
                        hasNull = true;
                }
            }
            else
            { // the meaning of null characters was determined before
                while (file.read (&c, charSize) > 0)
                    data.append (c);
            }
        }
    }
    file.close();

    if (charset_.isEmpty())
    {
        if (hasNull)
            charset_ = "UTF-8"; // always open non-text files as UTF-8
        else
            charset_ = detectCharset (data);
    }

    QTextCodec *codec = QTextCodec::codecForName (charset_.toUtf8()); // or charset.toStdString().c_str()
    if (!codec) // prevent any chance of crash if there's a bug
    {
        charset_ = "UTF-8";
        codec = QTextCodec::codecForName ("UTF-8");
    }

    QString text = codec->toUnicode (data);
    emit completed (text, fname_, charset_, enforced, reload_, saveCursor_, multiple_);
}

}
