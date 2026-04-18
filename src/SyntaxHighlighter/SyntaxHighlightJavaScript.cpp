/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <weditor/SyntaxHighlighter/SyntaxHighlightJavaScript.h>
#include <cctype>
#include <string>
#include <unordered_set>

static const std::unordered_set<std::string> s_keywords = {
    //declarations
    "class", "const", "function", "let", "var",
    //modules
    "as", "default", "export", "from", "import",
    //control flow
    "break", "case", "catch", "continue", "do", "else",
    "finally", "for", "if", "return", "switch", "throw",
    "try", "while",
    //expressions
    "async", "await", "delete", "in", "instanceof",
    "new", "of", "typeof", "void", "yield",
    //object model
    "extends", "get", "set", "static", "super", "this",
    //reserved / contextual
    "debugger", "enum", "implements", "interface",
    "package", "private", "protected", "public", "with"
};

static const std::unordered_set<std::string> s_types = {
    //primitive wrappers
    "BigInt", "Boolean", "Number", "String", "Symbol",
    //core objects
    "Array", "Function", "Object", "Promise", "Proxy", "RegExp",
    //collections
    "Map", "Set", "WeakMap", "WeakSet", "WeakRef",
    //binary data
    "ArrayBuffer", "DataView", "Int8Array", "Int16Array", "Int32Array",
    "Uint8Array", "Uint8ClampedArray", "Uint16Array", "Uint32Array",
    "BigInt64Array", "BigUint64Array", "Float32Array", "Float64Array",
    "SharedArrayBuffer",
    //dates and internationalization
    "Date", "Intl",
    //errors
    "AggregateError", "Error", "EvalError", "RangeError",
    "ReferenceError", "SyntaxError", "TypeError", "URIError",
    //built-in namespaces
    "Atomics", "JSON", "Math", "Reflect", "WebAssembly"
};

static const std::unordered_set<std::string> s_stdFuncs = {
    //global functions
    "decodeURI", "decodeURIComponent", "encodeURI", "encodeURIComponent",
    "eval", "isFinite", "isNaN", "parseFloat", "parseInt",
    "queueMicrotask", "structuredClone",
    //timers
    "clearInterval", "clearTimeout", "setInterval", "setTimeout",
    //console
    "console.debug", "console.dir", "console.error", "console.info",
    "console.log", "console.table", "console.warn",
    //array
    "Array.from", "Array.isArray", "Array.of",
    //object
    "Object.assign", "Object.create", "Object.entries", "Object.freeze",
    "Object.fromEntries", "Object.keys", "Object.values",
    //number
    "Number.isFinite", "Number.isInteger", "Number.isNaN",
    "Number.parseFloat", "Number.parseInt",
    //string
    "String.fromCharCode", "String.fromCodePoint", "String.raw",
    //date
    "Date.now", "Date.parse", "Date.UTC",
    //json
    "JSON.parse", "JSON.stringify",
    //math
    "Math.abs", "Math.ceil", "Math.floor", "Math.max", "Math.min",
    "Math.pow", "Math.random", "Math.round", "Math.sqrt", "Math.trunc",
    //promise
    "Promise.all", "Promise.allSettled", "Promise.any", "Promise.race",
    "Promise.reject", "Promise.resolve",
    //symbol
    "Symbol.for", "Symbol.keyFor",
    //reflect
    "Reflect.apply", "Reflect.construct", "Reflect.defineProperty",
    "Reflect.deleteProperty", "Reflect.get", "Reflect.has",
    "Reflect.ownKeys", "Reflect.set"
};

static const std::unordered_set<std::string> s_literals = {
    "false", "globalThis", "Infinity", "NaN", "null", "true", "undefined"
};

//helper functions for tokenization
static bool IsIdentChar(char c)
{
    return std::isalnum((unsigned char)c) || c == '_' || c == '$';
}

static bool IsIdentStart(char c)
{
    return std::isalpha((unsigned char)c) || c == '_' || c == '$';
}

static bool IsWhitespace(char c)
{
    return std::isspace((unsigned char)c) != 0;
}

static bool IsDecimalDigitOrSeparator(char c)
{
    return std::isdigit((unsigned char)c) || c == '_';
}

static bool IsHexDigitOrSeparator(char c)
{
    return std::isxdigit((unsigned char)c) || c == '_';
}

