/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "SyntaxHighlightAssembly.h"
#include <unordered_set>
#include <cctype>

static const std::unordered_set<std::string> s_keywords = {
    "mov","add","sub","mul","div","jmp","call","ret",
    "push","pop","cmp","je","jne","jg","jl",
    "and","or","xor","not","shl","shr",
    "eax","ebx","ecx","edx","esi","edi",
    "esp","ebp"
};

static const std::unordered_set<std::string> s_directives = {
    "section","global","extern","bits","org"
};

static const std::unordered_set<std::string> s_registers = {
    "eax","ebx","ecx","edx","esi","edi",
    "esp","ebp","rip", "rax","rbx","rcx","rdx","rsi","rdi",
    "rsp","rbp","r8","r9","r10","r11","r12","r13","r14","r15"
};

static const std::unordered_set<std::string> s_literals = {
    "0","1","2","3","4","5","6","7","8","9",
    "0x0","0x1","0x2","0x3","0x4","0x5","0x6","0x7","0x8","0x9",
    "true","false"
};

//helper functions for tokenization
static bool IsIdentChar(char c) { return std::isalnum((unsigned char)c) || c == '_' || c == '.' || c == '$'; }
static bool IsIdentStart(char c) { return std::isalpha((unsigned char)c) || c == '_' || c == '.' || c == '$'; }

void SyntaxHighlightAssembly::ApplyHighlight(wxStyledTextCtrl* textCtrl)
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
    while (i < len)    {
        const char c  = text[i];
        const char c1 = (i + 1 < len) ? text[i + 1] : '\0';
        //comments (line comments starting with ';')
        if (c == ';')
        {
            int start = i;
            while (i < len && text[i] != '\n') ++i;
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        //comments // (some assemblers support C-style comments)
        if (c == '/' && c1 == '/')
        {         
            int start = i;
            while (i < len && text[i] != '\n') ++i;
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        //comments /* */ (some assemblers support C-style comments)
        if (c == '/' && c1 == '*')
        {
            int start = i;
            i += 2;
            while (i < len - 1 && !(text[i] == '*' && text[i+1] == '/')) ++i;
            i += 2; //skip */
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        //section headers (for example: "section .data")
        if (c == 's' && text.compare(i, 7, "section") == 0 && (i + 7 >= len || !IsIdentChar(text[i + 7])))
        {
            int start = i;
            i += 7;
            while (i < len && IsIdentChar(text[i])) ++i;
            setStyle(start, i, STYLE_KEYWORD);
            continue;
        }

        //sections (for example: ".data", ".text")
        if (c == '.' && IsIdentStart(c1))
        {            
            int start = i;
            ++i; // skip dot
            while (i < len && IsIdentChar(text[i])) ++i;
            setStyle(start, i, STYLE_KEYWORD);
             continue;
        }

        //string literals (enclosed in double quotes)
        if (c == '"')
        {
            int start = i;
            ++i; // skip opening quote
            while (i < len)
            {
                if (text[i] == '\\') { i += 2; continue; } // escape sequence
                if (text[i] == '"') { ++i; break; } // closing quote
                ++i;
            }
            setStyle(start, i, STYLE_STRING);
            continue;
        }

        //identifiers and keywords
        if (IsIdentStart(c))
        {
            int start = i;
            while (i < len && IsIdentChar(text[i])) ++i;

            const std::string word = text.substr(start, i - start);

            if (s_keywords.count(word))       setStyle(start, i, STYLE_KEYWORD);
            else if (s_directives.count(word)) setStyle(start, i, STYLE_PREPROCESSOR);
            else if (s_registers.count(word))  setStyle(start, i, STYLE_NAMESPACE);
            else if (s_literals.count(word))   setStyle(start, i, STYLE_NUMBER);
            //else: STYLE_DEFAULT
            continue;
        }

        //operators and punctuation
        static const std::unordered_set<char> s_operators = {
            '+','-','*','/','%','=','<','>','!','&','|','^','~',':',','
        };
        if (s_operators.count(c))
        {
            setStyle(i, i + 1, STYLE_OPERATOR);
            ++i;
            continue;
        }

        //whitespace and other characters are default style
        ++i;
    }

    //apply styles at once
    textCtrl->StartStyling(0);
    textCtrl->SetStyleBytes(len, styles.data());
}
