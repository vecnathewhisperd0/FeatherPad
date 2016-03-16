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

#include "singleton.h"
#include "ui_fp.h"

namespace FeatherPad {

static QString getMimeType (const QString &fname)
{
    QMimeDatabase mimeDatabase;
    QMimeType mimeType = mimeDatabase.mimeTypeForFile (QFileInfo (fname));
    QString result ("application/octet-stream");
    result = mimeType.name();

    //qDebug() << "Mime type: " + result;

    return result;
}
/*************************/
void FPwin::toggleSyntaxHighlighting()
{
    int count = ui->tabWidget->count();
    if (count == 0) return;

    Highlighter *highlighter = NULL;
    if (ui->actionSyntax->isChecked())
    {
        for (int i = 0; i < count; ++i)
            syntaxHighlighting (i);
    }
    else
    {
        QHash<TextEdit*,tabInfo*>::iterator it;
        for (it = tabsInfo_.begin(); it != tabsInfo_.end(); ++it)
        {
            tabInfo *tabinfo = tabsInfo_[it.key()];
            highlighter = tabinfo->highlighter;
            tabinfo->highlighter = NULL;
            delete highlighter; highlighter = NULL;
        }
    }
}
/*************************/
void FPwin::getSyntax (const int index)
{
    if (index == -1) return;

    tabInfo *tabinfo = tabsInfo_[qobject_cast< TextEdit *>(ui->tabWidget->widget (index))];
    QString fname = tabinfo->fileName;
    if (fname.isEmpty()) return;

    if (fname.endsWith (".sub"))
    {
        return;
    }

    QString progLan;

    /* first check some endings */
    QRegExp rx ("*.*");
    rx.setPatternSyntax (QRegExp::Wildcard);
    QString baseName = fname.section ('/', -1);
    if (rx.exactMatch (baseName))
    {
        if (fname.endsWith (".cpp") || fname.endsWith (".h"))
            progLan = "cpp";
        else if (fname.endsWith (".c"))
            progLan = "c";
        else if (fname.endsWith (".sh"))
            progLan = "sh";
        else if (fname.endsWith (".rb"))
            progLan = "ruby";
        else if (fname.endsWith (".lua"))
            progLan = "lua";
        else if (fname.endsWith (".py"))
            progLan = "python";
        else if (fname.endsWith (".pl"))
            progLan = "perl";
        else if (fname.endsWith (".pro"))
            progLan = "qmake";
        else if (fname.endsWith (".tr") || fname.endsWith (".t") || fname.endsWith (".roff"))
            progLan = "troff";
        else if (fname.endsWith (".xml") || fname.endsWith (".svg") || fname.endsWith (".qrc"))
            progLan = "xml";
        else if (fname.endsWith (".css"))
            progLan = "css";
        else if (fname.endsWith (".desktop") || fname.endsWith (".desktop.in") || fname.endsWith (".directory"))
             progLan = "desktop";
        else if (fname.endsWith (".js"))
            progLan = "javascript";
        else if (fname.endsWith (".log", Qt::CaseInsensitive))
            progLan = "log";
        else if (fname.endsWith (".php"))
            progLan = "php";
        else if (fname.endsWith (".url"))
             progLan = "url";
        else if (fname.endsWith (".diff") || fname.endsWith (".patch"))
            progLan = "diff";
        else if (fname.endsWith (".srt"))
            progLan = "srt";
        else if (fname.endsWith (".theme"))
             progLan = "theme";
        else if (fname.endsWith (".rc"))
             progLan = "gtkrc";
        else if (fname.endsWith (".htm", Qt::CaseInsensitive) || fname.endsWith (".html", Qt::CaseInsensitive))
            progLan = "html";
        else if (baseName == "sources.list" || baseName == "sources.list.save")
            progLan = "sourceslist";
        else if (baseName.startsWith ("makefile.", Qt::CaseInsensitive) && !baseName.endsWith (".txt"))
            progLan = "makefile";
        else if (baseName.compare ("CMakeLists.txt", Qt::CaseInsensitive) == 0)
            progLan = "cmake";
    }
    /* makefile is an exception */
    else if (baseName.compare ("makefile", Qt::CaseInsensitive) == 0)
        progLan = "makefile";
    else if (baseName.compare ("changelog", Qt::CaseInsensitive) == 0)
        progLan = "changelog";
    else if (baseName.compare ("gtkrc") == 0)
        progLan = "gtkrc";

    if (progLan.isEmpty()) // now, check mime types
    {
        QString mime;
        if (!QFileInfo (fname).isSymLink())
            mime = getMimeType (fname);
        else
            mime = getMimeType (QFileInfo (fname).symLinkTarget());

        if (mime == "text/x-c++")
            progLan = "cpp";
        else if (mime == "text/x-c")
            progLan = "c";
        else if (mime == "application/x-shellscript" || mime == "text/x-shellscript")
            progLan = "sh";
        else if (mime == "application/x-ruby")
            progLan = "ruby";
        else if (mime == "text/x-lua")
            progLan = "lua";
        else if (mime == "text/x-python")
            progLan = "python";
        else if (mime == "application/x-perl")
            progLan = "perl";
        else if (mime == "text/x-makefile")
            progLan = "makefile";
        else if (mime == "text/x-cmake")
            progLan = "cmake";
        else if (mime == "application/vnd.nokia.qt.qmakeprofile")
            progLan = "qmake";
        else if (mime == "text/troff")
            progLan = "troff";
        else if (mime == "application/xml" || mime == "image/svg+xml")
            progLan = "xml";
        else if (mime == "text/css")
            progLan = "xml";
        else if (mime == "application/x-designer")
            progLan = "xml";
        else if (mime == "text/x-changelog")
            progLan = "changelog";
        else if (mime == "application/x-desktop")
            progLan = "desktop";
        else if (mime == "application/javascript")
            progLan = "javascript";
        else if (mime == "text/x-log")
            progLan = "log";
        else if (mime == "application/x-php" || mime == "text/x-php")
            progLan = "php";
        else if (mime == "application/x-mswinurl")
            progLan = "url";
        else if (mime == "application/x-theme")
            progLan = "theme";
        else if (mime == "text/x-diff" || mime == "text/x-patch")
            progLan = "diff";
        else if (mime == "text/html")
            progLan = "html";
        /* start checking contents */
        /*else
        {
            QString firstBlock = textEdit->document()->firstBlock().text();
            if (firstBlock.startsWith ("<html>")
                || firstBlock.startsWith ("<!DOCTYPE html"))
            {
                highlighter = new Highlighter (textEdit->document(), "html");
            }
        }*/
    }

    if (!progLan.isEmpty())
        tabinfo->prog = progLan;
}
/*************************/
void FPwin::syntaxHighlighting (const int index)
{
    if (index == -1) return;

    TextEdit *textEdit = qobject_cast< TextEdit *>(ui->tabWidget->widget (index));
    tabInfo *tabinfo = tabsInfo_[textEdit];

    if (tabinfo->size > static_cast<FPsingleton*>(qApp)->getConfig().getMaxSHSize()*1024*1024)
        return;

    QString progLan = tabinfo->prog;
    if (!progLan.isEmpty())
    {
        Highlighter *highlighter = new Highlighter (textEdit->document(), progLan);
        tabinfo->highlighter = highlighter;
    }
}

}
