/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "SyntaxHighlightC.h"

void SyntaxHighlightC::ApplyHighlight(wxStyledTextCtrl* textCtrl)
{
    textCtrl->ClearDocumentStyle(); //fix
    textCtrl->SetLexer(wxSTC_LEX_NULL); //fix
    wxString text = textCtrl->GetValue();
    int length = text.length();
    
        //skip highlighting for empty text
    if (length == 0) return;

    highlightRange.occupiedRanges.clear();

        //create style array
    std::string styles(length, STYLE_DEFAULT);

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

        //preprocessor directives
    std::vector<wxString> preprocessorDirectives = {
        "#include", "#define", "#ifdef", "#ifndef", "#endif", "#pragma"
    };
    for (const auto& directive : preprocessorDirectives)
    {
        size_t pos = text.find(directive);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + directive.length())) {
                for (size_t i = pos; i < pos + directive.length(); i++) {
                    styles[i] = STYLE_PREPROCESSOR;
                }
                highlightRange.Mark(pos, pos + directive.length());
            }
            pos = text.find(directive, pos + 1);
        }
    }

        //namespaces
    std::vector<wxString> namespaces = {
        "std", "namespace"
    };
    for (const auto& ns : namespaces)
    {
        size_t pos = text.find(ns);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + ns.length())) {
                for (size_t i = pos; i < pos + ns.length(); i++) {
                    styles[i] = STYLE_NAMESPACE;
                }
                highlightRange.Mark(pos, pos + ns.length());
            }
            pos = text.find(ns, pos + 1);
        }
    }

        //function highlighting
    std::vector<wxString> controlStatements = {
        "if", "while", "for", "switch", "catch"
    };
    size_t pos = text.find("(");
    while (pos != wxString::npos) {
        if (!highlightRange.IsOccupied(pos, pos + 1)) {
            bool isControlStatement = false;
            for (const auto& stmt : controlStatements) {
                size_t checkPos = pos;
                while (checkPos > 0 && (text[checkPos - 1] == ' ' || text[checkPos - 1] == '\t')) {
                    checkPos--;
                }
                if (checkPos >= stmt.length() && 
                    text.substr(checkPos - stmt.length(), stmt.length()) == stmt &&
                    (checkPos == stmt.length() || !isalnum(text[checkPos - stmt.length() - 1]))) {
                    isControlStatement = true;
                    break;
                }
            }
            
            if (!isControlStatement) {
                size_t nameEnd = pos;
                size_t nameStart = pos;
                
                while (nameStart > 0) {
                    char ch = text[nameStart - 1];
                    if (isalnum(ch) || ch == '_' || ch == ':') {
                        nameStart--;
                    } else {
                        break;
                    }
                }
                
                if (nameStart < nameEnd) {
                    for (size_t i = nameStart; i < pos; i++) {
                        styles[i] = STYLE_FUNCTION;
                    }
                    highlightRange.Mark(nameStart, pos);
                }
            }
        }
        pos = text.find("(", pos + 1);
    }

        //types
    std::vector<wxString> types = {
        "int", "float", "double", "char", "void", "bool", "long", "short", "unsigned", "signed", "string"
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

        //standard library functions
    std::vector<wxString> standardLibraryFunctions = {
        "printf", "scanf", "endl"
    };
    for (const auto& func : standardLibraryFunctions)
    {
        size_t pos = text.find(func);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + func.length())) {
                for (size_t i = pos; i < pos + func.length(); i++) {
                    styles[i] = STYLE_FUNCTION;
                }
                highlightRange.Mark(pos, pos + func.length());
            }
            pos = text.find(func, pos + 1);
        }
    }

        //keywords
    std::vector<wxString> keywords = {
        "return", "if", "else", "while", "override", "virtual", "const", "static", "new", "delete", "this"
    };
    for (const auto& word : keywords)
    {
        size_t pos = text.find(word);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + word.length())) {
                for (size_t i = pos; i < pos + word.length(); i++) {
                    styles[i] = STYLE_KEYWORD;
                }
                highlightRange.Mark(pos, pos + word.length());
            }
            pos = text.find(word, pos + 1);
        }
    }

        //control structures
    std::vector<wxString> controlStructures = {
        "for", "while", "do", "switch", "case", "break"
    };
    for (const auto& cs : controlStructures)
    {
        size_t pos = text.find(cs);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + cs.length())) {
                for (size_t i = pos; i < pos + cs.length(); i++) {
                    styles[i] = STYLE_KEYWORD;
                }
                highlightRange.Mark(pos, pos + cs.length());
            }
            pos = text.find(cs, pos + 1);
        }
    }

        //access modifiers
    std::vector<wxString> accessModifiers = {
        "public", "private", "protected"
    };
    for (const auto& modifier : accessModifiers)
    {
        size_t pos = text.find(modifier);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + modifier.length())) {
                for (size_t i = pos; i < pos + modifier.length(); i++) {
                    styles[i] = STYLE_KEYWORD;
                }
                highlightRange.Mark(pos, pos + modifier.length());
            }
            pos = text.find(modifier, pos + 1);
        }
    }

        //literals
    std::vector<wxString> literals = {
        "true", "false", "NULL"
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

        //multi char operators
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
