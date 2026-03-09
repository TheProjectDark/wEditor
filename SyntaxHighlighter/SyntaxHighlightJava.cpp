/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "SyntaxHighlightJava.h"

void SyntaxHighlightJava::ApplyHighlight(wxStyledTextCtrl* textCtrl)
{
    textCtrl->ClearDocumentStyle(); //fix
    textCtrl->SetLexer(wxSTC_LEX_NULL); //fix
    wxString text = textCtrl->GetValue();
    int length = text.length();
    
    //skip highlighting for empty text
    if (length == 0) return;
    
    highlightRange.occupiedRanges.clear();

    std::string styles(length, STYLE_DEFAULT);
    for (int i = 0; i < length; i++) {
        styles[i] = STYLE_DEFAULT;
    }

    //highlightign import as preprocessor for simplicity
    size_t pos = text.find("import");
    while (pos != wxString::npos) {
        if (!highlightRange.IsOccupied(pos, pos + 6)) {
            for (size_t i = pos; i < pos + 6; i++) {
                styles[i] = STYLE_PREPROCESSOR;
            }
            highlightRange.Mark(pos, pos + 6);
        }
        pos = text.find("import", pos + 1);
    }
    //highlighting import path as string for simplicity
    pos = text.find("import");
    while (pos != wxString::npos) {
        size_t startPos = text.find(" ", pos);
        if (startPos != wxString::npos) {
            startPos++;
            size_t endPos = text.find(";", startPos);
            if (endPos != wxString::npos) {
                for (size_t i = startPos; i < endPos; i++) {
                    styles[i] = STYLE_STRING;
                }
                highlightRange.Mark(startPos, endPos);
                pos = text.find("import", endPos);
            } else {
                break;
            }
        } else {            break;
        }
    }

    //comments
    std::vector<wxString> comments = {
        "//", "/*", "*/"
    };
    for (const auto& comment : comments)
    {
        size_t pos = text.find(comment);
        while (pos != wxString::npos) {
            size_t endPos;
            if (comment == "//") {
                endPos = text.find("\n", pos);
                if (endPos == wxString::npos) endPos = text.length();
            } else if (comment == "/*") {
                endPos = text.find("*/", pos);
                if (endPos != wxString::npos) endPos += 2;
                else endPos = text.length();
            } else {
                pos = text.find(comment, pos + 1);
                continue;
            }
            for (size_t i = pos; i < endPos; i++) {
                styles[i] = STYLE_COMMENT;
            }
            highlightRange.Mark(pos, endPos);
            pos = text.find(comment, endPos);
        }
    }

    //strings
    std::vector<wxString> stringDelimiters = {
        "\"", "'"
    };
    for (const auto& delimiter : stringDelimiters)
    {
        size_t pos = text.find(delimiter);
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
        "public", "private", "protected", "static", "final", "if", "else", "switch", "case", 
        "for", "while", "do", "return", "new", "this", "super", "extends", "implements", 
        "try", "catch", "finally", "throw", "throws", "package", "class", "interface"
    };
    for (const auto& keyword : keywords)
    {
        size_t pos = text.find(keyword);
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

    //types
    std::vector<wxString> types = {
        "int", "float", "double", "char", "void", "boolean", "long", "short", "byte", "String"
    };
    for (const auto& type : types)
    {
        size_t pos = text.find(type);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + type.length())) {
                for (size_t i = pos; i < pos + type.length(); i++) {
                    styles[i] = STYLE_KEYWORD;
                }
                highlightRange.Mark(pos, pos + type.length());
            }
            pos = text.find(type, pos + 1);
        }
    }

    //literals
    std::vector<wxString> literals = {
        "true", "false", "null"
    };
    for (const auto& literal : literals)
    {
        size_t pos = text.find(literal);
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
    std::vector<wxString> operators_multi = {
        "==", "!=", "<=", ">=", "&&", "||", "++", "--"
    };
    for (const auto& op : operators_multi)
    {
        size_t pos = text.find(op);
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
    
    //single char operators
    std::vector<wxString> operators_single = {
        "+", "-", "*", "/", "=", "<", ">"
    };
    for (const auto& op : operators_single)
    {
        size_t pos = text.find(op);
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
    std::vector<wxString> symbols = {
        "{", "}", "(", ")", "[", "]", ";", ",", "."
    };
    for (const auto& symbol : symbols)
    {
        size_t pos = text.find(symbol);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + symbol.length())) {
                for (size_t i = pos; i < pos + symbol.length(); i++) {
                    styles[i] = STYLE_OPERATOR;
                }
                highlightRange.Mark(pos, pos + symbol.length());
            }
            pos = text.find(symbol, pos + 1);
        }
    }
    
    //apply all styles at once
    textCtrl->StartStyling(0);
    textCtrl->SetStyleBytes(length, styles.data());
}
