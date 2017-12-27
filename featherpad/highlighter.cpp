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
#include <QTextDocument>

Q_DECLARE_METATYPE(QTextBlock)

namespace FeatherPad {

TextBlockData::~TextBlockData()
{
    while (!allParentheses.isEmpty())
    {
        ParenthesisInfo *info = allParentheses.at (0);
        allParentheses.remove (0);
        delete info;
    }
    while (!allBraces.isEmpty())
    {
        BraceInfo *info = allBraces.at (0);
        allBraces.remove(0);
        delete info;
    }
    while (!allBrackets.isEmpty())
    {
        BracketInfo *info = allBrackets.at (0);
        allBrackets.remove(0);
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
QVector<BracketInfo *> TextBlockData::brackets()
{
    return allBrackets;
}
/*************************/
QString TextBlockData::delimiter()
{
    return Delimiter;
}
/*************************/
bool TextBlockData::isHighlighted()
{
    return Highlighted;
}
/*************************/
int TextBlockData::openNests()
{
    return OpenNests;
}
/*************************/
void TextBlockData::insertInfo (ParenthesisInfo *info)
{
    int i = 0;
    while (i < allParentheses.size()
           && info->position > allParentheses.at (i)->position)
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
           && info->position > allBraces.at (i)->position)
    {
        ++i;
    }

    allBraces.insert (i, info);
}
/*************************/
void TextBlockData::insertInfo (BracketInfo *info)
{
    int i = 0;
    while (i < allBrackets.size()
           && info->position > allBrackets.at (i)->position)
    {
        ++i;
    }

    allBrackets.insert (i, info);
}
/*************************/
void TextBlockData::insertInfo (QString str)
{
    Delimiter = str;
}
/*************************/
void TextBlockData::insertHighlightInfo (bool highlighted)
{
    Highlighted = highlighted;
}
/*************************/
void TextBlockData::insertNestInfo (int nests)
{
    OpenNests = nests;
}
/*************************/
// Here, the order of formatting is important because of overrides.
Highlighter::Highlighter (QTextDocument *parent, QString lang, QTextCursor start, QTextCursor end,
                          bool darkColorScheme,
                          bool showWhiteSpace, bool showEndings) : QSyntaxHighlighter (parent)
{
    if (lang.isEmpty()) return;

    if (showWhiteSpace || showEndings)
    {
        QTextOption opt =  document()->defaultTextOption();
        QTextOption::Flags flags = opt.flags();
        if (showWhiteSpace)
            flags |= QTextOption::ShowTabsAndSpaces;
        if (showEndings)
            flags |= QTextOption::ShowLineAndParagraphSeparators
#if QT_VERSION >= 0x050700
                     | QTextOption::ShowDocumentTerminator
#endif
                     ;
        opt.setFlags (flags);
        document()->setDefaultTextOption (opt);
    }

    /* for highlighting next block inside highlightBlock() when needed */
    qRegisterMetaType<QTextBlock>();

    startCursor = start;
    endCursor = end;
    progLan = lang;

    quoteMark = QRegExp ("\""); // the standard quote mark

    HighlightingRule rule;
    QColor Faded;
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
        DarkYellow = Qt::darkYellow;
        Faded = QColor (140, 140, 140);
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
        DarkYellow = Qt::yellow;
        Faded = QColor (120, 120, 120);
    }
    DarkGreenAlt = DarkGreen.lighter (101); // almost identical

    neutralFormat.setForeground (QBrush());
    whiteSpaceFormat.setForeground (Faded);

    quoteFormat.setForeground (DarkGreen);
    altQuoteFormat.setForeground (DarkGreen);
    altQuoteFormat.setFontItalic (true);
    /*quoteStartExpression = QRegExp ("\"([^\"'])");
    quoteEndExpression = QRegExp ("([^\"'])\"");*/

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
        /* before parentheses... */
        rule.pattern = QRegExp ("\\b[A-Za-z0-9_]+(?=\\s*\\()");
        rule.format = functionFormat;
        highlightingRules.append (rule);
        /* ... but make exception for what comes after "#define" */
        if (progLan == "c" || progLan == "cpp")
        {
            rule.pattern = QRegExp ("^\\s*#\\s*define\\s+[^\"\']" // may contain slash but no quote
                                    "+(?=\\s*\\()");
            rule.format = neutralFormat;
            highlightingRules.append (rule);
        }
        else if (progLan == "python")
        { // built-in functions
            functionFormat.setFontWeight (QFont::Bold);
            functionFormat.setForeground (Qt::magenta);
            rule.pattern = QRegExp ("\\b(abs|add|all|append|any|as_integer_ratio|ascii|basestring|bin|bit_length|bool|bytearray|bytes|callable|c\\.conjugate|capitalize|center|chr|classmethod|clear|cmp|compile|complex|count|critical|debug|decode|delattr|dict|difference_update|dir|discard|divmod|encode|endswith|enumerate|error|eval|expandtabs|exception|exec|execfile|extend|file|filter|find|float|format|fromhex|fromkeys|frozenset|get|getattr|globals|hasattr|hash|has_key|help|hex|id|index|info|input|insert|int|intersection_update|isalnum|isalpha|isdecimal|isdigit|isinstance|islower|isnumeric|isspace|issubclass|istitle|items|iter|iteritems|iterkeys|itervalues|isupper|is_integer|join|keys|len|list|ljust|locals|log|long|lower|lstrip|map|max|memoryview|min|next|object|oct|open|ord|partition|pop|popitem|pow|print|property|range|raw_input|read|reduce|reload|remove|replace|repr|reverse|reversed|rfind|rindex|rjust|rpartition|round|rsplit|rstrip|run|seek|set|setattr|slice|sort|sorted|split|splitlines|staticmethod|startswith|str|strip|sum|super|symmetric_difference_update|swapcase|title|translate|tuple|type|unichr|unicode|update|upper|values|vars|viewitems|viewkeys|viewvalues|warning|write|xrange|zip|zfill|(__(abs|add|and|cmp|coerce|complex|contains|delattr|delete|delitem|delslice|div|divmod|enter|eq|exit|float|floordiv|ge|get|getattr|getattribute|getitem|getslice|gt|hex|iadd|iand|idiv|ifloordiv|ilshift|invert|imod|import|imul|init|instancecheck|index|int|ior|ipow|irshift|isub|iter|itruediv|ixor|le|len|long|lshift|lt|missing|mod|mul|neg|nonzero|oct|or|pos|pow|radd|rand|rdiv|rdivmod|reversed|rfloordiv|rlshift|rmod|rmul|ror|rpow|rshift|rsub|rrshift|rtruediv|rxor|set|setattr|setitem|setslice|sub|subclasses|subclasscheck|truediv|unicode|xor)__))(?=\\s*\\()");
            rule.format = functionFormat;
            highlightingRules.append (rule);
        }
    }

    /**********************
     * Keywords and Types *
     **********************/

