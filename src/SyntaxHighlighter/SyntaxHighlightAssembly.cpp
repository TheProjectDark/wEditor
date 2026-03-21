/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <weditor/SyntaxHighlighter/SyntaxHighlightAssembly.h>
#include <unordered_set>
#include <cctype>

static const std::unordered_set<std::string> s_keywords = {
    //x86 data movement
    "mov", "lea", "push", "pop",
    //x86 arithmetic
    "add", "sub", "mul", "div", "idiv", "imul", "inc", "dec", "neg",
    //x86 logic
    "and", "or", "xor", "not", "shl", "shr",
    //x86 control flow
    "call", "ret", "jmp",
    "je", "jne", "jg", "jge", "jl", "jle", "jz", "jnz",
    //x86 comparison / misc
    "cmp", "test", "nop",
    //x86 system
    "syscall", "int",
    //arm data movement
    "ldr", "str", "adrp", "adr",
    //arm arithmetic
    "add", "sub", "mul",
    //arm logic
    "and", "orr", "eor", "lsl", "lsr",
    //arm control flow
    "b", "bl", "bx", "cbz", "cbnz",
    //arm system
    "svc"
};

static const std::unordered_set<std::string> s_directives = {
    "section", "global", "extern", "bits", "org",
    "db", "dw", "dd", "dq", "resb", "resw", "resd", "resq", "equ"
};

static const std::unordered_set<std::string> s_registers = {
    //x86 segment
    "cs", "ds", "es", "fs", "gs", "ss",
    //x86 8-bit
    "al", "ah", "bl", "bh", "cl", "ch", "dl", "dh",
    //x86 16-bit
    "ax", "bx", "cx", "dx", "si", "di", "bp", "sp",
    //x86 32-bit
    "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp",
    //x86 64-bit
    "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp", "rip",
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
    //arm 64-bit general purpose
    "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",
    "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
    "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
    "x24", "x25", "x26", "x27", "x28", "x29", "x30",
    //arm 32-bit general purpose
    "w0",  "w1",  "w2",  "w3",  "w4",  "w5",  "w6",  "w7",
    "w8",  "w9",  "w10", "w11", "w12", "w13", "w14", "w15",
    "w16", "w17", "w18", "w19", "w20", "w21", "w22", "w23",
    "w24", "w25", "w26", "w27", "w28", "w29", "w30",
    //arm special
    "sp", "lr", "pc", "xzr", "wzr"
};

//helper functions for tokenization
static bool IsIdentChar(char c)  { return std::isalnum((unsigned char)c) || c == '_' || c == '$'; }
static bool IsIdentStart(char c) { return std::isalpha((unsigned char)c) || c == '_' || c == '$'; }

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
    while (i < len)
    {
        const char c  = text[i];
        const char c1 = (i + 1 < len) ? text[i + 1] : '\0';

        // line comment ';'
        if (c == ';')
        {
            int start = i;
            while (i < len && text[i] != '\n') ++i;
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        // line comment '//'
        if (c == '/' && c1 == '/')
        {
            int start = i;
            while (i < len && text[i] != '\n') ++i;
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        // block comment '/* */'
        if (c == '/' && c1 == '*')
        {
            int start = i;
            i += 2;
            while (i < len - 1 && !(text[i] == '*' && text[i + 1] == '/')) ++i;
            if (i < len - 1) i += 2; // skip '*/' only if found, avoid out-of-bounds
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        // section label (e.g. ".data", ".text") — dot-led identifiers
        if (c == '.' && i + 1 < len && IsIdentStart(c1))
        {
            int start = i;
            ++i; // skip dot
            while (i < len && IsIdentChar(text[i])) ++i;
            setStyle(start, i, STYLE_KEYWORD);
            continue;
        }

        // numeric literals: decimal, hex (0x), binary (0b), float
        if (std::isdigit((unsigned char)c) ||
            (c == '.' && std::isdigit((unsigned char)c1)))
        {
            int start = i;
            if (c == '0' && (c1 == 'x' || c1 == 'X')) // hex
            {
                i += 2;
                while (i < len && std::isxdigit((unsigned char)text[i])) ++i;
            }
            else if (c == '0' && (c1 == 'b' || c1 == 'B')) // binary
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
                // optional exponent
                if (i < len && (text[i] == 'e' || text[i] == 'E'))
                {
                    ++i;
                    if (i < len && (text[i] == '+' || text[i] == '-')) ++i;
                    while (i < len && std::isdigit((unsigned char)text[i])) ++i;
                }
            }
            // optional suffix: h (hex suffix style), b, d
            if (i < len && (text[i] == 'h' || text[i] == 'H' ||
                            text[i] == 'b' || text[i] == 'B' ||
                            text[i] == 'd' || text[i] == 'D')) ++i;
            setStyle(start, i, STYLE_NUMBER);
            continue;
        }

        // character literal 'A' or '\n'
        if (c == '\'' && c1 != '\'')
        {
            int start = i;
            ++i; // skip opening quote
            if (i < len)
            {
                if (text[i] == '\\') i += 2; // escape sequence
                else ++i;
            }
            if (i < len && text[i] == '\'') ++i; // skip closing quote
            setStyle(start, i, STYLE_NUMBER);
            continue;
        }

        // string literals
        if (c == '"')
        {
            int start = i;
            ++i;
            while (i < len)
            {
                if (text[i] == '\\') { i += 2; continue; }
                if (text[i] == '"')  { ++i; break; }
                ++i;
            }
            setStyle(start, i, STYLE_STRING);
            continue;
        }

        // identifiers, keywords, directives, registers
        if (IsIdentStart(c))
        {
            int start = i;
            while (i < len && IsIdentChar(text[i])) ++i;
            const std::string word = text.substr(start, i - start);

            if      (s_keywords.count(word))   setStyle(start, i, STYLE_KEYWORD);
            else if (s_directives.count(word))  setStyle(start, i, STYLE_PREPROCESSOR);
            else if (s_registers.count(word))   setStyle(start, i, STYLE_NAMESPACE);
            // else: STYLE_DEFAULT (labels, user identifiers)
            continue;
        }

        // operators and punctuation
        static const std::unordered_set<char> s_operators = {
            '+','-','*','/','%','=','<','>','!','&','|','^','~',':',','
        };
        if (s_operators.count(c))
        {
            setStyle(i, i + 1, STYLE_OPERATOR);
            ++i;
            continue;
        }

        ++i;
    }

    //apply styles at once
    textCtrl->StartStyling(0);
    textCtrl->SetStyleBytes(len, styles.data());
}
