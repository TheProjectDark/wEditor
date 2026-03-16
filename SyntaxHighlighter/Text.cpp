/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "Text.h"

void Text::ApplyHighlight(wxStyledTextCtrl* textCtrl) {
    textCtrl->ClearDocumentStyle(); //fix
    textCtrl->SetLexer(wxSTC_LEX_NULL); //fix
    wxString text = textCtrl->GetValue();
    int length = text.length();
    
    //skip highlight for empty text
    if (length == 0) return;
    highlightRange.occupiedRanges.clear();

    highlightRange.occupiedRanges.clear();

    //clear all styles for plain text
    std::string styles(length, STYLE_DEFAULT);
    textCtrl->SetStyleBytes(length, (char*)styles.c_str());
    highlightRange.Mark(0, text.length());
}