    /* keywords */
    QTextCharFormat keywordFormat;
    /* bash extra keywords */
    if (progLan == "sh" || progLan == "makefile" || progLan == "cmake")
    {
        if (progLan == "cmake")
        {
            keywordFormat.setForeground (Brown);
            rule.pattern = QRegExp ("\\$\\{\\s*[A-Za-z0-9_.+/\\?#\\-:]*\\s*\\}");
            rule.format = keywordFormat;
            highlightingRules.append (rule);

            keywordFormat.setForeground (DarkBlue);
            rule.pattern = QRegExp ("((^\\s*|[\\(\\);&`\\|]+\\s*)((if|then|elif|else|fi|while|do|done|esac)\\s+)*)(CMAKE_ARGC|CMAKE_ARGV0|CMAKE_AR|CMAKE_BINARY_DIR|CMAKE_BUILD_TOOL|CMAKE_CACHEFILE_DIR|CMAKE_CACHE_MAJOR_VERSION|CMAKE_CACHE_MINOR_VERSION|CMAKE_CACHE_PATCH_VERSION|CMAKE_CFG_INTDIR|CMAKE_COMMAND|CMAKE_CROSSCOMPILING|CMAKE_CTEST_COMMAND|CMAKE_CURRENT_BINARY_DIR|CMAKE_CURRENT_LIST_DIR|CMAKE_CURRENT_LIST_FILE|CMAKE_CURRENT_LIST_LINE|CMAKE_CURRENT_SOURCE_DIR|CMAKE_DL_LIBS|CMAKE_EDIT_COMMAND|CMAKE_EXECUTABLE_SUFFIX|CMAKE_EXTRA_GENERATOR|CMAKE_EXTRA_SHARED_LIBRARY_SUFFIXES|CMAKE_GENERATOR|CMAKE_GENERATOR_TOOLSET|CMAKE_HOME_DIRECTORY|CMAKE_IMPORT_LIBRARY_PREFIX|CMAKE_IMPORT_LIBRARY_SUFFIX|CMAKE_JOB_POOL_COMPILE|CMAKE_JOB_POOL_LINK|CMAKE_LINK_LIBRARY_SUFFIX|CMAKE_MAJOR_VERSION|CMAKE_MAKE_PROGRAM|CMAKE_MINIMUM_REQUIRED_VERSION|CMAKE_MINOR_VERSION|CMAKE_PARENT_LIST_FILE|CMAKE_PATCH_VERSION|CMAKE_PROJECT_NAME|CMAKE_RANLIB|CMAKE_ROOT|CMAKE_SCRIPT_MODE_FILE|CMAKE_SHARED_LIBRARY_PREFIX|CMAKE_SHARED_LIBRARY_SUFFIX|CMAKE_SHARED_MODULE_PREFIX|CMAKE_SHARED_MODULE_SUFFIX|CMAKE_SIZEOF_VOID_P|CMAKE_SKIP_INSTALL_RULES|CMAKE_SKIP_RPATH|CMAKE_SOURCE_DIR|CMAKE_STANDARD_LIBRARIES|CMAKE_STATIC_LIBRARY_PREFIX|CMAKE_STATIC_LIBRARY_SUFFIX|CMAKE_TOOLCHAIN_FILE|CMAKE_TWEAK_VERSION|CMAKE_VERBOSE_MAKEFILE|CMAKE_VERSION|CMAKE_VS_DEVENV_COMMAND|CMAKE_VS_INTEL_Fortran_PROJECT_VERSION|CMAKE_VS_MSBUILD_COMMAND|CMAKE_VS_MSDEV_COMMAND|CMAKE_VS_PLATFORM_TOOLSETCMAKE_XCODE_PLATFORM_TOOLSET|PROJECT_BINARY_DIR|PROJECT_NAME|PROJECT_SOURCE_DIR|PROJECT_VERSION|PROJECT_VERSION_MAJOR|PROJECT_VERSION_MINOR|PROJECT_VERSION_PATCH|PROJECT_VERSION_TWEAK|BUILD_SHARED_LIBS|CMAKE_ABSOLUTE_DESTINATION_FILES|CMAKE_APPBUNDLE_PATH|CMAKE_AUTOMOC_RELAXED_MODE|CMAKE_BACKWARDS_COMPATIBILITY|CMAKE_BUILD_TYPE|CMAKE_COLOR_MAKEFILE|CMAKE_CONFIGURATION_TYPES|CMAKE_DEBUG_TARGET_PROPERTIES|CMAKE_ERROR_DEPRECATED|CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION|CMAKE_SYSROOT|CMAKE_FIND_LIBRARY_PREFIXES|CMAKE_FIND_LIBRARY_SUFFIXES|CMAKE_FIND_NO_INSTALL_PREFIX|CMAKE_FIND_PACKAGE_WARN_NO_MODULE|CMAKE_FIND_ROOT_PATH|CMAKE_FIND_ROOT_PATH_MODE_INCLUDE|CMAKE_FIND_ROOT_PATH_MODE_LIBRARY|CMAKE_FIND_ROOT_PATH_MODE_PACKAGE|CMAKE_FIND_ROOT_PATH_MODE_PROGRAM|CMAKE_FRAMEWORK_PATH|CMAKE_IGNORE_PATH|CMAKE_INCLUDE_PATH|CMAKE_INCLUDE_DIRECTORIES_BEFORE|CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE|CMAKE_INSTALL_DEFAULT_COMPONENT_NAME|CMAKE_INSTALL_PREFIX|CMAKE_LIBRARY_PATH|CMAKE_MFC_FLAG|CMAKE_MODULE_PATH|CMAKE_NOT_USING_CONFIG_FLAGS|CMAKE_PREFIX_PATH|CMAKE_PROGRAM_PATH|CMAKE_SKIP_INSTALL_ALL_DEPENDENCY|CMAKE_STAGING_PREFIX|CMAKE_SYSTEM_IGNORE_PATH|CMAKE_SYSTEM_INCLUDE_PATH|CMAKE_SYSTEM_LIBRARY_PATH|CMAKE_SYSTEM_PREFIX_PATH|CMAKE_SYSTEM_PROGRAM_PATH|CMAKE_USER_MAKE_RULES_OVERRIDE|CMAKE_WARN_DEPRECATED|CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION|APPLE|BORLAND|CMAKE_CL_64|CMAKE_COMPILER_2005|CMAKE_HOST_APPLE|CMAKE_HOST_SYSTEM_NAME|CMAKE_HOST_SYSTEM_PROCESSOR|CMAKE_HOST_SYSTEM|CMAKE_HOST_SYSTEM_VERSION|CMAKE_HOST_UNIX|CMAKE_HOST_WIN32|CMAKE_LIBRARY_ARCHITECTURE_REGEX|CMAKE_LIBRARY_ARCHITECTURE|CMAKE_OBJECT_PATH_MAX|CMAKE_SYSTEM_NAME|CMAKE_SYSTEM_PROCESSOR|CMAKE_SYSTEM|CMAKE_SYSTEM_VERSION|CYGWIN|ENV|MSVC10|MSVC11|MSVC12|MSVC60|MSVC70|MSVC71|MSVC80|MSVC90|MSVC_IDE|MSVC|MSVC_VERSION|UNIX|WIN32|XCODE_VERSION|CMAKE_ARCHIVE_OUTPUT_DIRECTORY|CMAKE_AUTOMOC_MOC_OPTIONS|CMAKE_AUTOMOC|CMAKE_AUTORCC|CMAKE_AUTORCC_OPTIONS|CMAKE_AUTOUIC|CMAKE_AUTOUIC_OPTIONS|CMAKE_BUILD_WITH_INSTALL_RPATH|CMAKE_DEBUG_POSTFIX|CMAKE_EXE_LINKER_FLAGS|CMAKE_Fortran_FORMAT|CMAKE_Fortran_MODULE_DIRECTORY|CMAKE_GNUtoMS|CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE|CMAKE_INCLUDE_CURRENT_DIR|CMAKE_INSTALL_NAME_DIR|CMAKE_INSTALL_RPATH|CMAKE_INSTALL_RPATH_USE_LINK_PATH|CMAKE_LIBRARY_OUTPUT_DIRECTORY|CMAKE_LIBRARY_PATH_FLAG|CMAKE_LINK_DEF_FILE_FLAG|CMAKE_LINK_DEPENDS_NO_SHARED|CMAKE_LINK_INTERFACE_LIBRARIES|CMAKE_LINK_LIBRARY_FILE_FLAG|CMAKE_LINK_LIBRARY_FLAG|CMAKE_MACOSX_BUNDLE|CMAKE_MACOSX_RPATH|CMAKE_MODULE_LINKER_FLAGS|CMAKE_NO_BUILTIN_CHRPATH|CMAKE_NO_SYSTEM_FROM_IMPORTED|CMAKE_OSX_ARCHITECTURES|CMAKE_OSX_DEPLOYMENT_TARGET|CMAKE_OSX_SYSROOT|CMAKE_PDB_OUTPUT_DIRECTORY|CMAKE_POSITION_INDEPENDENT_CODE|CMAKE_RUNTIME_OUTPUT_DIRECTORY|CMAKE_SHARED_LINKER_FLAGS|CMAKE_SKIP_BUILD_RPATH|CMAKE_SKIP_INSTALL_RPATH|CMAKE_STATIC_LINKER_FLAGS|CMAKE_TRY_COMPILE_CONFIGURATION|CMAKE_USE_RELATIVE_PATHS|CMAKE_VISIBILITY_INLINES_HIDDEN|CMAKE_WIN32_EXECUTABLE|EXECUTABLE_OUTPUT_PATH|LIBRARY_OUTPUT_PATH|CMAKE_Fortran_MODDIR_DEFAULT|CMAKE_Fortran_MODDIR_FLAG|CMAKE_Fortran_MODOUT_FLAG|CMAKE_INTERNAL_PLATFORM_ABI)(?!(\\.|-|@|#|\\$))\\b");
            rule.format = keywordFormat;
            highlightingRules.append (rule);

            rule.pattern = QRegExp ("((^\\s*|[\\(\\);&`\\|]+\\s*)((if|then|elif|else|fi|while|do|done|esac)\\s+)*)[A-Za-z0-9_]+(_BINARY_DIR|_SOURCE_DIR|_VERSION|_VERSION_MAJOR|_VERSION_MINOR|_VERSION_PATCH|_VERSION_TWEAK)(?!(\\.|-|@|#|\\$))\\b");
            highlightingRules.append (rule);

            rule.pattern = QRegExp ("((^\\s*|[\\(\\);&`\\|]+\\s*)((if|then|elif|else|fi|while|do|done|esac)\\s+)*)(CMAKE_DISABLE_FIND_PACKAGE_|CMAKE_EXE_LINKER_FLAGS_|CMAKE_MAP_IMPORTED_CONFIG_|CMAKE_MODULE_LINKER_FLAGS_|CMAKE_PDB_OUTPUT_DIRECTORY_|CMAKE_SHARED_LINKER_FLAGS_|CMAKE_STATIC_LINKER_FLAGS_|CMAKE_COMPILER_IS_GNU)[A-Za-z0-9_]+(?!(\\.|-|@|#|\\$))\\b");
            highlightingRules.append (rule);

            rule.pattern = QRegExp ("((^\\s*|[\\(\\);&`\\|]+\\s*)((if|then|elif|else|fi|while|do|done|esac)\\s+)*)(CMAKE_)[A-Za-z0-9_]+(_POSTFIX|_VISIBILITY_PRESET|_ARCHIVE_APPEND|_ARCHIVE_CREATE|_ARCHIVE_FINISH|_COMPILE_OBJECT|_COMPILER_ABI|_COMPILER_ID|_COMPILER_LOADED|_COMPILER|_COMPILER_EXTERNAL_TOOLCHAIN|_COMPILER_TARGET|_COMPILER_VERSION|_CREATE_SHARED_LIBRARY|_CREATE_SHARED_MODULE|_CREATE_STATIC_LIBRARY|_FLAGS_DEBUG|_FLAGS_MINSIZEREL|_FLAGS_RELEASE|_FLAGS_RELWITHDEBINFO|_FLAGS|_IGNORE_EXTENSIONS|_IMPLICIT_INCLUDE_DIRECTORIES|_IMPLICIT_LINK_DIRECTORIES|_IMPLICIT_LINK_FRAMEWOR)(?!(\\.|-|@|#|\\$))\\b");
            highlightingRules.append (rule);

            rule.pattern = QRegExp ("((^\\s*|[\\(\\);&`\\|]+\\s*)((if|then|elif|else|fi|while|do|done|esac)\\s+)*)(CMAKE_PROJECT_)[A-Za-z0-9_]+(_INCLUDE)(?!(\\.|-|@|#|\\$))\\b");
            highlightingRules.append (rule);
        }
        keywordFormat.setFontWeight (QFont::Bold);
        keywordFormat.setForeground (Qt::magenta);
        rule.pattern = QRegExp ("((^\\s*|[\\(\\);&`\\|]+\\s*)((if|then|elif|else|fi|while|do|done|esac)\\s+)*)(adduser|addgroup|apropos|apt-get|aspell|awk|basename|bash|bc|bzip2|cal|cat|cd|cfdisk|chgrp|chmod|chown|chroot|chkconfig|cksum|clear|cmake|cmp|comm|cp|cron|crontab|csplit|cut|date|dc|dd|ddrescue|df|diff|diff3|dig|dir|dircolors|dirname|dirs|dmesg|dpkg|du|egrep|eject|env|ethtool|expect|expand|expr|fdformat|fdisk|fgrep|file|find|fmt|fold|format|free|fsck|ftp|function|fuser|gawk|git|grep|groups|gzip|head|hostname|id|ifconfig|ifdown|ifup|import|install|join|kdialog|kill|killall|less|ln|locate|logname|look|lpc|lpr|lprint|lprintd|lprintq|lprm|ls|lsof|make|man|mkdir|mkfifo|mkisofs|mknod|more|mount|mtools|mv|mmv|netstat|nice|nl|nohup|nslookup|open|op|passwd|paste|pathchk|ping|pkill|popd|pr|printcap|printenv|ps|pwd|qarma|qmake(-qt[3-9])*|quota|quotacheck|quotactl|ram|rcp|readarray|reboot|rename|renice|remsync|rev|rm|rmdir|rsync|screen|scp|sdiff|sed|seq|sftp|shutdown|sleep|slocate|sort|split|ssh|strace|su|sudo|sum|symlink|sync|tail|tar|tee|time|touch|top|traceroute|tr|tsort|tty|type|ulimit|umount|uname|unexpand|uniq|units|unshar|useradd|usermod|users|uuencode|uudecode|vdir|vi|vmstat|watch|wc|whereis|which|who|whoami|Wget|write|xargs|yad|yes|zenity)(?!\\.)(?!-)(?!(\\.|-|@|#|\\$))\\b");
        rule.format = keywordFormat;
        highlightingRules.append (rule);
    }
    else
        keywordFormat.setFontWeight (QFont::Bold);
    keywordFormat.setForeground (DarkBlue);

