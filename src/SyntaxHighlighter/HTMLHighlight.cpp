/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <weditor/SyntaxHighlighter/HTMLHighlight.h>
#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_set>

static const std::unordered_set<std::string> s_tags = {
    //document
    "html", "head", "body", "title", "base", "link", "meta", "style",
    //sections
    "article", "aside", "footer", "header", "h1", "h2", "h3", "h4", "h5", "h6",
    "hgroup", "main", "nav", "section", "search",
    //text content
    "blockquote", "dd", "div", "dl", "dt", "figcaption", "figure", "hr",
    "li", "menu", "ol", "p", "pre", "ul",
    //inline text
    "a", "abbr", "b", "bdi", "bdo", "br", "cite", "code", "data", "dfn",
    "em", "i", "kbd", "mark", "q", "rp", "rt", "ruby", "s", "samp", "small",
    "span", "strong", "sub", "sup", "time", "u", "var", "wbr",
    //media
    "area", "audio", "img", "map", "track", "video", "picture", "source",
    //embedded
    "embed", "iframe", "object", "param", "portal",
    //scripting
    "canvas", "noscript", "script",
    //edits and tables
    "del", "ins", "caption", "col", "colgroup", "table", "tbody", "td",
    "tfoot", "th", "thead", "tr",
    //forms
    "button", "datalist", "fieldset", "form", "input", "label", "legend",
    "meter", "optgroup", "option", "output", "progress", "select", "textarea",
    //interactive
    "details", "dialog", "summary",
    //web components
    "slot", "template",
    //obsolete but still common
    "center", "font", "frame", "frameset", "marquee", "tt"
};

//helper functions for tokenization
static bool IsWhitespace(char c)
{
    return std::isspace((unsigned char)c) != 0;
}

static bool IsTagStart(char c)
{
    return std::isalpha((unsigned char)c) != 0;
}

static bool IsTagChar(char c)
{
    return std::isalnum((unsigned char)c) || c == '-' || c == '_' || c == ':' || c == '.';
}

static bool IsAttrNameEnd(char c)
{
    return IsWhitespace(c) || c == '=' || c == '>' || c == '/' ||
           c == '<' || c == '"' || c == '\'';
}

void HTMLHighlight::ApplyHighlight(wxStyledTextCtrl* textCtrl)
{
    textCtrl->ClearDocumentStyle();
    textCtrl->SetLexer(wxSTC_LEX_NULL);

    const wxString wxText = textCtrl->GetValue();
    if (wxText.empty()) return;

    //STC works with UTF-8 bytes, so indexes must match its byte length
    const std::string text = wxText.ToUTF8().data();
    const int len = static_cast<int>(text.size());

    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char ch) { return (char)std::tolower(ch); });

    std::string styles(len, STYLE_DEFAULT);

    auto setStyle = [&](int from, int to, char style) {
        for (int j = from; j < to && j < len; ++j)
            styles[j] = style;
    };

    int i = 0;
    while (i < len)
    {
        const char c = text[i];
        const char c1 = (i + 1 < len) ? text[i + 1] : '\0';

        //comments <!-- -->
        if (c == '<' && lower.compare(i, 4, "<!--") == 0)
        {
            const int start = i;
            const size_t end = text.find("-->", i + 4);
            i = (end == std::string::npos) ? len : static_cast<int>(end) + 3;
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        //processing instructions <? ... ?>
        if (c == '<' && c1 == '?')
        {
            const int start = i;
            const size_t end = text.find("?>", i + 2);
            i = (end == std::string::npos) ? len : static_cast<int>(end) + 2;
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        //declarations <!DOCTYPE html>
        if (c == '<' && c1 == '!')
        {
            const int start = i;
            setStyle(i, i + 2, STYLE_OPERATOR);
            i += 2;

            const int kwStart = i;
            while (i < len && IsTagChar(text[i])) ++i;
            setStyle(kwStart, i, STYLE_KEYWORD);

            const int restStart = i;
            while (i < len && text[i] != '>') ++i;
            setStyle(restStart, i, STYLE_NAMESPACE);

            if (i < len)
            {
                setStyle(i, i + 1, STYLE_OPERATOR);
                ++i;
            }
            (void)start;
            continue;
        }

        //opening and closing tags with attributes
        if (c == '<' && (IsTagStart(c1) ||
            (c1 == '/' && i + 2 < len && IsTagStart(text[i + 2]))))
        {
            const bool closing = (c1 == '/');
            const int openLen = closing ? 2 : 1;
            setStyle(i, i + openLen, STYLE_OPERATOR);
            i += openLen;

            //tag name
            const int nameStart = i;
            while (i < len && IsTagChar(text[i])) ++i;
            const std::string tagName = lower.substr(nameStart, i - nameStart);
            setStyle(nameStart, i,
                     s_tags.count(tagName) ? STYLE_KEYWORD : STYLE_NAMESPACE);

            //attributes
            bool ended = false;
            bool selfClosed = false;
            while (i < len)
            {
                const char a = text[i];

                if (IsWhitespace(a))
                {
                    ++i;
                    continue;
                }

                if (a == '>')
                {
                    setStyle(i, i + 1, STYLE_OPERATOR);
                    ++i;
                    ended = true;
                    break;
                }

                if (a == '/' && i + 1 < len && text[i + 1] == '>')
                {
                    setStyle(i, i + 2, STYLE_OPERATOR);
                    i += 2;
                    ended = true;
                    selfClosed = true;
                    break;
                }

                //broken tag, a new one starts
                if (a == '<') break;

                if (a == '/')
                {
                    setStyle(i, i + 1, STYLE_OPERATOR);
                    ++i;
                    continue;
                }

                if (a == '=')
                {
                    setStyle(i, i + 1, STYLE_OPERATOR);
                    ++i;

                    while (i < len && IsWhitespace(text[i])) ++i;
                    if (i >= len) break;

                    const int valStart = i;
                    if (text[i] == '"' || text[i] == '\'')
                    {
                        const char delim = text[i];
                        ++i;
                        while (i < len && text[i] != delim) ++i;
                        if (i < len) ++i; //closing quote
                    }
                    else
                    {
                        //unquoted value
                        while (i < len && !IsWhitespace(text[i]) && text[i] != '>') ++i;
                    }
                    setStyle(valStart, i, STYLE_STRING);
                    continue;
                }

                //attribute name
                const int attrStart = i;
                while (i < len && !IsAttrNameEnd(text[i])) ++i;
                if (i == attrStart) ++i; //stray quote, avoid infinite loop
                else setStyle(attrStart, i, STYLE_FUNCTION);
            }

            //content of <script> and <style> is left as is
            if (ended && !closing && !selfClosed &&
                (tagName == "script" || tagName == "style"))
            {
                const size_t end = lower.find("</" + tagName, i);
                i = (end == std::string::npos) ? len : static_cast<int>(end);
            }
            continue;
        }

        //entities &amp; &#123; &#x1F;
        if (c == '&')
        {
            int j = i + 1;
            if (j < len && text[j] == '#') ++j;
            const int bodyStart = j;
            while (j < len && std::isalnum((unsigned char)text[j])) ++j;

            if (j > bodyStart && j - i <= 10 && j < len && text[j] == ';')
            {
                setStyle(i, j + 1, STYLE_NUMBER);
                i = j + 1;
                continue;
            }
        }

        ++i;
    }

    //apply styles at once
    textCtrl->StartStyling(0);
    textCtrl->SetStyleBytes(len, styles.data());
}