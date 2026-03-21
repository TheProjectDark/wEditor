/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once
#include <wx/wx.h>
#include <wx/stc/stc.h>
#include <vector>
#include <set>

//style indices for token types
enum StyleIndices {
    STYLE_DEFAULT = 0,
    STYLE_COMMENT = 1,
    STYLE_STRING = 2,
    STYLE_KEYWORD = 3,
    STYLE_PREPROCESSOR = 4,
    STYLE_NAMESPACE = 5,
    STYLE_NUMBER = 6,
    STYLE_OPERATOR = 7,
    STYLE_FUNCTION = 8
};

class HighlightRange {
public:
    std::set<std::pair<size_t, size_t>> occupiedRanges;
    
    bool IsOccupied(size_t start, size_t end) const {
        for (const auto& range : occupiedRanges) {
            if (!(end <= range.first || start >= range.second)) {
                return true;
            }
        }
        return false;
    }
    
    void Mark(size_t start, size_t end) {
        occupiedRanges.insert({start, end});
    }
};

class SyntaxHighlighter {
public:
    virtual ~SyntaxHighlighter() = default;
    virtual void ApplyHighlight(wxStyledTextCtrl* textCtrl) = 0;
    virtual wxString GetLanguageName() const = 0;
protected:
    HighlightRange highlightRange;
    
};

class HighlighterFactory {
public:
    static SyntaxHighlighter* CreateHighlighter(const wxString& language);
    static std::vector<wxString> GetAvailableLanguages();
};
