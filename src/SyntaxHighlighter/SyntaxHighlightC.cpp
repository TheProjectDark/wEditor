/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <weditor/SyntaxHighlighter/SyntaxHighlightC.h>
#include <unordered_set>
#include <cctype>

//i will make a separate lexer for C to support C-unique features and it's legacy which is not supported/never used in C++

static const std::unordered_set<std::string> s_keywords = {
    //access modifiers
    "private", "protected", "public",
    //type modifiers
    "auto", "const", "extern", "inline", "register", "static", "typedef", "volatile",
    //type declarations
    "class", "enum", "namespace", "struct",
    //control flow
    "break", "case", "continue", "do", "else", "for", "goto", "if", "return", "switch", "while",
    //memory
    "delete", "new",
    //expressions / operators
    "alignof", "sizeof", "using",
    //misc
    "this"
};

static const std::unordered_set<std::string> s_types = {
    //primitives
    "bool", "char", "double", "float", "int", "long", "short",
    "signed", "unsigned", "void", "wchar_t",
    //platform types
    "nullptr_t", "ptrdiff_t", "size_t",
    //strings
    "string"
};

static const std::unordered_set<std::string> s_stdFuncs = {
    //c string / memory
    "memcpy", "memset", "strcat", "strcmp", "strcpy", "strlen",
    //memory management
    "calloc", "free", "malloc", "realloc",
    //program control
    "abort", "assert", "exit",
    //c i/o
    "printf", "scanf"
};

static const std::unordered_set<std::string> s_literals = {
    "NULL", "false", "nullptr", "this", "true"
};

//helper functions for tokenization
static bool IsIdentChar(char c) { return std::isalnum((unsigned char)c) || c == '_'; }
static bool IsIdentStart(char c) { return std::isalpha((unsigned char)c) || c == '_'; }

