/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <weditor/SyntaxHighlighter/SyntaxHighlightBash.h>
#include <unordered_set>
#include <cctype>

static const std::unordered_set<std::string> s_keywords = {
    //control flow
    "if", "then", "else", "elif", "fi", "case", "esac",
    "for", "select", "while", "until", "do", "done", "in",
    //bash-specific reserved words
    "function", "time", "coproc"
};

static const std::unordered_set<std::string> s_builtins = {
    //shell control
    ":", ".", "exec", "exit", "logout", "return", "set", "shift", "times", "trap", "unset",
    //flow / job control
    "bg", "break", "continue", "fg", "jobs", "wait",
    //environment and scope
    "builtin", "cd", "command", "declare", "dirs", "disown", "enable",
    "export", "hash", "local", "popd", "pushd", "pwd", "readonly", "source",
    "type", "typeset", "ulimit", "umask",
    //i/o and text
    "echo", "mapfile", "printf", "read", "readarray",
    //shell introspection and completion
    "alias", "bind", "caller", "compgen", "complete", "compopt",
    "fc", "getopts", "help", "history", "kill", "let", "shopt",
    "suspend", "test", "true", "false", "unalias", "eval"
};

static const std::unordered_set<std::string> s_variables = {
    //shell / bash runtime
    "BASH", "BASHOPTS", "BASHPID", "BASH_COMMAND", "BASH_REMATCH", "BASH_SOURCE",
    "BASH_SUBSHELL", "BASH_VERSION", "BASH_VERSINFO", "DIRSTACK", "EPOCHREALTIME",
    "EPOCHSECONDS", "FUNCNAME", "GROUPS", "HISTCMD", "HOSTNAME", "HOSTTYPE",
    "IFS", "LINENO", "MACHTYPE", "OPTARG", "OPTIND", "OSTYPE", "PIPESTATUS",
    "PPID", "RANDOM", "REPLY", "SECONDS", "SHELL", "SHELLOPTS", "SHLVL",
    //common environment variables
    "HOME", "OLDPWD", "PATH", "PWD", "TERM", "TMPDIR", "USER"
};

static bool IsIdentChar(char c)  { return std::isalnum((unsigned char)c) || c == '_'; }
static bool IsIdentStart(char c) { return std::isalpha((unsigned char)c) || c == '_'; }

static bool IsCommentStart(const std::string& text, int index)
{
    if (text[index] != '#') return false;
    if (index == 0) return true;

    const char prev = text[index - 1];
    return std::isspace((unsigned char)prev) ||
           prev == ';' || prev == '(' || prev == ')' ||
           prev == '{' || prev == '}' || prev == '|' ||
           prev == '&';
}

