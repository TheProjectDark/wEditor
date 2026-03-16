/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <unordered_set>
#include <cctype>
#include "SyntaxHighlightJava.h"
//declaring Java keywords, types, literals and standard functions for syntax highlighting
static const std::unordered_set<std::string> javaKeywords = {
    "abstract", "assert", "boolean", "break", "byte",
    "case", "try", "catch", "class", "const", "continue",
    "default", "do", "else", "enum", "extends",
    "final", "finally", "while", "for", "goto", "if", "implements",
    "public", "private", "protected", "return", "static", "strictfp", "super",
    "switch", "synchronized", "this", "throw", "throws",
    "volatile", "transient", "native", "interface",
    "import", "package", "instanceof", "new"
};
static const std::unordered_set<std::string> javaTypes = {
    "int", "long", "float", "double", "void",
    "String", "Object", "List", "Map", "Set",
};
static const std::unordered_set<std::string> javaLiterals = {
    "true", "false", "null"
};
static const std::unordered_set<std::string> javaStdFuncs = {
    "System.out.println", "System.out.print", "System.err.println", "System.err.print",
    "Math.abs", "Math.max", "Math.min", "Math.sqrt", "Math.pow",
    "String.length", "String.charAt", "String.substring", "String.indexOf",
    "List.add", "List.remove", "List.size", "List.get",
    "Map.put", "Map.get", "Map.containsKey", "Map.containsValue"
};

//helper functions for tokenization
static bool IsIdentChar(char c) { return std::isalnum((unsigned char)c) || c == '_'; }
static bool IsIdentStart(char c) { return std::isalpha((unsigned char)c) || c == '_'; }

void SyntaxHighlightJava::ApplyHighlight(wxStyledTextCtrl* textCtrl)
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

        //import keyword → STYLE_PREPROCESSOR, path → STYLE_STRING
        if (c == 'i' && text.compare(i, 6, "import") == 0 && (i + 6 >= len || !IsIdentChar(text[i + 6])))
        {
            setStyle(i, i + 6, STYLE_PREPROCESSOR);
            i += 6;
            while (i < len && (text[i] == ' ' || text[i] == '\t')) ++i; // skip whitespace
            int pathStart = i;
            while (i < len && text[i] != ';' && text[i] != '\n') ++i;
            setStyle(pathStart, i, STYLE_STRING);
            continue;
        }
        //package keyword → STYLE_PREPROCESSOR, path → STYLE_STRING
        if (c == 'p' && text.compare(i, 7, "package") == 0 && (i + 7 >= len || !IsIdentChar(text[i + 7])))
        {
            setStyle(i, i + 7, STYLE_PREPROCESSOR);
            i += 7;
            while (i < len && (text[i] == ' ' || text[i] == '\t')) ++i; // skip whitespace
            int pathStart = i;
           while (i < len && text[i] != ';' && text[i] != '\n') ++i;
            setStyle(pathStart, i, STYLE_STRING);
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

            //handle qualified names (e.g. System.out.println) as single tokens
            while (i < len && text[i] == '.')
            {
                int dotPos = i;
                ++i; //skip dot
                if (i < len && IsIdentStart(text[i]))
                {
                    while (i < len && IsIdentChar(text[i])) ++i;
                }
                else
                {
                    i = dotPos; //rollback if not a valid identifier after dot
                    break;
                }
            }
            const std::string word = text.substr(start, i - start);

            //check for function call (lookahead for '(')
            int j = i;
            while (j < len && (text[j] == ' ' || text[j] == '\t')) ++j;
            if (j < len && text[j] == '(')
            {
                //still classify as keyword if it's a known type or keyword followed by '(' (e.g. "sizeof(")
                if (javaKeywords.count(word))
                    setStyle(start, i, STYLE_KEYWORD);
                else
                    setStyle(start, i, STYLE_FUNCTION);
                continue;
            }

            //classify as keyword, type, function or literal
            if (javaKeywords.count(word))        setStyle(start, i, STYLE_KEYWORD);
            else if (javaTypes.count(word))       setStyle(start, i, STYLE_KEYWORD);
            else if (javaStdFuncs.count(word))    setStyle(start, i, STYLE_FUNCTION);
            else if (javaLiterals.count(word))    setStyle(start, i, STYLE_NUMBER);
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