    /* types */
    QTextCharFormat typeFormat;
    typeFormat.setForeground (DarkMagenta); //(QColor (102, 0, 0));

    const QStringList keywordPatterns = keywords (lang);
    for (const QString &pattern : keywordPatterns)
    {
        rule.pattern = QRegExp (pattern);
        rule.format = keywordFormat;
        highlightingRules.append (rule);
    }

    if (progLan == "qmake")
    {
        QTextCharFormat qmakeFormat;
        /* qmake test functions */
        qmakeFormat.setForeground (DarkMagenta);
        rule.pattern = QRegExp ("\\b(cache|CONFIG|contains|count|debug|defined|equals|error|eval|exists|export|files|for|greaterThan|if|include|infile|isActiveConfig|isEmpty|isEqual|lessThan|load|log|message|mkpath|packagesExist|prepareRecursiveTarget|qtCompileTest|qtHaveModule|requires|system|touch|unset|warning|write_file)(?=\\s*\\()");
        rule.format = qmakeFormat;
        highlightingRules.append (rule);
        /* qmake paths */
        qmakeFormat.setForeground (Blue);
        rule.pattern = QRegExp ("\\${1,2}([A-Za-z0-9_]+|\\[[A-Za-z0-9_]+\\]|\\([A-Za-z0-9_]+\\))");
        rule.format = qmakeFormat;
        highlightingRules.append (rule);
    }

    const QStringList typePatterns = types();
    for (const QString &pattern : typePatterns)
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
            rule.pattern = QRegExp ("\\bQ[A-Za-z]+(?!(\\.|-|@|#|\\$))\\b");
        else
            rule.pattern = QRegExp ("\\bG[A-Za-z]+(?!(\\.|-|@|#|\\$))\\b");
        rule.format = cFormat;
        highlightingRules.append (rule);

        /* QtGlobal functions and enum Qt::GlobalColor */
        if (progLan == "cpp")
        {
            cFormat.setFontItalic (true);
            rule.pattern = QRegExp ("\\bq(App)(?!(\\@|#|\\$))\\b|\\bq(Abs|Bound|Critical|Debug|Fatal|FuzzyCompare|InstallMsgHandler|MacVersion|Max|Min|Round64|Round|Version|Warning|getenv|putenv|rand|srand|tTrId|_check_ptr|t_set_sequence_auto_mnemonic|t_symbian_exception2Error|t_symbian_exception2LeaveL|t_symbian_throwIfError)(?!(\\.|-|@|#|\\$))\\b");
            rule.format = cFormat;
            highlightingRules.append (rule);
            cFormat.setFontItalic (false);

            cFormat.setForeground (Qt::magenta);
            rule.pattern = QRegExp ("\\bQt\\s*::\\s*(white|black|red|darkRed|green|darkGreen|blue|darkBlue|cyan|darkCyan|magenta|darkMagenta|yellow|darkYellow|gray|darkGray|lightGray|transparent|color0|color1)(?!(\\.|-|@|#|\\$))\\b");
            rule.format = cFormat;
            highlightingRules.append (rule);
        }

