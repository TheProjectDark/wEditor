/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "SyntaxHighlightSQL.h"

//case-insensitive search helper
static size_t FindCI(const wxString& text, const wxString& needle, size_t from = 0) {
    wxString textUpper = text.Upper();
    wxString needleUpper = needle.Upper();
    return textUpper.find(needleUpper, from);
}

void SyntaxHighlightSQL::ApplyHighlight(wxStyledTextCtrl* textCtrl)
{
    textCtrl->ClearDocumentStyle();
    textCtrl->SetLexer(wxSTC_LEX_NULL);
    wxString text = textCtrl->GetValue();
    int length = textCtrl->GetTextLength(); //use STC length to avoid wxString length issues with certain encodings

    if (length == 0) return;

    //ensure styles array matches actual text length
    if ((int)text.length() < length) length = (int)text.length();

    highlightRange.occupiedRanges.clear();

    std::string styles(length, STYLE_DEFAULT);

    //coms /* */
    size_t pos = text.find("/*");
    while (pos != wxString::npos) {
        if (!highlightRange.IsOccupied(pos, pos + 2)) {
            size_t endPos = text.find("*/", pos + 2);
            if (endPos != wxString::npos) {
                endPos += 2;
                for (size_t i = pos; i < endPos; i++)
                    styles[i] = STYLE_COMMENT;
                highlightRange.Mark(pos, endPos);
                pos = text.find("/*", endPos);
            } else {
                for (size_t i = pos; i < (size_t)length; i++)
                    styles[i] = STYLE_COMMENT;
                highlightRange.Mark(pos, length);
                break;
            }
        } else {
            pos = text.find("/*", pos + 2);
        }
    }

    //comms --
    pos = text.find("--");
    while (pos != wxString::npos) {
        if (!highlightRange.IsOccupied(pos, pos + 2)) {
            size_t endPos = text.find("\n", pos);
            size_t commentEnd = (endPos != wxString::npos) ? endPos : (size_t)length;
            for (size_t i = pos; i < commentEnd; i++)
                styles[i] = STYLE_COMMENT;
            highlightRange.Mark(pos, commentEnd);
            pos = (endPos != wxString::npos) ? text.find("--", endPos) : wxString::npos;
        } else {
            pos = text.find("--", pos + 2);
        }
    }

    //strings (handle both ' and " with simple matching, ignoring escapes for simplicity)
    std::vector<wxString> stringDelimiters = {"'", "\""};
    for (const auto& delimiter : stringDelimiters) {
        pos = text.find(delimiter);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + 1)) {
                size_t endPos = text.find(delimiter, pos + 1);
                if (endPos != wxString::npos) {
                    for (size_t i = pos; i <= endPos; i++)
                        styles[i] = STYLE_STRING;
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

    //keywords (case-insensitive, whole-word)
    std::vector<wxString> keywords = {
        "GROUP BY", "ORDER BY",
        "SELECT", "FROM", "WHERE", "INSERT", "UPDATE", "DELETE",
        "JOIN", "INNER", "LEFT", "RIGHT", "OUTER", "CROSS", "ON",
        "CREATE", "TABLE", "ALTER", "DROP", "INDEX", "VIEW",
        "TRIGGER", "PROCEDURE", "FUNCTION",
        "UNION", "HAVING", "DISTINCT", "AS",
        "AND", "OR", "NOT", "IN", "IS", "NULL",
        "LIKE", "BETWEEN", "EXISTS", "CONSTRAINT",
        "SET", "INTO", "VALUES", "ALL", "ANY", "CASE", "WHEN", "THEN", "ELSE", "END",
        "PRIMARY", "FOREIGN", "KEY", "REFERENCES", "DEFAULT", "UNIQUE"
    };

    for (const auto& keyword : keywords) {
        size_t klen = keyword.length();
        pos = FindCI(text, keyword);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + klen)) {
                bool validStart = (pos == 0) || (!wxIsalnum(text[pos - 1]) && text[pos - 1] != '_');
                bool validEnd   = (pos + klen >= (size_t)length) ||
                                  (!wxIsalnum(text[pos + klen]) && text[pos + klen] != '_');

                if (validStart && validEnd) {
                    for (size_t i = pos; i < pos + klen; i++)
                        styles[i] = STYLE_KEYWORD;
                    highlightRange.Mark(pos, pos + klen);
                }
            }
            pos = FindCI(text, keyword, pos + klen);
        }
    }

    //data types (case-insensitive, whole-word)
    std::vector<wxString> dataTypes = {
        "BIGINT", "INT", "SMALLINT", "TINYINT",
        "VARCHAR", "NVARCHAR", "CHAR", "NCHAR", "TEXT",
        "FLOAT", "REAL", "DOUBLE", "DECIMAL", "NUMERIC",
        "DATE", "DATETIME", "TIMESTAMP", "TIME",
        "BOOLEAN", "BOOL", "BLOB", "BINARY"
    };
    for (const auto& dtype : dataTypes) {
        size_t dlen = dtype.length();
        pos = FindCI(text, dtype);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + dlen)) {
                bool validStart = (pos == 0) || (!wxIsalnum(text[pos - 1]) && text[pos - 1] != '_');
                bool validEnd   = (pos + dlen >= (size_t)length) ||
                                  (!wxIsalnum(text[pos + dlen]) && text[pos + dlen] != '_');
                if (validStart && validEnd) {
                    for (size_t i = pos; i < pos + dlen; i++)
                        styles[i] = STYLE_NAMESPACE;
                    highlightRange.Mark(pos, pos + dlen);
                }
            }
            pos = FindCI(text, dtype, pos + dlen);
        }
    }

    //functions (case-insensitive, whole-word)
    std::vector<wxString> functions = {
        "COUNT", "SUM", "AVG", "MIN", "MAX",
        "ROUND", "CEIL", "FLOOR", "ABS",
        "UPPER", "LOWER", "LENGTH", "TRIM", "LTRIM", "RTRIM",
        "COALESCE", "NULLIF", "CAST", "CONVERT",
        "NOW", "GETDATE", "DATEPART", "DATEDIFF", "DATEADD",
        "SUBSTRING", "REPLACE", "CHARINDEX", "CONCAT"
    };
    for (const auto& func : functions) {
        size_t flen = func.length();
        pos = FindCI(text, func);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + flen)) {
                bool validStart = (pos == 0) || (!wxIsalnum(text[pos - 1]) && text[pos - 1] != '_');
                bool validEnd   = (pos + flen >= (size_t)length) ||
                                  (!wxIsalnum(text[pos + flen]) && text[pos + flen] != '_');
                if (validStart && validEnd) {
                    for (size_t i = pos; i < pos + flen; i++)
                        styles[i] = STYLE_FUNCTION;
                    highlightRange.Mark(pos, pos + flen);
                }
            }
            pos = FindCI(text, func, pos + flen);
        }
    }

    //operators and symbols
    std::vector<wxString> operators = {
        "<>", "!=", "<=", ">=", "<", ">", "=",
        "+", "-", "*", "/", "%",
        "(", ")", ",", ";"
    };
    for (const auto& op : operators) {
        size_t olen = op.length();
        pos = text.find(op);
        while (pos != wxString::npos) {
            if (!highlightRange.IsOccupied(pos, pos + olen)) {
                for (size_t i = pos; i < pos + olen; i++)
                    styles[i] = STYLE_OPERATOR;
                highlightRange.Mark(pos, pos + olen);
            }
            pos = text.find(op, pos + olen);
        }
    }

    //apply all styles at once
    textCtrl->StartStyling(0);
    textCtrl->SetStyleBytes(styles.size(), styles.data());
}