//tokenize and apply styles
void SyntaxHighlightC::ApplyHighlight(wxStyledTextCtrl* textCtrl)
{
    textCtrl->ClearDocumentStyle();
    textCtrl->SetLexer(wxSTC_LEX_NULL);

    const wxString wxText = textCtrl->GetValue();
    if (wxText.empty()) return;

    //work with std::string for easier processing
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

        //one line comment //
        if (c == '/' && c1 == '/')
        {
            int start = i;
            while (i < len && text[i] != '\n') ++i;
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        //multi line comments /* */
        if (c == '/' && c1 == '*')
        {
            int start = i;
            i += 2;
            while (i < len - 1 && !(text[i] == '*' && text[i+1] == '/')) ++i;
            i += 2; //skip */
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        //string/char literals
        if (c == '"' || c == '\'')
        {
            int start = i;
            const char delim = c;
            ++i;
            while (i < len)
            {
                if (text[i] == '\\') { i += 2; continue; } // escape
                if (text[i] == delim) { ++i; break; }
                ++i;
            }
            setStyle(start, i, STYLE_STRING);
            continue;
        }

        //preprocessor directives
        if (c == '#')
        {
            int start = i;
            //consume directive name
            ++i;
            while (i < len && std::isalpha((unsigned char)text[i])) ++i;
            setStyle(start, i, STYLE_PREPROCESSOR);

            //for #include, also style the path
            const std::string directive = text.substr(start + 1, i - start - 1);
            if (directive == "include")
            {
                //skip whitespace
                while (i < len && text[i] == ' ') ++i;
                if (i < len && text[i] == '<')
                {
                    int angleStart = i;
                    while (i < len && text[i] != '>' && text[i] != '\n') ++i;
                    if (i < len && text[i] == '>') ++i;
                    setStyle(angleStart, i, STYLE_STRING);
                }
            }
            continue;
        }

        //numeric literals (int, float.. whatever)
        if (std::isdigit((unsigned char)c) ||
            (c == '.' && std::isdigit((unsigned char)c1)))
        {
            int start = i;
            if (c == '0' && (c1 == 'x' || c1 == 'X')) //hex
            {
                i += 2;
                while (i < len && std::isxdigit((unsigned char)text[i])) ++i;
            }
            else if (c == '0' && (c1 == 'b' || c1 == 'B')) //binary
            {
                i += 2;
                while (i < len && (text[i] == '0' || text[i] == '1')) ++i;
            }
            else
            {
                while (i < len && std::isdigit((unsigned char)text[i])) ++i;
                if (i < len && text[i] == '.')
                {
                    ++i;
                    while (i < len && std::isdigit((unsigned char)text[i])) ++i;
                }
                if (i < len && (text[i] == 'e' || text[i] == 'E'))
                {
                    ++i;
                    if (i < len && (text[i] == '+' || text[i] == '-')) ++i;
                    while (i < len && std::isdigit((unsigned char)text[i])) ++i;
                }
            }
            //optional suffix: u, l, f, ul, ll …
            while (i < len && (text[i] == 'u' || text[i] == 'U' ||
                                text[i] == 'l' || text[i] == 'L' ||
                                text[i] == 'f' || text[i] == 'F')) ++i;
            setStyle(start, i, STYLE_NUMBER);
            continue;
        }

        //identifiers and keywords
        if (IsIdentStart(c))
        {
            int start = i;
            while (i < len && IsIdentChar(text[i])) ++i;

            //handle namespaces and class scopes (e.g. std::vector)
            while (i + 1 < len && text[i] == ':' && text[i+1] == ':')
            {
                i += 2;
                while (i < len && IsIdentChar(text[i])) ++i;
            }

            const std::string word = text.substr(start, i - start);

            //check for function call (lookahead for '(')
            int j = i;
            while (j < len && (text[j] == ' ' || text[j] == '\t')) ++j;
            if (j < len && text[j] == '(')
            {
                //still classify as keyword if it's a known type or keyword followed by '(' (e.g. "sizeof(")
                if (s_keywords.count(word))
                    setStyle(start, i, STYLE_KEYWORD);
                else
                    setStyle(start, i, STYLE_FUNCTION);
                continue;
            }

            //classify as keyword, type, function or literal
            if (s_keywords.count(word))        setStyle(start, i, STYLE_KEYWORD);
            else if (s_types.count(word))       setStyle(start, i, STYLE_KEYWORD);
            else if (s_stdFuncs.count(word))    setStyle(start, i, STYLE_FUNCTION);
            else if (s_literals.count(word))    setStyle(start, i, STYLE_NUMBER);
            //else: STYLE_DEFAULT (normal identifier)
            continue;
        }

        //digraphs (alternative tokens, legacy but still valid in C, while being almost never used in C++)
        if (c == '<' && c1 == ':')
        {
            setStyle(i, i + 2, STYLE_OPERATOR);
            i += 2;
            continue;
        }
        if (c == ':' && c1 == '>')
        {            setStyle(i, i + 2, STYLE_OPERATOR);
            i += 2;
            continue;
        }
        if (c == '<' && c1 == '%')
        {            setStyle(i, i + 2, STYLE_OPERATOR);
            i += 2;
            continue;
        }
        if (c == '%' && c1 == '>')
        {            setStyle(i, i + 2, STYLE_OPERATOR);
            i += 2;
            continue;
        }
        if (c == '%' && c1 == ':')
        {            setStyle(i, i + 2, STYLE_OPERATOR);
            i += 2;
            continue;
        }

        //multi-char operators
        if (i + 1 < len)
        {
            const std::string op2 = text.substr(i, 2);
            static const std::unordered_set<std::string> s_ops2 = {
                "==","!=","<=",">=","&&","||","++","--","->",
                "+=","-=","*=","/=","%=","&=","|=","^=","<<",">>"
            };
            if (s_ops2.count(op2))
            {
                setStyle(i, i + 2, STYLE_OPERATOR);
                i += 2;
                continue;
            }
        }

        //single-char operators and symbols
        static const std::string s_singles = "+-*/%=<>!&|^~?:;,.(){}[]";
        if (s_singles.find(c) != std::string::npos)
        {
            setStyle(i, i + 1, STYLE_OPERATOR);
        }

        ++i;
    }

    //apply styles at once
    textCtrl->StartStyling(0);
    textCtrl->SetStyleBytes(len, styles.data());
}
