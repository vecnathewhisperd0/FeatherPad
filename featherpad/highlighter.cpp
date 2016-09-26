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

#include "highlighter.h"

namespace FeatherPad {

TextBlockData::~TextBlockData()
{
    while (!allParentheses.isEmpty())
    {
        ParenthesisInfo *info = allParentheses.at (0);
        allParentheses.remove(0);
        delete info;
    }
    while (!allBraces.isEmpty())
    {
        BraceInfo *info = allBraces.at (0);
        allBraces.remove(0);
        delete info;
    }
}
/*************************/
QVector<ParenthesisInfo *> TextBlockData::parentheses()
{
    return allParentheses;
}
/*************************/
QVector<BraceInfo *> TextBlockData::braces()
{
    return allBraces;
}
/*************************/
QString TextBlockData::delimiter()
{
    return Delimiter;
}
/*************************/
void TextBlockData::insertInfo (ParenthesisInfo *info)
{
    int i = 0;
    while (i < allParentheses.size()
           && info->position > allParentheses.at(i)->position)
    {
        ++i;
    }

    allParentheses.insert (i, info);
}
/*************************/
void TextBlockData::insertInfo (BraceInfo *info)
{
    int i = 0;
    while (i < allBraces.size()
           && info->position > allBraces.at(i)->position)
    {
        ++i;
    }

    allBraces.insert (i, info);
}
/*************************/
void TextBlockData::insertInfo (QString str)
{
    Delimiter = str;
}
/*************************/
// Here, the order of formatting is important because of overrides.
Highlighter::Highlighter (QTextDocument *parent, QString lang, QTextCursor start, QTextCursor end, bool darkColorScheme) : QSyntaxHighlighter (parent)
{
    if (lang.isEmpty()) return;

    startCursor = start;
    endCursor = end;
    progLan = lang;

    HighlightingRule rule;
    if (!darkColorScheme)
    {
        Blue = Qt::blue;
        DarkBlue = Qt::darkBlue;
        Red = Qt::red;
        DarkRed = QColor (150, 0, 0);
        DarkGreen = Qt::darkGreen;
        DarkMagenta = Qt::darkMagenta;
        Violet = QColor (126, 0, 230); //d556e6
        Brown = QColor (160, 80, 0);
    }
    else
    {
        Blue = QColor (85, 227, 255);
        DarkBlue = QColor (65, 154, 255);
        Red = QColor (255, 120, 120);
        DarkRed = QColor (255, 0, 0);
        DarkGreen = Qt::green;
        DarkMagenta = QColor (255, 153, 255);
        Violet = QColor (255, 255, 0);
        Brown = QColor (255, 200, 0);
    }

    /*************
     * Functions *
     *************/

    /* there may be javascript inside html */
    if (lang == "html")
        lang = "javascript";

    /* may be overridden by the keywords format */
    if (progLan == "c" || progLan == "cpp"
        || progLan == "lua" || progLan == "python"
        || lang == "javascript" || lang == "qml" || progLan == "php")
    {
        QTextCharFormat functionFormat;
        functionFormat.setFontItalic (true);
        functionFormat.setForeground (Blue);
        /* before parentheses */
        rule.pattern = QRegExp ("\\b[A-Za-z0-9_]+(?=\\s*\\()");
        rule.format = functionFormat;
        highlightingRules.append (rule);
    }

    /**********************
     * Keywords and Types *
     **********************/

    /* keywords */
    QTextCharFormat keywordFormat;
    keywordFormat.setFontWeight (QFont::Bold);
    /* bash extra keywords */
    if (progLan == "sh" || progLan == "makefile" || progLan == "cmake")
    {
        keywordFormat.setForeground (Qt::magenta);
        rule.pattern = QRegExp ("((^\\s*|[\\(\\);&`\\|]+\\s*)((if|then|elif|else|fi|while|do|done|esac)\\s+)*)(adduser|addgroup|apropos|apt-get|aspell|awk|basename|bash|bc|bzip2|cal|cat|cd|cfdisk|chgrp|chmod|chown|chroot|chkconfig|cksum|clear|cmp|comm|cp|cron|crontab|csplit|cut|date|dc|dd|ddrescue|df|diff|diff3|dig|dir|dircolors|dirname|dirs|dmesg|dpkg|du|egrep|eject|env|ethtool|expect|expand|expr|fdformat|fdisk|fgrep|file|find|fmt|fold|format|free|fsck|ftp|function|fuser|gawk|git|grep|groups|gzip|head|hostname|id|ifconfig|ifdown|ifup|import|install|join|kdialog|kill|killall|less|ln|locate|logname|look|lpc|lpr|lprint|lprintd|lprintq|lprm|ls|lsof|make|man|mkdir|mkfifo|mkisofs|mknod|more|mount|mtools|mv|mmv|netstat|nice|nl|nohup|nslookup|open|op|passwd|paste|pathchk|ping|pkill|popd|pr|printcap|printenv|ps|pwd|qarma|qmake(-qt[3-9])*|quota|quotacheck|quotactl|ram|rcp|readarray|reboot|rename|renice|remsync|rev|rm|rmdir|rsync|screen|scp|sdiff|sed|seq|sftp|shutdown|sleep|slocate|sort|split|ssh|strace|su|sudo|sum|symlink|sync|tail|tar|tee|time|touch|top|traceroute|tr|tsort|tty|type|ulimit|umount|uname|unexpand|uniq|units|unshar|useradd|usermod|users|uuencode|uudecode|vdir|vi|vmstat|watch|wc|whereis|which|who|whoami|Wget|write|xargs|yad|yes|zenity)\\b");
        rule.format = keywordFormat;
        highlightingRules.append (rule);
    }
    keywordFormat.setForeground (DarkBlue);

    /* types */
    QTextCharFormat typeFormat;
    typeFormat.setForeground (DarkMagenta); //(QColor (102, 0, 0));

    QStringList patterns;

    patterns = keywords (lang);
    foreach (const QString &pattern, patterns)
    {
        rule.pattern = QRegExp (pattern);
        rule.format = keywordFormat;
        highlightingRules.append (rule);
    }

    /* qmake paths */
    if (progLan == "qmake")
    {
        QTextCharFormat qmpathFormat;
        qmpathFormat.setForeground (Blue);
        rule.pattern = QRegExp ("\\${1,2}([A-Za-z0-9_]+|\\[[A-Za-z0-9_]+\\]|\\([A-Za-z0-9_]+\\))");
        rule.format = qmpathFormat;
        highlightingRules.append (rule);
    }

    patterns = types();
    foreach (const QString &pattern, patterns)
    {
        rule.pattern = QRegExp (pattern);
        rule.format = typeFormat;
        highlightingRules.append (rule);
    }

    /***********
     * Details *
     ***********/

    /* these are used for all comments */
    commentFormat.setForeground (Red);
    commentFormat.setFontItalic (true);

    /* these can also be used inside multiline comments */
    urlFormat.setFontUnderline (true);
    urlFormat.setForeground (Blue);

    if (progLan == "c" || progLan == "cpp")
    {
        QTextCharFormat cFormat;

        /* Qt and Gtk+ specific classes */
        cFormat.setFontWeight (QFont::Bold);
        cFormat.setForeground (DarkMagenta);
        if (progLan == "cpp")
            rule.pattern = QRegExp ("\\bQ[A-Za-z]+\\b");
        else
            rule.pattern = QRegExp ("\\bG[A-Za-z]+\\b");
        rule.format = cFormat;
        highlightingRules.append (rule);

        /* QtGlobal functions and enum Qt::GlobalColor */
        if (progLan == "cpp")
        {
            cFormat.setFontItalic (true);
            rule.pattern = QRegExp ("\\bq(App|Abs|Bound|Critical|Debug|Fatal|FuzzyCompare|InstallMsgHandler|MacVersion|Max|Min|Round64|Round|Version|Warning|getenv|putenv|rand|srand|tTrId|_check_ptr|t_set_sequence_auto_mnemonic|t_symbian_exception2Error|t_symbian_exception2LeaveL|t_symbian_throwIfError)\\b");
            rule.format = cFormat;
            highlightingRules.append (rule);
            cFormat.setFontItalic (false);

            cFormat.setForeground (Qt::magenta);
            rule.pattern = QRegExp ("\\bQt\\s*::\\s*(white|black|red|darkRed|green|darkGreen|blue|darkBlue|cyan|darkCyan|magenta|darkMagenta|yellow|darkYellow|gray|darkGray|lightGray|transparent|color0|color1)\\b");
            rule.format = cFormat;
            highlightingRules.append (rule);
        }

        /* preprocess */
        cFormat.setForeground (Blue);
        rule.pattern = QRegExp ("^\\s*#\\s*include\\s|^\\s*#\\s*ifdef\\s|^\\s*#\\s*elif\\s|^\\s*#\\s*ifndef\\s|^\\s*#\\s*endif\\b|^\\s*#\\s*define\\s|^\\s*#\\s*undef\\s|^\\s*#\\s*error\\s|^\\s*#\\s*if\\s|^\\s*#\\s*else\\b");
        rule.format = cFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "qml")
    {
        QTextCharFormat qmlFormat;
        qmlFormat.setFontWeight (QFont::Bold);
        qmlFormat.setForeground (DarkMagenta);
        rule.pattern = QRegExp ("\\b(Qt[A-Za-z]+|Accessible|AnchorAnimation|AnchorChanges|AnimatedImage|AnimatedSprite|Animation|AnimationController|Animator|Behavior|BorderImage|Canvas|CanvasGradient|CanvasImageData|CanvasPixelArray|ColorAnimation|Column|Context2D|DoubleValidator|Drag|DragEvent|DropArea|EnterKey|Flickable|Flipable|Flow|FocusScope|FontLoader|FontMetrics|Gradient|GradientStop|Grid|GridMesh|GridView|Image|IntValidator|Item|ItemGrabResult|KeyEvent|KeyNavigation|Keys|LayoutMirroring|ListView|Loader|Matrix4x4|MouseArea|MouseEvent|MultiPointTouchArea|NumberAnimation|OpacityAnimator|OpenGLInfo|ParallelAnimation|ParentAnimation|ParentChange|Path|PathAnimation|PathArc|PathAttribute|PathCubic|PathCurve|PathElement|PathInterpolator|PathLine|PathPercent|PathQuad|PathSvg|PathView|PauseAnimation|PinchArea|PinchEvent|Positioner|PropertyAction|PropertyAnimation|PropertyChanges|Rectangle|RegExpValidator|Repeater|Rotation|RotationAnimation|RotationAnimator|Row|Scale|ScaleAnimator|ScriptAction|SequentialAnimation|ShaderEffect|ShaderEffectSource|Shortcut|SmoothedAnimation|SpringAnimation|Sprite|SpriteSequence|State|StateChangeScript|StateGroup|SystemPalette|Text|TextEdit|TextInput|TextMetrics|TouchPoint|Transform|Transition|Translate|UniformAnimator|Vector3dAnimation|ViewTransition|WheelEvent|XAnimator|YAnimator|CloseEvent|ColorDialog|ColumnLayout|Dialog|FileDialog|FontDialog|GridLayout|Layout|MessageDialog|RowLayout|StackLayout|LocalStorage|Screen|SignalSpy|TestCase|Window|XmlListModel|XmlRole|Action|ApplicationWindow|BusyIndicator|Button|Calendar|CheckBox|ComboBox|ExclusiveGroup|GroupBox|Label|Menu|MenuBar|MenuItem|MenuSeparator|ProgressBar|RadioButton|ScrollView|Slider|SpinBox|SplitView|Stack|StackView|StackViewDelegate|StatusBar|Switch|Tab|TabView|TableView|TableViewColumn|TextArea|TextField|ToolBar|ToolButton|TreeView|Affector|Age|AngleDirection|Attractor|CumulativeDirection|CustomParticle|Direction|EllipseShape|Emitter|Friction|Gravity|GroupGoal|ImageParticle|ItemParticle|LineShape|MaskShape|Particle|ParticleGroup|ParticlePainter|ParticleSystem|PointDirection|RectangleShape|Shape|SpriteGoal|TargetDirection|TrailEmitter|Turbulence|Wander|Timer)\\b");
        rule.format = qmlFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "xml")
    {
        QTextCharFormat xmlElementFormat;
        xmlElementFormat.setFontWeight (QFont::Bold);
        xmlElementFormat.setForeground (Violet);
        /* after </ or before /> */
        rule.pattern = QRegExp ("\\s*</?[A-Za-z0-9_\\-:]+|\\s*<!DOCTYPE\\s|\\s*/?>");
        rule.format = xmlElementFormat;
        highlightingRules.append (rule);

        QTextCharFormat xmlAttributeFormat;
        xmlAttributeFormat.setFontItalic (true);
        xmlAttributeFormat.setForeground (Blue);
        /* before = */
        rule.pattern = QRegExp ("\\b[A-Za-z0-9_\\-:]+(?=\\s*\\=)");
        rule.format = xmlAttributeFormat;
        highlightingRules.append (rule);

        /* <?xml ... ?> */
        rule.pattern = QRegExp ("^\\s*<\\?xml\\s+(?=.*\\?>)|\\s*\\?>");
        rule.format = keywordFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "changelog")
    {
        /* before colon */
        rule.pattern = QRegExp ("^\\s+\\*\\s+[^:]+:");
        rule.format = keywordFormat;
        highlightingRules.append (rule);

        QTextCharFormat asteriskFormat;
        asteriskFormat.setForeground (DarkMagenta);
        /* the first asterisk */
        rule.pattern = QRegExp ("^\\s+\\*\\s+");
        rule.format = asteriskFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "sh" || progLan == "makefile" || progLan == "cmake"
             || progLan == "perl" || progLan == "ruby")
    {
        /* # is the sh comment sign when it doesn't follow a character */
        if (progLan == "sh" || progLan == "makefile" || progLan == "cmake")
            rule.pattern = QRegExp ("^#.*|\\s+#.*");
        else
            rule.pattern = QRegExp ("#.*");
        rule.format = commentFormat;
        highlightingRules.append (rule);

        QTextCharFormat shFormat;

        if (progLan == "sh" || progLan == "makefile" || progLan == "cmake")
        {
            QTextCharFormat neutral;
            neutral.setForeground (QBrush());
            /* make parentheses and ; neutral as they were in keyword patterns */
            rule.pattern = QRegExp ("[\\(\\);]");
            rule.format = neutral;
            highlightingRules.append (rule);

            shFormat.setForeground (Blue);
            /* words before = */
             if (progLan == "sh")
                 rule.pattern = QRegExp ("\\b[A-Za-z0-9_]+(?=\\=)");
             else
                 rule.pattern = QRegExp ("\\b[A-Za-z0-9_\\s]+(?=\\+{,1}\\=)");
            rule.format = shFormat;
            highlightingRules.append (rule);

            /* but don't format a word before =
               if it follows a dash */
            rule.pattern = QRegExp ("-+[^\\s]+(?=\\=)");
            rule.format = neutral;
            highlightingRules.append (rule);
        }

        if (progLan == "makefile" || progLan == "cmake")
        {
            shFormat.setForeground (Qt::darkYellow);
            /* automake/autoconf variables */
            rule.pattern = QRegExp ("@[A-Za-z0-9_-]+@|^[a-zA-Z0-9_-]+\\s*(?=:)");
            rule.format = shFormat;
            highlightingRules.append (rule);
        }

        shFormat.setForeground (DarkMagenta);
        /* operators */
        rule.pattern = QRegExp ("[=\\+\\-*/%<>&`\\|~\\^\\!,]|\\s+-eq\\s+|\\s+-ne\\s+|\\s+-gt\\s+|\\s+-ge\\s+|\\s+-lt\\s+|\\s+-le\\s+|\\s+-z\\s+");
        rule.format = shFormat;
        highlightingRules.append (rule);

        if (progLan == "sh" || progLan == "makefile" || progLan == "cmake")
        {
            shFormat.setFontWeight (QFont::Bold);
            /* brackets */
            rule.pattern = QRegExp ("\\s+\\[\\s+|^\\[\\s+|\\s+\\]\\s+|\\s+\\]$|\\s+\\]\\s*(?=;)");
            rule.format = shFormat;
            highlightingRules.append (rule);
        }
    }
    else if (progLan == "diff")
    {
        QTextCharFormat diffMinusFormat;
        diffMinusFormat.setForeground (Red);
        rule.pattern = QRegExp ("^\\-\\s*.*");
        rule.format = diffMinusFormat;
        highlightingRules.append (rule);

        QTextCharFormat diffPlusFormat;
        diffPlusFormat.setForeground (Blue);
        rule.pattern = QRegExp ("^\\+\\s*.*");
        rule.format = diffPlusFormat;
        highlightingRules.append (rule);

        QTextCharFormat diffLinesFormat;
        diffLinesFormat.setFontWeight (QFont::Bold);
        diffLinesFormat.setForeground (DarkGreen);
        rule.pattern = QRegExp ("^@{2}[0-9,\\-\\+\\s]+@{2}");
        rule.format = diffLinesFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "log")
    {
        /* example:
         * May 19 02:01:44 debian sudo:
         *   blue  green  magenta bold */
        QTextCharFormat logFormat;
        logFormat.setFontWeight (QFont::Bold);
        rule.pattern = QRegExp ("^[A-Za-z]{3}\\s+[0-9]{1,2}\\s{1}[0-9]{2}:[0-9]{2}:[0-9]{2}\\s+[A-Za-z0-9_\\[\\]\\s]+(?=\\s*:)");
        rule.format = logFormat;
        highlightingRules.append (rule);

        QTextCharFormat logFormat1;
        logFormat1.setForeground (Qt::magenta);
        rule.pattern = QRegExp ("^[A-Za-z]{3}\\s+[0-9]{1,2}\\s{1}[0-9]{2}:[0-9]{2}:[0-9]{2}\\s+[A-Za-z]+");
        rule.format = logFormat1;
        highlightingRules.append (rule);

        QTextCharFormat logDateFormat;
        logDateFormat.setFontWeight (QFont::Bold);
        logDateFormat.setForeground (Blue);
        rule.pattern = QRegExp ("^[A-Za-z]{3}\\s+[0-9]{1,2}(?=\\s{1}[0-9]{2}:[0-9]{2}:[0-9]{2})");
        rule.format = logDateFormat;
        highlightingRules.append (rule);

        QTextCharFormat logTimeFormat;
        logTimeFormat.setFontWeight (QFont::Bold);
        logTimeFormat.setForeground (DarkGreen);
        rule.pattern = QRegExp ("\\s{1}[0-9]{2}:[0-9]{2}:[0-9]{2}\\b");
        rule.format = logTimeFormat;
        highlightingRules.append (rule);

        QTextCharFormat logInOutFormat;
        logInOutFormat.setFontWeight (QFont::Bold);
        logInOutFormat.setForeground (Brown);
        rule.pattern = QRegExp ("\\s+IN(?=\\s*\\=)|\\s+OUT(?=\\s*\\=)");
        rule.format = logInOutFormat;
        highlightingRules.append (rule);

        QTextCharFormat logRootFormat;
        logRootFormat.setFontWeight (QFont::Bold);
        logRootFormat.setForeground (Red);
        rule.pattern = QRegExp ("\\broot\\b");
        rule.format = logRootFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "srt")
    {
        QTextCharFormat srtFormat;
        srtFormat.setFontWeight (QFont::Bold);

        /* <...> */
        srtFormat.setForeground (Violet);
        rule.pattern = QRegExp ("</?[A-Za-z0-9_#\\s\"\\=]+>");
        rule.format = srtFormat;
        highlightingRules.append (rule);

        /* hh:mm:ss,ttt */
        srtFormat.setForeground (QBrush());
        srtFormat.setFontItalic (true);
        rule.pattern = QRegExp ("^[0-9]+$|^[0-9]{2}:[0-9]{2}:[0-9]{2},[0-9]{3}\\s-->\\s[0-9]{2}:[0-9]{2}:[0-9]{2},[0-9]{3}$");
        rule.format = srtFormat;
        highlightingRules.append (rule);

        /* subtitle line */
        srtFormat.setForeground (Red);
        rule.pattern = QRegExp ("^[0-9]+$");
        rule.format = srtFormat;
        highlightingRules.append (rule);

        /* mm */
        srtFormat.setForeground (DarkGreen);
        rule.pattern = QRegExp ("[0-9]{2}(?=:[0-9]{2},[0-9]{3}\\s-->\\s[0-9]{2}:[0-9]{2}:[0-9]{2},[0-9]{3}$)|[0-9]{2}(?=:[0-9]{2},[0-9]{3}$)");
        rule.format = srtFormat;
        highlightingRules.append (rule);

        /* hh */
        srtFormat.setForeground (Blue);
        rule.pattern = QRegExp ("^[0-9]{2}(?=:[0-9]{2}:[0-9]{2},[0-9]{3}\\s-->\\s[0-9]{2}:[0-9]{2}:[0-9]{2},[0-9]{3}$)|\\s[0-9]{2}(?=:[0-9]{2}:[0-9]{2},[0-9]{3}$)");
        rule.format = srtFormat;
        highlightingRules.append (rule);

        /* ss */
        srtFormat.setForeground (Brown);
        rule.pattern = QRegExp ("[0-9]{2}(?=,[0-9]{3}\\s-->\\s[0-9]{2}:[0-9]{2}:[0-9]{2},[0-9]{3}$)|[0-9]{2}(?=,[0-9]{3}$)");
        rule.format = srtFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "desktop" || progLan == "theme")
    {
        QTextCharFormat desktopSignsFormat;
        desktopSignsFormat.setForeground (DarkMagenta);
        rule.pattern = QRegExp ("=|;|/|%|\\+|-");
        rule.format = desktopSignsFormat;
        highlightingRules.append (rule);

        QTextCharFormat desktopFormat;

        desktopFormat.setFontWeight (QFont::Bold);
        /* [...] */
        rule.pattern = QRegExp ("^\\[.*\\]$");
        rule.format = desktopFormat;
        highlightingRules.append (rule);

        desktopFormat.setForeground (DarkGreen);
        /* before = */
        rule.pattern = QRegExp ("^[^\\=]+(?=\\s*\\=)");
        rule.format = desktopFormat;
        highlightingRules.append (rule);

        desktopFormat.setForeground (Blue);
        /* [...] and before = */
        rule.pattern = QRegExp ("\\[.*\\](?=\\s*\\=)");
        rule.format = desktopFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "url" || progLan == "sourceslist")
    {
        if (progLan == "sourceslist")
        {
            QTextCharFormat slFormat;
            slFormat.setFontWeight (QFont::Bold);
            rule.pattern = QRegExp ("\\bdeb(?=\\s+)|\\bdeb-src(?=\\s+)");
            rule.format = slFormat;
            highlightingRules.append (rule);

            slFormat.setFontItalic (true);
            rule.pattern = QRegExp ("\\bstable\\b|\\btesting\\b|\\bunstable\\b|\\bsid\\b|\\bexperimental\\b");
            rule.format = slFormat;
            highlightingRules.append (rule);
        }
        rule.pattern = QRegExp ("\\b[A-Za-z0-9_]+://[A-Za-z0-9_.+/\\?\\=~&%#\\-:\\(\\)]+");
        rule.format = urlFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "gtkrc")
    {
        /* color value format (#xyz) */
        QTextCharFormat gtkrcFormat;
        gtkrcFormat.setForeground (DarkGreen);
        gtkrcFormat.setFontWeight (QFont::Bold);
        rule.pattern = QRegExp ("#\\b([A-Za-z0-9]{3}){,4}(?![A-Za-z0-9_]+)");
        rule.format = gtkrcFormat;
        highlightingRules.append (rule);

        gtkrcFormat.setForeground (Blue);
        rule.pattern = QRegExp ("(fg|bg|base|text)(\\[NORMAL\\]|\\[PRELIGHT\\]|\\[ACTIVE\\]|\\[SELECTED\\]|\\[INSENSITIVE\\])");
        rule.format = gtkrcFormat;
        highlightingRules.append (rule);
    }

    /*******************
     * Quotation Marks *
     *******************/

    quotationFormat.setForeground (DarkGreen);
    /*quoteStartExpression = QRegExp ("\"([^\"'])");
    quoteEndExpression = QRegExp ("([^\"'])\"");*/

    /************
     * Comments *
     ************/

    /* single line comments */
    rule.pattern = QRegExp();
    if (progLan == "c" || progLan == "cpp" || lang == "javascript" // javascript inside html
        || lang == "qml" || progLan == "php" /*|| progLan == "css"*/)
    {
        rule.pattern = QRegExp ("//.*"); // why had I set it to QRegExp ("//(?!\\*).*")?
    }
    else if (progLan == "python" || progLan == "desktop"
             || progLan == "sourceslist" || progLan == "qmake"
             || progLan == "gtkrc")
    {
        rule.pattern = QRegExp ("#.*"); // or "#[^\n]*"
    }
    else if (progLan == "lua")
        rule.pattern = QRegExp ("--(?!\\[).*");
    else if (progLan == "troff")
        rule.pattern = QRegExp ("\\\\\"|\\.\\s*\\\\\"");
    if (!rule.pattern.isEmpty())
    {
        rule.format = commentFormat;
        highlightingRules.append (rule);
    }

    /* multiline comments */
    if (progLan == "c" || progLan == "cpp" || progLan == "javascript"
        || lang == "qml" || progLan == "php" || progLan == "css")
    {
        commentStartExpression = QRegExp ("/\\*");
        commentEndExpression = QRegExp ("\\*/");
    }
    else if (progLan == "lua")
    {
        commentStartExpression = QRegExp ("\\[\\[|--\\[\\[");
        commentEndExpression = QRegExp ("\\]\\]");
    }
    else if (progLan == "python")
    {
        commentStartExpression = QRegExp ("\"\"\"|\'\'\'");
        commentEndExpression = commentStartExpression;
    }
    else if (progLan == "xml" || progLan == "html")
    {
        commentStartExpression = QRegExp ("<!--");
        commentEndExpression = QRegExp ("-->");
    }
}
/*************************/
// Check if a start or end quotation mark is escaped at some position.
bool Highlighter::escapedQuote (const QString &text, const int pos, bool canEscapeStart)
{
    if (progLan == "html") return false;

    if (pos < 0) return false;
    if (pos != QRegExp ("\"").indexIn (text, pos)
        && pos != QRegExp ("\'").indexIn (text, pos))
    {
        return false;
    }

    /* check if the quote surrounds a here-doc delimiter */
    if ((currentBlockState() >= endState || currentBlockState() < -1)
        && currentBlockState() % 2 == 0)
    {
        QRegExp delimPart = QRegExp ("<<\\s*");
        QRegExp delimPart1 = QRegExp ("<<(?:\\s*)(\'[A-Za-z0-9_]+)|<<(?:\\s*)(\"[A-Za-z0-9_]+)");
        if (text.lastIndexOf (delimPart, pos) == pos - delimPart.matchedLength()
            || text.lastIndexOf (delimPart1, pos) == pos - delimPart1.matchedLength())
        {
            return true;
        }
    }

    /* escaped start quotes are just for Bash or in '\"' */
    if (canEscapeStart
       && progLan != "sh" && progLan != "makefile" && progLan != "cmake"
       && (pos < 2
           || pos - 2 != text.indexOf ("'\\\"'", pos - 2)))
    {
        return false;
    }

    int i = 0;
    while (pos - i >= 1 && pos - i - 1 == QRegExp ("\\\\").indexIn (text, pos - i - 1))
        ++i;
    /* only an odd number of backslashes
       means that the quote is escaped */
    if (
        i % 2 != 0
            /* for these languages, only double quote can be escaped */
        && (((progLan == "cpp" || progLan == "c" || progLan == "perl")
             && pos == QRegExp ("\"").indexIn (text, pos))
            /* but for these, single quote can be escaped too */
            || progLan == "python"
            /* however, in Bash, single quote can be escaped only at start */
            || ((progLan == "sh" || progLan == "makefile" || progLan == "cmake") && (canEscapeStart || pos == QRegExp ("\"").indexIn (text, pos))))
       )
    {
        return true;
    }

    return false;
}
/*************************/
// Check if a character is inside quotation marks but consider the language.
bool Highlighter::isQuoted (const QString &text, const int index)
{
    if (index < 0) return false;

    bool res = false;
    int pos = -1;
    int N;
    bool scriptLang = false;
    if (progLan == "python" || progLan == "sh"
        || progLan == "makefile" || progLan == "cmake"
        || progLan == "lua" || progLan == "perl"
        || progLan == "ruby" || progLan == "html" || progLan == "javascript")
    {
        scriptLang = true;
    }
    QRegExp quoteExpression;
    if (scriptLang)
        quoteExpression = QRegExp ("\"|\'");
    else
        quoteExpression = QRegExp ("\"");
    if (previousBlockState() != doubleQuoteState
        && previousBlockState() != singleQuoteState
        && previousBlockState() != htmlStyleSingleQuoteState
        && previousBlockState() != htmlStyleDoubleQuoteState)
    {
        N = 0;
    }
    else
    {
        N = 1;
        res = true;
        if (scriptLang)
        {
            if (previousBlockState() == doubleQuoteState
                || previousBlockState() == htmlStyleDoubleQuoteState)
                quoteExpression = QRegExp ("\"");
            else
                quoteExpression = QRegExp ("\'");
        }
    }

    while ((pos = quoteExpression.indexIn (text, pos + 1)) >= 0)
    {
        /* skip formatted comments */
        if (format (pos) == commentFormat)
            continue;

        ++N;
        if ((N % 2 == 0 || progLan == "sh" || progLan == "makefile" || progLan == "cmake") // it's an end quote (except for Bash)
            && escapedQuote (text, pos, false))
        {
            --N;
            continue;
        }

        if (index < pos)
        {
            if (N % 2 == 0) res = true;
            else res = false;
            break;
        }

        if (N % 2 == 0) res = false;
        else res = true;

        if (scriptLang)
        {
            if (N % 2 != 0)
            {
                if (pos == QRegExp ("\"").indexIn (text, pos))
                    quoteExpression = QRegExp ("\"");
                else
                    quoteExpression = QRegExp ("\'");
            }
            else
                quoteExpression = QRegExp ("\"|\'");
        }
    }

    return res;
}
/*************************/
// Check if a character is inside a multiline comment.
bool Highlighter::isMLCommented (const QString &text, const int index)
{
    /* not for Python */
    if (progLan == "python") return false;

    if (index < 0 || commentStartExpression.isEmpty())
        return false;

    bool res = false;
    int pos = -1;
    int N;
    QRegExp commentExpression;
    if (previousBlockState() != commentState)
    {
        N = 0;
        commentExpression = commentStartExpression;
    }
    else
    {
        N = 1;
        res = true;
        commentExpression = commentEndExpression;
    }

    while ((pos = commentExpression.indexIn (text, pos + 1)) >= 0)
    {
        /* skip formatted quotations */
        if (format (pos) == quotationFormat)
            continue;

        ++N;

        if (index <= pos + 1) /* all multiline comments have more than
                                 one character and this trick is needed
                                 for knowing if double slashes follow an
                                 asterisk, for example, because QRegExp
                                 lacks "lookbehind" */
        {
            if (N % 2 == 0) res = true;
            else res = false;
            break;
        }

        if (N % 2 != 0)
        {
            commentExpression = commentEndExpression;
            res = true;
        }
        else
        {
            commentExpression = commentStartExpression;
            res = false;
        }
    }

    return res;
}
/*************************/
// This handles multiline python comments separately because they
// they aren't normal. It comes before multiline quotations highlighting.
void Highlighter::pythonMLComment (const QString &text, const int indx)
{
    if (progLan != "python") return;

    QRegExp urlPattern = QRegExp ("[A-Za-z0-9_]+://[A-Za-z0-9_.+/\\?\\=~&%#\\-:]+|[A-Za-z0-9_.\\-]+@[A-Za-z0-9_\\-]+\\.[A-Za-z0-9.]+");
    QRegExp notePattern = QRegExp ("\\b(NOTE|TODO|FIXME|WARNING)\\b");
    int pIndex = 0;
    QTextCharFormat noteFormat;
    noteFormat.setFontWeight (QFont::Bold);
    noteFormat.setFontItalic (true);
    noteFormat.setForeground (DarkRed);

    /* we reset tha block state because this method is also called
       during the multiline quotation formatting after clearing formats */
    setCurrentBlockState (-1);

    int index = indx;
    int quote;

    /* find the start quote */
    if (previousBlockState() != pyDoubleQuoteState
        && previousBlockState() != pySingleQuoteState)
    {
        index = commentStartExpression.indexIn (text, indx);

        while (format (index) == quotationFormat)
            index = commentStartExpression.indexIn (text, index + 3);
        while (format (index) == commentFormat)
            index = commentStartExpression.indexIn (text, index + 3);

        /* if the comment start is found... */
        if (index >= indx)
        {
            /* ... distinguish between double and single quotes */
            if (index == QRegExp ("\"\"\"").indexIn (text, index))
            {
                commentStartExpression = QRegExp ("\"\"\"");
                quote = pyDoubleQuoteState;
            }
            else
            {
                commentStartExpression = QRegExp ("\'\'\'");
                quote = pySingleQuoteState;
            }
        }
    }
    else // but if we're inside a triple quotation...
    {
        /* ... distinguish between the two quote kinds
           by checking the previous line */
        quote = previousBlockState();
        if (quote == pyDoubleQuoteState)
            commentStartExpression = QRegExp ("\"\"\"");
        else
            commentStartExpression = QRegExp ("\'\'\'");
    }

    while (index >= indx)
    {
        /* if the search is continued... */
        if (commentStartExpression == QRegExp ("\"\"\"|\'\'\'"))
        {
            /* ... distinguish between double and single quotes
               again because the quote mark may have changed... */
            if (index == QRegExp ("\"").indexIn (text, index))
            {
                commentStartExpression = QRegExp ("\"\"\"");
                quote = pyDoubleQuoteState;
            }
            else
            {
                commentStartExpression = QRegExp ("\'\'\'");
                quote = pySingleQuoteState;
            }
        }

        /* search for the end quote from the start quote */
        int endIndex = commentStartExpression.indexIn (text, index + 3);

        /* but if there's no start quote ... */
        if (index == indx
            && (previousBlockState() == pyDoubleQuoteState || previousBlockState() == pySingleQuoteState))
        {
            /* ... search for the end quote from the line start */
            endIndex = commentStartExpression.indexIn (text, indx);
        }

        /* check if the quote is escaped */
        while ((endIndex >= 1 && endIndex  - 1 == QRegExp ("\\\\").indexIn (text, endIndex - 1)
                /* backslash shouldn't be escaped itself */
                && (endIndex < 2 || endIndex  - 2 != QRegExp ("\\\\").indexIn (text, endIndex - 2)))
                   /* also consider ^' and ^" */
                   || ((endIndex >= 1 && endIndex  - 1 == QRegExp ("\\^").indexIn (text, endIndex - 1))
                       && (endIndex < 2 || endIndex  - 2 != QRegExp ("\\\\").indexIn (text, endIndex - 2))))
        {
            endIndex = commentStartExpression.indexIn (text, endIndex + 3);
        }

        int quoteLength;
        if (endIndex == -1)
        {
            setCurrentBlockState (quote);
            quoteLength = text.length() - index;
        }
        else
            quoteLength = endIndex - index
                          + commentStartExpression.matchedLength(); // 3
        setFormat (index, quoteLength, commentFormat);

        /* format urls and email addresses inside the comment */
        QString str = text.mid (index, quoteLength);
        int indx = 0;
        while ((pIndex = str.indexOf (urlPattern, indx)) > -1)
        {
            int ml = urlPattern.matchedLength();
            setFormat (pIndex + index, ml, urlFormat);
            indx = indx + ml;
        }
        /* format note patterns too */
        indx = 0;
        while ((pIndex = str.indexOf (notePattern, indx)) > -1)
        {
            int ml = notePattern.matchedLength();
            if (format (pIndex) != urlFormat)
              setFormat (pIndex + index, ml, noteFormat);
            indx = indx + ml;
        }

        /* the next quote may be different */
        commentStartExpression = QRegExp ("\"\"\"|\'\'\'");
        index = commentStartExpression.indexIn (text, index + quoteLength);
        while (format (index) == quotationFormat)
            index = commentStartExpression.indexIn (text, index + 3);
        while (format (index) == commentFormat)
            index = commentStartExpression.indexIn (text, index + 3);
    }
}
/*************************/
// This should come before multiline comments highlighting.
int Highlighter::cssHighlighter (const QString &text)
{
    if (progLan != "css") return -1;

    int cssIndx = -1;

    /**************************
     * (Multiline) CSS Blocks *
     **************************/

    QRegExp cssStartExpression = QRegExp ("\\{");
    QRegExp cssEndExpression = QRegExp ("\\}");
    int index = 0;
    QTextCharFormat cssFormat;
    cssFormat.setFontUnderline (true);
    cssFormat.setForeground (Red);
    if (previousBlockState() != cssBlockState
        && previousBlockState() != commentInCssState
        && previousBlockState() != cssValueState)
    {
        index = cssStartExpression.indexIn (text);
        if (index >= 0) cssIndx = index;
    }

    while (index >= 0)
    {
        int endIndex;
        /* when the css block start is in the prvious line
           and the search for the block end has just begun... */
        if ((previousBlockState() == cssBlockState
             || previousBlockState() == commentInCssState
             || previousBlockState() == cssValueState) // subset of cssBlockState
            && index == 0)
            /* search for the block end from the line start */
            endIndex = cssEndExpression.indexIn (text, 0);
        else
            endIndex = cssEndExpression.indexIn (text,
                                                 index + cssStartExpression.matchedLength());

        int cssLength;
        if (endIndex == -1)
        {
            endIndex = text.length() - 1;
            setCurrentBlockState (cssBlockState);
            cssLength = text.length() - index;
        }
        else
            cssLength = endIndex - index
                        + cssEndExpression.matchedLength();

        /* at first, we suppose all syntax is wrong */
        QRegExp expression = QRegExp ("[^\\{^\\}^\\s]+");
        int indxTmp = expression.indexIn (text, index);
        while (isQuoted (text, indxTmp))
            indxTmp = expression.indexIn (text, indxTmp + 1);
        while (indxTmp >= 0 && indxTmp < endIndex)
        {
            int length = expression.matchedLength();
            setFormat (indxTmp, length, cssFormat);
            indxTmp = expression.indexIn (text, indxTmp + length);
        }

        /* css attribute format (before :...;) */
        QTextCharFormat cssAttFormat;
        cssAttFormat.setFontItalic (true);
        cssAttFormat.setForeground (Blue);
        expression = QRegExp ("[A-Za-z0-9_\\-]+(?=\\s*:.*;*)");
        indxTmp = expression.indexIn (text, index);
        while (isQuoted (text, indxTmp))
            indxTmp = expression.indexIn (text, indxTmp + 1);
        while (indxTmp >= 0 && indxTmp < endIndex)
        {
            int length = expression.matchedLength();
            setFormat (indxTmp, length, cssAttFormat);
            indxTmp = expression.indexIn (text, indxTmp + length);
        }

        index = cssStartExpression.indexIn (text, index + cssLength);
    }

    /**************************
     * (Multiline) CSS Values *
     **************************/

    cssStartExpression = QRegExp (":");
    cssEndExpression = QRegExp (";|\\}");
    index = 0;
    if (previousBlockState() != cssValueState)
    {
        index = cssStartExpression.indexIn (text);
        if (index > -1)
        {
            while (format (index) != cssFormat)
            {
                index = cssStartExpression.indexIn (text, index + 1);
                if (index == -1) break;
            }
        }
    }

    while (index >= 0)
    {
        int endIndex;
        int startMatch = 0;
        if (previousBlockState() == cssValueState
            && index == 0)
            endIndex = cssEndExpression.indexIn (text, 0);
        else
        {
            startMatch = cssStartExpression.matchedLength();
            endIndex = cssEndExpression.indexIn (text,
                                                 index + startMatch);
        }

        int cssLength;
        if (endIndex == -1)
        {
            setCurrentBlockState (cssValueState);
            cssLength = text.length() - index;
        }
        else
            cssLength = endIndex - index
                        + cssEndExpression.matchedLength();
        /* css value format */
        QTextCharFormat cssValueFormat;
        cssValueFormat.setFontItalic (true);
        cssValueFormat.setForeground (DarkGreen);
        setFormat (index, cssLength, cssValueFormat);

        QTextCharFormat neutral;
        neutral.setForeground (QBrush());
        setFormat (index, startMatch, neutral);
        if (endIndex > -1)
            setFormat (endIndex, 1, neutral);

        index = cssStartExpression.indexIn (text, index + cssLength);
        if (index > -1)
        {
            while (format (index) != cssFormat)
            {
                index = cssStartExpression.indexIn (text, index + 1);
                if (index == -1) break;
            }
        }
    }

    /* color value format (#xyz) */
    QTextCharFormat cssColorFormat;
    cssColorFormat.setFontItalic (true);
    cssColorFormat.setForeground (DarkGreen);
    cssColorFormat.setFontWeight (QFont::Bold);
    QRegExp expression = QRegExp ("#\\b([A-Za-z0-9]{3}){,4}(?![A-Za-z0-9_]+)");
    int indxTmp = expression.indexIn (text);
    while (isQuoted (text, indxTmp))
        indxTmp = expression.indexIn (text, indxTmp + 1);
    while (indxTmp >= 0)
    {
        int length = expression.matchedLength();
        /* now cssFormat is really the error format */
        if (format (indxTmp) != cssFormat)
            setFormat (indxTmp, length, cssColorFormat);
        indxTmp = expression.indexIn (text, indxTmp + length);
    }

    return cssIndx;
}
/*************************/
void Highlighter::singleLineComment (const QString &text, int index)
{
    QRegExp urlPattern = QRegExp ("[A-Za-z0-9_]+://[A-Za-z0-9_.+/\\?\\=~&%#\\-:]+|[A-Za-z0-9_.\\-]+@[A-Za-z0-9_\\-]+\\.[A-Za-z0-9.]+");
    QRegExp notePattern = QRegExp ("\\b(NOTE|TODO|FIXME|WARNING)\\b");
    int pIndex = 0;
    QTextCharFormat noteFormat;
    noteFormat.setFontWeight (QFont::Bold);
    noteFormat.setFontItalic (true);
    noteFormat.setForeground (DarkRed);

    foreach (const HighlightingRule &rule, highlightingRules)
    {
        if (rule.format == commentFormat)
        {
            int end = -1;
            if (progLan == "html") // javascript end
                end = QRegExp ("</script\\s*>").indexIn (text, index);
            if (end == -1) end = text.length();
            QRegExp expression (rule.pattern);
            index = expression.indexIn (text, index);
            if (index >= 0 && index < end)
            {
                setFormat (index, end - index, commentFormat);

                /* also format urls and email addresses inside the comment */
                QString str = text.mid (index, end - index);
                int indx = 0;
                while ((pIndex = str.indexOf (urlPattern, indx)) > -1)
                {
                    int ml = urlPattern.matchedLength();
                    setFormat (pIndex + index, ml, urlFormat);
                    indx = indx + ml;
                }
                /* format note patterns too */
                indx = 0;
                while ((pIndex = str.indexOf (notePattern, indx)) > -1)
                {
                    int ml = notePattern.matchedLength();
                    if (format (pIndex) != urlFormat)
                      setFormat (pIndex + index, ml, noteFormat);
                    indx = indx + ml;
                }
            }
            break;
        }
    }
}
/*************************/
void Highlighter::multiLineComment (const QString &text,
                                    int index, int cssIndx,
                                    QRegExp commentStartExp, QRegExp commentEndExp,
                                    int commState)
{
    bool commentBeforeBrace = false; // in css, not as: "{...
    QRegExp urlPattern = QRegExp ("[A-Za-z0-9_]+://[A-Za-z0-9_.+/\\?\\=~&%#\\-:]+|[A-Za-z0-9_.\\-]+@[A-Za-z0-9_\\-]+\\.[A-Za-z0-9.]+");
    QRegExp notePattern = QRegExp ("\\b(NOTE|TODO|FIXME|WARNING)\\b");
    int pIndex = 0;
    QTextCharFormat noteFormat;
    noteFormat.setFontWeight (QFont::Bold);
    noteFormat.setFontItalic (true);
    noteFormat.setForeground (DarkRed);

    if (previousBlockState() != commState
        && previousBlockState() != commentInCssState)
    {
        index = commentStartExp.indexIn (text, index);
        /* skip single-line comments */
        if (format (index) == commentFormat)
            index = -1;
        /* skip quotations (all formatted to this point) */
        while (format (index) == quotationFormat)
            index = commentStartExp.indexIn (text, index + 1);
        /* skip javascript in html */
        while (index >= 0 && progLan == "html" && index == QRegExp ("<!--.*</script\\s*>").indexIn (text))
            index = commentStartExp.indexIn (text, index + 1);
        if (index >=0 && index < cssIndx)
            commentBeforeBrace = true;
    }

    while (index >= 0)
    {
        int badIndex = -1;
        int endIndex;
        /* when the comment start is in the prvious line
           and the search for the comment end has just begun... */
        if ((previousBlockState() == commState
             || previousBlockState() == commentInCssState)
            && index == 0)
            /* search for the comment end from the line start */
            endIndex = commentEndExp.indexIn (text, 0);
        else
            endIndex = commentEndExp.indexIn (text,
                                              index + commentStartExp.matchedLength());

        /* skip quotations */
        while (format (endIndex) == quotationFormat)
            endIndex = commentEndExp.indexIn (text, endIndex + 1);

        /* if there's a comment end ... */
        if (endIndex >= 0)
        {
            /* ... clear the comment format from there to reformat later as
               a single-line comment sign may have been commented out now */
            badIndex = endIndex + 1;
            QTextCharFormat neutral;
            neutral.setForeground (QBrush());
            for (int i = badIndex; i < text.length(); ++i)
            {
                if (format (i) == commentFormat)
                    setFormat (i, 1, neutral);
            }
        }

        int commentLength;
        if (endIndex == -1)
        {
            if ((currentBlockState() != cssBlockState
                 && currentBlockState() != cssValueState)
                || commentBeforeBrace)
                setCurrentBlockState (commState);
            else
                setCurrentBlockState (commentInCssState);
            commentLength = text.length() - index;
        }
        else
        {
            if (cssIndx >= index && cssIndx < endIndex)
            {
                /* if '{' is inside the comment,
                   this isn't a CSS block */
                setCurrentBlockState (-1);
                QTextCharFormat neutral;
                neutral.setForeground (QBrush());
                setFormat (endIndex + 1, text.length() - endIndex - 1, neutral);
            }
            commentLength = endIndex - index
                            + commentEndExp.matchedLength();
        }
        setFormat (index, commentLength, commentFormat);

        /* format urls and email addresses inside the comment */
        QString str = text.mid (index, commentLength);
        int indx = 0;
        while ((pIndex = str.indexOf (urlPattern, indx)) > -1)
        {
            int ml = urlPattern.matchedLength();
            setFormat (pIndex + index, ml, urlFormat);
            indx = indx + ml;
        }
        /* format note patterns too */
        indx = 0;
        while ((pIndex = str.indexOf (notePattern, indx)) > -1)
        {
            int ml = notePattern.matchedLength();
            if (format (pIndex) != urlFormat)
              setFormat (pIndex + index, ml, noteFormat);
            indx = indx + ml;
        }

        index = commentStartExp.indexIn (text, index + commentLength);

        /* reformat from here if the format was cleared before */
        if (badIndex >= 0)
        {
            foreach (const HighlightingRule &rule, highlightingRules)
            {
                if (rule.format == commentFormat)
                {
                    QRegExp expression (rule.pattern);
                    int INDX = expression.indexIn (text, badIndex);
                    while (format (INDX) == quotationFormat || isMLCommented (text, INDX))
                        INDX = expression.indexIn (text, INDX + 1);
                    if (INDX >= 0)
                        setFormat (INDX, text.length() - INDX, commentFormat);
                    break;
                }
            }
        }

        /* skip single-line comments and quotations again */
        if (format (index) == commentFormat)
            index = -1;
        while (format (index) == quotationFormat)
            index = commentStartExp.indexIn (text, index + 1);
    }
}
/*************************/
void Highlighter::multiLineQuote (const QString &text)
{
    int index = 0;
    bool scriptLang = false;
    if (progLan == "python" || progLan == "sh"
        || progLan == "makefile" || progLan == "cmake"
        || progLan == "lua" || progLan == "perl"
        || progLan == "ruby" || progLan == "javascript")
    {
        scriptLang = true;
    }
    QRegExp quoteExpression;
    if (scriptLang)
        quoteExpression = QRegExp ("\"|\'");
    else
        quoteExpression = QRegExp ("\"");
    int quote = doubleQuoteState;

    /* find the start quote */
    if (previousBlockState() != doubleQuoteState
        && previousBlockState() != singleQuoteState)
    {
        index = quoteExpression.indexIn (text);
        /* skip escaped start quotes */
        while (escapedQuote (text, index, true))
            index = quoteExpression.indexIn (text, index + 1);
        /* skip all comments */
        while (isMLCommented (text, index)) // multiline
            index = quoteExpression.indexIn (text, index + 1);
        while (format (index) == commentFormat) // single-line and Python
            index = quoteExpression.indexIn (text, index + 1);

        /* if the start quote is found... */
        if (index >= 0)
        {
            if (scriptLang)
            {
                /* ... distinguish between double and single quotes */
                if (index == QRegExp ("\"").indexIn (text, index))
                {
                    quoteExpression = QRegExp ("\"");
                    quote = doubleQuoteState;
                }
                else
                {
                    quoteExpression = QRegExp ("\'");
                    quote = singleQuoteState;
                }
            }
        }
    }
    else // but if we're inside a quotation...
    {
        /* ... distinguish between the two quote kinds
           by checking the previous line */
        if (scriptLang)
        {
            quote = previousBlockState();
            if (quote == doubleQuoteState)
                quoteExpression = QRegExp ("\"");
            else
                quoteExpression = QRegExp ("\'");
        }
    }

    while (index >= 0)
    {
        int badIndex = -1;

        /* if the search is continued... */
        if (quoteExpression == QRegExp ("\"|\'"))
        {
            /* ... distinguish between double and single quotes
               again because the quote mark may have changed */
            if (index == QRegExp ("\"").indexIn (text, index))
            {
                quoteExpression = QRegExp ("\"");
                quote = doubleQuoteState;
            }
            else
            {
                quoteExpression = QRegExp ("\'");
                quote = singleQuoteState;
            }
        }

        /* search for the end quote from the start quote */
        int endIndex = quoteExpression.indexIn (text, index + 1);

        /* but if there's no start quote ... */
        if (index == 0
            && (previousBlockState() == doubleQuoteState || previousBlockState() == singleQuoteState))
        {
            /* ... search for the end quote from the line start */
            endIndex = quoteExpression.indexIn (text, 0);
        }

        /* check if the quote is escaped */
        while (escapedQuote (text, endIndex, false))
            endIndex = quoteExpression.indexIn (text, endIndex + 1);

        /* in c and cpp, multiline quotes need backslash */
        if (endIndex == -1 && (progLan == "c" || progLan == "cpp") && !text.endsWith("\\"))
            endIndex = text.size() + 1; // quoteExpression.matchedLength() is -1 here

        /* if there's an end quote ... */
        if (endIndex >= 0)
        {
            /* ... clear the comment format from there to reformat later since
               a single-line or Python comment sign may have been quoted now */
            badIndex = endIndex + 1;
            QTextCharFormat neutral;
            neutral.setForeground (QBrush());
            /* only clear the comment format because
               here-doc formatting is done before */
            for (int i = badIndex; i < text.length(); ++i)
            {
                if (format (i) == commentFormat)
                    setFormat (i, 1, neutral);
            }
        }

        int quoteLength;
        if (endIndex == -1)
        {
            setCurrentBlockState (quote);
            quoteLength = text.length() - index;
        }
        else
            quoteLength = endIndex - index
                          + quoteExpression.matchedLength(); // 1
        setFormat (index, quoteLength, quotationFormat);

        /* the next quote may be different */
        if (scriptLang)
            quoteExpression = QRegExp ("\"|\'");
        index = quoteExpression.indexIn (text, index + quoteLength);

        /* reformat from here if the format was cleared before */
        if (badIndex >= 0)
        {
            foreach (const HighlightingRule &rule, highlightingRules)
            {
                if (rule.format == commentFormat)
                {
                    QRegExp expression (rule.pattern);
                    int indx = expression.indexIn (text, badIndex);
                    /*
                       skip quotes and multi-line comments;
                       needed for cases like: "a//b"c
                    */
                    while (isQuoted (text, indx) || isMLCommented (text, indx))
                        indx = expression.indexIn (text, indx + 1);
                    if (indx >= 0)
                    {
                        //setFormat (indx, text.length() - indx, commentFormat);
                        /* instead of only using setFormat(),
                           also take into account url and note patterns */
                        singleLineComment (text, indx);
                    }
                    break;
                }
            }
            /* the end quote could have been the start of a Python comment */
            pythonMLComment (text, badIndex);
        }

        /* skip all comments */
        while (isMLCommented (text, index))
            index = quoteExpression.indexIn (text, index + 1);
        while (format (index) == commentFormat)
            index = quoteExpression.indexIn (text, index + 1);
        /* skip escaped start quotes */
        while (escapedQuote (text, index, true))
            index = quoteExpression.indexIn (text, index + 1);
    }
}
/*************************/
// Check if the current block is inside a "here document" and format it accordingly.
bool Highlighter::isHereDocument (const QString &text)
{
    QTextCharFormat blockFormat;
    blockFormat.setForeground (Violet);
    QTextCharFormat delimFormat = blockFormat;
    delimFormat.setFontWeight (QFont::Bold);
    QString delimStr;
    /* Kate uses something like "<<(?:\\s*)([\\\\]{,1}[^\\s]+)" */
    QRegExp delim = QRegExp ("<<(?:\\s*)([\\\\]{,1}[A-Za-z0-9_]+)|<<(?:\\s*)(\'[A-Za-z0-9_]+\')|<<(?:\\s*)(\"[A-Za-z0-9_]+\")");
    int insideCommentPos = QRegExp ("\\s+#").indexIn (text);
    int pos = 0;

    /* format the start delimiter */
    if (previousBlockState() >= 0 && previousBlockState() < endState
        && QRegExp ("\\s*#").indexIn (text) != 0 // the whole line isn't commented out
        && (pos = delim.indexIn (text)) >= 0
        && !isQuoted (text, pos)
        && (insideCommentPos == -1 || pos < insideCommentPos))
    {
        int i = 1;
        while ((delimStr = delim.cap (i)).isEmpty() && i <= 3)
        {
            ++i;
            delimStr = delim.cap (i);
        }
        /* remove quotes */
        if (delimStr.contains ('\''))
            delimStr = delimStr.split ('\'').at (1);
        if (delimStr.contains ('\"'))
            delimStr = delimStr.split ('\"').at (1);
        /* remove the start backslash if it exists */
        if (QString (delimStr.at (0)) == "\\")
            delimStr = delimStr.remove (0, 1);

        if (!delimStr.isEmpty())
        {
            int n = qHash (delimStr);
            setCurrentBlockState (2 * (n + (n >= 0 ? 9 : 0))); // always an even number but maybe negative
            setFormat (text.indexOf (delimStr, pos),
                       delimStr.length(),
                       delimFormat);

            TextBlockData *data = static_cast<TextBlockData *>(currentBlock().userData());
            data->insertInfo (delimStr);
            setCurrentBlockUserData (data);

            return false;
        }
    }

    if (previousBlockState() >= endState || previousBlockState() < -1)
    {
        QTextBlock block = currentBlock().previous();
        TextBlockData *data = static_cast<TextBlockData *>(block.userData());
        delimStr = data->delimiter();
        if (text == delimStr
            || (text.startsWith (delimStr)
                && text.indexOf (QRegExp ("\\s+")) == delimStr.length()))
        {
            /* format the end delimiter */
            setFormat (0,
                       delimStr.length(),
                       delimFormat);
            return false;
        }
        else
        {
            /* format the contents */
            TextBlockData *data = static_cast<TextBlockData *>(currentBlock().userData());
            data->insertInfo (delimStr);
            setCurrentBlockUserData (data);
            if (previousBlockState() % 2 == 0)
                setCurrentBlockState (previousBlockState() - 1);
            else
                setCurrentBlockState (previousBlockState());
            setFormat (0, text.length(), blockFormat);
            return true;
        }
    }

    return false;
}
/*************************/
// Start syntax highlighting!
void Highlighter::highlightBlock (const QString &text)
{
    if (progLan.isEmpty()) return;

    int index;
    TextBlockData *data = new TextBlockData;
    setCurrentBlockUserData (data); // to be fed in later
    setCurrentBlockState (0);

    /********************
     * "Here" Documents *
     ********************/

    if (progLan == "sh" || progLan == "makefile" || progLan == "cmake"
        || progLan == "perl" || progLan == "ruby")
    {
        if (isHereDocument (text))
            return;
    }

    /************************
     * Single-Line Comments *
     ************************/

    if (progLan != "html")
        singleLineComment (text, 0);

    /*******************
     * Python Comments *
     *******************/

    pythonMLComment (text, 0);

    /**************************
     * (Multiline) Quotations *
     **************************/

    if (progLan != "diff" && progLan != "log"
        && progLan != "changelog" && progLan != "url"
        && progLan != "srt" && progLan != "html")
    {
        multiLineQuote (text);
    }

    /*******
     * CSS *
     *******/

    /* helps to see if a comment destroys a css block */
    int cssIndx = cssHighlighter (text);

    /**********************
     * Multiline Comments *
     **********************/

    if (!commentStartExpression.isEmpty() && progLan != "python")
        multiLineComment (text, 0, cssIndx, commentStartExpression, commentEndExpression, commentState);

    /*************
     * HTML Only *
     *************/

    if (progLan == "html")
    {
        htmlBrackets (text);
        htmlStyleHighlighter (text);
        htmlJavascript (text);
        /* go to braces matching */
    }

    /*******************
     * Main Formatting *
     *******************/

    int bn = currentBlock().blockNumber();
    /* we don't know whether this block is highlighted because of a change in it
       or just because it has become visible, so we remove it from the list here
       but, later, it'll be added to the list again if it's visible */
    highlighted.remove (bn);
    if (progLan != "html" // we format html embedded javascript in htmlJavascript()
        && bn >= startCursor.blockNumber()
        && bn <= endCursor.blockNumber())
    {
        highlighted.insert (bn);
        foreach (const HighlightingRule &rule, highlightingRules)
        {
            /* single-line comments are already formatted */
            if (rule.format == commentFormat)
                continue;

            QRegExp expression (rule.pattern);
            index = expression.indexIn (text);
            /* skip quotes and all comments */
            while ((format (index) == quotationFormat && progLan != "gtkrc") // "#xyz"
                   || format (index) == commentFormat
                   || format (index) == urlFormat)
            {
                index = expression.indexIn (text, index + 1);
            }

            while (index >= 0)
            {
                int length = expression.matchedLength();
                /*if (format (index + length) != quotationFormat
                    && format (index + length) != commentFormat)
                {
                    setFormat (index, length, rule.format);
                }*/
                setFormat (index, length, rule.format);
                index = expression.indexIn (text, index + length);

                while ((format (index) == quotationFormat && progLan != "gtkrc")
                       || format (index) == commentFormat)
                {
                    index = expression.indexIn (text, index + 1);
                }
            }
        }
    }

    /***********************************
     * Parentheses and Braces Matching *
     ***********************************/

    /* left parenthesis */
    index = text.indexOf ('(');
    while (format (index) == quotationFormat || format (index) == commentFormat)
        index = text.indexOf ('(', index + 1);
    while (index != -1)
    {
        ParenthesisInfo *info = new ParenthesisInfo;
        info->character = '(';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf ('(', index + 1);
        while (format (index) == quotationFormat || format (index) == commentFormat)
            index = text.indexOf ('(', index + 1);
    }

    /* right parenthesis */
    index = text.indexOf (')');
    while (format (index) == quotationFormat || format (index) == commentFormat)
        index = text.indexOf (')', index + 1);
    while (index != -1)
    {
        ParenthesisInfo *info = new ParenthesisInfo;
        info->character = ')';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf (')', index +1);
        while (format (index) == quotationFormat || format (index) == commentFormat)
            index = text.indexOf (')', index + 1);
    }

    /* left brace */
    index = text.indexOf ('{');
    while (format (index) == quotationFormat || format (index) == commentFormat)
        index = text.indexOf ('{', index + 1);
    while (index != -1)
    {
        BraceInfo *info = new BraceInfo;
        info->character = '{';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf ('{', index + 1);
        while (format (index) == quotationFormat || format (index) == commentFormat)
            index = text.indexOf ('{', index + 1);
    }

    /* right brace */
    index = text.indexOf ('}');
    while (format (index) == quotationFormat || format (index) == commentFormat)
        index = text.indexOf ('}', index + 1);
    while (index != -1)
    {
        BraceInfo *info = new BraceInfo;
        info->character = '}';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf ('}', index +1);
        while (format (index) == quotationFormat || format (index) == commentFormat)
            index = text.indexOf ('}', index + 1);
    }

    setCurrentBlockUserData (data);
}

}
