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
    //access modifiers
    "private", "protected", "public",
    //type modifiers
    "auto", "const", "explicit", "extern", "inline", "override",
    "register", "static", "typedef", "typename", "virtual", "volatile",
    //type declarations
    "class", "enum", "namespace", "struct", "template",
    //control flow
    "break", "case", "continue", "do", "else", "for", "goto", "if", "return", "switch", "while",
    //memory
    "delete", "new",
    //expressions / operators
    "alignof", "decltype", "sizeof", "using",
    //misc
    "this",
    //c++11
    "constexpr", "final", "noexcept", "static_assert", "thread_local",
    //c++17
    "if constexpr",
    //c++20
    "concept", "consteval", "constinit", "co_await", "co_return", "co_yield", "export", "import", "module", "requires"
};

static const std::unordered_set<std::string> s_types = {
    //primitives
    "bool", "char", "double", "float", "int", "long", "short",
    "signed", "unsigned", "void", "wchar_t",
    //platform types
    "nullptr_t", "ptrdiff_t", "size_t",
    //c++11 — fixed width integers
    "int8_t", "int16_t", "int32_t", "int64_t",
    "uint8_t", "uint16_t", "uint32_t", "uint64_t",
    "int_fast8_t", "int_fast16_t", "int_fast32_t", "int_fast64_t",
    "int_least8_t", "int_least16_t", "int_least32_t", "int_least64_t",
    "intmax_t", "intptr_t", "uintmax_t", "uintptr_t",
    //c++11 — char types
    "char16_t", "char32_t",
    //c++20 — char types
    "char8_t",
    //strings and containers
    "map", "pair", "set", "string", "tuple", "unordered_map", "vector",
    //c++11 — containers and utilities
    "array", "forward_list", "unordered_set", "unordered_multimap", "unordered_multiset",
    "initializer_list", "nullptr_t", "type_index",
    //c++11 — smart pointers
    "shared_ptr", "unique_ptr", "weak_ptr",
    //c++17 — utilities
    "any", "optional", "string_view", "variant",
    //c++17 — filesystem
    "path",
    //c++20 — utilities
    "span", "source_location",
    //c++23 — utilities
    "expected", "stacktrace"
};

static const std::unordered_set<std::string> s_stdFuncs = {
    //i/o streams
    "cerr", "cin", "clog", "cout", "endl",
    "std::cerr", "std::cin", "std::cout", "std::endl",
    //c string / memory
    "memcpy", "memset", "strcat", "strcmp", "strcpy", "strlen",
    //memory management
    "calloc", "free", "malloc", "realloc",
    //std containers
    "std::vector",
    //program control
    "abort", "assert", "exit",
    //c i/o
    "printf", "scanf",
    //c++11 — memory
    "std::make_shared", "std::make_unique",
    "std::move", "std::forward", "std::swap",
    //c++11 — algorithms
    "std::sort", "std::find", "std::find_if", "std::count", "std::count_if",
    "std::copy", "std::transform", "std::accumulate",
    "std::for_each", "std::remove", "std::remove_if",
    "std::lower_bound", "std::upper_bound", "std::binary_search",
    "std::min", "std::max", "std::min_element", "std::max_element",
    "std::reverse", "std::fill", "std::fill_n",
    //c++11 — threading
    "std::thread", "std::mutex", "std::lock_guard",
    "std::unique_lock", "std::condition_variable",
    "std::async", "std::future", "std::promise",
    //c++11 — utilities
    "std::bind", "std::function", "std::ref", "std::cref",
    "std::pair", "std::make_pair", "std::tuple", "std::make_tuple", "std::tie",
    "std::to_string", "std::stoi", "std::stof", "std::stod", "std::stol",
    //c++11 — type traits
    "std::is_same", "std::is_base_of", "std::is_convertible",
    "std::enable_if", "std::decay", "std::remove_reference",
    //c++14 — utilities
    "std::make_unique",
    //c++17 — utilities
    "std::optional", "std::any", "std::variant",
    "std::visit", "std::holds_alternative", "std::get_if",
    "std::string_view",
    "std::apply", "std::invoke",
    "std::clamp",
    //c++17 — filesystem
    "std::filesystem::path", "std::filesystem::exists",
    "std::filesystem::copy", "std::filesystem::remove",
    "std::filesystem::create_directory",
    //c++20 — ranges
    "std::ranges::sort", "std::ranges::find", "std::ranges::for_each",
    "std::ranges::copy", "std::ranges::transform", "std::ranges::filter",
    //c++20 — utilities
    "std::span", "std::bit_cast", "std::midpoint", "std::lerp",
    "std::format", "std::source_location::current",
    //c++23 — utilities
    "std::expected", "std::print", "std::println"
};

static const std::unordered_set<std::string> s_literals = {
    //boolean / null
    "NULL", "false", "nullptr", "true",
    //c++11 — user-defined literal suffixes (as identifiers)
    "this"
};

//helper functions for tokenization
static bool IsIdentChar(char c) { return std::isalnum((unsigned char)c) || c == '_'; }
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