/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <weditor/SyntaxHighlighter/CSSHighlight.h>
#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_set>

//element names used in selectors
static const std::unordered_set<std::string> s_tags = {
    "html", "body", "head", "title", "base", "link", "meta", "style",
    "article", "aside", "footer", "header", "h1", "h2", "h3", "h4", "h5", "h6",
    "hgroup", "main", "nav", "section", "search",
    "blockquote", "dd", "div", "dl", "dt", "figcaption", "figure", "hr",
    "li", "menu", "ol", "p", "pre", "ul",
    "a", "abbr", "b", "bdi", "bdo", "br", "cite", "code", "data", "dfn",
    "em", "i", "kbd", "mark", "q", "rp", "rt", "ruby", "s", "samp", "small",
    "span", "strong", "sub", "sup", "time", "u", "var", "wbr",
    "area", "audio", "img", "map", "track", "video", "picture", "source",
    "embed", "iframe", "object", "param", "canvas", "noscript", "script",
    "del", "ins", "caption", "col", "colgroup", "table", "tbody", "td",
    "tfoot", "th", "thead", "tr",
    "button", "datalist", "fieldset", "form", "input", "label", "legend",
    "meter", "optgroup", "option", "output", "progress", "select", "textarea",
    "details", "dialog", "summary", "slot", "template",
    //svg
    "svg", "g", "path", "circle", "rect", "line", "ellipse", "polygon",
    "polyline", "text", "defs", "use", "symbol",
    //keyframe selectors
    "from", "to"
};

//global values valid for any property
static const std::unordered_set<std::string> s_globalValues = {
    "inherit", "initial", "unset", "revert", "revert-layer"
};

//words inside at-rule preludes: @media, @supports, @container ...
static const std::unordered_set<std::string> s_atWords = {
    "and", "or", "not", "only", "screen", "print", "all", "speech"
};

//helper functions for tokenization
static bool IsWhitespace(char c)
{
    return std::isspace((unsigned char)c) != 0;
}

static bool IsIdentChar(char c)
{
    return std::isalnum((unsigned char)c) || c == '-' || c == '_' ||
           (unsigned char)c >= 0x80;
}

static bool IsIdentStartAt(const std::string& t, int i)
{
    const int len = static_cast<int>(t.size());
    const char c = t[i];
    if (std::isalpha((unsigned char)c) || c == '_' || c == '\\' ||
        (unsigned char)c >= 0x80)
        return true;

    if (c == '-' && i + 1 < len)
    {
        const char n = t[i + 1];
        return std::isalpha((unsigned char)n) || n == '-' || n == '_' ||
               n == '\\' || (unsigned char)n >= 0x80;
    }
    return false;
}

static bool IsNumberStartAt(const std::string& t, int i)
{
    const int len = static_cast<int>(t.size());
    const char c = t[i];
    if (std::isdigit((unsigned char)c)) return true;

    if (c == '.')
        return i + 1 < len && std::isdigit((unsigned char)t[i + 1]);

    if ((c == '+' || c == '-') && i + 1 < len)
    {
        if (std::isdigit((unsigned char)t[i + 1])) return true;
        return t[i + 1] == '.' && i + 2 < len &&
               std::isdigit((unsigned char)t[i + 2]);
    }
    return false;
}

static int ScanIdent(const std::string& t, int i)
{
    const int len = static_cast<int>(t.size());
    while (i < len)
    {
        if (t[i] == '\\' && i + 1 < len) i += 2; //escaped char
        else if (IsIdentChar(t[i])) ++i;
        else break;
    }
    return i;
}

//number with optional sign, fraction, exponent and unit (10px, 1.5em, 50%)
static int ScanNumber(const std::string& t, int i)
{
    const int len = static_cast<int>(t.size());

    if (i < len && (t[i] == '+' || t[i] == '-')) ++i;
    while (i < len && std::isdigit((unsigned char)t[i])) ++i;

    if (i + 1 < len && t[i] == '.' && std::isdigit((unsigned char)t[i + 1]))
    {
        ++i;
        while (i < len && std::isdigit((unsigned char)t[i])) ++i;
    }

    if (i < len && (t[i] == 'e' || t[i] == 'E'))
    {
        int k = i + 1;
        if (k < len && (t[k] == '+' || t[k] == '-')) ++k;
        if (k < len && std::isdigit((unsigned char)t[k]))
        {
            i = k;
            while (i < len && std::isdigit((unsigned char)t[i])) ++i;
        }
    }

    if (i < len && t[i] == '%') ++i;
    else
        while (i < len && (std::isalpha((unsigned char)t[i]) ||
                           (unsigned char)t[i] >= 0x80)) ++i;
    return i;
}

