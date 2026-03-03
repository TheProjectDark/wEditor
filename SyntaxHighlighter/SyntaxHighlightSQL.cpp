/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "SyntaxHighlightSQL.h"

void SyntaxHighlightSQL::ApplyHighlight(wxStyledTextCtrl* textCtrl)
{
    textCtrl->ClearDocumentStyle(); //fix
    textCtrl->SetLexer(wxSTC_LEX_NULL); //fix
    wxString text = textCtrl->GetValue();
    int length = text.length();
    
    // Skip highlighting for empty text
    if (length == 0) return;
    
    highlightRange.occupiedRanges.clear();

    std::string styles(length, STYLE_DEFAULT);

    // Comments--
    size_t pos = text.find("--");
    while (pos != wxString::npos) {
        if (!highlightRange.IsOccupied(pos, pos + 2)) {
            size_t endPos = text.find("\n", pos);
            if (endPos == wxString::npos) endPos = text.length();
            for (size_t i = pos; i < endPos; i++) {
                styles[i] = STYLE_COMMENT;
            }
            highlightRange.Mark(pos, endPos);
            pos = text.find("--", endPos);
        } else {
            pos = text.find("--", pos + 2);
        }
    }

    // Comments /* */
    pos = text.find("/*");
    while (pos != wxString::npos) {
        if (!highlightRange.IsOccupied(pos, pos + 2)) {
            size_t endPos = text.find("*/", pos);
            if (endPos != wxString::npos) {
                endPos += 2;
                for (size_t i = pos; i < endPos; i++) {
                    styles[i] = STYLE_COMMENT;
                }
                highlightRange.Mark(pos, endPos);
                pos = text.find("/*", endPos);
            } else {
                break;
            }
        } else {
            pos = text.find("/*", pos + 2);
        }
    }

    // Strings
    std::vector<wxString> stringDelimiters = {"'", "\""};
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

    // Keywords
    std::vector<wxString> keywords = {
        "SELECT", "FROM", "WHERE", "INSERT", "UPDATE", "DELETE", "JOIN", "INNER", "LEFT", "RIGHT",
        "ON", "CREATE", "TABLE", "ALTER", "DROP", "INDEX", "VIEW", "TRIGGER", "PROCEDURE",
        "FUNCTION", "UNION", "GROUP", "ORDER", "BY", "HAVING", "DISTINCT", "AS", "AND", "OR",
        "NOT", "IN", "IS", "NULL", "LIKE", "BETWEEN", "EXISTS",
        "select", "from", "where", "insert", "update", "delete", "join", "inner", "left", "right",
        "on", "create", "table", "alter", "drop", "index", "view", "trigger", "procedure",
        "function", "union", "group", "order", "by", "having", "distinct", "as", "and", "or",
        "not", "in", "is", "null", "like", "between", "exists"
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

    // Data types
    std::vector<wxString> dataTypes = {
        "INT", "VARCHAR", "CHAR", "TEXT", "FLOAT", "DOUBLE", "DECIMAL", "DATE", "TIMESTAMP",
        "int", "varchar", "char", "text", "float", "double", "decimal", "date", "timestamp"
    };
    for (const auto& dtype : dataTypes) {
        pos = text.find(dtype);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + dtype.length())) {
                for (size_t i = pos; i < pos + dtype.length(); i++) {
                    styles[i] = STYLE_NAMESPACE;
                }
                highlightRange.Mark(pos, pos + dtype.length());
            }
            pos = text.find(dtype, pos + 1);
        }
    }

    // Functions
    std::vector<wxString> functions = {
        "COUNT", "SUM", "AVG", "MIN", "MAX", "ROUND", "UPPER", "LOWER", "LENGTH",
        "count", "sum", "avg", "min", "max", "round", "upper", "lower", "length"
    };
    for (const auto& func : functions) {
        pos = text.find(func);
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

    // Operators and symbols
    std::vector<wxString> operators = {"=", "<", ">", "<=", ">=", "<>", "!=", "+", "-", "*", "/", "%", "(", ")", ",", ";"};
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
    
    //apply all styles at once
    textCtrl->StartStyling(0);
    textCtrl->SetStyleBytes(length, styles.data());
}
