/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <weditor/SyntaxHighlighter/SyntaxHighlightBatch.h>
#include <unordered_set>
#include <cctype>

static const std::unordered_set<std::string> s_keywords = {
    //flow control
    "if", "else", "for", "in", "do", "goto", "call", "exit", "shift", "pause",
    //condition helpers
    "not", "exist", "defined", "errorlevel",
    //comparisons
    "equ", "neq", "lss", "leq", "gtr", "geq",
    //batch directives
    "setlocal", "endlocal"
};

static const std::unordered_set<std::string> s_builtins = {
    //console and environment
    "echo", "set", "title", "prompt", "path", "cls", "color", "ver",
    //filesystem
    "cd", "chdir", "copy", "del", "erase", "dir", "md", "mkdir", "move",
    "rd", "rmdir", "ren", "rename", "type", "xcopy", "robocopy",
    //navigation
    "pushd", "popd", "start",
    //system tools
    "assoc", "attrib", "break", "chcp", "choice", "date", "time",
    "find", "findstr", "more", "sort", "taskkill", "tasklist", "where"
};

static bool IsIdentChar(char c)  { return std::isalnum((unsigned char)c) || c == '_' || c == '-'; }
static bool IsIdentStart(char c) { return std::isalpha((unsigned char)c) || c == '_'; }

static std::string ToLower(const std::string& s)
{
    std::string out = s;
    for (char& c : out) c = static_cast<char>(std::tolower((unsigned char)c));
    return out;
}

static bool IsLineStartOrSpace(const std::string& text, int index)
{
    if (index == 0) return true;
    return std::isspace((unsigned char)text[index - 1]) != 0;
}

void SyntaxHighlightBatch::ApplyHighlight(wxStyledTextCtrl* textCtrl)
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

        //comments: rem ... or :: ...
        if ((c == 'r' || c == 'R') &&
            i + 2 < len &&
            (text[i + 1] == 'e' || text[i + 1] == 'E') &&
            (text[i + 2] == 'm' || text[i + 2] == 'M') &&
            (i + 3 >= len || std::isspace((unsigned char)text[i + 3])) &&
            IsLineStartOrSpace(text, i))
        {
            int start = i;
            while (i < len && text[i] != '\n') ++i;
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }
        if (c == ':' && c1 == ':' && IsLineStartOrSpace(text, i))
        {
            int start = i;
            while (i < len && text[i] != '\n') ++i;
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        //labels
        if (c == ':' && c1 != ':' && (i == 0 || text[i - 1] == '\n'))
        {
            int start = i;
            ++i;
            while (i < len && text[i] != '\n' && !std::isspace((unsigned char)text[i])) ++i;
            setStyle(start, i, STYLE_PREPROCESSOR);
            continue;
        }

        //strings
        if (c == '"')
        {
            int start = i;
            ++i;
            while (i < len)
            {
                if (text[i] == '"') { ++i; break; }
                ++i;
            }
            setStyle(start, i, STYLE_STRING);
            continue;
        }

        //variables: %VAR%, %%i, %1, %~dp0, !VAR!
        if (c == '%' || c == '!')
        {
            int start = i;
            if (c == '%' && i + 1 < len && text[i + 1] == '%')
            {
                i += 2;
                while (i < len && IsIdentChar(text[i])) ++i;
                setStyle(start, i, STYLE_PREPROCESSOR);
                continue;
            }

            if (c == '%' && i + 1 < len && std::isdigit((unsigned char)text[i + 1]))
            {
                i += 2;
                setStyle(start, i, STYLE_PREPROCESSOR);
                continue;
            }

            if (c == '%' && i + 1 < len && text[i + 1] == '~')
            {
                i += 2;
                while (i < len && (std::isalnum((unsigned char)text[i]) || text[i] == '~')) ++i;
                setStyle(start, i, STYLE_PREPROCESSOR);
                continue;
            }

            if (c == '%' && i + 1 < len && IsIdentStart(text[i + 1]))
            {
                int j = i + 2;
                while (j < len && text[j] != '%' && text[j] != '\n') ++j;
                if (j < len && text[j] == '%')
                {
                    i = j + 1;
                    setStyle(start, i, STYLE_PREPROCESSOR);
                    continue;
                }
            }

            if (c == '!' && i + 1 < len && IsIdentStart(text[i + 1]))
            {
                int j = i + 2;
                while (j < len && text[j] != '!' && text[j] != '\n') ++j;
                if (j < len && text[j] == '!')
                {
                    i = j + 1;
                    setStyle(start, i, STYLE_PREPROCESSOR);
                    continue;
                }
            }
        }

        //numbers
        if (std::isdigit((unsigned char)c))
        {
            int start = i;
            while (i < len && std::isdigit((unsigned char)text[i])) ++i;
            setStyle(start, i, STYLE_NUMBER);
            continue;
        }

        //identifiers and commands (case-insensitive)
        if (IsIdentStart(c))
        {
            int start = i;
            while (i < len && IsIdentChar(text[i])) ++i;
            const std::string word = text.substr(start, i - start);
            const std::string wordLow = ToLower(word);

            if      (s_keywords.count(wordLow))  setStyle(start, i, STYLE_KEYWORD);
            else if (s_builtins.count(wordLow))  setStyle(start, i, STYLE_FUNCTION);
            continue;
        }

        //multi-char operators
        if (i + 1 < len)
        {
            static const std::unordered_set<std::string> s_ops2 = {
                "&&", "||", "==", "/i", "/a", "/f", "/l", "/r", "/d"
            };
            const std::string op2 = ToLower(text.substr(i, 2));
            if (s_ops2.count(op2))
            {
                setStyle(i, i + 2, STYLE_OPERATOR);
                i += 2;
                continue;
            }
        }

        //single-char operators and punctuation
        static const std::string s_singles = "=<>+-*/%()!|&,@";
        if (s_singles.find(c) != std::string::npos)
        {
            setStyle(i, i + 1, STYLE_OPERATOR);
        }

        ++i;
    }

    textCtrl->StartStyling(0);
    textCtrl->SetStyleBytes(len, styles.data());
}