//looks ahead from the start of a statement: is it "prop: value;" or "selector {"
static bool IsDeclaration(const std::string& t, int i)
{
    const int len = static_cast<int>(t.size());
    while (i < len)
    {
        const char c = t[i];

        if (c == '/' && i + 1 < len && t[i + 1] == '*')
        {
            const size_t e = t.find("*/", i + 2);
            if (e == std::string::npos) return true;
            i = static_cast<int>(e) + 2;
            continue;
        }

        if (c == '"' || c == '\'')
        {
            ++i;
            while (i < len && t[i] != c && t[i] != '\n')
            {
                if (t[i] == '\\') ++i;
                ++i;
            }
            ++i;
            continue;
        }

        if (c == '{') return false;
        if (c == ';' || c == '}') return true;
        ++i;
    }
    return true;
}

static std::string Lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char ch) { return (char)std::tolower(ch); });
    return s;
}

void CSSHighlight::ApplyHighlight(wxStyledTextCtrl* textCtrl)
{
    textCtrl->ClearDocumentStyle();
    textCtrl->SetLexer(wxSTC_LEX_NULL);

    const wxString wxText = textCtrl->GetValue();
    if (wxText.empty()) return;

    //STC works with UTF-8 bytes, so indexes must match its byte length
    const std::string text = wxText.ToUTF8().data();
    const int len = static_cast<int>(text.size());

    std::string styles(len, STYLE_DEFAULT);

    auto setStyle = [&](int from, int to, char style) {
        for (int j = from; j < to && j < len; ++j)
            styles[j] = style;
    };

    enum State { Start, Selector, Property, Value, AtPrelude };
    State state = Start;
    bool inAttr = false;         //inside [attr=value]
    bool expectAttrName = false;
    bool inNth = false;          //inside :nth-child(...) and similar

    int i = 0;
    while (i < len)
    {
        const char c = text[i];
        const char c1 = (i + 1 < len) ? text[i + 1] : '\0';

        //comments /* */
        if (c == '/' && c1 == '*')
        {
            const int start = i;
            const size_t end = text.find("*/", i + 2);
            i = (end == std::string::npos) ? len : static_cast<int>(end) + 2;
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        //strings
        if (c == '"' || c == '\'')
        {
            const int start = i;
            ++i;
            while (i < len)
            {
                if (text[i] == '\\')
                {
                    i += 2;
                    continue;
                }
                if (text[i] == c)
                {
                    ++i;
                    break;
                }
                if (text[i] == '\n') break; //unterminated string ends at line end
                ++i;
            }
            if (i > len) i = len;
            setStyle(start, i, STYLE_STRING);
            continue;
        }

        if (IsWhitespace(c))
        {
            ++i;
            continue;
        }

        //block structure, valid in any state
        if (c == '{' || c == '}' || c == ';')
        {
            setStyle(i, i + 1, STYLE_OPERATOR);
            ++i;
            state = Start;
            inAttr = false;
            inNth = false;
            continue;
        }

        switch (state)
        {
        case Start:
        {
            //at-rules @media, @import, @font-face ...
            if (c == '@')
            {
                const int start = i;
                i = ScanIdent(text, i + 1);
                setStyle(start, i, STYLE_KEYWORD);
                state = AtPrelude;
                continue;
            }
            state = IsDeclaration(text, i) ? Property : Selector;
            continue;
        }

        case Property:
        {
            if (c == ':')
            {
                setStyle(i, i + 1, STYLE_OPERATOR);
                ++i;
                state = Value;
                continue;
            }

            if (IsIdentChar(c) || c == '\\')
            {
                const int start = i;
                i = ScanIdent(text, i);
                const bool custom = (i - start >= 2 && text[start] == '-' &&
                                     text[start + 1] == '-');
                setStyle(start, i, custom ? STYLE_NAMESPACE : STYLE_KEYWORD);
                continue;
            }

            ++i; //stray char, for example an old "*zoom" hack
            continue;
        }

        case Value:
        case AtPrelude:
        {
            const bool atRule = (state == AtPrelude);

            //!important
            if (c == '!')
            {
                const int start = i;
                ++i;
                while (i < len && IsWhitespace(text[i])) ++i;
                i = ScanIdent(text, i);
                setStyle(start, i, STYLE_KEYWORD);
                continue;
            }

            //hex colors #fff, #1a2b3c
            if (c == '#')
            {
                const int start = i;
                ++i;
                while (i < len && std::isalnum((unsigned char)text[i])) ++i;
                setStyle(start, i, STYLE_NUMBER);
                continue;
            }

            //numbers with units
            if (IsNumberStartAt(text, i))
            {
                const int start = i;
                i = ScanNumber(text, i);
                setStyle(start, i, STYLE_NUMBER);
                continue;
            }

            //identifiers, functions and keywords
            if (IsIdentStartAt(text, i))
            {
                const int start = i;
                i = ScanIdent(text, i);
                const std::string word = Lower(text.substr(start, i - start));

                //unicode-range: U+0025-00FF
                if (word == "u" && i < len && text[i] == '+')
                {
                    ++i;
                    while (i < len && (std::isxdigit((unsigned char)text[i]) ||
                                       text[i] == '?' || text[i] == '-')) ++i;
                    setStyle(start, i, STYLE_NUMBER);
                    continue;
                }

                //function call: rgb(...), var(...), calc(...)
                if (i < len && text[i] == '(')
                {
                    setStyle(start, i, STYLE_FUNCTION);

                    //url(unquoted/path.png) is a string, may contain ; and :
                    if (word == "url")
                    {
                        int k = i + 1;
                        while (k < len && IsWhitespace(text[k])) ++k;
                        if (k < len && text[k] != '"' && text[k] != '\'')
                        {
                            setStyle(i, i + 1, STYLE_OPERATOR);
                            int end = k;
                            while (end < len && text[end] != ')' && text[end] != '\n') ++end;
                            setStyle(k, end, STYLE_STRING);
                            if (end < len && text[end] == ')')
                            {
                                setStyle(end, end + 1, STYLE_OPERATOR);
                                ++end;
                            }
                            i = end;
                        }
                    }
                    continue;
                }

                int j = i;
                while (j < len && IsWhitespace(text[j])) ++j;

                if (atRule && j < len && text[j] == ':')
                    setStyle(start, i, STYLE_KEYWORD); //media feature (min-width: ...)
                else if (word.size() >= 2 && word[0] == '-' && word[1] == '-')
                    setStyle(start, i, STYLE_NAMESPACE); //custom property in var(--x)
                else if ((atRule ? s_atWords : s_globalValues).count(word))
                    setStyle(start, i, STYLE_KEYWORD);
                continue;
            }

            //operators and punctuation
            if (std::string(",/()+-*:<>=").find(c) != std::string::npos)
                setStyle(i, i + 1, STYLE_OPERATOR);
            ++i;
            continue;
        }

        case Selector:
        {
            //inside [attr operator value]
            if (inAttr)
            {
                if (c == ']')
                {
                    setStyle(i, i + 1, STYLE_OPERATOR);
                    ++i;
                    inAttr = false;
                    continue;
                }
                if (std::string("~|^$*=").find(c) != std::string::npos)
                {
                    setStyle(i, i + 1, STYLE_OPERATOR);
                    ++i;
                    continue;
                }
                if (IsIdentStartAt(text, i) || std::isdigit((unsigned char)c))
                {
                    const int start = i;
                    i = ScanIdent(text, i);
                    if (i == start) ++i;
                    setStyle(start, i, expectAttrName ? STYLE_FUNCTION : STYLE_STRING);
                    expectAttrName = false;
                    continue;
                }
                ++i;
                continue;
            }

            //.class and #id
            if ((c == '.' || c == '#') && i + 1 < len &&
                (IsIdentChar(text[i + 1]) || text[i + 1] == '\\'))
            {
                const int start = i;
                i = ScanIdent(text, i + 1);
                setStyle(start, i, STYLE_NAMESPACE);
                continue;
            }

            //pseudo-classes and pseudo-elements :hover ::before
            if (c == ':')
            {
                const int start = i;
                ++i;
                if (i < len && text[i] == ':') ++i;
                const int nameStart = i;
                i = ScanIdent(text, i);
                setStyle(start, i, STYLE_FUNCTION);
                inNth = Lower(text.substr(nameStart, i - nameStart)).compare(0, 4, "nth-") == 0;
                continue;
            }

            //attribute selector start
            if (c == '[')
            {
                setStyle(i, i + 1, STYLE_OPERATOR);
                ++i;
                inAttr = true;
                expectAttrName = true;
                continue;
            }

            if (c == ')') inNth = false;

            //numbers: keyframe percentages, nth-child arguments
            if (IsNumberStartAt(text, i) && c != '+' && c != '-')
            {
                const int start = i;
                i = ScanNumber(text, i);
                setStyle(start, i, STYLE_NUMBER);
                continue;
            }

            //element names
            if (IsIdentStartAt(text, i))
            {
                const int start = i;
                i = ScanIdent(text, i);
                if (!inNth)
                {
                    const std::string word = Lower(text.substr(start, i - start));
                    setStyle(start, i, s_tags.count(word) ? STYLE_KEYWORD : STYLE_NAMESPACE);
                }
                continue;
            }

            //combinators and other symbols
            if (std::string(",>+~*&|()=^$").find(c) != std::string::npos)
                setStyle(i, i + 1, STYLE_OPERATOR);
            ++i;
            continue;
        }
        }
    }

    //apply styles at once
    textCtrl->StartStyling(0);
    textCtrl->SetStyleBytes(len, styles.data());
}