void SyntaxHighlightJavaScript::ApplyHighlight(wxStyledTextCtrl* textCtrl)
{
    textCtrl->ClearDocumentStyle();
    textCtrl->SetLexer(wxSTC_LEX_NULL);

    const wxString wxText = textCtrl->GetValue();
    if (wxText.empty()) return;

    const std::string text = wxText.ToStdString();
    const int len = static_cast<int>(text.size());

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

        //one line comment //
        if (c == '/' && c1 == '/')
        {
            const int start = i;
            while (i < len && text[i] != '\n') ++i;
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        //multi line comments /* */
        if (c == '/' && c1 == '*')
        {
            const int start = i;
            i += 2;
            while (i + 1 < len && !(text[i] == '*' && text[i + 1] == '/')) ++i;
            if (i + 1 < len) i += 2;
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        //single, double and template strings
        if (c == '"' || c == '\'' || c == '`')
        {
            const int start = i;
            const char delim = c;
            ++i;
            while (i < len)
            {
                if (text[i] == '\\')
                {
                    i += 2;
                    continue;
                }
                if (text[i] == delim)
                {
                    ++i;
                    break;
                }
                ++i;
            }
            setStyle(start, i, STYLE_STRING);
            continue;
        }

        //numbers
        if (std::isdigit((unsigned char)c) ||
            (c == '.' && i + 1 < len && std::isdigit((unsigned char)text[i + 1])))
        {
            const int start = i;

            if (c == '0' && (c1 == 'x' || c1 == 'X'))
            {
                i += 2;
                while (i < len && IsHexDigitOrSeparator(text[i])) ++i;
            }
            else if (c == '0' && (c1 == 'b' || c1 == 'B'))
            {
                i += 2;
                while (i < len && (text[i] == '0' || text[i] == '1' || text[i] == '_')) ++i;
            }
            else if (c == '0' && (c1 == 'o' || c1 == 'O'))
            {
                i += 2;
                while (i < len && ((text[i] >= '0' && text[i] <= '7') || text[i] == '_')) ++i;
            }
            else
            {
                if (c != '.')
                {
                    while (i < len && IsDecimalDigitOrSeparator(text[i])) ++i;
                }

                if (i < len && text[i] == '.')
                {
                    ++i;
                    while (i < len && IsDecimalDigitOrSeparator(text[i])) ++i;
                }

                if (i < len && (text[i] == 'e' || text[i] == 'E'))
                {
                    ++i;
                    if (i < len && (text[i] == '+' || text[i] == '-')) ++i;
                    while (i < len && IsDecimalDigitOrSeparator(text[i])) ++i;
                }
            }

            if (i < len && text[i] == 'n') ++i; //bigint suffix

            setStyle(start, i, STYLE_NUMBER);
            continue;
        }

        //identifiers, dotted names and keywords
        if (IsIdentStart(c))
        {
            const int start = i;
            while (i < len && IsIdentChar(text[i])) ++i;

            while (i + 1 < len && text[i] == '.' && IsIdentStart(text[i + 1]))
            {
                ++i; //skip dot
                while (i < len && IsIdentChar(text[i])) ++i;
            }

            const std::string word = text.substr(start, i - start);

            int j = i;
            while (j < len && IsWhitespace(text[j])) ++j;
            const bool isCall = j < len && (text[j] == '(' || text[j] == '`');

            if (s_keywords.count(word))
                setStyle(start, i, STYLE_KEYWORD);
            else if (s_literals.count(word))
                setStyle(start, i, STYLE_NUMBER);
            else if (s_types.count(word))
                setStyle(start, i, STYLE_NAMESPACE);
            else if (s_stdFuncs.count(word) || isCall)
                setStyle(start, i, STYLE_FUNCTION);
            continue;
        }

        //multi-char operators
        if (i + 3 < len)
        {
            const std::string op4 = text.substr(i, 4);
            static const std::unordered_set<std::string> s_ops4 = {
                ">>>=", "<<=", ">>=", "**=", "&&=", "||=", "?" "?="
            };
            if (s_ops4.count(op4))
            {
                setStyle(i, i + 4, STYLE_OPERATOR);
                i += 4;
                continue;
            }
        }

        if (i + 2 < len)
        {
            const std::string op3 = text.substr(i, 3);
            static const std::unordered_set<std::string> s_ops3 = {
                "===", "!==", ">>>"
            };
            if (s_ops3.count(op3))
            {
                setStyle(i, i + 3, STYLE_OPERATOR);
                i += 3;
                continue;
            }
        }

        if (i + 1 < len)
        {
            const std::string op2 = text.substr(i, 2);
            static const std::unordered_set<std::string> s_ops2 = {
                "=>", "?.", "?" "?", "&&", "||", "==", "!=", "<=", ">=",
                "++", "--", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
                "<<", ">>", "**"
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
