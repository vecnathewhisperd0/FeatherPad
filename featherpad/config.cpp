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

#include "config.h"
#include <QSettings>

namespace FeatherPad {

Config::Config():
    remSize_ (true),
    sysIcon_ (false),
    noToolbar_ (false),
    hideSearchbar_ (false),
    showStatusbar_ (false),
    remFont_ (true),
    wrapByDefault_ (true),
    indentByDefault_ (true),
    lineByDefault_ (false),
    syntaxByDefault_ (true),
    isMaxed_ (false),
    isFull_ (false),
    darkColScheme_ (false),
    scrollJumpWorkaround_ (false),
    translucencyWorkaround_ (false),
    maxSHSize_ (1),
    winSize_ (QSize (700, 500)),
    startSize_ (QSize (700, 500)),
    font_ (QFont ("Monospace", 9)) {}
/*************************/
Config::~Config() {}
/*************************/
void Config::readConfig()
{
    QSettings settings ("featherpad", "fp");

    /**************
     *** Window ***
     **************/

    settings.beginGroup ("window");

    if (settings.value ("size") == "none")
        remSize_ = false; // true by default
    else
    {
        winSize_ = settings.value ("size", QSize(700, 500)).toSize();
        isMaxed_ = settings.value ("max", false).toBool();
        isFull_ = settings.value ("fullscreen", false).toBool();
    }
    startSize_ = settings.value ("startSize", QSize(700, 500)).toSize();

    if (settings.value ("sysIcon").toBool())
        sysIcon_ = true; // false by default

    if (settings.value ("noToolbar").toBool())
        noToolbar_ = true; // false by default

    if (settings.value ("hideSearchbar").toBool())
        hideSearchbar_ = true; // false by default

    if (settings.value ("showStatusbar").toBool())
        showStatusbar_ = true; // false by default

    if (settings.value ("translucencyWorkaround").toBool())
        translucencyWorkaround_ = true; // false by default

    settings.endGroup();

    /************
     *** Text ***
     ************/

    settings.beginGroup ("text");

    if (settings.value ("font") == "none")
        remFont_ = false; // true by default
    else
        font_.fromString ((settings.value ("font", "Monospace,9,-1,5,50,0,0,0,0,0")).toString());

    if (settings.value ("noWrap").toBool())
        wrapByDefault_ = false; // true by default

    if (settings.value ("noIndent").toBool())
        indentByDefault_ = false; // true by default

    if (settings.value ("lineNumbers").toBool())
        lineByDefault_ = true; // false by default

    if (settings.value ("noSyntaxHighlighting").toBool())
        syntaxByDefault_ = false; // true by default

    if (settings.value ("darkColorScheme").toBool())
        darkColScheme_ = true; // false by default

    if (settings.value ("scrollJumpWorkaround").toBool())
        scrollJumpWorkaround_ = true; // false by default

    maxSHSize_ = qBound(1, settings.value ("maxSHSize", 1).toInt(), 10);

    settings.endGroup();
}
/*************************/
// Prevent redundant writings! (QSettings should have done that.)
void Config::writeConfig()
{
    QSettings settings ("featherpad", "fp");
    if (!settings.isWritable()) return;

    /**************
     *** Window ***
     **************/

    settings.beginGroup ("window");

    if (remSize_)
    {
        if (settings.value ("size").toSize() != winSize_)
            settings.setValue ("size", winSize_);
        if (settings.value ("max").toBool() != isMaxed_)
            settings.setValue ("max", isMaxed_);
        if (settings.value ("fullscreen").toBool() != isFull_)
            settings.setValue ("fullscreen", isFull_);
    }
    else if (settings.value ("size").toString() != "none")
    {
        settings.setValue ("size", "none");
        settings.remove ("max");
        settings.remove ("fullscreen");
    }

    if (settings.value ("startSize").toSize() != startSize_)
        settings.setValue ("startSize", startSize_);

    if (settings.value ("sysIcon").toBool() != sysIcon_)
        settings.setValue ("sysIcon", sysIcon_);

    if (settings.value ("noToolbar").toBool() != noToolbar_)
        settings.setValue ("noToolbar", noToolbar_);

    if (settings.value ("hideSearchbar").toBool() != hideSearchbar_)
        settings.setValue ("hideSearchbar", hideSearchbar_);

    if (settings.value ("showStatusbar").toBool() != showStatusbar_)
        settings.setValue ("showStatusbar", showStatusbar_);

    if (settings.value ("translucencyWorkaround").toBool() != translucencyWorkaround_)
        settings.setValue ("translucencyWorkaround", translucencyWorkaround_);

    settings.endGroup();

    /************
     *** Text ***
     ************/

    settings.beginGroup ("text");

    if (remFont_)
    {
        if (settings.value ("font").toString() != font_.toString())
            settings.setValue ("font", font_.toString());
    }
    else if (settings.value ("font").toString() != "none")
        settings.setValue ("font", "none");

    if (settings.value ("noWrap").toBool() != !wrapByDefault_)
        settings.setValue ("noWrap", !wrapByDefault_);

    if (settings.value ("noIndent").toBool() != !indentByDefault_)
        settings.setValue ("noIndent", !indentByDefault_);

    if (settings.value ("lineNumbers").toBool() != lineByDefault_)
        settings.setValue ("lineNumbers", lineByDefault_);

    if (settings.value ("noSyntaxHighlighting").toBool() != !syntaxByDefault_)
        settings.setValue ("noSyntaxHighlighting", !syntaxByDefault_);

    if (settings.value ("darkColorScheme").toBool() != darkColScheme_)
        settings.setValue ("darkColorScheme", darkColScheme_);

    if (settings.value ("scrollJumpWorkaround").toBool() != scrollJumpWorkaround_)
        settings.setValue ("scrollJumpWorkaround", scrollJumpWorkaround_);

    if (settings.value ("maxSHSize").toInt() != maxSHSize_)
        settings.setValue ("maxSHSize", maxSHSize_);

    settings.endGroup();
}

}
