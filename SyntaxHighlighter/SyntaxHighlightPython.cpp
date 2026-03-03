/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "SyntaxHighlightPython.h"

void SyntaxHighlightPython::ApplyHighlight(wxStyledTextCtrl* textCtrl)
{
    textCtrl->ClearDocumentStyle(); //fix
    textCtrl->SetLexer(wxSTC_LEX_NULL); //fix
    wxString text = textCtrl->GetValue();
    int length = text.length();
    
        //skip highlighting for empty text
    if (length == 0) return;
    
    highlightRange.occupiedRanges.clear();

    std::string styles(length, STYLE_DEFAULT);

        //comments
    size_t pos = text.find("#");
    while (pos != wxString::npos) {
        if (!highlightRange.IsOccupied(pos, pos + 1)) {
            size_t endPos = text.find("\n", pos);
            if (endPos == wxString::npos) endPos = text.length();
            for (size_t i = pos; i < endPos; i++) {
                styles[i] = STYLE_COMMENT;
            }
            highlightRange.Mark(pos, endPos);
            pos = text.find("#", endPos);
        } else {
            pos = text.find("#", pos + 1);
        }
    }

        //strings
    std::vector<wxString> stringDelimiters = {"\"", "'"};
    for (const auto& delimiter : stringDelimiters) {
        pos = text.find(delimiter);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + 1)) {
                size_t endPos = text.find(delimiter, pos + 1);
                if (endPos != wxString::npos) {
                    for (size_t i = pos; i <= endPos; i++) {
                        styles[i] = STYLE_STRING;
                    }
                    highlightRange.Mark(pos, endPos + 1);
                    pos = text.find(delimiter, endPos + 1);
                } else {
                    break;
                }
            } else {
                pos = text.find(delimiter, pos + 1);
            }
        }
    }

        //keywords
    std::vector<wxString> keywords = {
        "def", "class", "if", "elif", "else", "for", "while", "return", "try", "except",
        "finally", "with", "pass", "break", "continue", "lambda", "import", "from", "as"
    };
    for (const auto& keyword : keywords) {
        pos = text.find(keyword);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + keyword.length())) {
                for (size_t i = pos; i < pos + keyword.length(); i++) {
                    styles[i] = STYLE_KEYWORD;
                }
                highlightRange.Mark(pos, pos + keyword.length());
            }
            pos = text.find(keyword, pos + 1);
        }
    }

        //built-in functions
    std::vector<wxString> builtins = {
        "print", "len", "range", "enumerate", "map", "filter", "zip", "sum", "min", "max", "open"
    };
    for (const auto& builtin : builtins) {
        pos = text.find(builtin);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + builtin.length())) {
                for (size_t i = pos; i < pos + builtin.length(); i++) {
                    styles[i] = STYLE_FUNCTION;
                }
                highlightRange.Mark(pos, pos + builtin.length());
            }
            pos = text.find(builtin, pos + 1);
        }
    }

        //literals
    std::vector<wxString> literals = {"True", "False", "None"};
    for (const auto& literal : literals) {
        pos = text.find(literal);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + literal.length())) {
                for (size_t i = pos; i < pos + literal.length(); i++) {
                    styles[i] = STYLE_NUMBER;
                }
                highlightRange.Mark(pos, pos + literal.length());
            }
            pos = text.find(literal, pos + 1);
        }
    }

        //operators
    std::vector<wxString> operators = {
        "==", "!=", "<=", ">=", "**", "//", "->", "+=", "-=", "*=", "/=",
        "+", "-", "*", "/", "=", "<", ">", "&", "|", "^"
    };
    for (const auto& op : operators) {
        pos = text.find(op);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + op.length())) {
                for (size_t i = pos; i < pos + op.length(); i++) {
                    styles[i] = STYLE_OPERATOR;
                }
                highlightRange.Mark(pos, pos + op.length());
            }
            pos = text.find(op, pos + 1);
        }
    }

        //symbols
    std::vector<wxString> symbols = {"(", ")", "[", "]", "{", "}", ":", ";", ",", "."};
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
    
    //apply all styles at once
    textCtrl->StartStyling(0);
    textCtrl->SetStyleBytes(length, styles.data());
}
