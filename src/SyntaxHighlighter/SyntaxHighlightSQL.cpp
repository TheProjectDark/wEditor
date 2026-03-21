/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <weditor/SyntaxHighlighter/SyntaxHighlightSQL.h>
#include <unordered_set>
#include <cctype>

static const std::unordered_set<std::string> s_keywords = {
    //dml
    "select", "from", "where", "insert", "update", "delete", "into", "values", "set",
    //ddl
    "create", "alter", "drop", "table", "index", "view", "trigger", "procedure", "function",
    //joins
    "join", "inner", "left", "right", "outer", "cross", "on",
    //clauses
    "group", "by", "order", "having", "distinct", "as", "union", "all",
    //logic
    "and", "or", "not", "in", "is", "like", "between", "exists", "any",
    //conditionals
    "case", "when", "then", "else", "end",
    //constraints
    "primary", "foreign", "key", "references", "default", "unique", "constraint", "null",
    //multi-word handled as separate tokens
    "with", "limit", "offset", "returning"
};

static const std::unordered_set<std::string> s_types = {
    //integer
    "bigint", "int", "integer", "smallint", "tinyint",
    //string
    "varchar", "nvarchar", "char", "nchar", "text",
    //float
    "float", "real", "double", "decimal", "numeric",
    //date / time
    "date", "datetime", "timestamp", "time",
    //misc
    "boolean", "bool", "blob", "binary"
};

static const std::unordered_set<std::string> s_functions = {
    //aggregate
    "count", "sum", "avg", "min", "max",
    //math
    "abs", "ceil", "ceiling", "floor", "round", "power", "sqrt", "mod",
    //string
    "upper", "lower", "length", "trim", "ltrim", "rtrim",
    "substring", "replace", "charindex", "concat", "coalesce", "nullif",
    //conversion
    "cast", "convert",
    //date
    "now", "getdate", "datepart", "datediff", "dateadd", "curdate", "curtime"
};

//helpers
static bool IsIdentChar(char c)  { return std::isalnum((unsigned char)c) || c == '_'; }
static bool IsIdentStart(char c) { return std::isalpha((unsigned char)c) || c == '_'; }

static std::string ToLower(const std::string& s)
{
    std::string out = s;
    for (char& c : out) c = static_cast<char>(std::tolower((unsigned char)c));
    return out;
}

void SyntaxHighlightSQL::ApplyHighlight(wxStyledTextCtrl* textCtrl)
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

        //block comment /* */
        if (c == '/' && c1 == '*')
        {
            int start = i;
            i += 2;
            while (i < len - 1 && !(text[i] == '*' && text[i + 1] == '/')) ++i;
            if (i < len - 1) i += 2; //skip '*/' only if found, avoid out-of-bounds
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        //line comment --
        if (c == '-' && c1 == '-')
        {
            int start = i;
            while (i < len && text[i] != '\n') ++i;
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        //line comment # (mysql style)
        if (c == '#')
        {
            int start = i;
            while (i < len && text[i] != '\n') ++i;
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        //string literals (single or double quoted)
        if (c == '\'' || c == '"')
        {
            int start = i;
            const char delim = c;
            ++i; //skip opening quote
            while (i < len)
            {
                if (text[i] == '\\') { i += 2; continue; } //escape sequence
                if (text[i] == delim) { ++i; break; }       //closing quote
                ++i;
            }
            setStyle(start, i, STYLE_STRING);
            continue;
        }

        //backtick quoted identifiers (mysql style)
        if (c == '`')
        {
            int start = i;
            ++i;
            while (i < len && text[i] != '`') ++i;
            if (i < len) ++i; //skip closing backtick
            setStyle(start, i, STYLE_STRING);
            continue;
        }

        //numeric literals
        if (std::isdigit((unsigned char)c) || (c == '.' && std::isdigit((unsigned char)c1)))
        {
            int start = i;
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
            setStyle(start, i, STYLE_NUMBER);
            continue;
        }

        //identifiers and keywords (case-insensitive)
        if (IsIdentStart(c))
        {
            int start = i;
            while (i < len && IsIdentChar(text[i])) ++i;
            const std::string word     = text.substr(start, i - start);
            const std::string wordLow  = ToLower(word);

            //lookahead for '(' to detect function call
            int j = i;
            while (j < len && (text[j] == ' ' || text[j] == '\t')) ++j;
            if (j < len && text[j] == '(')
            {
                if (s_keywords.count(wordLow))       setStyle(start, i, STYLE_KEYWORD);
                else if (s_functions.count(wordLow)) setStyle(start, i, STYLE_FUNCTION);
                else                                  setStyle(start, i, STYLE_FUNCTION);
                continue;
            }

            if      (s_keywords.count(wordLow))  setStyle(start, i, STYLE_KEYWORD);
            else if (s_types.count(wordLow))      setStyle(start, i, STYLE_NAMESPACE);
            else if (s_functions.count(wordLow))  setStyle(start, i, STYLE_FUNCTION);
            //else: STYLE_DEFAULT (table names, column names, aliases)
            continue;
        }

        //multi-char operators
        if (i + 1 < len)
        {
            static const std::unordered_set<std::string> s_ops2 = {
                "<>", "!=", "<=", ">=", ":="
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
        static const std::string s_singles = "=<>+-*/%(),;.!&|^~";
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