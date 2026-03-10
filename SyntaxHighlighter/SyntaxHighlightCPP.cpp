/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "SyntaxHighlightCPP.h"
#include <unordered_set>
#include <cctype>

static const std::unordered_set<std::string> s_keywords = {
    "return","if","else","while","for","switch","case","break","continue",
    "override","virtual","const","static","new","delete","this",
    "public","private","protected","namespace","class","struct","enum",
    "typedef","using","template","typename","inline","extern","volatile",
    "do","goto","sizeof","alignof","decltype","auto","register","explicit"
};

static const std::unordered_set<std::string> s_types = {
    "int","float","double","char","void","bool","long","short",
    "unsigned","signed","wchar_t","size_t","ptrdiff_t","nullptr_t",
    "string","vector","map","unordered_map","set","pair","tuple",
    "unique_ptr","shared_ptr","weak_ptr","optional","variant","any"
};

static const std::unordered_set<std::string> s_stdFuncs = {
    "printf","scanf","cout","cin","endl","cerr","clog",
    "strlen","strcmp","strcpy","strcat","memset","memcpy",
    "malloc","calloc","realloc","free","assert","abort","exit",
    "std::cout","std::cin","std::cerr","std::endl", "std::vector"
};
static const std::unordered_set<std::string> s_literals = {
    "true","false","NULL","nullptr","this"
};

//helper functions for tokenization

static bool IsIdentChar(char c)  { return std::isalnum((unsigned char)c) || c == '_'; }
static bool IsIdentStart(char c) { return std::isalpha((unsigned char)c) || c == '_'; }

//tokenize and apply styles
void SyntaxHighlightCPP::ApplyHighlight(wxStyledTextCtrl* textCtrl)
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