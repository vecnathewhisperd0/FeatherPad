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

QStringList Highlighter::keywords (QString& lang)
{
    QStringList keywordPatterns;
    if (lang == "c" || lang == "cpp")
    {
        keywordPatterns << "\\b(asm|auto)\\b"
                        << "\\b(const|case|catch|cdecl|continue)\\b"
                        << "\\b(break|default|do)\\b"
                        << "\\b(enum|explicit|else|extern)\\b"
                        << "\\b(for|goto|if|NULL|pasca|register|return)\\b"
                        << "\\b(signals|sizeof|static|struct|switch)\\b"
                        << "\\b(typedef|typename|union|volatile|while)\\b";

        if (lang == "c")
            keywordPatterns << "\\b(FALSE|TRUE)\\b";
        else
            keywordPatterns << "\\b(class|const_cast|delete|dynamic_cast)\\b"
                            << "\\b(false|foreach|friend|inline|namespace|new|operator)\\b"
                            << "\\b(nullptr|private|protected|public|reinterpret_cast|slots|static_cast)\\b"
                            << "\\b(template|true|this|throw|try|typeid|using|virtual)\\b";
    }
    else if (lang == "sh" || lang == "makefile" || lang == "cmake") // the characters "(", ";" and "&" will be reformatted after this
    {
        keywordPatterns << "((^\\s*|[\\(\\);&`\\|]+\\s*)((if|then|elif|else|fi|while|do|done|esac)\\s+)*)(alias|bg|bind|break|builtin)\\b"
                        << "((^\\s*|[\\(\\);&`\\|]+\\s*)((if|then|elif|else|fi|while|do|done|esac)\\s+)*)(caller|case|command|compgen|complete|continue)\\b"
                        << "((^\\s*|[\\(\\);&`\\|]+\\s*)((if|then|elif|else|fi|while|do|done|esac)\\s+)*)(declare|dirs|disown|do|done)\\b"
                        << "((^\\s*|[\\(\\);&`\\|]+\\s*)((if|then|elif|else|fi|while|do|done|esac)\\s+)*)(echo|enable|esac|eval|exec|exit|export)\\b"
                        << "(^\\s*|[\\(\\);&`\\|]+\\s*)elif\\b" << "(^\\s*|[\\(\\);&`\\|]+\\s*)else\\b"
                        << "\\bfalse\\b"
                        << "((^\\s*|[\\(\\);&`\\|]+\\s*)((if|then|elif|else|fi|while|do|done|esac)\\s+)*)(fg|fi|for|getopts|hash|help|history)\\b"
                        << "((^\\s*|[\\(\\);&`\\|]+\\s*)((then|else|done|esac)\\s+)*)if\\b" << "\\bin\\b"
                        << "((^\\s*|[\\(\\);&`\\|]+\\s*)((if|then|elif|else|fi|while|do|done|esac)\\s+)*)(jobs|let|local|logout|popd|printf|pushd|read|readonly|return)\\b"
                        << "((^\\s*|[\\(\\);&`\\|]+\\s*)((if|then|elif|else|fi|while|do|done|esac)\\s+)*)(select|set|shift|shopt|source|suspend|test|times|trap|type|typeset)\\b"
                        << "(^\\s*|[\\(\\);&`\\|]+\\s*)then\\b" << "\\btrue\\b"
                        << "((^\\s*|[\\(\\);&`\\|]+\\s*)((if|then|elif|else|fi|while|do|done|esac)\\s+)*)(umask|unalias|unset|until|wait|while)\\b";
    }
    else if (lang == "qmake")
    {
        keywordPatterns << "\\bCONFIG\\b"
                        << "\\bDEFINES\\b" << "\\bDESTDIR\\b"
                        << "\\bFORMS\\b"
                        << "\\bHEADERS\\b"
                        << "\\bINSTALLS\\b" << "\\bINCLUDEPATH\\b"
                        << "\\bLIBS\\b"
                        << "\\bMOC_DIR\\b"
                        << "\\bOBJECTS_DIR\\b" << "\\bOTHER_FILES\\b"
                        << "\\bQT\\b" << "\\bQT_MAJOR_VERSION\\b"
                        << "\\bRESOURCES\\b"
                        << "\\bSOURCES\\b" << "\\bSUBDIRS\\b"
                        << "\\bTARGET\\b" << "\\bTEMPLATE\\b" << "\\bTRANSLATIONS\\b"
                        << "\\bVERSION\\b";
    }
    else if (lang == "troff")
    {
        keywordPatterns << "^\\.TH\\b" << "^\\.SH\\b" << "^\\.I\\b" << "^\\.B\\b";
    }
    else if (lang == "perl")
    {
        keywordPatterns << "\\b(abs|alarm|and|atan2|binmode|bless)\\b"
                        << "\\b(caller|chdir|chmod|chown|chroot|chomp|chop|chr|close|closedir|cmp|continue|cos|crypt)\\b"
                        << "\\b(dbmclose|dbmopen|define|delete|die|do|dump)\\b"
                        << "\\b(each|else|elsif|eof|eq|eva|exec|exists|exit|exp)\\b"
                        << "\\b(fcntl|fileno|flock|for|foreach|fork|format)\\b"
                        << "\\b(g|getc|getpgrp|getppid|getpriority|glob|goto|grep|hex)\\b"
                        << "\\b(i|if|import|index|int|ioctl|join|keys|kill)\\b"
                        << "\\b(last|lc|lcfirst|length|link|local|log|lstat|m|map|mkdir|my|next)\\b"
                        << "\\b(oct|open|opendir|ord|our|pack|package|pipe|pop|print|printf|push)\\b"
                        << "\\b(q|qq|qw|qx)\\b"
                        << "\\b(rand|read|readdir|readlink|redo|ref|rename|require|return|reverse|rewinddir|rindex|rmdir)\\b"
                        << "\\b(s|seek|seekdir|select|setpgrp|setpriority|shift|sin|sleep|sort|splice|split|sprintf|sqrt|srand|stat|sub|substr|symlink|syscall|sysread|sysseek|system|syswrite|switch)\\b"
                        << "\\b(tell|telldir|tie|tied|times|tr|truncate)\\b"
                        << "\\b(uc|ucfirst|umask|undef|unless|unlink|unpack|unshift|untie|use|utime)\\b"
                        << "\\b(values|vec|x)\\b"
                        << "\\b(wait|waitpid|warn|wantarray|while|write)\\b";
    }
    else if (lang == "ruby")
    {
        keywordPatterns << "\\b(__FILE__|__LINE__)\\b"
                        << "\\b(alias|and|begin|BEGIN|break)\\b"
                        << "\\b(case|class|catch|defined|do|def)\\b"
                        << "\\b(else|elsif|end|END|ensure|for|false|fail)\\b"
                        << "\\b(if|in|include|loop|load|module)\\b"
                        << "\\b(next|nil|not|or)\\b"
                        << "\\b(raise|redo|rescue|retry|return)\\b"
                        << "\\b(super|self|then|true|throw)\\b"
                        << "\\b(undef|unless|until|when|while|yield)\\b";
    }
    else if (lang == "lua")
        keywordPatterns << "\\b(and|break|do)\\b"
                        << "\\b(else|elseif|end)\\b"
                        << "\\b(false|for|function)\\b"
                        << "\\b(if|in|local|nil|not|or|repeat|return)\\b"
                        << "\\b(true|until|while)\\b";
    else if (lang == "python")
        keywordPatterns << "\\b(and|assert|break|class|continue)\\b"
                        << "\\b(def|del|elif|else|except|exec|False|finally|for|from|global)\\b"
                        << "\\b(if|is|import|in|lambda|None|not|or|print|raise|return|True|try|while|with|yield)\\b";
    else if (lang == "javascript")
        keywordPatterns << "\\b(abstract|break)\\b"
                        << "\\b(case|catch|class|const|continue)\\b"
                        << "\\b(debugger|default|delete|do)\\b"
                        << "\\b(else|enum|export|extends)\\b"
                        << "\\b(false|final|finally|for|function|goto)\\b"
                        << "\\b(if|implements|in|instanceof|interface|let)\\b"
                        << "\\b(native|new|null)\\b"
                        << "\\b(private|protected|prototype|public|return)\\b"
                        << "\\b(static|super|switch|synchronized)\\b"
                        << "\\b(throw|throws|this|transient|true|try|typeof)\\b"
                        << "\\b(var|volatile|while|with)\\b";
    else if (lang == "php")
        keywordPatterns << "\\b(__FILE__|__LINE__|__FUNCTION__|__CLASS__|__METHOD__|__DIR__|__NAMESPACE__)\\b"
                        << "\\b(and|abstract|array|as|break)\\b"
                        << "\\b(case|catch|cfunction|class|clone|const|continue)\\b"
                        << "\\b(declare|default|die|do)\\b"
                        << "\\b(each|echo|else|elseif|empty|enddeclare|endfor|endforeach|endif|endswitch|endwhile|eval|exception|exit|extends)\\b"
                        << "\\b(false|final|for|foreach|function)\\b"
                        << "\\b(global|goto|if|implements|interface|instanceof|isset)\\b"
                        << "\\b(list|namespace|new|null|old_function|or)\\b"
                        << "\\b(php_user_filter|print|private|protected|public|return)\\b"
                        << "\\b(static|switch|throw|true|try)\\b"
                        << "\\b(unset|use|var|while|xor)\\b";

    return keywordPatterns;
}
/*************************/
QStringList Highlighter::types()
{
    QStringList typePatterns;
    if (progLan == "c" || progLan == "cpp")
    {
        typePatterns << "\\b(bool|char|double|float)\\b"
                     << "\\b(gchar|gint|guint|guint8|gboolean)\\b"
                     << "\\b(int|long|short)\\b"
                     << "\\b(unsigned|uint32|uint32_t|uint8_t)\\b"
                     << "\\b(void|wchar_t)\\b";
        if (progLan == "cpp")
            typePatterns << "\\b(qreal|qint8|quint8|qint16|quint16|qint32|quint32|qint64|quint64|qlonglong|qulonglong|qptrdiff|quintptr)\\b"
                         << "\\b(uchar|uint|ulong|ushort)\\b";
    }
    else if (progLan == "troff")
    {
        typePatterns << "^\\.IP\\b" << "^\\.RS\\b" << "^\\.RE\\b";
    }
    return typePatterns;
}

}