void SyntaxHighlightBash::ApplyHighlight(wxStyledTextCtrl* textCtrl)
{
    textCtrl->ClearDocumentStyle();
    textCtrl->SetLexer(wxSTC_LEX_NULL);

    const wxString wxText = textCtrl->GetValue();
    if (wxText.empty()) return;

    const std::string text = wxText.ToStdString();
    const int len = static_cast<int>(text.size());

    std::string styles(len, STYLE_DEFAULT);

    auto setStyle = [&](int from, int to, char style) {
        for (int i = from; i < to && i < len; ++i)
            styles[i] = style;
    };

    int i = 0;
    while (i < len)
    {
        const char c  = text[i];
        const char c1 = (i + 1 < len) ? text[i + 1] : '\0';
        const char c2 = (i + 2 < len) ? text[i + 2] : '\0';

        //line comments
        if (IsCommentStart(text, i))
        {
            int start = i;
            while (i < len && text[i] != '\n') ++i;
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        //single, double and legacy command-substitution strings
        if (c == '\'' || c == '"' || c == '`')
        {
            int start = i;
            const char delim = c;
            ++i;
            while (i < len)
            {
                if (text[i] == '\\' && delim != '\'') { i += 2; continue; }
                if (text[i] == delim) { ++i; break; }
                ++i;
            }
            setStyle(start, i, STYLE_STRING);
            continue;
        }

        //variables and substitutions: $var, ${var}, $(cmd), $((expr)), $1, $?
        if (c == '$')
        {
            int start = i;

            if (c1 == '{')
            {
                i += 2;
                int depth = 1;
                while (i < len && depth > 0)
                {
                    if (text[i] == '\\') { i += 2; continue; }
                    if (text[i] == '{') ++depth;
                    else if (text[i] == '}') --depth;
                    ++i;
                }
                setStyle(start, i, STYLE_PREPROCESSOR);
                continue;
            }

            if (c1 == '(' && c2 == '(')
            {
                i += 3;
                while (i + 1 < len && !(text[i] == ')' && text[i + 1] == ')'))
                {
                    if (text[i] == '\\') { i += 2; continue; }
                    ++i;
                }
                if (i + 1 < len) i += 2;
                setStyle(start, i, STYLE_PREPROCESSOR);
                continue;
            }

            if (c1 == '(')
            {
                i += 2;
                int depth = 1;
                while (i < len && depth > 0)
                {
                    if (text[i] == '\\') { i += 2; continue; }
                    if (text[i] == '(') ++depth;
                    else if (text[i] == ')') --depth;
                    ++i;
                }
                setStyle(start, i, STYLE_PREPROCESSOR);
                continue;
            }

            if (std::isdigit((unsigned char)c1))
            {
                ++i;
                while (i < len && std::isdigit((unsigned char)text[i])) ++i;
                setStyle(start, i, STYLE_PREPROCESSOR);
                continue;
            }

            if (c1 == '@' || c1 == '*' || c1 == '#' || c1 == '?' ||
                c1 == '-' || c1 == '$' || c1 == '!' || c1 == '_')
            {
                i += 2;
                setStyle(start, i, STYLE_PREPROCESSOR);
                continue;
            }

            if (IsIdentStart(c1))
            {
                i += 2;
                while (i < len && IsIdentChar(text[i])) ++i;
                setStyle(start, i, STYLE_PREPROCESSOR);
                continue;
            }
        }

        //numeric literals
        if (std::isdigit((unsigned char)c))
        {
            int start = i;
            if (c == '0' && (c1 == 'x' || c1 == 'X'))
            {
                i += 2;
                while (i < len && std::isxdigit((unsigned char)text[i])) ++i;
            }
            else
            {
                while (i < len && std::isdigit((unsigned char)text[i])) ++i;
            }
            setStyle(start, i, STYLE_NUMBER);
            continue;
        }

        //identifiers and builtins
        if (IsIdentStart(c))
        {
            int start = i;
            while (i < len && IsIdentChar(text[i])) ++i;
            const std::string word = text.substr(start, i - start);

            //check for function call (lookahead for '(')
            int j = i;
            while (j < len && (text[j] == ' ' || text[j] == '\t')) ++j;
            if (j < len && text[j] == '(')
            {
                if (s_keywords.count(word))
                    setStyle(start, i, STYLE_KEYWORD);
                else
                    setStyle(start, i, STYLE_FUNCTION);
                continue;
            }

            if      (s_keywords.count(word))   setStyle(start, i, STYLE_KEYWORD);
            else if (s_builtins.count(word))   setStyle(start, i, STYLE_FUNCTION);
            else if (s_variables.count(word))  setStyle(start, i, STYLE_NAMESPACE);
            continue;
        }

        //multi-char operators
        if (i + 2 < len)
        {
            const std::string op3 = text.substr(i, 3);
            if (op3 == ";;&")
            {
                setStyle(i, i + 3, STYLE_OPERATOR);
                i += 3;
                continue;
            }
        }
        if (i + 1 < len)
        {
            static const std::unordered_set<std::string> s_ops2 = {
                "&&", "||", ";;", ";&", "<<", ">>", "<=", ">=", "==", "!=",
                "=~", "+=", "-=", "*=", "/=", "%=", "[[", "]]"
            };
            const std::string op2 = text.substr(i, 2);
            if (s_ops2.count(op2))
            {
                setStyle(i, i + 2, STYLE_OPERATOR);
                i += 2;
                continue;
            }
        }

        //single-char operators and punctuation
        static const std::string s_singles = "=<>+-*/%(){}[]!&|;:,";
        if (s_singles.find(c) != std::string::npos)
        {
            setStyle(i, i + 1, STYLE_OPERATOR);
        }

        ++i;
    }

    textCtrl->StartStyling(0);
    textCtrl->SetStyleBytes(len, styles.data());
}
