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
#include "SyntaxHighlighter.h"

class SyntaxHighlightCPP : public SyntaxHighlighter {
public:
    void ApplyHighlight(wxStyledTextCtrl* textCtrl) override;
    wxString GetLanguageName() const override { return "C++"; }
};