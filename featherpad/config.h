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

#ifndef CONFIG_H
#define CONFIG_H

#include <QSettings>
#include <QSize>
#include <QFont>

namespace FeatherPad {

// Prevent redundant writings! (Why does QSettings write to the config file when no setting is changed?)
class Settings : public QSettings
{
    Q_OBJECT
public:
    Settings (const QString &organization, const QString &application = QString(), QObject *parent = nullptr)
             : QSettings (organization, application, parent) {}

    void setValue (const QString &key, const QVariant &v) {
        if (value (key) == v)
            return;
        QSettings::setValue (key, v);
    }
};

class Config {
public:
    Config();
    ~Config();

    void readConfig();
    void writeConfig();

    bool getRemSize() const {
        return remSize_;
    }
    void setRemSize (bool rem) {
        remSize_ = rem;
    }

    bool getSysIcon() const {
        return sysIcon_;
    }
    void setSysIcon (bool own) {
        sysIcon_ = own;
    }

    bool getIsMaxed() const {
        return isMaxed_;
    }
    void setIsMaxed (bool isMaxed) {
        isMaxed_ = isMaxed;
    }

    bool getIsFull() const {
        return isFull_;
    }
    void setIsFull (bool isFull) {
        isFull_ = isFull;
    }

    bool getDarkColScheme() const {
        return darkColScheme_;
    }
    void setDarkColScheme (bool dark) {
        darkColScheme_ = dark;
    }

    int getLightBgColorValue() const {
        return lightBgColorValue_;
    }
    void setLightBgColorValue (int lightness) {
        lightBgColorValue_ = lightness;
    }

    int getDarkBgColorValue() const {
        return darkBgColorValue_;
    }
    void setDarkBgColorValue (int darkness) {
        darkBgColorValue_ = darkness;
    }

    int getRecentFilesNumber() const {
        return recentFilesNumber_;
    }
    void setRecentFilesNumber (int number) {
        recentFilesNumber_ = number;
    }
    int getCurRecentFilesNumber() const {
        return curRecentFilesNumber_;
    }

    bool getTabWrapAround() const {
        return tabWrapAround_;
    }
    void setTabWrapAround (bool wrap) {
        tabWrapAround_ = wrap;
    }

    bool getHideSingleTab() const {
        return hideSingleTab_;
    }
    void setHideSingleTab (bool hide) {
        hideSingleTab_ = hide;
    }

    QSize getWinSize() const {
        return winSize_;
    }
    void setWinSize (QSize s) {
        winSize_ = s;
    }

    QSize getStartSize() const {
        return startSize_;
    }
    void setStartSize (QSize s) {
        startSize_ = s;
    }

    bool getNoToolbar() const {
        return noToolbar_;
    }
    void setNoToolbar (bool noTB) {
        noToolbar_ = noTB;
    }

    bool getNoMenubar() const {
        return noMenubar_;
    }
    void setNoMenubar (bool noMB) {
        noMenubar_ = noMB;
    }

    bool getHideSearchbar() const {
        return hideSearchbar_;
    }
    void setHideSearchbar (bool hide) {
        hideSearchbar_ = hide;
    }

    bool getShowStatusbar() const {
        return showStatusbar_;
    }
    void setShowStatusbar (bool show) {
        showStatusbar_ = show;
    }

    int getTabPosition() const {
        return tabPosition_;
    }
    void setTabPosition (int pos) {
        tabPosition_ = pos;
    }

    QFont getFont() const {
        return font_;
    }
    void setFont (QFont font) {
        font_ = font;
    }

    bool getRemFont() const {
        return remFont_;
    }
    void setRemFont (bool rem) {
        remFont_ = rem;
    }

    bool getWrapByDefault() const {
        return wrapByDefault_;
    }
    void setWrapByDefault (bool wrap) {
        wrapByDefault_ = wrap;
    }

    bool getIndentByDefault() const {
        return indentByDefault_;
    }
    void setIndentByDefault (bool indent) {
        indentByDefault_ = indent;
    }

    bool getLineByDefault() const {
        return lineByDefault_;
    }
    void setLineByDefault (bool line) {
        lineByDefault_ = line;
    }

    bool getSyntaxByDefault() const {
        return syntaxByDefault_;
    }
    void setSyntaxByDefault (bool syntax) {
        syntaxByDefault_ = syntax;
    }

    int getMaxSHSize() const {
        return maxSHSize_;
    }
    void setMaxSHSize (int max) {
        maxSHSize_ = max;
    }

    bool getScrollJumpWorkaround() const {
        return scrollJumpWorkaround_;
    }
    void setScrollJumpWorkaround (bool workaround) {
        scrollJumpWorkaround_ = workaround;
    }

    bool getExecuteScripts() const {
        return executeScripts_;
    }
    void setExecuteScripts (bool excute) {
        executeScripts_ = excute;
    }
    QString getExecuteCommand() const {
        return executeCommand_;
    }
    void setExecuteCommand (QString commnad) {
        executeCommand_ = commnad;
    }

    int getOpenRecentFiles_() const {
        return openRecentFiles_;
    }
    void setOpenRecentFiles_ (int number) {
        openRecentFiles_ = qBound (0, number, 20);
    }

    QStringList getLastSavedFiles() const;

    QStringList getRecentFiles() const {
        return recentFiles_;
    }
    void clearRecentFiles() {
        recentFiles_ = QStringList();
    }
    void addRecentFile (QString file);

private:
    bool remSize_, sysIcon_, noToolbar_, noMenubar_, hideSearchbar_, showStatusbar_, remFont_, wrapByDefault_,
         indentByDefault_, lineByDefault_, syntaxByDefault_, isMaxed_, isFull_, darkColScheme_,
         tabWrapAround_, hideSingleTab_, executeScripts_,
         scrollJumpWorkaround_; // Should a workaround for Qt5's "scroll jump" bug be applied?
    int tabPosition_, maxSHSize_, lightBgColorValue_, darkBgColorValue_, recentFilesNumber_;
    int curRecentFilesNumber_; // the start value of recentFilesNumber_ -- fixed during a session
    QSize winSize_, startSize_;
    QFont font_;
    QString executeCommand_;
    int openRecentFiles_;
    QStringList recentFiles_;
};

}

#endif // CONFIG_H
