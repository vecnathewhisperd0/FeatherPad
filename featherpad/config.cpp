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
#include <QFileInfo>

namespace FeatherPad {

Config::Config():
    remSize_ (true),
    sysIcon_ (false),
    noToolbar_ (false),
    noMenubar_ (false),
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
    tabWrapAround_ (false),
    hideSingleTab_ (false),
    executeScripts_ (false),
    scrollJumpWorkaround_ (false),
    tabPosition_ (0),
    maxSHSize_ (2),
    lightBgColorValue_ (255),
    darkBgColorValue_ (15),
    winSize_ (QSize (700, 500)),
    startSize_ (QSize (700, 500)),
    font_ (QFont ("Monospace", 9)),
    openRecentFiles_ (0){}
/*************************/
Config::~Config() {}
/*************************/
void Config::readConfig()
{
    Settings settings ("featherpad", "fp");

    /**************
     *** Window ***
     **************/

    settings.beginGroup ("window");

    if (settings.value ("size") == "none")
        remSize_ = false; // true by default
    else
    {
        winSize_ = settings.value ("size", QSize(700, 500)).toSize();
        if (!winSize_.isValid() || winSize_.isNull())
            winSize_ = QSize (700, 500);
        isMaxed_ = settings.value ("max", false).toBool();
        isFull_ = settings.value ("fullscreen", false).toBool();
    }
    startSize_ = settings.value ("startSize", QSize(700, 500)).toSize();
    if (!startSize_.isValid() || startSize_.isNull())
        startSize_ = QSize (700, 500);

    if (settings.value ("sysIcon").toBool())
        sysIcon_ = true; // false by default

    if (settings.value ("noToolbar").toBool())
        noToolbar_ = true; // false by default

    if (settings.value ("noMenubar").toBool())
        noMenubar_ = true; // false by default

    if (noToolbar_ && noMenubar_)
    { // we don't want to hide all actions
        noToolbar_ = false;
        noMenubar_ = true;
    }

    if (settings.value ("hideSearchbar").toBool())
        hideSearchbar_ = true; // false by default

    if (settings.value ("showStatusbar").toBool())
        showStatusbar_ = true; // false by default

    int pos = settings.value ("tabPosition").toInt();
    if (pos > 0 && pos <= 3)
        tabPosition_ = pos; // 0 by default

    if (settings.value ("tabWrapAround").toBool())
        tabWrapAround_ = true; // false by default

    if (settings.value ("hideSingleTab").toBool())
        hideSingleTab_ = true; // false by default

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

    maxSHSize_ = qBound (1, settings.value ("maxSHSize", 2).toInt(), 10);

    /* don't let the dark bg be darker than #e6e6e6 */
    lightBgColorValue_ = qBound (230, settings.value ("lightBgColorValue", 255).toInt(), 255);

    /* don't let the dark bg be lighter than #323232 */
    darkBgColorValue_ = qBound (0, settings.value ("darkBgColorValue", 15).toInt(), 50);

    if (settings.value ("executeScripts").toBool())
        executeScripts_ = true; // false by default
    executeCommand_ = settings.value ("executeCommand").toString();

    recentFiles_ = settings.value ("recentFiles").toStringList();
    recentFiles_.removeAll ("");
    recentFiles_.removeDuplicates();
    while (recentFiles_.count() > 10)
        recentFiles_.removeLast();

    openRecentFiles_ = qBound (0, settings.value ("openRecentFiles", 0).toInt(), 10);

    settings.endGroup();
}
/*************************/
void Config::writeConfig()
{
    Settings settings ("featherpad", "fp");
    if (!settings.isWritable()) return;

    /**************
     *** Window ***
     **************/

    settings.beginGroup ("window");

    if (remSize_)
    {
        settings.setValue ("size", winSize_);
        settings.setValue ("max", isMaxed_);
        settings.setValue ("fullscreen", isFull_);
    }
    else
    {
        settings.setValue ("size", "none");
        settings.remove ("max");
        settings.remove ("fullscreen");
    }

    settings.setValue ("startSize", startSize_);
    settings.setValue ("sysIcon", sysIcon_);
    settings.setValue ("noToolbar", noToolbar_);
    settings.setValue ("noMenubar", noMenubar_);
    settings.setValue ("hideSearchbar", hideSearchbar_);
    settings.setValue ("showStatusbar", showStatusbar_);
    settings.setValue ("tabPosition", tabPosition_);
    settings.setValue ("tabWrapAround", tabWrapAround_);
    settings.setValue ("hideSingleTab", hideSingleTab_);

    settings.endGroup();

    /************
     *** Text ***
     ************/

    settings.beginGroup ("text");

    if (remFont_)
        settings.setValue ("font", font_.toString());
    else
        settings.setValue ("font", "none");

    settings.setValue ("noWrap", !wrapByDefault_);
    settings.setValue ("noIndent", !indentByDefault_);
    settings.setValue ("lineNumbers", lineByDefault_);
    settings.setValue ("noSyntaxHighlighting", !syntaxByDefault_);
    settings.setValue ("darkColorScheme", darkColScheme_);
    settings.setValue ("scrollJumpWorkaround", scrollJumpWorkaround_);
    settings.setValue ("maxSHSize", maxSHSize_);
    settings.setValue ("lightBgColorValue", lightBgColorValue_);
    settings.setValue ("darkBgColorValue", darkBgColorValue_);
    settings.setValue ("executeScripts", executeScripts_);
    settings.setValue ("executeCommand", executeCommand_);
    settings.setValue ("recentFiles", recentFiles_);
    settings.setValue ("openRecentFiles", openRecentFiles_);

    settings.endGroup();
}
/*************************/
void Config::addRecentFile (QString file)
{
    recentFiles_.removeAll (file);
    recentFiles_.prepend (file);
    while (recentFiles_.count() > 10)
        recentFiles_.removeLast();
}
/*************************/
QStringList Config::getLastSavedFiles() const
{
    QStringList res;
    int n = qMin (openRecentFiles_, recentFiles_.count());
    for (int i = 0; i < n; ++i)
        res << recentFiles_.at (i);
    return res;
}

}
