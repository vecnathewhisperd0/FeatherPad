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

    bool getTranslucencyWorkaround() const {
        return translucencyWorkaround_;
    }
    void setTranslucencyWorkaround (bool workaround) {
        translucencyWorkaround_ = workaround;
    }

private:
    bool remSize_, sysIcon_, noToolbar_, hideSearchbar_, showStatusbar_, remFont_, wrapByDefault_,
         indentByDefault_, lineByDefault_, syntaxByDefault_, isMaxed_, isFull_, darkColScheme_,
         scrollJumpWorkaround_, // Should a workaround for Qt5's "scroll jump" bug be applied?
         translucencyWorkaround_; // Should a workaround for the translucency bug of Qt-5.7 be applied?
    int maxSHSize_;
    QSize winSize_, startSize_;
    QFont font_;
};

}

#endif // CONFIG_H
