/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

 #include "SyntaxHighlightCSharp.h"

 void SyntaxHighlightCSharp::ApplyHighlight(wxStyledTextCtrl* textCtrl) {
    textCtrl->ClearDocumentStyle();
    textCtrl->SetLexer(wxSTC_LEX_NULL);
    wxString text = textCtrl->GetValue();
    int length = text.length();

    //skip highlighting for empty text
    if (length == 0) return;

    highlightRange.occupiedRanges.clear();

    //create style array
    std::string styles(length, STYLE_DEFAULT);

    //highlight preprocessor's like using and namespace
    std::vector<std::string> preprocessorKeywords = {
        "using", "namespace", "assembly", "module", "typeforwardedto"
    };
    for (const auto& keyword : preprocessorKeywords) {
        size_t pos = text.find(keyword);
        while (pos != std::string::npos) {
            //check if this keyword is not part of another word
            if ((pos == 0 || !isalnum(text[pos - 1])) && (pos + keyword.length() >= text.length() || !isalnum(text[pos + keyword.length()]))) {
                size_t end = pos + keyword.length();
                if (!highlightRange.IsOccupied(pos, end)) {
                    std::fill(styles.begin() + pos, styles.begin() + end, STYLE_PREPROCESSOR);
                    highlightRange.Mark(pos, end);
                }
            }
            pos = text.find(keyword, pos + 1);
        }
    }
    //highlight path after using keyword as string
    size_t usingPos = text.find("using");
    while (usingPos != std::string::npos) {
        size_t semicolonPos = text.find(';', usingPos);
        if (semicolonPos != std::string::npos) {
            size_t start = usingPos + 5; // length of "using"
            if (start < semicolonPos) {
                if (!highlightRange.IsOccupied(start, semicolonPos)) {
                    std::fill(styles.begin() + start, styles.begin() + semicolonPos, STYLE_STRING);
                    highlightRange.Mark(start, semicolonPos);
                }
            }
            usingPos = text.find("using", semicolonPos);
        } else {
            break;
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
        "abstract", "as", "base", "break", "case", "catch", "class", "const", "continue",
        "default", "delegate", "do", "else", "enum", "event", "explicit", "extern", "finally",
        "fixed", "for", "foreach", "goto", "if", "implicit", "in", "interface", "internal",
        "is", "lock", "new", "operator", "out", "override", "params", "private", "protected", "public",
        "readonly", "ref", "return", "sealed", "sizeof", "stackalloc", "switch", "this", "throw", "try",
        "typeof", "unchecked", "unsafe", "virtual", "void", "volatile", "while"
    };
    for (const auto& keyword : keywords)
    {
        size_t pos = text.find(keyword);
        while (pos != wxString::npos) {
            //check if this keyword is not part of another word
            if ((pos == 0 || !isalnum(text[pos - 1])) && (pos + keyword.length() >= text.length() || !isalnum(text[pos + keyword.length()]))) {
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
                    styles[i] = STYLE_KEYWORD;
                }
                highlightRange.Mark(pos, pos + literal.length());
            }
            pos = text.find(literal, pos + 1);
        }
    }

    //double char operators
    std::vector<wxString> operators_multi = {
        "==", "!=", "<=", ">=", "&&", "||", "++", "--", "::", "->", "+=", "-=", "*=", "/=", "%="
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
        "+", "-", "*", "/", "=", "<", ">", "%", "&", "|", "^", "!", "~", "?", ":"
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