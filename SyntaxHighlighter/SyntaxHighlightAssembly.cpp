/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "SyntaxHighlightAssembly.h"

void SyntaxHighlightAssembly::ApplyHighlight(wxStyledTextCtrl* textCtrl)
{
    textCtrl->ClearDocumentStyle(); //fix
    textCtrl->SetLexer(wxSTC_LEX_NULL); //fix
    wxString text = textCtrl->GetValue();
    int length = text.length();
    
    //skip highlight for empty text
    if (length == 0) return;
    
    highlightRange.occupiedRanges.clear();

    std::string styles(length, STYLE_DEFAULT);

    // Comments
    size_t pos = text.find(";");
    while (pos != wxString::npos) {
        if (!highlightRange.IsOccupied(pos, pos + 1)) {
            size_t endPos = text.find("\n", pos);
            if (endPos == wxString::npos) endPos = text.length();
            for (size_t i = pos; i < endPos; i++) {
                styles[i] = STYLE_COMMENT;
            }
            highlightRange.Mark(pos, endPos);
            pos = text.find(";", endPos);
        } else {
            pos = text.find(";", pos + 1);
        }
    }

    // Assembly instructions
    std::vector<wxString> instructions = {
        "mov", "add", "sub", "mul", "div", "jmp", "cmp", "je", "jne", "call", "ret",
        "push", "pop", "jg", "jl", "jge", "jle", "and", "or", "xor", "not", "shl", "shr",
        "lea", "int", "nop"
    };
    for (const auto& instr : instructions) {
        pos = text.find(instr);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + instr.length())) {
                for (size_t i = pos; i < pos + instr.length(); i++) {
                    styles[i] = STYLE_KEYWORD;
                }
                highlightRange.Mark(pos, pos + instr.length());
            }
            pos = text.find(instr, pos + 1);
        }
    }

    // Registers
    std::vector<wxString> registers = {
        "eax", "ebx", "ecx", "edx", "esi", "edi", "esp", "ebp",
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rsp", "rbp",
        "ax", "bx", "cx", "dx", "si", "di", "sp", "bp",
        "al", "bl", "cl", "dl"
    };
    for (const auto& reg : registers) {
        pos = text.find(reg);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + reg.length())) {
                for (size_t i = pos; i < pos + reg.length(); i++) {
                    styles[i] = STYLE_NAMESPACE;
                }
                highlightRange.Mark(pos, pos + reg.length());
            }
            pos = text.find(reg, pos + 1);
        }
    }

    // Symbols and operators
    std::vector<wxString> symbols = {":", ",", "[", "]", "+", "-", "*"};
    for (const auto& symbol : symbols) {
        pos = text.find(symbol);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + 1)) {
                styles[pos] = STYLE_OPERATOR;
                highlightRange.Mark(pos, pos + 1);
            }
            pos = text.find(symbol, pos + 1);
        }
    }

    // Labels (text followed by :)
    for (int i = 0; i < length; i++) {
        if ((isalpha(text[i]) || text[i] == '_') && (i == 0 || text[i-1] == '\n')) {
            size_t j = i;
            while (j < (size_t)length && (isalnum(text[j]) || text[j] == '_')) {
                j++;
            }
            if (j < (size_t)length && text[j] == ':' && !highlightRange.IsOccupied(i, j)) {
                for (size_t k = i; k < j; k++) {
                    styles[k] = STYLE_FUNCTION;
                }
                highlightRange.Mark(i, j);
                i = j;
            }
        }
    }
    
    //apply all styles at once
    textCtrl->StartStyling(0);
    textCtrl->SetStyleBytes(length, styles.data());
}