        /* preprocess */
        cFormat.setForeground (Blue);
        rule.pattern = QRegExp ("^\\s*#\\s*include\\s|^\\s*#\\s*ifdef\\s|^\\s*#\\s*elif\\s|^\\s*#\\s*ifndef\\s|^\\s*#\\s*endif\\b|^\\s*#\\s*define\\s|^\\s*#\\s*undef\\s|^\\s*#\\s*error\\s|^\\s*#\\s*if\\s|^\\s*#\\s*else(?!(\\.|-|@|#|\\$))\\b");
        rule.format = cFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "python")
    {
        QTextCharFormat pFormat;
        pFormat.setFontWeight (QFont::Bold);
        pFormat.setForeground (DarkMagenta);
        rule.pattern = QRegExp ("\\bself(?!(@|\\$))\\b");
        rule.format = pFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "qml")
    {
        QTextCharFormat qmlFormat;
        qmlFormat.setFontWeight (QFont::Bold);
        qmlFormat.setForeground (DarkMagenta);
        rule.pattern = QRegExp ("\\b(Qt[A-Za-z]+|Accessible|AnchorAnimation|AnchorChanges|AnimatedImage|AnimatedSprite|Animation|AnimationController|Animator|Behavior|BorderImage|Canvas|CanvasGradient|CanvasImageData|CanvasPixelArray|ColorAnimation|Column|Context2D|DoubleValidator|Drag|DragEvent|DropArea|EnterKey|Flickable|Flipable|Flow|FocusScope|FontLoader|FontMetrics|Gradient|GradientStop|Grid|GridMesh|GridView|Image|IntValidator|Item|ItemGrabResult|KeyEvent|KeyNavigation|Keys|LayoutMirroring|ListView|Loader|Matrix4x4|MouseArea|MouseEvent|MultiPointTouchArea|NumberAnimation|OpacityAnimator|OpenGLInfo|ParallelAnimation|ParentAnimation|ParentChange|Path|PathAnimation|PathArc|PathAttribute|PathCubic|PathCurve|PathElement|PathInterpolator|PathLine|PathPercent|PathQuad|PathSvg|PathView|PauseAnimation|PinchArea|PinchEvent|Positioner|PropertyAction|PropertyAnimation|PropertyChanges|Rectangle|RegExpValidator|Repeater|Rotation|RotationAnimation|RotationAnimator|Row|Scale|ScaleAnimator|ScriptAction|SequentialAnimation|ShaderEffect|ShaderEffectSource|Shortcut|SmoothedAnimation|SpringAnimation|Sprite|SpriteSequence|State|StateChangeScript|StateGroup|SystemPalette|Text|TextEdit|TextInput|TextMetrics|TouchPoint|Transform|Transition|Translate|UniformAnimator|Vector3dAnimation|ViewTransition|WheelEvent|XAnimator|YAnimator|CloseEvent|ColorDialog|ColumnLayout|Dialog|FileDialog|FontDialog|GridLayout|Layout|MessageDialog|RowLayout|StackLayout|LocalStorage|Screen|SignalSpy|TestCase|Window|XmlListModel|XmlRole|Action|ApplicationWindow|BusyIndicator|Button|Calendar|CheckBox|ComboBox|ExclusiveGroup|GroupBox|Label|Menu|MenuBar|MenuItem|MenuSeparator|ProgressBar|RadioButton|ScrollView|Slider|SpinBox|SplitView|Stack|StackView|StackViewDelegate|StatusBar|Switch|Tab|TabView|TableView|TableViewColumn|TextArea|TextField|ToolBar|ToolButton|TreeView|Affector|Age|AngleDirection|Attractor|CumulativeDirection|CustomParticle|Direction|EllipseShape|Emitter|Friction|Gravity|GroupGoal|ImageParticle|ItemParticle|LineShape|MaskShape|Particle|ParticleGroup|ParticlePainter|ParticleSystem|PointDirection|RectangleShape|Shape|SpriteGoal|TargetDirection|TrailEmitter|Turbulence|Wander|Timer)(?!(\\-|@|#|\\$))\\b");
        rule.format = qmlFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "xml")
    {
        QTextCharFormat xmlElementFormat;
        xmlElementFormat.setFontWeight (QFont::Bold);
        xmlElementFormat.setForeground (Violet);
        /* after </ or before /> */
        rule.pattern = QRegExp ("\\s*</?[A-Za-z0-9_\\-:]+|\\s*<!(DOCTYPE|ENTITY)\\s|\\s*/?>");
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
            /* make parentheses and ; neutral as they were in keyword patterns */
            rule.pattern = QRegExp ("[\\(\\);]");
            rule.format = neutralFormat;
            highlightingRules.append (rule);

            shFormat.setForeground (Blue);
            /* words before = */
             if (progLan == "sh")
                 rule.pattern = QRegExp ("\\b[A-Za-z0-9_]+(?=\\=)");
             else
                 rule.pattern = QRegExp ("\\b[A-Za-z0-9_]+\\s*(?=\\+{,1}\\=)");
            rule.format = shFormat;
            highlightingRules.append (rule);

            /* but don't format a word before =
               if it follows a dash */
            rule.pattern = QRegExp ("-+[^\\s\\\"\\\']+(?=\\=)");
            rule.format = neutralFormat;
            highlightingRules.append (rule);
        }

        if (progLan == "makefile" || progLan == "cmake")
        {
            shFormat.setForeground (DarkYellow);
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
            rule.pattern = QRegExp ("\\s+\\[{1,2}\\s+|^\\[{1,2}\\s+|\\s+\\]{1,2}\\s+|\\s+\\]{1,2}$|\\s+\\]{1,2}\\s*(?=;)");
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
        diffLinesFormat.setForeground (DarkGreenAlt);
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
        logTimeFormat.setForeground (DarkGreenAlt);
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
        srtFormat.setForeground (DarkGreenAlt);
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
    else if (progLan == "desktop" || progLan == "config" || progLan == "theme")
    {
        QTextCharFormat desktopFormat;
        if (progLan == "config")
        {
            desktopFormat.setFontWeight (QFont::Bold);
            desktopFormat.setFontItalic (true);
            /* color values */
            rule.pattern = QRegExp ("#([A-Fa-f0-9]{3}){,2}(?![A-Za-z0-9_]+)|#([A-Fa-f0-9]{3}){2}[A-Fa-f0-9]{2}(?![A-Za-z0-9_]+)");
            rule.format = desktopFormat;
            highlightingRules.append (rule);
            desktopFormat.setFontItalic (false);
            desktopFormat.setFontWeight (QFont::Normal);
        }

        desktopFormat.setForeground (DarkMagenta);
        rule.pattern = QRegExp ("^[^\\=]+=|^[^\\=]+\\[.*\\]=|;|/|%|\\+|-");
        rule.format = desktopFormat;
        highlightingRules.append (rule);

        desktopFormat.setForeground (QBrush());
        desktopFormat.setFontWeight (QFont::Bold);
        /* [...] */
        rule.pattern = QRegExp ("^\\[.*\\]$");
        rule.format = desktopFormat;
        highlightingRules.append (rule);

        desktopFormat.setForeground (Blue);
        /* [...] and before = (like ...[en]=)*/
        rule.pattern = QRegExp ("^[^\\=]+\\[.*\\](?=\\s*\\=)");
        rule.format = desktopFormat;
        highlightingRules.append (rule);

        desktopFormat.setForeground (DarkGreenAlt);
        /* before = and [] */
        rule.pattern = QRegExp ("^[^\\=\\[]+(?=(\\[.*\\])*\\s*\\=)");
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
        QTextCharFormat gtkrcFormat;
        gtkrcFormat.setFontWeight (QFont::Bold);
        /* color value format (#xyz) */
        /*gtkrcFormat.setForeground (DarkGreenAlt);
        rule.pattern = QRegExp ("#([A-Fa-f0-9]{3}){,2}(?![A-Za-z0-9_]+)|#([A-Fa-f0-9]{3}){2}[A-Fa-f0-9]{2}(?![A-Za-z0-9_]+)");
        rule.format = gtkrcFormat;
        highlightingRules.append (rule);*/

        gtkrcFormat.setForeground (Blue);
        rule.pattern = QRegExp ("(fg|bg|base|text)(\\[NORMAL\\]|\\[PRELIGHT\\]|\\[ACTIVE\\]|\\[SELECTED\\]|\\[INSENSITIVE\\])");
        rule.format = gtkrcFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "markdown")
    {
        quoteMark = QRegExp ("`"); // inline code is almost like a single-line quote
        blockQuoteFormat.setForeground (DarkGreen);

        QTextCharFormat markdownFormat;

        /* italic */
        markdownFormat.setFontItalic (true);
        rule.pattern = QRegExp ("\\*[^\\*\\s]\\*|\\*[^\\*\\s][^\\*]*[^\\*\\s]\\*"
                                "|"
                                "_[^_\\s]_|_[^_\\s][^_]*[^_\\s]_");
        rule.format = markdownFormat;
        highlightingRules.append (rule);
        markdownFormat.setFontItalic (false);

        /* bold */
        markdownFormat.setFontWeight (QFont::Bold);
        rule.pattern = QRegExp ("\\*{2}[^\\*\\s]\\*{2}|\\*{2}[^\\*\\s][^\\*]*[^\\*\\s]\\*{2}"
                                "|"
                                "_{2}[^_\\s]_{2}|_{2}[^_\\s][^_]*[^_\\s]_{2}");
        rule.format = markdownFormat;
        highlightingRules.append (rule);

        /* lists */
        markdownFormat.setForeground (DarkBlue);
        rule.pattern = QRegExp ("^ {,3}(\\*|\\+|\\-|[0-9]+\\.|[0-9]+\\))\\s+");
        rule.format = markdownFormat;
        highlightingRules.append (rule);

        /* footnotes */
        markdownFormat.setFontItalic (true);
        rule.pattern = QRegExp ("\\[\\^.+\\]");
        rule.format = markdownFormat;
        highlightingRules.append (rule);
        markdownFormat.setFontItalic (false);

        /* horizontal rules */
        markdownFormat.setForeground (DarkMagenta);
        rule.pattern = QRegExp ("^ {,3}(\\* {,2}){3,}\\s*$"
                                "|"
                                "^ {,3}(- {,2}){3,}\\s*$"
                                "|"
                                "^ {,3}(\\= {,2}){3,}\\s*$");
        rule.format = markdownFormat;
        highlightingRules.append (rule);

        /*
           links:
           [link text] [link]
           [link text] (http://example.com "Title")
           [link text]: http://example.com
           <http://example.com>
        */
        rule.pattern = QRegExp ("\\[[^\\]\\^]*\\]\\s*\\[[^\\]\\s]*\\]"
                                "|"
                                "\\[[^\\]\\^]*\\]\\s*\\(\\s*[^\\)\\(\\s]+(\\s+\\\".*\\\")*\\s*\\)"
                                "|"
                                "\\[[^\\]\\^]*\\]:\\s+\\s*[^\\)\\(\\s]+(\\s+\\\".*\\\")*"
                                "|"
                                "<([A-Za-z0-9_]+://[A-Za-z0-9_.+/\\?\\=~&%#\\-:]+|[A-Za-z0-9_.\\-]+@[A-Za-z0-9_\\-]+\\.[A-Za-z0-9.]+)>");
        rule.format = urlFormat;
        highlightingRules.append (rule);

        /*
           images:
           ![image](image.jpg "An image")
           ![Image][1]
           [1]: /path/to/image "alt text"
        */
        markdownFormat.setFontWeight (QFont::Normal);
        markdownFormat.setForeground (Violet);
        markdownFormat.setFontUnderline (true);
        rule.pattern = QRegExp ("\\!\\[[^\\]\\^]*\\]\\s*"
                                "(\\(\\s*[^\\)\\(\\s]+(\\s+\\\".*\\\")*\\s*\\)|\\s*\\[[^\\]]*\\])");
        rule.format = markdownFormat;
        highlightingRules.append (rule);
        markdownFormat.setFontUnderline (false);

        /* code blocks */
        codeBlockFormat.setForeground (DarkRed);
        rule.pattern = QRegExp ("^( {4,}|\\s*\\t+\\s*).*");
        rule.format = codeBlockFormat;
        highlightingRules.append (rule);

        /* headings */
        markdownFormat.setFontWeight (QFont::Bold);
        markdownFormat.setForeground (Blue);
        rule.pattern = QRegExp ("^#+\\s+.*");
        rule.format = markdownFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "lua")
    {
        QTextCharFormat luaFormat;
        luaFormat.setFontWeight (QFont::Bold);
        luaFormat.setFontItalic (true);
        luaFormat.setForeground (DarkMagenta);
        rule.pattern = QRegExp ("\\bos(?=\\.)");
        rule.format = luaFormat;
        highlightingRules.append (rule);
    }
    else if (progLan == "m3u")
    {
        QTextCharFormat plFormat;
        plFormat.setFontWeight (QFont::Bold);
        rule.pattern = QRegExp ("^#EXTM3U\\b");
        rule.format = plFormat;
        highlightingRules.append (rule);

        /* after "," */
        plFormat.setFontWeight (QFont::Normal);
        plFormat.setForeground (DarkRed);
        rule.pattern = QRegExp ("^#EXTINF\\s*:\\s*-*[0-9]+\\s*,.*|^#EXTINF\\s*:\\s*,.*");
        rule.format = plFormat;
        highlightingRules.append (rule);

        /* before "," and after "EXTINF:" */
        plFormat.setForeground (DarkYellow);
        rule.pattern = QRegExp ("^#EXTINF\\s*:\\s*-*[0-9]+\\b");
        rule.format = plFormat;
        highlightingRules.append (rule);

        plFormat.setForeground (QBrush());
        rule.pattern = QRegExp ("^#EXTINF\\s*:");
        rule.format = plFormat;
        highlightingRules.append (rule);

        plFormat.setForeground (DarkGreen);
        rule.pattern = QRegExp ("^#EXTINF\\b");
        rule.format = plFormat;
        highlightingRules.append (rule);
    }

    if (showWhiteSpace)
    {
        whiteSpaceFormat.setForeground (Faded);
        rule.pattern = QRegExp ("\\s+");
        rule.format = whiteSpaceFormat;
        highlightingRules.append (rule);
    }

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
    else if (progLan == "python"
             || progLan == "sourceslist" || progLan == "qmake"
             || progLan == "gtkrc")
    {
        rule.pattern = QRegExp ("#.*"); // or "#[^\n]*"
    }
    else if (progLan == "desktop" || progLan == "config")
    {
        rule.pattern = QRegExp ("^\\s*#.*"); // only at start
    }
    else if (progLan == "deb")
    {
        rule.pattern = QRegExp ("^#[^\\s:]+:(?=\\s*)");
    }
    else if (progLan == "m3u")
    {
        rule.pattern = QRegExp ("^\\s+#|^#(?!(EXTM3U|EXTINF))");
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
    else if (progLan == "perl")
    {
        commentStartExpression = QRegExp ("^=[A-Za-z0-9_]+($|\\s+)");
        commentEndExpression = QRegExp ("^=cut.*");
    }
    else if (progLan == "markdown")
    {
        quoteFormat.setForeground (DarkRed); // not a quote but a code block
        commentStartExpression = QRegExp ("<!--");
        commentEndExpression = QRegExp ("-->");
    }
}
/*************************/
Highlighter::~Highlighter()
{
    if (QTextDocument *doc = document())
    {
        QTextOption opt =  doc->defaultTextOption();
        opt.setFlags (opt.flags() & ~QTextOption::ShowTabsAndSpaces
                                  & ~QTextOption::ShowLineAndParagraphSeparators
#if QT_VERSION >= 0x050700
                                  & ~QTextOption::ShowDocumentTerminator
#endif
                                  );
        doc->setDefaultTextOption (opt);
    }
}
/*************************/
// Check if a start or end quotation mark (positioned at "pos") is escaped.
bool Highlighter::isEscapedQuote (const QString &text, const int pos, bool isStartQuote)
{
    if (pos < 0) return false;

    if (progLan == "html" || progLan == "xml")
        return false;

    if (pos != quoteMark.indexIn (text, pos)
        && (progLan == "markdown" || pos != QRegExp ("\'").indexIn (text, pos)))
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

    /* escaped start quotes are just for Bash, Perl and markdown */
    if (isStartQuote
        && progLan != "sh" && progLan != "makefile" && progLan != "cmake"
        && progLan != "perl" && progLan != "markdown")
    {
        return false;
    }

    /* in Perl, $' has a (deprecated?) meaning */
    if (progLan == "perl" && pos >= 1 && pos - 1 == QRegExp ("\\$").indexIn (text, pos - 1))
        return true;

    int i = 0;
    while (pos - i >= 1 && pos - i - 1 == QRegExp ("\\\\").indexIn (text, pos - i - 1))
        ++i;
    /* only an odd number of backslashes means that the quote is escaped */
    if (
        i % 2 != 0
            /* for perl, only double quote can be escaped? */
        && (/*(progLan == "perl"
             && pos == quoteMark.indexIn (text, pos)) ||*/
            /* for these languages, both single and double quotes can be escaped */
            progLan == "cpp" || progLan == "c"
            || progLan == "python"
            || progLan == "perl"
            /* markdown is an exception */
            || progLan == "markdown"
            /* however, in Bash, single quote can be escaped only at start */
            || ((progLan == "sh" || progLan == "makefile" || progLan == "cmake")
                && (isStartQuote || pos == quoteMark.indexIn (text, pos))))
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
    bool mixedQuotes = false;
    if (progLan == "c" || progLan == "cpp"
        || progLan == "python" || progLan == "sh"
        || progLan == "makefile" || progLan == "cmake"
        || progLan == "lua" || progLan == "perl" || progLan == "xml"
        || progLan == "ruby" || progLan == "html" || progLan == "javascript")
    {
        mixedQuotes = true;
    }
    QRegExp quoteExpression;
    if (mixedQuotes)
        quoteExpression = QRegExp ("\"|\'");
    else
        quoteExpression = quoteMark;
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
        if (mixedQuotes)
        {
            if (previousBlockState() == doubleQuoteState
                || previousBlockState() == htmlStyleDoubleQuoteState)
                quoteExpression = quoteMark;
            else
                quoteExpression = QRegExp ("\'");
        }
    }

    while ((pos = quoteExpression.indexIn (text, pos + 1)) >= 0)
    {
        /* skip formatted comments */
        if (format (pos) == commentFormat) continue;

        ++N;
        if ((N % 2 == 0 && isEscapedQuote (text, pos, false)) // an escaped end quote
            || isEscapedQuote (text, pos, true)) // or an escaped start quote in Perl or Bash
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

        /* "pos" might be negative next time */
        if (N % 2 == 0) res = false;
        else res = true;

        if (mixedQuotes)
        {
            if (N % 2 != 0)
            { // each quote neutralizes the other until it's closed
                if (pos == quoteMark.indexIn (text, pos))
                    quoteExpression = quoteMark;
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
// Checks if a character is inside a multiline comment
// (works only with real comments whose state is commentState).
bool Highlighter::isMLCommented (const QString &text, const int index)
{
    /* not for Python */
    if (progLan == "python") return false;

    if (index < 0 || commentStartExpression.isEmpty())
        return false;

    if (previousBlockState() == nextLineCommentState)
        return true; // see singleLineComment()

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
        if (format (pos) == quoteFormat
            || format (pos) == altQuoteFormat)
        {
            continue;
        }

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

        while (format (index) == quoteFormat
               || format (index) == altQuoteFormat)
        {
            index = commentStartExpression.indexIn (text, index + 3);
        }
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
            if (index == quoteMark.indexIn (text, index))
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
        while (format (index) == quoteFormat
               || format (index) == altQuoteFormat)
        {
            index = commentStartExpression.indexIn (text, index + 3);
        }
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
    QTextCharFormat cssValueFormat;
    cssValueFormat.setFontItalic (true);
    cssValueFormat.setForeground (DarkGreenAlt);
    QTextCharFormat cssErrorFormat;
    cssErrorFormat.setFontUnderline (true);
    cssErrorFormat.setForeground (Red);
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
            setFormat (indxTmp, length, cssErrorFormat);
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
            while (format (index) != cssErrorFormat)
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
        setFormat (index, cssLength, cssValueFormat);

        setFormat (index, startMatch, neutralFormat);
        if (endIndex > -1)
            setFormat (endIndex, 1, neutralFormat);

        index = cssStartExpression.indexIn (text, index + cssLength);
        if (index > -1)
        {
            while (format (index) != cssErrorFormat)
            {
                index = cssStartExpression.indexIn (text, index + 1);
                if (index == -1) break;
            }
        }
    }

    /* color value format (#xyz, #abcdef, #abcdefxy) */
    QTextCharFormat cssColorFormat;

    cssColorFormat.setForeground (DarkGreenAlt);
    cssColorFormat.setFontWeight (QFont::Bold);
    cssColorFormat.setFontItalic (true);
    // previously: "#\\b([A-Za-z0-9]{3}){,4}(?![A-Za-z0-9_]+)"
    QRegExp expression = QRegExp ("#([A-Fa-f0-9]{3}){,2}(?![A-Za-z0-9_]+)|#([A-Fa-f0-9]{3}){2}[A-Fa-f0-9]{2}(?![A-Za-z0-9_]+)");
    int indxTmp = expression.indexIn (text);
    while (isQuoted (text, indxTmp))
        indxTmp = expression.indexIn (text, indxTmp + 1);
    while (indxTmp >= 0)
    {
        int length = expression.matchedLength();
        if (/*format (indxTmp) == cssValueFormat // should be a value
            &&*/ format (indxTmp) != cssErrorFormat) // not an error
        {
            setFormat (indxTmp, length, cssColorFormat);
        }
        indxTmp = expression.indexIn (text, indxTmp + length);
    }

    /* definitions (starting with @) */
    QTextCharFormat cssDefinitionFormat;
    cssDefinitionFormat.setForeground (Brown);
    expression = QRegExp ("^\\s*@[A-Za-z-]+\\s+|;\\s*@[A-Za-z-]+\\s+");
    indxTmp = expression.indexIn (text);
    while (isQuoted (text, indxTmp))
        indxTmp = expression.indexIn (text, indxTmp + 1);
    while (indxTmp >= 0)
    {
        int length = expression.matchedLength();
        if (format (indxTmp) != cssValueFormat
            && format (indxTmp) != cssErrorFormat)
        {
            if (text.at (indxTmp) == ';')
            {
                ++indxTmp;
                --length;
            }
            setFormat (indxTmp, length, cssDefinitionFormat);
        }
        indxTmp = expression.indexIn (text, indxTmp + length);
    }

    return cssIndx;
}
/*************************/
// "canBeQuoted" and "end" are used with comments in double quoted bash commands.
void Highlighter::singleLineComment (const QString &text, int start, int end, bool canBeQuoted)
{
    QRegExp urlPattern = QRegExp ("[A-Za-z0-9_]+://[A-Za-z0-9_.+/\\?\\=~&%#\\-:]+|[A-Za-z0-9_.\\-]+@[A-Za-z0-9_\\-]+\\.[A-Za-z0-9.]+");
    QRegExp notePattern = QRegExp ("\\b(NOTE|TODO|FIXME|WARNING)\\b");
    int pIndex = 0;
    QTextCharFormat noteFormat;
    noteFormat.setFontWeight (QFont::Bold);
    noteFormat.setFontItalic (true);
    noteFormat.setForeground (DarkRed);

    for (const HighlightingRule &rule : static_cast<const QVector<HighlightingRule>&>(highlightingRules))
    {
        if (rule.format == commentFormat)
        {
            if (progLan == "html") // javascript end
                end = QRegExp ("</script\\s*>").indexIn (text, start);
            if (end == -1) end = text.length();
            QRegExp expression (rule.pattern);
            if (previousBlockState() == nextLineCommentState)
                start = 0;
            else
            {
                start = expression.indexIn (text, start);
                if (!canBeQuoted)
                { // skip quoted comments
                    while (start >= 0 && start < end
                           && isQuoted (text, start))
                    {
                        start = expression.indexIn (text, start + 1);
                    }
                }
            }
            if (start >= 0 && start < end)
            {
                setFormat (start, end - start, commentFormat);

                /* also format urls and email addresses inside the comment */
                QString str = text.mid (start, end - start);
                int indx = 0;
                while ((pIndex = str.indexOf (urlPattern, indx)) > -1)
                {
                    int ml = urlPattern.matchedLength();
                    setFormat (pIndex + start, ml, urlFormat);
                    indx = indx + ml;
                }
                /* format note patterns too */
                indx = 0;
                while ((pIndex = str.indexOf (notePattern, indx)) > -1)
                {
                    int ml = notePattern.matchedLength();
                    if (format (pIndex) != urlFormat)
                      setFormat (pIndex + start, ml, noteFormat);
                    indx = indx + ml;
                }
                /* take care of next-line comments with languages, for which
                   no highlighting function is called after singleLineComment()
                   and before the main formaatting in highlightBlock()
                   (only c and c++ for now) */
                if ((progLan == "c" || progLan == "cpp")
                    && text.endsWith (QLatin1Char('\\')))
                {
                    setCurrentBlockState (nextLineCommentState);
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
                                    int commState,
                                    QTextCharFormat comFormat)
{
    if (previousBlockState() == nextLineCommentState)
        return;  // was processed by singleLineComment()

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
        /* skip quotations (usually all formatted to this point) */
        while (format (index) == quoteFormat
               || format (index) == altQuoteFormat)
        {
            index = commentStartExp.indexIn (text, index + 1);
        }
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
        /* when the comment starts is in the prvious line
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
        while (format (endIndex) == quoteFormat
               || format (endIndex) == altQuoteFormat)
        {
            endIndex = commentEndExp.indexIn (text, endIndex + 1);
        }

        /* if there's a comment end ... */
        if (endIndex >= 0)
        {
            /* ... clear the comment format from there to reformat later as
               a single-line comment sign may have been commented out now */
            badIndex = endIndex + 1;
            for (int i = badIndex; i < text.length(); ++i)
            {
                if (format (i) == commentFormat)
                    setFormat (i, 1, neutralFormat);
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
                setFormat (endIndex + 1, text.length() - endIndex - 1, neutralFormat);
            }
            commentLength = endIndex - index
                            + commentEndExp.matchedLength();
        }
        setFormat (index, commentLength, comFormat);

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
            for (const HighlightingRule &rule : static_cast<const QVector<HighlightingRule>&>(highlightingRules))
            {
                if (rule.format == commentFormat)
                {
                    QRegExp expression (rule.pattern);
                    int INDX = expression.indexIn (text, badIndex);
                    while (format (INDX) == quoteFormat
                           || format (INDX) == altQuoteFormat
                           || isMLCommented (text, INDX))
                    {
                        INDX = expression.indexIn (text, INDX + 1);
                    }
                    if (INDX >= 0)
                        setFormat (INDX, text.length() - INDX, commentFormat);
                    break;
                }
            }
        }

        /* skip single-line comments and quotations again */
        if (format (index) == commentFormat)
            index = -1;
        while (format (index) == quoteFormat
               || format (index) == altQuoteFormat)
        {
            index = commentStartExp.indexIn (text, index + 1);
        }
    }

    /* reset the block state if this line created a next-line comment
       whose starting single-line comment sign is commented out now */
    if (currentBlockState() == nextLineCommentState
        && format (text.size() - 1) != commentFormat)
    {
        setCurrentBlockState (0);
    }
}
/*************************/
// Handles escaped backslashes too.
bool Highlighter::textEndsWithBackSlash (const QString &text)
{
    QString str = text;
    int n = 0;
    while (!str.isEmpty() && str.endsWith ("\\"))
    {
        str.truncate(str.size() - 1);
        ++n;
    }
    return (n % 2 != 0);
}
/*************************/
// This covers single-line quotes too
// and comes before comments are formatted.
void Highlighter::multiLineQuote (const QString &text)
{
    int index = 0;
    bool mixedQuotes = false;
    if (progLan == "c" || progLan == "cpp"
        || progLan == "python"
        /*|| progLan == "sh"*/ // bash uses SH_MultiLineQuote()
        || progLan == "makefile" || progLan == "cmake"
        || progLan == "lua" || progLan == "perl"
        || progLan == "ruby" || progLan == "javascript")
    {
        mixedQuotes = true;
    }
    QRegExp quoteExpression;
    if (mixedQuotes)
        quoteExpression = QRegExp ("\"|\'");
    else
        quoteExpression = quoteMark;
    int quote = doubleQuoteState;

    /* find the start quote */
    if (previousBlockState() != doubleQuoteState
        && previousBlockState() != singleQuoteState)
    {
        index = quoteExpression.indexIn (text);
        /* skip escaped start quotes and all comments */
        while (isEscapedQuote (text, index, true)
               || isMLCommented (text, index)) // multiline
        {
            index = quoteExpression.indexIn (text, index + 1);
        }
        while (format (index) == commentFormat) // single-line and Python
            index = quoteExpression.indexIn (text, index + 1);

        /* if the start quote is found... */
        if (index >= 0)
        {
            if (mixedQuotes)
            {
                /* ... distinguish between double and single quotes */
                if (index == quoteMark.indexIn (text, index))
                {
                    quoteExpression = quoteMark;
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
        if (mixedQuotes)
        {
            quote = previousBlockState();
            if (quote == doubleQuoteState)
                quoteExpression = quoteMark;
            else
                quoteExpression = QRegExp ("\'");
        }
    }

    while (index >= 0)
    {
        /* if the search is continued... */
        if (quoteExpression == QRegExp ("\"|\'"))
        {
            /* ... distinguish between double and single quotes
               again because the quote mark may have changed */
            if (index == quoteMark.indexIn (text, index))
            {
                quoteExpression = quoteMark;
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
        while (isEscapedQuote (text, endIndex, false))
            endIndex = quoteExpression.indexIn (text, endIndex + 1);

        bool isQuotation = true;
        if (endIndex == -1)
        {
            if (progLan == "c" || progLan == "cpp")
            {
                /* in c and cpp, multiline double quotes need backslash
                   and there's no multiline single quote */
                if (quoteExpression == QRegExp ("\'")
                    || (quoteExpression == quoteMark && !textEndsWithBackSlash (text)))
                {
                    endIndex = text.size() + 1; // quoteExpression.matchedLength() is -1 here
                }
            }
            else if (progLan == "markdown")
            { // this is the main differenct of a markdown inline code from a single-line quote
                isQuotation = false;
            }
        }
        else if (endIndex == index + 1 && progLan == "markdown")
        { //  don't format `` because of ``` for code block
            isQuotation = false;
        }

        int quoteLength;
        if (endIndex == -1)
        {
            if (isQuotation)
                setCurrentBlockState (quote);
            quoteLength = text.length() - index;
        }
        else
            quoteLength = endIndex - index
                          + quoteExpression.matchedLength(); // 1
        if (isQuotation)
            setFormat (index, quoteLength, quoteExpression == quoteMark ? quoteFormat
                                                                        : altQuoteFormat);

        /* the next quote may be different */
        if (mixedQuotes)
            quoteExpression = QRegExp ("\"|\'");
        index = quoteExpression.indexIn (text, index + quoteLength);

        /* skip escaped start quotes and all comments */
        while (isEscapedQuote (text, index, true)
               || isMLCommented (text, index))
        {
            index = quoteExpression.indexIn (text, index + 1);
        }
        while (format (index) == commentFormat)
            index = quoteExpression.indexIn (text, index + 1);
    }
}
/*************************/
// Generalized form of setFormat(), where "oldFormat" shouldn't be reformatted.
void Highlighter::setFormatWithoutOverwrite (int start,
                                             int count,
                                             const QTextCharFormat &newFormat,
                                             const QTextCharFormat &oldFormat)
{
    int index = start;
    int indx;
    while (index < start + count)
    {
        while (index < start + count
               && (format (index) == oldFormat
                   /* skip comments and quotes */
                   || format (index) == commentFormat
                   || format (index) == quoteFormat
                   || format (index) == altQuoteFormat))
        {
            ++ index;
        }
        if (index < start + count)
        {
            indx = index;
            while (indx < start + count
                   && format (indx) != oldFormat
                   && format (indx) != commentFormat
                   && format (indx) != quoteFormat
                   && format (indx) != altQuoteFormat)
            {
                ++ indx;
            }
            setFormat (index, indx - index , newFormat);
            index = indx;
        }
    }
}
/*************************/
// XML quotes are handled as multiline quotes for
// possible XML doc mistakes to be seen easily.
// This comes after values are formatted.
void Highlighter::xmlQuotes (const QString &text)
{
    int index = 0;
    /* mixed quotes aren't really needed here
       but they're harmless and easy to handle */
    QRegExp quoteExpression = QRegExp ("\"|\'");
    int quote = doubleQuoteState;

    /* find the start quote */
    if (previousBlockState() != doubleQuoteState
        && previousBlockState() != singleQuoteState)
    {
        index = quoteExpression.indexIn (text);
        /* skip all comments */
        while (format (index) == commentFormat)
            index = quoteExpression.indexIn (text, index + 1);
        /* skip all values (that are formatted by multiLineComment()) */
        while (format (index) == neutralFormat)
            index = quoteExpression.indexIn (text, index + 1);

        /* if the start quote is found... */
        if (index >= 0)
        {
            /* ... distinguish between double and single quotes */
            if (index == quoteMark.indexIn (text, index))
            {
                quoteExpression = quoteMark;
                quote = doubleQuoteState;
            }
            else
            {
                quoteExpression = QRegExp ("\'");
                quote = singleQuoteState;
            }
        }
    }
    else // but if we're inside a quotation...
    {
        /* ... distinguish between the two quote kinds
           by checking the previous line */
        quote = previousBlockState();
        if (quote == doubleQuoteState)
            quoteExpression = quoteMark;
        else
            quoteExpression = QRegExp ("\'");
    }

    while (index >= 0)
    {
        /* if the search is continued... */
        if (quoteExpression == QRegExp ("\"|\'"))
        {
            /* ... distinguish between double and single quotes
               again because the quote mark may have changed */
            if (index == quoteMark.indexIn (text, index))
            {
                quoteExpression = quoteMark;
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

        int quoteLength;
        if (endIndex == -1)
        {
            setCurrentBlockState (quote);
            quoteLength = text.length() - index;
        }
        else
            quoteLength = endIndex - index
                          + quoteExpression.matchedLength(); // 1
        setFormat (index, quoteLength, quoteExpression == quoteMark ? quoteFormat
                                                                    : altQuoteFormat);

        /* the next quote may be different */
        quoteExpression = QRegExp ("\"|\'");
        index = quoteExpression.indexIn (text, index + quoteLength);

        /* skip all values */
        while (format (index) == neutralFormat)
            index = quoteExpression.indexIn (text, index + 1);
        /* skip all comments */
        while (format (index) == commentFormat)
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
    QRegExp comment = (progLan == "sh" || progLan == "makefile" || progLan == "cmake")
                        ? QRegExp ("\\s+#")
                        : QRegExp ("\\s*#");
    int insideCommentPos = comment.indexIn (text);
    int pos = 0;

    /* format the start delimiter */
    if ((!currentBlock().previous().isValid()
         || (previousBlockState() >= 0 && previousBlockState() < endState))
        && comment.indexIn (text) != 0 // the whole line isn't commented out
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
            setCurrentBlockState (2 * (n + (n >= 0 ? endState/2 + 1 : 0))); // always an even number but maybe negative
            setFormat (text.indexOf (delimStr, pos),
                       delimStr.length(),
                       delimFormat);

            TextBlockData *data = static_cast<TextBlockData *>(currentBlock().userData());
            if (!data) return false;
            data->insertInfo (delimStr);
            setCurrentBlockUserData (data);

            return false;
        }
    }

    if (previousBlockState() >= endState || previousBlockState() < -1)
    {
        QTextBlock block = currentBlock().previous();
        TextBlockData *data = static_cast<TextBlockData *>(block.userData());
        if (!data) return false;
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
            if (!data) return false;
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
void Highlighter::debControlFormatting (const QString &text)
{
    if (text.isEmpty()) return;
    bool formatFurther (false);
    QRegExp exp;
    int indx = 0;
    QTextCharFormat debFormat;
    if (QRegExp ("^[^\\s:]+:(?=\\s*)").indexIn (text) == 0)
    {
        formatFurther = true;
        exp = QRegExp ("^[^\\s:]+(?=:)");
        if (exp.indexIn (text) == 0)
        {
            /* before ":" */
            debFormat.setFontWeight (QFont::Bold);
            debFormat.setForeground (DarkBlue);
            setFormat (0, exp.matchedLength(), debFormat);

            /* ":" */
            debFormat.setForeground (DarkMagenta);
            indx = text.indexOf (":");
            setFormat (indx, 1, debFormat);
            indx ++;

            if (indx < text.count())
            {
                /* after ":" */
                debFormat.setFontWeight (QFont::Normal);
                debFormat.setForeground (DarkGreenAlt);
                setFormat (indx, text.count() - indx , debFormat);
            }
        }
    }
    else if (QRegExp ("^\\s+").indexIn (text) == 0)
    {
        formatFurther = true;
        debFormat.setForeground (DarkGreenAlt);
        setFormat (0, text.count(), debFormat);
    }

    if (formatFurther)
    {
        /* parentheses and brackets */
        exp = QRegExp ("\\([^\\(\\)\\[\\]]+\\)|\\[[^\\(\\)\\[\\]]+\\]");
        int index = indx;
        debFormat.setForeground (QBrush());
        debFormat.setFontItalic (true);
        while ((index = text.indexOf (exp, index)) > -1)
        {
            int ml = exp.matchedLength();
            setFormat (index, ml, neutralFormat);
            if (ml > 2)
            {
                setFormat (index + 1, ml - 2 , debFormat);

                QRegExp rel = QRegExp ("<|>|\\=|~");
                int i = index;
                while ((i = text.indexOf (rel, i)) > -1 && i < index + ml - 1)
                {
                    QTextCharFormat relFormat;
                    relFormat.setForeground (DarkMagenta);
                    setFormat (i, 1, relFormat);
                    ++i;
                }
            }
            index = index + ml;
        }

        /* non-commented URLs */
        debFormat.setForeground (DarkGreenAlt);
        debFormat.setFontUnderline (true);
        exp = QRegExp ("[A-Za-z0-9_]+://[A-Za-z0-9_.+/\\?\\=~&%#\\-:]+|[A-Za-z0-9_.\\-]+@[A-Za-z0-9_\\-]+\\.[A-Za-z0-9.]+");
        while ((indx = text.indexOf (exp, indx)) > -1)
        {
            int ml = exp.matchedLength();
            setFormat (indx, ml, debFormat);
            indx = indx + ml;
        }
    }
}
/*************************/
// Start syntax highlighting!
void Highlighter::highlightBlock (const QString &text)
{
    if (progLan.isEmpty()) return;

    bool rehighlightNextBlock = false;
    int prevOpenNests = 0; // to be used in SH_CmndSubstVar()
    if (TextBlockData *prevData = static_cast<TextBlockData *>(currentBlockUserData()))
        prevOpenNests = prevData->openNests();

    int index;
    TextBlockData *data = new TextBlockData;
    data->insertHighlightInfo (false); // not highlighted yet
    setCurrentBlockUserData (data); // to be fed in later
    setCurrentBlockState (0);

    /********************
     * "Here" Documents *
     ********************/

    if (progLan == "sh" || progLan == "makefile" || progLan == "cmake"
        || progLan == "perl" || progLan == "ruby")
    {
        if (isHereDocument (text))
        {
            data->insertHighlightInfo (true); // completely highlighted
            return;
        }
    }
    /* just for debian control file */
    else if (progLan == "deb")
        debControlFormatting (text);

    /************************
     * Single-Line Comments *
     ************************/

    if (progLan != "html")
        singleLineComment (text, 0);

    /* this is only for setting the format of
       command substitution variables in bash */
    rehighlightNextBlock = SH_CmndSubstVar (text, data, prevOpenNests);

    /*******************
     * Python Comments *
     *******************/

    pythonMLComment (text, 0);

    /*******************************
     * XML Quotations and Comments *
     *******************************/

    if (progLan == "xml")
    {
        /* value is handled as a kind of comment */
        multiLineComment (text, 0, -1, QRegExp (">"), QRegExp ("<"), xmlValueState, neutralFormat);
        /* multiline quotes as signs of errors in the xml doc */
        xmlQuotes (text);
    }
    /**************************
     * (Multiline) Quotations *
     **************************/
    else if (progLan == "sh") // bash has its own method
        SH_MultiLineQuote (text);
    else if (progLan != "diff" && progLan != "log"
             && progLan != "desktop" && progLan != "config" && progLan != "theme"
             && progLan != "changelog" && progLan != "url"
             && progLan != "srt" && progLan != "html"
             && progLan != "deb" && progLan != "m3u")
    {
        multiLineQuote (text);
    }

    /*******
     * CSS *
     *******/

    /* helps seeing if a comment destroys a css block */
    int cssIndx = cssHighlighter (text);

    /**********************
     * Multiline Comments *
     **********************/

    if (!commentStartExpression.isEmpty() && progLan != "python")
        multiLineComment (text, 0, cssIndx, commentStartExpression, commentEndExpression, commentState, commentFormat);

    /**************
     * Exceptions *
     **************/

    if (progLan == "markdown")
    {
        /* the block quote of markdown is like a multiline comment
           but shouldn't be formatted inside a real comment */
        if (previousBlockState() != commentState && blockQuoteFormat.isValid())
            multiLineComment (text, 0, -1, QRegExp ("^>.*"), QRegExp ("^$"), markdownBlockQuoteState, blockQuoteFormat);
        /* the ``` code block of markdown is like a multiline comment
           but shouldn't be formatted inside a comment or block quote */
        if (previousBlockState() != commentState && previousBlockState() != markdownBlockQuoteState && codeBlockFormat.isValid())
            multiLineComment (text, 0, -1, QRegExp ("^```[^\\s^`]*$"), QRegExp ("^```$"), markdownCodeBlockState, codeBlockFormat);
    }

    /*************
     * HTML Only *
     *************/

    int bn = currentBlock().blockNumber();
    if (progLan == "html")
    {
        htmlBrackets (text);
        htmlStyleHighlighter (text);
        htmlJavascript (text);
        /* go to braces matching */
        data->insertHighlightInfo (true); // completely highlighted
    }

    /*******************
     * Main Formatting *
     *******************/

    // we format html embedded javascript in htmlJavascript()
    else if (bn >= startCursor.blockNumber() && bn <= endCursor.blockNumber())
    {
        data->insertHighlightInfo (true); // completely highlighted
        for (const HighlightingRule &rule : static_cast<const QVector<HighlightingRule>&>(highlightingRules))
        {
            /* single-line comments are already formatted */
            if (rule.format == commentFormat)
                continue;

            QRegExp expression (rule.pattern);
            index = expression.indexIn (text);
            /* skip quotes and all comments */
            while (rule.format != whiteSpaceFormat
                   && (format (index) == quoteFormat
                       || format (index) == altQuoteFormat
                       || format (index) == commentFormat
                       || format (index) == urlFormat))
            {
                index = expression.indexIn (text, index + 1);
            }

            while (index >= 0)
            {
                int length = expression.matchedLength();
                int l = length;
                /* in c/c++, the neutral pattern after "#define" may contain
                   a (double-)slash but it's harmless to do this always: */
                while (rule.format != whiteSpaceFormat
                       && format (index + l - 1) == commentFormat)
                {
                    -- l;
                }
                setFormat (index, l, rule.format);
                index = expression.indexIn (text, index + length);

                while (rule.format != whiteSpaceFormat
                       && (format (index) == quoteFormat
                           || format (index) == altQuoteFormat
                           || format (index) == commentFormat))
                {
                    index = expression.indexIn (text, index + 1);
                }
            }
        }
    }

    /*********************************************
     * Parentheses, Braces and brackets Matching *
     *********************************************/

    /* left parenthesis */
    index = text.indexOf ('(');
    while (format (index) == quoteFormat || format (index) == altQuoteFormat
           || format (index) == commentFormat)
    {
        index = text.indexOf ('(', index + 1);
    }
    while (index != -1)
    {
        ParenthesisInfo *info = new ParenthesisInfo;
        info->character = '(';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf ('(', index + 1);
        while (format (index) == quoteFormat || format (index) == altQuoteFormat
               || format (index) == commentFormat)
        {
            index = text.indexOf ('(', index + 1);
        }
    }

    /* right parenthesis */
    index = text.indexOf (')');
    while (format (index) == quoteFormat || format (index) == altQuoteFormat
           || format (index) == commentFormat)
    {
        index = text.indexOf (')', index + 1);
    }
    while (index != -1)
    {
        ParenthesisInfo *info = new ParenthesisInfo;
        info->character = ')';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf (')', index +1);
        while (format (index) == quoteFormat || format (index) == altQuoteFormat
               || format (index) == commentFormat)
        {
            index = text.indexOf (')', index + 1);
        }
    }

    /* left brace */
    index = text.indexOf ('{');
    while (format (index) == quoteFormat || format (index) == altQuoteFormat
           || format (index) == commentFormat)
    {
        index = text.indexOf ('{', index + 1);
    }
    while (index != -1)
    {
        BraceInfo *info = new BraceInfo;
        info->character = '{';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf ('{', index + 1);
        while (format (index) == quoteFormat || format (index) == altQuoteFormat
               || format (index) == commentFormat)
        {
            index = text.indexOf ('{', index + 1);
        }
    }

    /* right brace */
    index = text.indexOf ('}');
    while (format (index) == quoteFormat || format (index) == altQuoteFormat
           || format (index) == commentFormat)
    {
        index = text.indexOf ('}', index + 1);
    }
    while (index != -1)
    {
        BraceInfo *info = new BraceInfo;
        info->character = '}';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf ('}', index +1);
        while (format (index) == quoteFormat || format (index) == altQuoteFormat
               || format (index) == commentFormat)
        {
            index = text.indexOf ('}', index + 1);
        }
    }

    /* left bracket */
    index = text.indexOf ('[');
    while (format (index) == quoteFormat || format (index) == altQuoteFormat
           || format (index) == commentFormat)
    {
        index = text.indexOf ('[', index + 1);
    }
    while (index != -1)
    {
        BracketInfo *info = new BracketInfo;
        info->character = '[';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf ('[', index + 1);
        while (format (index) == quoteFormat || format (index) == altQuoteFormat
               || format (index) == commentFormat)
        {
            index = text.indexOf ('[', index + 1);
        }
    }

    /* right bracket */
    index = text.indexOf (']');
    while (format (index) == quoteFormat || format (index) == altQuoteFormat
           || format (index) == commentFormat)
    {
        index = text.indexOf (']', index + 1);
    }
    while (index != -1)
    {
        BracketInfo *info = new BracketInfo;
        info->character = ']';
        info->position = index;
        data->insertInfo (info);

        index = text.indexOf (']', index +1);
        while (format (index) == quoteFormat || format (index) == altQuoteFormat
               || format (index) == commentFormat)
        {
            index = text.indexOf (']', index + 1);
        }
    }

    setCurrentBlockUserData (data);

    if (rehighlightNextBlock)
    {
        QTextBlock block = currentBlock().next();
        if (block.isValid())
            QMetaObject::invokeMethod (this, "rehighlightBlock", Qt::QueuedConnection, Q_ARG (QTextBlock, block));
    }
}

}
